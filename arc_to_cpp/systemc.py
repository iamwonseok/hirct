"""SystemC wrapper generator."""
from .parser import HWMod
from .emitter import trace_clock_to_port_name


def emit_systemc_wrapper(mod: HWMod) -> str:
    name = mod.name
    sc_name = f"{name}SC"

    clock_names = {
        trace_clock_to_port_name(s.clock_id, mod.calls, mod.in_ports)
        for s in mod.states
    }
    clock_names |= {
        trace_clock_to_port_name(w.clock_id, mod.calls, mod.in_ports)
        for w in mod.mem_writes
    }

    data_in = [p for p in mod.in_ports if p.name not in clock_names]
    clk_ports = [p for p in mod.in_ports if p.name in clock_names]

    lines = [
        f"// Auto-generated SystemC wrapper for {name}",
        "#pragma once",
        "#include <systemc.h>",
        f'#include "{name}.h"',
        "",
        f"SC_MODULE({sc_name}) {{",
    ]

    for p in mod.in_ports:
        lines.append(f"  sc_in<{p.ctype}>  {p.name};")
    for p in mod.out_ports:
        lines.append(f"  sc_out<{p.ctype}> {p.name};")

    lines += ["", f"  {name} model;", ""]

    for clk in clk_ports:
        lines.append(f"  void eval_{clk.name}() {{")
        for p in data_in:
            lines.append(f"    model.set_{p.name}({p.name}.read());")
        lines.append(f"    model.eval_{clk.name}();")
        for p in mod.out_ports:
            lines.append(f"    {p.name}.write(model.get_{p.name}());")
        lines.append("  }")

    lines += ["", f"  SC_CTOR({sc_name}) {{"]
    for clk in clk_ports:
        lines += [
            f"    SC_METHOD(eval_{clk.name});",
            f"    sensitive << {clk.name}.pos();",
        ]
    lines += ["  }", "};", ""]

    return "\n".join(lines)
