"""Arc MLIR text parser → data classes."""
import re
from dataclasses import dataclass, field
from typing import Optional
from .types import cpp_type, bits_of, cpp_uint

def strip_ssa(s: str) -> str:
    """'%foo' → 'foo', '%0' → 'v0', '%202#1' → 'v202_1' (multi-result access)"""
    s = s.strip()
    if not s.startswith("%"): return s
    inner = s[1:]
    # Multi-result access: %N#K → vN_K, %name#K → name_K
    m = re.match(r'^(\d+)#(\d+)$', inner)
    if m:
        return f"v{m.group(1)}_{m.group(2)}"
    m = re.match(r'^(\w+)#(\d+)$', inner)
    if m:
        base = m.group(1)
        return f"{base}_{m.group(2)}" if not base.isdigit() else f"v{base}_{m.group(2)}"
    return inner if not inner.isdigit() else f"v{inner}"

@dataclass
class ArcArg:
    name: str; ctype: str; bits: int

@dataclass
class ArcDef:
    name: str; args: list; ret_ctypes: list; body: list

@dataclass
class StateInst:
    result: str; reg_name: str; arc_ref: str; arg_ids: list; clock_id: str; ctype: str
    enable_id: Optional[str] = None
    reset_id: Optional[str] = None
    initial_ids: list = field(default_factory=list)

@dataclass
class CallInst:
    result: str; arc_ref: str; arg_ids: list; ctype: str
    # For multi-result calls (e.g., %202:4 → results=['v202_0','v202_1','v202_2','v202_3'])
    results: list = field(default_factory=list)

@dataclass
class MemInst:
    ssa_id: str; name: str; num_words: int; word_ctype: str; addr_bits: int

@dataclass
class MemReadPort:
    result: str; mem_id: str; addr_id: str; word_ctype: str

@dataclass
class MemWritePort:
    mem_id: str; arc_ref: str; arg_ids: list; clock_id: str; has_enable: bool

@dataclass
class InlineOp:
    """A simple inline comb.* op in the hw.module body that isn't an arc.call."""
    result: str; expr: str; ctype: str
    # No sub-results for inline ops

@dataclass
class HWMod:
    name: str; in_ports: list; out_ports: list
    states: list; calls: list
    memories: list; mem_reads: list; mem_writes: list
    output_ids: list
    # hw.constant ops in the module body: {stripped_ssa_name: literal_value_str}
    constants: dict = field(default_factory=dict)
    # Inline comb.* ops that need evaluation in order
    inline_ops: list = field(default_factory=list)

_STATE_RE = re.compile(
    r'^\s*(%[\w]+)\s*=\s*arc\.state\s+@(\w+)\(([^)]*)\)'
    r'(?:\s+clock\s+(%[\w]+))?'
    r'(?:\s+enable\s+(%[\w]+))?'
    r'(?:\s+reset\s+(%[\w]+)(?:,\s*%[\w]+)?)?'
    r'.*?'
    r'(?:\{(?:names\s*=\s*\["([^"]+)"\]|name\s*=\s*"([^"]+)")[^}]*\})?'
    r'\s*:\s*\([^)]*\)\s*->\s*(\(.+?\)|\S+)',
    re.DOTALL
)
# Multi-result arc.state: %N:K = arc.state @f(args) ... -> (T1, T2, ...)
_STATE_MULTI_RE = re.compile(
    r'^\s*(%[\w]+):(\d+)\s*=\s*arc\.state\s+@(\w+)\(([^)]*)\)'
    r'(?:\s+clock\s+(%[\w]+))?'
    r'(?:\s+enable\s+(%[\w]+))?'
    r'(?:\s+reset\s+(%[\w]+)(?:,\s*%[\w]+)?)?'
    r'.*?'
    r'\s*:\s*\([^)]*\)\s*->\s*\((.+?)\)',
    re.DOTALL
)
_MEM_RE = re.compile(r'^\s*(%[\w]+)\s*=\s*arc\.memory\s+<(\d+)\s*x\s*(i\d+),\s*(i\d+)>')
_MEM_READ_RE = re.compile(
    r'^\s*(%[\w]+)\s*=\s*arc\.memory_read_port\s+(%[\w]+)\[(%[\w]+)\]'
    r'\s*:\s*<(\d+)\s*x\s*(i\d+),\s*(i\d+)>'
)
_MEM_WRITE_RE = re.compile(
    r'^\s*arc\.memory_write_port\s+(%[\w]+),\s*@(\w+)\(([^)]*)\)'
    r'(?:\s+clock\s+(%[\w]+))?'
    r'(\s+enable)?'
    r'\s+latency'
)
_CALL_RE = re.compile(
    r'^\s*(%[\w]+)\s*=\s*arc\.call\s+@(\w+)\(([^)]*)\)'
    r'\s*:\s*\([^)]*\)\s*->\s*(.+)'
)
# Multi-result: %N:K = arc.call @f(args) : (...) -> (T1, T2, ...)
_CALL_MULTI_RE = re.compile(
    r'^\s*(%[\w]+):(\d+)\s*=\s*arc\.call\s+@(\w+)\(([^)]*)\)'
    r'\s*:\s*\([^)]*\)\s*->\s*\((.+)\)'
)
# hw.constant in hw.module body (symbolic names like %c-8_i4 have dashes)
_HWMOD_CONST_BOOL_RE = re.compile(r'^\s*(%[^\s=]+)\s*=\s*hw\.constant\s+(true|false)')
_HWMOD_CONST_INT_RE = re.compile(r'^\s*(%[^\s=]+)\s*=\s*hw\.constant\s+(-?\d+)\s*:\s*(\S+)')

def _collect_block(lines: list, start: int) -> tuple:
    """Collect lines inside a {}-delimited block starting at line `start`."""
    body = []
    i, depth = start + 1, 1
    while i < len(lines) and depth > 0:
        l = lines[i]
        depth += l.count("{") - l.count("}")
        if depth > 0:
            body.append(l)
        i += 1
    return body, i

def _parse_arc_def(lines: list, start: int) -> tuple:
    header = lines[start]
    m = re.match(r'\s*arc\.define\s+@(\w+)\((.*?)\)\s*->\s*(.+?)\s*\{', header)
    assert m, f"bad arc.define: {header}"
    args = []
    for a in m.group(2).split(","):
        a = a.strip()
        if not a: continue
        am = re.match(r'(%\w+)\s*:\s*(.+)', a)
        if am:
            n, t = strip_ssa(am.group(1)), am.group(2).strip()
            args.append(ArcArg(n, cpp_type(t), bits_of(t)))
    raw_ret = m.group(3).strip().lstrip("(").rstrip(")")
    ret_ctypes = [cpp_type(t) for t in raw_ret.split(",") if t.strip()]
    body, end = _collect_block(lines, start)
    return ArcDef(m.group(1), args, ret_ctypes, body), end

def _parse_hw_module(lines: list, start: int) -> tuple:
    # Join continuation lines until we have the closing ') {'
    header = lines[start]
    j = start
    while j < len(lines) - 1 and not re.search(r'\)\s*\{', header):
        j += 1
        header = header.rstrip() + " " + lines[j].strip()
    m = re.match(r'\s*hw\.module\s+@(\w+)\((.+?)\)\s*\{', header)
    assert m, f"bad hw.module: {header}"
    # Block starts after the joined header (at line j)
    start = j
    in_ports, out_ports = [], []
    for p in re.split(r',\s*(?=(?:in|out)\s)', m.group(2)):
        p = p.strip()
        pm = re.match(r'(in|out)\s+%?(\w+)\s*:\s*(.+)', p)
        if pm:
            direction, pname, ptype = pm.group(1), pm.group(2), pm.group(3).strip()
            a = ArcArg(pname, cpp_type(ptype), bits_of(ptype))
            (in_ports if direction == "in" else out_ports).append(a)

    body_lines, end = _collect_block(lines, start)
    states, calls, memories, mem_reads, mem_writes, output_ids = [], [], [], [], [], []
    inline_ops: list = []
    mem_idx = 0
    constants: dict = {}  # hw.constant ops: stripped_name → literal_value_str

    for line in body_lines:
        # hw.constant true/false
        mc_bool = _HWMOD_CONST_BOOL_RE.match(line)
        if mc_bool:
            constants[strip_ssa(mc_bool.group(1))] = "1" if mc_bool.group(2) == "true" else "0"
            continue
        # hw.constant N : T — mask to bit width (handles negative + wide types)
        mc_int = _HWMOD_CONST_INT_RE.match(line)
        if mc_int:
            raw_val = int(mc_int.group(2))
            nbits = bits_of(mc_int.group(3))
            raw_val = raw_val & ((1 << nbits) - 1)
            if nbits > 64:
                raw_val = raw_val & 0xFFFFFFFFFFFFFFFF
            constants[strip_ssa(mc_int.group(1))] = (f"{raw_val}u" if raw_val >= (1 << 63) else str(raw_val))
            continue

        # Multi-result arc.state: %N:K = arc.state @f(...) -> (T1, T2, ...)
        msm = _STATE_MULTI_RE.match(line)
        if msm:
            base = strip_ssa(msm.group(1))
            count = int(msm.group(2))
            func = msm.group(3)
            arg_ids = [strip_ssa(a.strip()) for a in msm.group(4).split(",") if a.strip()]
            clock_id = strip_ssa(msm.group(5)) if msm.group(5) else ""
            enable_id = strip_ssa(msm.group(6)) if msm.group(6) else None
            reset_id = strip_ssa(msm.group(7)) if msm.group(7) else None
            ret_types_str = msm.group(8)
            ret_ctypes = [cpp_type(t.strip()) for t in ret_types_str.split(",") if t.strip()]
            tuple_t = f"std::tuple<{', '.join(ret_ctypes)}>"
            # Each result: base_0, base_1, ...
            result_names = [f"{base}_{i}" for i in range(count)]
            # Store as first result with all result names
            reg_name = f"reg_{base}"
            states.append(StateInst(result_names[0], reg_name, func, arg_ids,
                                    clock_id, tuple_t, enable_id, reset_id,
                                    initial_ids=result_names))
            continue

        ms = _STATE_RE.match(line)
        if ms:
            result = strip_ssa(ms.group(1))
            arg_ids = [strip_ssa(a.strip()) for a in ms.group(3).split(",") if a.strip()]
            clock_id = strip_ssa(ms.group(4)) if ms.group(4) else ""
            enable_id = strip_ssa(ms.group(5)) if ms.group(5) else None
            reset_id = strip_ssa(ms.group(6)) if ms.group(6) else None
            reg_name = ms.group(7) or ms.group(8) or f"reg_{result}"
            states.append(StateInst(result, reg_name, ms.group(2), arg_ids,
                                    clock_id, cpp_type(ms.group(9)), enable_id, reset_id))
            continue

        # Inline comb.xor in hw.module body (XOR with constant true = invert)
        m_cxor = re.match(r'^\s*(%[^\s=]+)\s*=\s*comb\.xor\s+(%[^\s,]+),\s*(%[^\s,]+)\s*:\s*(\S+)', line)
        if m_cxor:
            res = strip_ssa(m_cxor.group(1))
            a = strip_ssa(m_cxor.group(2))
            b = strip_ssa(m_cxor.group(3))
            t = cpp_type(m_cxor.group(4))
            inline_ops.append(InlineOp(res, f"{a} ^ {b}", t))
            continue

        mm = _MEM_RE.match(line)
        if mm:
            ssa_id = strip_ssa(mm.group(1))
            word_ctype = cpp_uint(int(mm.group(3)[1:]))
            addr_bits = int(mm.group(4)[1:])
            mem_name = f"mem{mem_idx}"; mem_idx += 1
            memories.append(MemInst(ssa_id, mem_name, int(mm.group(2)), word_ctype, addr_bits))
            continue

        mr = _MEM_READ_RE.match(line)
        if mr:
            word_ctype = cpp_uint(int(mr.group(5)[1:]))
            mem_reads.append(MemReadPort(strip_ssa(mr.group(1)), strip_ssa(mr.group(2)),
                                         strip_ssa(mr.group(3)), word_ctype))
            continue

        mw = _MEM_WRITE_RE.match(line)
        if mw:
            arg_ids = [strip_ssa(a.strip()) for a in mw.group(3).split(",") if a.strip()]
            clock_id = strip_ssa(mw.group(4)) if mw.group(4) else ""
            mem_writes.append(MemWritePort(strip_ssa(mw.group(1)), mw.group(2),
                                           arg_ids, clock_id, bool(mw.group(5))))
            continue

        # Multi-result arc.call (%N:K = arc.call ...) — must be checked before single-result
        mcm = _CALL_MULTI_RE.match(line)
        if mcm:
            base = strip_ssa(mcm.group(1))  # e.g. 'v202'
            count = int(mcm.group(2))
            func = mcm.group(3)
            arg_ids = [strip_ssa(a.strip()) for a in mcm.group(4).split(",") if a.strip()]
            ret_types = [t.strip() for t in mcm.group(5).split(",") if t.strip()]
            # Generate individual result names: v202_0, v202_1, ...
            result_names = [f"{base}_{i}" for i in range(count)]
            ret_ctypes = [cpp_type(t) for t in ret_types[:count]]
            tuple_t = f"std::tuple<{', '.join(ret_ctypes)}>"
            # Use first result name as the CallInst.result (for dependency tracking)
            ci = CallInst(result_names[0], func, arg_ids, tuple_t, result_names)
            calls.append(ci)
            continue

        mc = _CALL_RE.match(line)
        if mc:
            arg_ids = [strip_ssa(a.strip()) for a in mc.group(3).split(",") if a.strip()]
            calls.append(CallInst(strip_ssa(mc.group(1)), mc.group(2),
                                  arg_ids, cpp_type(mc.group(4).strip())))
            continue

        mo = re.match(r'\s*hw\.output\s+(.*?)\s*:', line)
        if mo:
            output_ids = [strip_ssa(v.strip()) for v in mo.group(1).split(",") if v.strip()]

    return HWMod(m.group(1), in_ports, out_ports, states, calls,
                 memories, mem_reads, mem_writes, output_ids, constants,
                 inline_ops), end

def parse(text: str) -> tuple:
    """Parse Arc MLIR text → (list[ArcDef], list[HWMod])."""
    lines = text.splitlines()
    arc_defs, hw_mods = [], []
    i = 0
    while i < len(lines):
        l = lines[i]
        if re.match(r'\s*arc\.define\s', l):
            d, i = _parse_arc_def(lines, i)
            arc_defs.append(d)
        elif re.match(r'\s*hw\.module\s', l):
            mod, i = _parse_hw_module(lines, i)
            hw_mods.append(mod)
        else:
            i += 1
    return arc_defs, hw_mods
