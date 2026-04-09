"""C++ code emitter for Arc dialect MLIR."""
import re
from .types import cpp_type, cpp_uint, bits_of

COMB_BINOPS = {
    "comb.add": "+", "comb.sub": "-", "comb.mul": "*",
    "comb.and": "&", "comb.or": "|", "comb.xor": "^",
    "comb.shl": "<<", "comb.shru": ">>",
}
ICMP_PRED = {
    "eq": "==", "ne": "!=",
    "slt": "<", "sle": "<=", "sgt": ">", "sge": ">=",
    "ult": "<", "ule": "<=", "ugt": ">", "uge": ">=",
}
SIGNED_TYPES = {8: "int8_t", 16: "int16_t", 32: "int32_t", 64: "int64_t"}

def strip_ssa(s: str) -> str:
    s = s.strip()
    if not s.startswith("%"): return s
    inner = s[1:]
    return inner if not inner.isdigit() else f"v{inner}"

def emit_arc_body(body_lines: list[str], arg_map: dict, ret_ctypes: list[str]) -> str:
    out = []
    ssa = dict(arg_map)

    for line in body_lines:
        line = line.strip()
        if not line: continue

        # N-ary comb binop (strip optional 'bin' qualifier)
        m = re.match(r'(%[\w]+)\s*=\s*(comb\.\w+)\s+(.*?)\s*:\s*(\S+)$', line)
        if m and m.group(2) in COMB_BINOPS:
            res = strip_ssa(m.group(1))
            op = COMB_BINOPS[m.group(2)]
            ops_str = re.sub(r'^bin\s+', '', m.group(3))
            operands = [ssa.get(strip_ssa(o.strip()), strip_ssa(o.strip()))
                        for o in ops_str.split(",") if o.strip()]
            t = cpp_type(m.group(4))
            out.append(f"  {t} {res} = {f' {op} '.join(operands)};")
            ssa[res] = res; continue

        # comb.shrs (arithmetic right shift)
        m = re.match(r'(%[\w]+)\s*=\s*comb\.shrs\s+(.*?)\s*:\s*(\S+)$', line)
        if m:
            res = strip_ssa(m.group(1))
            ops = [ssa.get(strip_ssa(o.strip()), strip_ssa(o.strip()))
                   for o in m.group(2).split(",") if o.strip()]
            bits = bits_of(m.group(3))
            t = cpp_uint(bits)
            signed_t = SIGNED_TYPES.get(bits, f"int{bits}_t")
            out.append(f"  {t} {res} = ({t})(({signed_t}){ops[0]} >> {ops[1]});")
            ssa[res] = res; continue

        # comb.icmp
        m = re.match(r'(%[\w]+)\s*=\s*comb\.icmp\s+(\w+)\s+(.*?)\s*:\s*(\S+)$', line)
        if m:
            res = strip_ssa(m.group(1))
            pred = ICMP_PRED.get(m.group(2), "==")
            ops = [ssa.get(strip_ssa(o.strip()), strip_ssa(o.strip()))
                   for o in m.group(3).split(",") if o.strip()]
            out.append(f"  bool {res} = {ops[0]} {pred} {ops[1]};")
            ssa[res] = res; continue

        # comb.mux (handles 'bin' qualifier)
        m = re.match(r'(%[\w]+)\s*=\s*comb\.mux\s+(?:bin\s+)?(%[\w]+),\s*(%[\w]+),\s*(%[\w]+)\s*:\s*(\S+)', line)
        if m:
            res = strip_ssa(m.group(1))
            sel, a, b = [ssa.get(strip_ssa(m.group(i)), strip_ssa(m.group(i))) for i in [2,3,4]]
            t = cpp_type(m.group(5))
            out.append(f"  {t} {res} = {sel} ? {a} : {b};")
            ssa[res] = res; continue

        # comb.concat
        m = re.match(r'(%[\w]+)\s*=\s*comb\.concat\s+(.*?)\s*:\s*(.+)$', line)
        if m:
            res = strip_ssa(m.group(1))
            operands = [strip_ssa(o.strip()) for o in m.group(2).split(",")]
            type_strs = [t.strip() for t in m.group(3).split(",")]
            bits_list = [bits_of(t) for t in type_strs]
            total = sum(bits_list)
            t = cpp_uint(total)
            exprs, shift = [], 0
            for i in range(len(operands)-1, -1, -1):
                v = ssa.get(operands[i], operands[i])
                exprs.insert(0, f"(({t}){v} << {shift})" if shift else f"({t}){v}")
                shift += bits_list[i]
            out.append(f"  {t} {res} = {' | '.join(exprs)};")
            ssa[res] = res; continue

        # comb.extract
        m = re.match(r'(%[\w]+)\s*=\s*comb\.extract\s+(%[\w]+)\s+from\s+(\d+)\s*:\s*\([^)]+\)\s*->\s*(\S+)', line)
        if m:
            res = strip_ssa(m.group(1))
            src = ssa.get(strip_ssa(m.group(2)), strip_ssa(m.group(2)))
            offset = int(m.group(3))
            t = cpp_type(m.group(4))
            bits = bits_of(m.group(4))
            mask = (1 << bits) - 1
            expr = f"({t})(({src}) >> {offset}) & 0x{mask:X}u" if offset else f"({t}){src} & 0x{mask:X}u"
            out.append(f"  {t} {res} = {expr};")
            ssa[res] = res; continue

        # comb.parity
        m = re.match(r'(%[\w]+)\s*=\s*comb\.parity\s+(%[\w]+)\s*:', line)
        if m:
            res = strip_ssa(m.group(1))
            src = ssa.get(strip_ssa(m.group(2)), strip_ssa(m.group(2)))
            out.append(f"  bool {res} = __builtin_parityll((unsigned long long){src});")
            ssa[res] = res; continue

        # comb.replicate
        m = re.match(r'(%[\w]+)\s*=\s*comb\.replicate\s+(%[\w]+)\s*:\s*\(([^)]+)\)\s*->\s*(\S+)', line)
        if m:
            res = strip_ssa(m.group(1))
            src = ssa.get(strip_ssa(m.group(2)), strip_ssa(m.group(2)))
            src_bits, ret_bits = bits_of(m.group(3)), bits_of(m.group(4))
            t = cpp_uint(ret_bits)
            count = ret_bits // src_bits
            parts = [f"(({t}){src} << {i*src_bits})" if i else f"({t}){src}" for i in range(count)]
            out.append(f"  {t} {res} = {' | '.join(parts)};")
            ssa[res] = res; continue

        # hw.constant true/false
        m = re.match(r'(%[\w]+)\s*=\s*hw\.constant\s+(true|false)', line)
        if m:
            ssa[strip_ssa(m.group(1))] = "1" if m.group(2) == "true" else "0"; continue

        # hw.constant N : T — fold inline (no variable declaration)
        m = re.match(r'(%[\w]+)\s*=\s*hw\.constant\s+(-?\d+)\s*:\s*(\S+)', line)
        if m:
            ssa[strip_ssa(m.group(1))] = m.group(2)  # store literal value, no declaration
            continue

        # seq.to_clock (passthrough)
        m = re.match(r'(%[\w]+)\s*=\s*seq\.to_clock\s+(%[\w]+)', line)
        if m:
            ssa[strip_ssa(m.group(1))] = ssa.get(strip_ssa(m.group(2)), strip_ssa(m.group(2))); continue

        # arc.output (single)
        m = re.match(r'arc\.output\s+(%[\w]+)\s*:', line)
        if m:
            v = ssa.get(strip_ssa(m.group(1)), strip_ssa(m.group(1)))
            out.append(f"  return {v};"); continue

        # arc.output (tuple)
        m = re.match(r'arc\.output\s+(.+?)\s*:', line)
        if m and "," in m.group(1):
            vals = [ssa.get(strip_ssa(v.strip()), strip_ssa(v.strip()))
                    for v in m.group(1).split(",")]
            out.append(f"  return std::make_tuple({', '.join(vals)});"); continue

    return "\n".join(out)


def trace_clock_to_port_name(clk_id: str, calls, in_ports) -> str:
    """Follow arc.call chain from clock SSA id back to input port name."""
    port_names = {p.name for p in in_ports}
    if clk_id in port_names:
        return clk_id
    for c in calls:
        if c.result == clk_id:
            for a in c.arg_ids:
                r = trace_clock_to_port_name(a, calls, in_ports)
                if r: return r
    return clk_id  # fallback


import textwrap

def emit(arc_defs, hw_mods) -> str:
    """Generate a complete C++ header from parsed Arc MLIR data structures."""
    parts = [textwrap.dedent("""\
        // Auto-generated by arc-to-cpp
        #pragma once
        #include <cstdint>
        #include <tuple>
    """)]

    # arc.define → static functions (marked maybe_unused to avoid -Wunused-function)
    for d in arc_defs:
        params = ", ".join(f"{a.ctype} {a.name}" for a in d.args)
        if len(d.ret_ctypes) == 1:
            ret = d.ret_ctypes[0]
        else:
            ret = f"std::tuple<{', '.join(d.ret_ctypes)}>"
        body = emit_arc_body(d.body, {a.name: a.name for a in d.args}, d.ret_ctypes)
        parts.append(f"[[maybe_unused]] static {ret} {d.name}({params}) {{\n{body}\n}}")

    for mod in hw_mods:
        # Collect clock domain names from states AND memory writes
        clock_names = {trace_clock_to_port_name(s.clock_id, mod.calls, mod.in_ports)
                       for s in mod.states}
        clock_names |= {trace_clock_to_port_name(w.clock_id, mod.calls, mod.in_ports)
                        for w in mod.mem_writes}

        # State struct
        struct = [f"struct {mod.name}State {{"]
        for mem in mod.memories:
            struct.append(f"  {mem.word_ctype} {mem.name}[{mem.num_words}];  // arc.memory")
        for s in mod.states:
            struct.append(f"  {s.ctype} {s.reg_name}{{}};  // register")
        for p in mod.in_ports:
            # Clock ports are included for internal SSA resolution but have no setter
            struct.append(f"  {p.ctype} {p.name}{{}};  // {'clock port' if p.name in clock_names else 'input port'}")
        for p in mod.out_ports:
            struct.append(f"  {p.ctype} {p.name}{{}};  // output port")
        struct.append("};")
        parts.append("\n".join(struct))

        # Class
        cls = [f"class {mod.name} {{", "public:",
               f"  {mod.name}State state{{}};"]

        for p in mod.in_ports:
            if p.name not in clock_names:
                cls.append(f"  void set_{p.name}({p.ctype} v) {{ state.{p.name} = v; }}")
        for p in mod.out_ports:
            cls.append(f"  {p.ctype} get_{p.name}() const {{ return state.{p.name}; }}")

        # Group states by clock domain
        clk_groups: dict = {}
        for s in mod.states:
            cname = trace_clock_to_port_name(s.clock_id, mod.calls, mod.in_ports)
            clk_groups.setdefault(cname, []).append(s)
        for w in mod.mem_writes:
            cname = trace_clock_to_port_name(w.clock_id, mod.calls, mod.in_ports)
            clk_groups.setdefault(cname, [])

        for clk_name, states in clk_groups.items():
            cls.append(f"  void eval_{clk_name}() {{")
            ssa: dict = {}
            for p in mod.in_ports:
                ssa[p.name] = f"state.{p.name}"
            for s in mod.states:
                ssa[s.result] = f"state.{s.reg_name}"
            for mem in mod.memories:
                ssa[mem.ssa_id] = f"state.{mem.name}"

            # Memory reads (combinational — before clocked writes)
            for r in mod.mem_reads:
                addr = ssa.get(r.addr_id, r.addr_id)
                mem_obj = next((m for m in mod.memories if m.ssa_id == r.mem_id), None)
                mem_name = f"state.{mem_obj.name}" if mem_obj else r.mem_id
                cls.append(f"    {r.word_ctype} {r.result} = {mem_name}[{addr}];")
                ssa[r.result] = r.result

            # Collect all SSA ids that are actually used in data flow
            # (call args, state args, enable/reset ids, mem write args, output ids)
            used_in_data: set = set()
            for c in mod.calls:
                used_in_data.update(c.arg_ids)
            for s in mod.states:
                used_in_data.update(s.arg_ids)
                if s.enable_id:
                    used_in_data.add(s.enable_id)
                if s.reset_id:
                    used_in_data.add(s.reset_id)
            for w in mod.mem_writes:
                used_in_data.update(w.arg_ids)
            used_in_data.update(mod.output_ids)

            # arc.call computations (skip calls whose result is never used in data)
            for c in mod.calls:
                args_cpp = [ssa.get(a, a) for a in c.arg_ids]
                if c.result not in used_in_data:
                    continue  # clock-conversion results not needed in data flow
                cls.append(f"    {c.ctype} {c.result} = {c.arc_ref}({', '.join(args_cpp)});")
                ssa[c.result] = c.result

            # Two-phase register update (compute next values, then assign)
            next_vars = []
            for s in states:
                args_cpp = [ssa.get(a, a) for a in s.arg_ids]
                nxt = f"next_{s.reg_name}"
                if s.reset_id and s.enable_id:
                    rst = ssa.get(s.reset_id, s.reset_id)
                    en = ssa.get(s.enable_id, s.enable_id)
                    cls.append(f"    {s.ctype} {nxt};")
                    cls.append(f"    if ({rst}) {{ {nxt} = ({s.ctype})0; }}")
                    cls.append(f"    else if ({en}) {{ {nxt} = {s.arc_ref}({', '.join(args_cpp)}); }}")
                    cls.append(f"    else {{ {nxt} = state.{s.reg_name}; }}")
                elif s.enable_id:
                    en = ssa.get(s.enable_id, s.enable_id)
                    cls.append(f"    {s.ctype} {nxt};")
                    cls.append(f"    if ({en}) {{ {nxt} = {s.arc_ref}({', '.join(args_cpp)}); }}")
                    cls.append(f"    else {{ {nxt} = state.{s.reg_name}; }}")
                else:
                    cls.append(f"    {s.ctype} {nxt} = {s.arc_ref}({', '.join(args_cpp)});")
                next_vars.append((s.reg_name, nxt))

            for reg, nxt in next_vars:
                cls.append(f"    state.{reg} = {nxt};")

            # Memory writes (clocked — only for this clock domain)
            for w in mod.mem_writes:
                w_clk = trace_clock_to_port_name(w.clock_id, mod.calls, mod.in_ports)
                if w_clk != clk_name:
                    continue
                mem_obj = next((m for m in mod.memories if m.ssa_id == w.mem_id), None)
                if not mem_obj: continue
                args_cpp = [ssa.get(a, a) for a in w.arg_ids]
                cls.append(f"    // arc.memory_write_port @{w.arc_ref}")
                if w.has_enable and len(w.arg_ids) >= 3:
                    addr, data, en = args_cpp[0], args_cpp[1], args_cpp[-1]
                    cls.append(f"    if ({en}) {{ state.{mem_obj.name}[{addr}] = {data}; }}")
                elif len(w.arg_ids) >= 2:
                    addr, data = args_cpp[0], args_cpp[1]
                    cls.append(f"    state.{mem_obj.name}[{addr}] = {data};")

            # Output ports
            for i, p in enumerate(mod.out_ports):
                if i < len(mod.output_ids):
                    v = ssa.get(mod.output_ids[i], mod.output_ids[i])
                    cls.append(f"    state.{p.name} = {v};")

            cls.append("  }")
        cls.append("};")
        parts.append("\n".join(cls))

    return "\n\n".join(parts) + "\n"
