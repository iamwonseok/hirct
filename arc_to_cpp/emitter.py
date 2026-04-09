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

        # hw.constant N : T
        m = re.match(r'(%[\w]+)\s*=\s*hw\.constant\s+(-?\d+)\s*:\s*(\S+)', line)
        if m:
            res = strip_ssa(m.group(1))
            out.append(f"  {cpp_type(m.group(3))} {res} = {m.group(2)};")
            ssa[res] = res; continue

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


def emit(arc_defs, hw_mods) -> str:
    """Stub — implemented in Task 4."""
    return ""
