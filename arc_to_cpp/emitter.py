"""C++ code emitter for Arc dialect MLIR."""
import re
import textwrap
from typing import Optional
from .types import cpp_type, cpp_uint, bits_of, cpp_array_type

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
SIGNED_PREDS = {"slt", "sle", "sgt", "sge"}

def _signed_type_for(bits: int) -> str:
    if bits <= 8:  return "int8_t"
    if bits <= 16: return "int16_t"
    if bits <= 32: return "int32_t"
    return "int64_t"

def strip_ssa(s: str) -> str:
    s = s.strip()
    if not s.startswith("%"): return s
    inner = s[1:]
    return inner if not inner.isdigit() else f"v{inner}"

# MLIR SSA names can include '-' (e.g. %c-8_i4, %c-128_i8).
# Use [^\s,():=]+ instead of [\w]+ to capture these names.
_SSA = r'%[^\s,():=]+'

def emit_arc_body(body_lines: list[str], arg_map: dict, ret_ctypes: list[str]) -> str:
    out = []
    ssa = dict(arg_map)

    for line in body_lines:
        line = line.strip()
        if not line: continue

        # N-ary comb binop (strip optional 'bin' qualifier)
        m = re.match(r'(' + _SSA + r')\s*=\s*(comb\.\w+)\s+(.*?)\s*:\s*(\S+)$', line)
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
        m = re.match(r'(' + _SSA + r')\s*=\s*comb\.shrs\s+(.*?)\s*:\s*(\S+)$', line)
        if m:
            res = strip_ssa(m.group(1))
            ops = [ssa.get(strip_ssa(o.strip()), strip_ssa(o.strip()))
                   for o in m.group(2).split(",") if o.strip()]
            bits = bits_of(m.group(3))
            t = cpp_uint(bits)
            signed_t = _signed_type_for(bits)
            out.append(f"  {t} {res} = ({t})(({signed_t}){ops[0]} >> {ops[1]});")
            ssa[res] = res; continue

        # comb.icmp
        m = re.match(r'(' + _SSA + r')\s*=\s*comb\.icmp\s+(\w+)\s+(.*?)\s*:\s*(\S+)$', line)
        if m:
            res = strip_ssa(m.group(1))
            predicate = m.group(2)
            pred = ICMP_PRED.get(predicate, "==")
            ops = [ssa.get(strip_ssa(o.strip()), strip_ssa(o.strip()))
                   for o in m.group(3).split(",") if o.strip()]
            if predicate in SIGNED_PREDS:
                b = bits_of(m.group(4))
                st = _signed_type_for(b)
                out.append(f"  bool {res} = ({st}){ops[0]} {pred} ({st}){ops[1]};")
            else:
                out.append(f"  bool {res} = {ops[0]} {pred} {ops[1]};")
            ssa[res] = res; continue

        # comb.mux (handles 'bin' qualifier)
        m = re.match(r'(' + _SSA + r')\s*=\s*comb\.mux\s+(?:bin\s+)?(' + _SSA + r'),\s*(' + _SSA + r'),\s*(' + _SSA + r')\s*:\s*(\S+)', line)
        if m:
            res = strip_ssa(m.group(1))
            sel, a, b = [ssa.get(strip_ssa(m.group(i)), strip_ssa(m.group(i))) for i in [2,3,4]]
            t = cpp_type(m.group(5))
            out.append(f"  {t} {res} = {sel} ? {a} : {b};")
            ssa[res] = res; continue

        # comb.concat
        m = re.match(r'(' + _SSA + r')\s*=\s*comb\.concat\s+(.*?)\s*:\s*(.+)$', line)
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
                # Clamp shift to 63 for 64-bit types to avoid UB (wide types are truncated anyway)
                safe_shift = min(shift, 63) if total > 64 else shift
                exprs.insert(0, f"(({t}){v} << {safe_shift})" if shift else f"({t}){v}")
                shift += bits_list[i]
            out.append(f"  {t} {res} = {' | '.join(exprs)};")
            ssa[res] = res; continue

        # comb.extract
        m = re.match(r'(' + _SSA + r')\s*=\s*comb\.extract\s+(' + _SSA + r')\s+from\s+(\d+)\s*:\s*\([^)]+\)\s*->\s*(\S+)', line)
        if m:
            res = strip_ssa(m.group(1))
            src = ssa.get(strip_ssa(m.group(2)), strip_ssa(m.group(2)))
            offset = int(m.group(3))
            t = cpp_type(m.group(4))
            bits = bits_of(m.group(4))
            # Clamp mask to 64-bit max (wide types are already truncated to uint64_t)
            mask = min((1 << bits) - 1, 0xFFFFFFFFFFFFFFFF)
            # Clamp offset to 63 for shifts on 64-bit types
            safe_offset = min(offset, 63)
            expr = f"({t})(({src}) >> {safe_offset}) & 0x{mask:X}u" if offset else f"({t}){src} & 0x{mask:X}u"
            out.append(f"  {t} {res} = {expr};")
            ssa[res] = res; continue

        # comb.parity
        m = re.match(r'(' + _SSA + r')\s*=\s*comb\.parity\s+(' + _SSA + r')\s*:', line)
        if m:
            res = strip_ssa(m.group(1))
            src = ssa.get(strip_ssa(m.group(2)), strip_ssa(m.group(2)))
            out.append(f"  bool {res} = __builtin_parityll((unsigned long long){src});")
            ssa[res] = res; continue

        # comb.replicate
        m = re.match(r'(' + _SSA + r')\s*=\s*comb\.replicate\s+(' + _SSA + r')\s*:\s*\(([^)]+)\)\s*->\s*(\S+)', line)
        if m:
            res = strip_ssa(m.group(1))
            src = ssa.get(strip_ssa(m.group(2)), strip_ssa(m.group(2)))
            src_bits, ret_bits = bits_of(m.group(3)), bits_of(m.group(4))
            t = cpp_uint(ret_bits)
            count = ret_bits // src_bits
            parts = [f"(({t}){src} << {i*src_bits})" if i else f"({t}){src}" for i in range(count)]
            out.append(f"  {t} {res} = {' | '.join(parts)};")
            ssa[res] = res; continue

        # hw.aggregate_constant [v0 : T, v1 : T, ...] : !hw.array<N x T>
        # MLIR aggregate_constant stores elements with index N-1 first (MSB to LSB).
        # std::array[0] = last element in MLIR list.
        m = re.match(r'(' + _SSA + r')\s*=\s*hw\.aggregate_constant\s+\[(.+)\]\s*:\s*(!?hw\.array<(\d+)\s*x\s*(i\d+)>)', line)
        if m:
            res = strip_ssa(m.group(1))
            # Parse [v : T, v : T, ...] elements — mask negatives to bit width
            elem_bits = int(m.group(5)[1:])
            elem_t = cpp_uint(elem_bits)
            n = int(m.group(4))
            raw_strs = re.findall(r'(-?\d+)\s*:\s*i\d+', m.group(2))
            # Mask each value to bit width (handles negative two's complement values)
            mask = (1 << elem_bits) - 1
            if elem_bits > 64:
                mask = 0xFFFFFFFFFFFFFFFF
            def _mask_val(s):
                v = int(s) & mask
                return f"{v}u" if v >= (1 << 63) else str(v)
            masked_vals = [_mask_val(s) for s in raw_strs]
            # Reverse so index 0 = last listed element (MLIR aggregate_constant convention)
            vals = list(reversed(masked_vals)) if masked_vals else ["0"] * n
            arr_t = f"std::array<{elem_t}, {n}>"
            out.append(f"  const {arr_t} {res} = {{{', '.join(vals)}}};")
            ssa[res] = res; continue

        # hw.array_create %v0, %v1, ... : T, T, ... → std::array literal
        # hw.array_create produces {v0, v1, ...} in MLIR element order (LSB to MSB).
        m = re.match(r'(' + _SSA + r')\s*=\s*hw\.array_create\s+(.*?)\s*:\s*(.+)$', line)
        if m:
            res = strip_ssa(m.group(1))
            elems = [ssa.get(strip_ssa(e.strip()), strip_ssa(e.strip()))
                     for e in m.group(2).split(",") if e.strip()]
            # Parse return type to get std::array<elem_t, N>
            type_strs = [t.strip() for t in m.group(3).split(",") if t.strip()]
            elem_t = cpp_type(type_strs[0]) if type_strs else "uint32_t"
            n = len(elems)
            arr_t = f"std::array<{elem_t}, {n}>"
            out.append(f"  {arr_t} {res} = {{{', '.join(elems)}}};")
            ssa[res] = res; continue

        # hw.array_get %arr[%idx] : !hw.array<N x T>, iK → elem_t
        m = re.match(r'(' + _SSA + r')\s*=\s*hw\.array_get\s+(' + _SSA + r')\[(' + _SSA + r')\]\s*:\s*(!?hw\.array<[^>]+>),\s*(\S+)', line)
        if m:
            res = strip_ssa(m.group(1))
            arr = ssa.get(strip_ssa(m.group(2)), strip_ssa(m.group(2)))
            idx = ssa.get(strip_ssa(m.group(3)), strip_ssa(m.group(3)))
            arr_info = cpp_array_type(m.group(4))
            elem_t = arr_info[0] if arr_info else cpp_type(m.group(4))
            out.append(f"  {elem_t} {res} = {arr}[{idx}];")
            ssa[res] = res; continue

        # hw.array_inject %arr[%idx], %val : !hw.array<N x T>, iK → new array with val at idx
        m = re.match(r'(' + _SSA + r')\s*=\s*hw\.array_inject\s+(' + _SSA + r')\[(' + _SSA + r')\],\s*(' + _SSA + r')\s*:\s*(!?hw\.array<[^>]+>),\s*\S+', line)
        if m:
            res = strip_ssa(m.group(1))
            arr = ssa.get(strip_ssa(m.group(2)), strip_ssa(m.group(2)))
            idx = ssa.get(strip_ssa(m.group(3)), strip_ssa(m.group(3)))
            val = ssa.get(strip_ssa(m.group(4)), strip_ssa(m.group(4)))
            arr_t = cpp_type(m.group(5))
            out.append(f"  {arr_t} {res} = {arr};")
            out.append(f"  {res}[{idx}] = {val};")
            ssa[res] = res; continue

        # hw.constant true/false
        m = re.match(r'(' + _SSA + r')\s*=\s*hw\.constant\s+(true|false)', line)
        if m:
            ssa[strip_ssa(m.group(1))] = "1" if m.group(2) == "true" else "0"; continue

        # hw.constant N : T — fold inline (no variable declaration)
        # Negative values are masked to the bit width (MLIR uses two's complement).
        # Wide types (>64-bit) are truncated to 64-bit (uint64_t is the widest C++ type we emit).
        m = re.match(r'(' + _SSA + r')\s*=\s*hw\.constant\s+(-?\d+)\s*:\s*(\S+)', line)
        if m:
            raw_val = int(m.group(2))
            nbits = bits_of(m.group(3))
            # Mask to actual bit width (handles negatives and oversized positives)
            raw_val = raw_val & ((1 << nbits) - 1)
            # Truncate to 64-bit if wider (cpp_uint maps anything >64 to uint64_t)
            if nbits > 64:
                raw_val = raw_val & 0xFFFFFFFFFFFFFFFF
            # Add 'u' suffix for values >= 2^63 to avoid signed-integer-too-large warnings
            ssa[strip_ssa(m.group(1))] = (f"{raw_val}u" if raw_val >= (1 << 63) else str(raw_val))
            continue

        # seq.to_clock (passthrough)
        m = re.match(r'(' + _SSA + r')\s*=\s*seq\.to_clock\s+(' + _SSA + r')', line)
        if m:
            ssa[strip_ssa(m.group(1))] = ssa.get(strip_ssa(m.group(2)), strip_ssa(m.group(2))); continue

        # arc.output (single)
        m = re.match(r'arc\.output\s+(' + _SSA + r')\s*:', line)
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


def trace_clock_to_port_name(clk_id: str, calls, in_ports) -> Optional[str]:
    """Follow arc.call chain from clock SSA id back to input port name.
    Returns the port name if found, None if the chain cannot be resolved."""
    port_names = {p.name for p in in_ports}
    if clk_id in port_names:
        return clk_id
    for c in calls:
        if c.result == clk_id:
            for a in c.arg_ids:
                r = trace_clock_to_port_name(a, calls, in_ports)
                if r is not None:
                    return r
    return None  # unresolvable


def emit(arc_defs, hw_mods) -> str:
    """Generate a complete C++ header from parsed Arc MLIR data structures."""
    parts = [textwrap.dedent("""\
        // Auto-generated by arc-to-cpp
        #pragma once
        #include <array>
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
        clock_names = {
            trace_clock_to_port_name(s.clock_id, mod.calls, mod.in_ports)
            for s in mod.states
        }
        clock_names |= {
            trace_clock_to_port_name(w.clock_id, mod.calls, mod.in_ports)
            for w in mod.mem_writes
        }
        # Filter None (unresolvable clock chains should not pollute port name sets)
        clock_names.discard(None)

        # State struct
        # Collect already-declared field names to avoid duplicate struct members
        # (output ports often share names with registers in the same module)
        declared_fields: set = set()
        struct = [f"struct {mod.name}State {{"]
        for mem in mod.memories:
            struct.append(f"  {mem.word_ctype} {mem.name}[{mem.num_words}];  // arc.memory")
            declared_fields.add(mem.name)
        for s in mod.states:
            struct.append(f"  {s.ctype} {s.reg_name}{{}};  // register")
            declared_fields.add(s.reg_name)
        for p in mod.in_ports:
            if p.name not in clock_names and p.name not in declared_fields:
                struct.append(f"  {p.ctype} {p.name}{{}};  // input port")
                declared_fields.add(p.name)
        for p in mod.out_ports:
            if p.name not in declared_fields:
                struct.append(f"  {p.ctype} {p.name}{{}};  // output port")
                declared_fields.add(p.name)
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
            cname = trace_clock_to_port_name(s.clock_id, mod.calls, mod.in_ports) or s.clock_id
            clk_groups.setdefault(cname, []).append(s)
        for w in mod.mem_writes:
            cname = trace_clock_to_port_name(w.clock_id, mod.calls, mod.in_ports) or w.clock_id
            clk_groups.setdefault(cname, [])

        for clk_name, states in clk_groups.items():
            cls.append(f"  void eval_{clk_name}() {{")
            _is_first_clk = clk_name == next(iter(clk_groups))
            ssa: dict = {}
            # Pre-populate hw.constant values (folded inline as literals)
            ssa.update(mod.constants)
            for p in mod.in_ports:
                ssa[p.name] = f"state.{p.name}"
            for s in mod.states:
                if s.initial_ids:
                    # Multi-result arc.state: individual results extracted from tuple field
                    for i, rn in enumerate(s.initial_ids):
                        cls.append(f"    auto {rn} = std::get<{i}>(state.{s.reg_name});")
                        ssa[rn] = rn
                else:
                    ssa[s.result] = f"state.{s.reg_name}"
            for mem in mod.memories:
                ssa[mem.ssa_id] = f"state.{mem.name}"

            # Collect all SSA ids that are actually used in data flow.
            # For multi-result calls, all result names must be included.
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
            # Also include memory read addresses so their defining calls are not skipped
            for r in mod.mem_reads:
                used_in_data.add(r.addr_id)
            # InlineOps and their operands feed into data flow
            for op in mod.inline_ops:
                # Extract identifier tokens from expr (may be referenced by calls/outputs)
                for tok in re.findall(r'\b[A-Za-z_]\w*\b', op.expr):
                    used_in_data.add(tok)
                used_in_data.add(op.result)

            # Build set of all result names produced by calls (single + multi-result)
            all_call_result_names: set = set()
            for c in mod.calls:
                if c.results:
                    all_call_result_names.update(c.results)
                else:
                    all_call_result_names.add(c.result)

            def _emit_call(c) -> None:
                """Emit a single or multi-result arc.call and update ssa."""
                args_cpp = [ssa.get(a, a) for a in c.arg_ids]
                if c.results:
                    # Multi-result: auto [r0, r1, ...] = func(args);
                    bindings = ", ".join(c.results)
                    cls.append(f"    auto [{bindings}] = {c.arc_ref}({', '.join(args_cpp)});")
                    for r in c.results:
                        ssa[r] = r
                else:
                    cls.append(f"    {c.ctype} {c.result} = {c.arc_ref}({', '.join(args_cpp)});")
                    ssa[c.result] = c.result

            def _call_is_needed(c) -> bool:
                """True if this call's result(s) are used in data flow."""
                if c.results:
                    return any(r in used_in_data for r in c.results)
                return c.result in used_in_data

            def _call_args_ready(c) -> bool:
                """True if all args of this call are available in ssa."""
                return all(a in ssa or a not in all_call_result_names for a in c.arg_ids)

            def _call_result_emitted(c) -> bool:
                """True if this call's primary result is already in ssa."""
                return (c.results[0] if c.results else c.result) in ssa

            def _emit_inline_op(op) -> None:
                """Emit an inline comb op (e.g. comb.xor in hw.module body)."""
                # Substitute ssa map into expr operands
                expr = op.expr
                for tok in re.findall(r'\b[A-Za-z_]\w*\b', op.expr):
                    if tok in ssa and ssa[tok] != tok:
                        expr = re.sub(r'\b' + re.escape(tok) + r'\b', ssa[tok], expr)
                cls.append(f"    {op.ctype} {op.result} = {expr};")
                ssa[op.result] = op.result

            def _inline_op_ready(op) -> bool:
                """True if all tokens in op.expr are available in ssa or are literals."""
                for tok in re.findall(r'\b[A-Za-z_]\w*\b', op.expr):
                    if tok not in ssa and tok in all_call_result_names:
                        return False
                return True

            # Pending memory reads — emit lazily once their address SSA is available.
            pending_mem_reads = list(mod.mem_reads) if _is_first_clk else []

            def _flush_ready_mem_reads():
                """Emit any pending mem reads whose address is now in ssa."""
                still_pending = []
                for r in pending_mem_reads:
                    if r.addr_id in ssa:
                        addr = ssa[r.addr_id]
                        mem_obj = next((m for m in mod.memories if m.ssa_id == r.mem_id), None)
                        mem_name = f"state.{mem_obj.name}" if mem_obj else r.mem_id
                        cls.append(f"    {r.word_ctype} {r.result} = {mem_name}[{addr}];")
                        ssa[r.result] = r.result
                    else:
                        still_pending.append(r)
                pending_mem_reads[:] = still_pending

            # Flush any reads whose addresses are already in ssa (from states/inputs)
            _flush_ready_mem_reads()

            # Topological ordering of arc.calls and inline comb ops.
            # Both may have forward-reference dependencies, so we interleave them.
            pending_calls = [c for c in mod.calls if _call_is_needed(c)]
            pending_inline = list(mod.inline_ops)

            max_passes = len(pending_calls) + len(pending_inline) + 1
            for _pass in range(max_passes):
                any_emitted = False
                still_pending_calls = []
                for c in pending_calls:
                    if _call_result_emitted(c):
                        continue
                    if _call_args_ready(c):
                        _emit_call(c)
                        any_emitted = True
                        _flush_ready_mem_reads()
                    else:
                        still_pending_calls.append(c)
                pending_calls = still_pending_calls

                still_pending_inline = []
                for op in pending_inline:
                    if op.result in ssa:
                        continue
                    if _inline_op_ready(op):
                        _emit_inline_op(op)
                        any_emitted = True
                        _flush_ready_mem_reads()
                    else:
                        still_pending_inline.append(op)
                pending_inline = still_pending_inline

                if not any_emitted:
                    break  # No progress — cycle detected, use DFS fallback below

            # Fallback for remaining calls that form cycles:
            # 1. Run Tarjan's SCC to find true cycle members.
            # 2. Pre-declare all cycle-member results with zero-initialisation so that
            #    later nodes that reference them can compile even before the cycle is
            #    fully evaluated.
            # 3. Emit all pending calls in Kahn-topological order; cycle members are
            #    emitted as reassignments (no type prefix) since they are already
            #    declared in step 2.
            if pending_calls:
                # Build primary-key maps
                _rem_primary: dict = {}
                for _c in pending_calls:
                    _pk = _c.results[0] if _c.results else _c.result
                    if _c.results:
                        for _r in _c.results:
                            _rem_primary[_r] = _pk
                    else:
                        _rem_primary[_c.result] = _pk
                _pk_to_call: dict = {}
                for _c in pending_calls:
                    _pk = _c.results[0] if _c.results else _c.result
                    _pk_to_call[_pk] = _c

                _rem_pks = list(_pk_to_call.keys())  # preserves original MLIR order
                _pk_orig_pos: dict = {pk: i for i, pk in enumerate(_rem_pks)}

                def _intra_deps_pks(pk: str) -> list:
                    _c = _pk_to_call[pk]
                    _seen: set = set()
                    _out: list = []
                    for _a in _c.arg_ids:
                        _dpk = _rem_primary.get(_a)
                        if _dpk and _dpk != pk and _dpk not in _seen:
                            _seen.add(_dpk)
                            _out.append(_dpk)
                    return _out

                # ── Tarjan's iterative SCC to detect cycle members ──────────
                _idx_ctr: list = [0]
                _t_stack: list = []
                _low: dict = {}
                _idx_map: dict = {}
                _on_stk: set = set()
                _sccs: list = []

                for _root in _rem_pks:
                    if _root in _idx_map:
                        continue
                    _cs = [(_root, iter(_intra_deps_pks(_root)))]
                    _idx_map[_root] = _low[_root] = _idx_ctr[0]
                    _idx_ctr[0] += 1
                    _t_stack.append(_root)
                    _on_stk.add(_root)
                    while _cs:
                        _v, _ch = _cs[-1]
                        try:
                            _w = next(_ch)
                            if _w not in _idx_map:
                                _idx_map[_w] = _low[_w] = _idx_ctr[0]
                                _idx_ctr[0] += 1
                                _t_stack.append(_w)
                                _on_stk.add(_w)
                                _cs.append((_w, iter(_intra_deps_pks(_w))))
                            elif _w in _on_stk:
                                _low[_v] = min(_low[_v], _idx_map[_w])
                        except StopIteration:
                            _cs.pop()
                            if _cs:
                                _low[_cs[-1][0]] = min(_low[_cs[-1][0]], _low[_v])
                            if _low[_v] == _idx_map[_v]:
                                _scc: list = []
                                while True:
                                    _w = _t_stack.pop()
                                    _on_stk.discard(_w)
                                    _scc.append(_w)
                                    if _w == _v:
                                        break
                                _sccs.append(_scc)

                # Set of PKs that are in a true cycle (SCC size > 1)
                _cycle_pks: set = set(
                    _n for _scc in _sccs if len(_scc) > 1 for _n in _scc
                )

                # ── Pre-declare all cycle-member results with zero-init ──────
                # This breaks forward-reference compile errors: later nodes can
                # reference a cycle-member variable even before it is computed.
                # Cycle members are then *reassigned* (no type prefix) during the
                # Kahn emission step below.
                import re as _re
                _predecl_pks: set = set()
                for _pk in _rem_pks:
                    if _pk not in _cycle_pks:
                        continue
                    _c = _pk_to_call[_pk]
                    if _c.results:
                        # Multi-result: extract individual types from tuple<T1,T2,...>
                        _m = _re.match(r'std::tuple<(.+)>', _c.ctype)
                        if _m:
                            _ind_types = [_t.strip() for _t in _m.group(1).split(",")]
                        else:
                            _ind_types = [_c.ctype] * len(_c.results)
                        for _r, _t in zip(_c.results, _ind_types):
                            cls.append(f"    {_t} {_r} = {{}};")
                            ssa[_r] = _r
                    else:
                        cls.append(f"    {_c.ctype} {_c.result} = {{}};")
                        ssa[_c.result] = _c.result
                    _predecl_pks.add(_pk)

                # ── Kahn topological sort for emission order ─────────────────
                # Build in-degree and reverse-dep maps
                _in_deg: dict = {pk: 0 for pk in _rem_pks}
                _dependents: dict = {pk: [] for pk in _rem_pks}
                for _pk in _rem_pks:
                    for _dp in _intra_deps_pks(_pk):
                        if _dp in _pk_to_call:
                            _in_deg[_pk] += 1
                            _dependents[_dp].append(_pk)

                _satisfied: set = set()
                _kahn_order: list = []

                def _kahn_drain():
                    from collections import deque as _deque
                    _q = _deque(sorted(
                        [pk for pk in _rem_pks if pk not in _satisfied and _in_deg[pk] == 0],
                        key=lambda pk: _pk_orig_pos[pk]))
                    while _q:
                        _pk = _q.popleft()
                        if _pk in _satisfied:
                            continue
                        _satisfied.add(_pk)
                        _kahn_order.append(_pk_to_call[_pk])
                        for _dp in _dependents[_pk]:
                            if _dp not in _satisfied:
                                _in_deg[_dp] -= 1
                                if _in_deg[_dp] == 0:
                                    _q.append(_dp)

                # Pre-declared cycle PKs have their results already in ssa, so
                # Kahn's in-degree will drop naturally as if they were already emitted.
                # Seed the satisfied set with pre-declared PKs so Kahn triggers their
                # dependents, but still add them to kahn_order for reassignment.
                for _pk in _rem_pks:
                    if _pk in _predecl_pks:
                        # Reduce in-degrees of dependents (as if already satisfied)
                        for _dp in _dependents[_pk]:
                            if _dp not in _satisfied:
                                _in_deg[_dp] -= 1

                # Now run Kahn's.  Pre-declared PKs start with in_deg already
                # decremented; non-cycle nodes with all deps satisfied start at 0.
                # We still need to emit pre-declared PKs (as reassignments), so we
                # do a separate pass that builds the full emission order.
                _emit_order: list = []

                def _kahn_full():
                    from collections import deque as _dq
                    _q = _dq(sorted(
                        [pk for pk in _rem_pks if pk not in _satisfied and _in_deg[pk] <= 0],
                        key=lambda pk: _pk_orig_pos[pk]))
                    while _q:
                        _pk = _q.popleft()
                        if _pk in _satisfied:
                            continue
                        _satisfied.add(_pk)
                        _emit_order.append(_pk_to_call[_pk])
                        for _dp in _dependents[_pk]:
                            if _dp not in _satisfied:
                                _in_deg[_dp] -= 1
                                if _in_deg[_dp] <= 0:
                                    _q.append(_dp)

                _kahn_full()
                # Any truly stuck nodes (shouldn't happen after pre-decl) — emit anyway
                for _pk in _rem_pks:
                    if _pk not in _satisfied:
                        _emit_order.append(_pk_to_call[_pk])

                def _emit_call_or_reassign(_c) -> None:
                    """Emit call as declaration or reassignment for pre-declared vars."""
                    _pk = _c.results[0] if _c.results else _c.result
                    _args_cpp = [ssa.get(_a, _a) for _a in _c.arg_ids]
                    if _pk in _predecl_pks:
                        # Reassign — variable already declared
                        if _c.results:
                            _binds = ", ".join(_c.results)
                            cls.append(
                                f"    std::tie({_binds}) = {_c.arc_ref}({', '.join(_args_cpp)});")
                        else:
                            cls.append(
                                f"    {_c.result} = {_c.arc_ref}({', '.join(_args_cpp)});")
                        # ssa entries already added during pre-decl; no update needed
                    else:
                        _emit_call(_c)

                for _c in _emit_order:
                    _emit_call_or_reassign(_c)
                    _flush_ready_mem_reads()

            for op in pending_inline:
                if op.result not in ssa:
                    _emit_inline_op(op)
                    _flush_ready_mem_reads()

            # Flush any remaining pending reads (addresses should all be resolved now)
            for r in pending_mem_reads:
                addr = ssa.get(r.addr_id, r.addr_id)
                mem_obj = next((m for m in mod.memories if m.ssa_id == r.mem_id), None)
                mem_name = f"state.{mem_obj.name}" if mem_obj else r.mem_id
                cls.append(f"    {r.word_ctype} {r.result} = {mem_name}[{addr}];")
                ssa[r.result] = r.result

            # Two-phase register update (compute next values, then assign)
            next_vars = []
            for s in states:
                args_cpp = [ssa.get(a, a) for a in s.arg_ids]
                nxt = f"next_{s.reg_name}"
                if s.reset_id and s.enable_id:
                    rst = ssa.get(s.reset_id, s.reset_id)
                    en = ssa.get(s.enable_id, s.enable_id)
                    zero = f"{s.ctype}{{}}" if s.initial_ids else f"({s.ctype})0"
                    cls.append(f"    {s.ctype} {nxt};")
                    cls.append(f"    if ({rst}) {{ {nxt} = {zero}; }}")
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
                w_clk = trace_clock_to_port_name(w.clock_id, mod.calls, mod.in_ports) or w.clock_id
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

            # Output ports — emit only in first clock domain
            if _is_first_clk:
                for i, p in enumerate(mod.out_ports):
                    if i < len(mod.output_ids):
                        v = ssa.get(mod.output_ids[i], mod.output_ids[i])
                        cls.append(f"    state.{p.name} = {v};")

            cls.append("  }")
        cls.append("};")
        parts.append("\n".join(cls))

    return "\n\n".join(parts) + "\n"
