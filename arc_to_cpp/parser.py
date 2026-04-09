"""Arc MLIR text parser → data classes."""
import re
from dataclasses import dataclass, field
from typing import Optional
from .types import cpp_type, bits_of, cpp_uint

def strip_ssa(s: str) -> str:
    """'%foo' → 'foo', '%0' → 'v0'"""
    s = s.strip()
    if not s.startswith("%"): return s
    inner = s[1:]
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
class HWMod:
    name: str; in_ports: list; out_ports: list
    states: list; calls: list
    memories: list; mem_reads: list; mem_writes: list
    output_ids: list

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
    mem_idx = 0

    for line in body_lines:
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
                 memories, mem_reads, mem_writes, output_ids), end

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
