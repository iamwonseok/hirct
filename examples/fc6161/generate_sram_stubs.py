#!/usr/bin/env python3
"""SRAM stub generator: hw_out.mlir → stub .v files.

Rules:
- Module name contains 'sp', 'spsram', 'utpsram', or NxMwK pattern
- Has ports: clk, cen, wen, a, d, q (or q_delayed1)
- Generates synchronous SRAM: posedge-clk write + read, cen active-low
- ECC wrappers: bit_error=0, uecc_error=0
"""
import re, sys
from pathlib import Path

def parse_modules(mlir_text: str) -> dict:
    """Return {module_name: [(direction, port_name, bits)]}"""
    modules = {}
    for m in re.finditer(
        r'hw\.module\s+(?:\w+\s+)*@(\w+)\(([^)]*)\)',
        mlir_text
    ):
        name = m.group(1)
        ports = []
        # in ports use %name, out ports use bare name (no %)
        for p in re.finditer(r'(in|out)\s+%?(\w+)\s*:\s*i(\d+)', m.group(2)):
            ports.append((p.group(1), p.group(2), int(p.group(3))))
        if ports:
            modules[name] = ports
    return modules

def classify_sram(name: str, ports: list) -> str | None:
    """Return 'sram' | 'ecc_sram' | None."""
    pnames = {p[1] for p in ports}
    has_sram_iface = (
        {'clk', 'cen', 'wen', 'a', 'd'} <= pnames and
        ('q' in pnames or 'q_delayed1' in pnames)
    )
    if not has_sram_iface:
        return None
    n = name.lower()
    if any(x in n for x in ['ecc_od1', 'ecc_ssd']):
        return 'ecc_sram'
    if any(x in n for x in ['spsram', 'utpsram', '_sp_']) or re.search(r'\d+x\d+w', n):
        return 'sram'
    return None

def depth_from_name(name: str, addr_bits: int) -> int:
    """Extract depth from module name (e.g. 103x458w1 → 103), else 2^addr_bits."""
    m = re.search(r'(\d+)x(\d+)w', name)
    return int(m.group(1)) if m else (1 << addr_bits)

def make_port_decl(direction: str, name: str, bits: int) -> str:
    kw = 'input' if direction == 'in' else 'output'
    if bits == 1:
        return f'    {kw} wire        {name}'
    return f'    {kw} wire [{bits-1}:0] {name}'

def generate_stub(name: str, ports: list, stype: str) -> str:
    pmap = {p[1]: (p[0], p[2]) for p in ports}
    addr_bits = pmap['a'][1]
    data_bits = pmap['d'][1]
    depth = depth_from_name(name, addr_bits)
    q_name = 'q_delayed1' if 'q_delayed1' in pmap else 'q'
    q_bits = pmap[q_name][1]

    port_decls = ',\n'.join(make_port_decl(*p) for p in ports)
    ecc_assigns = (
        '    assign bit_error  = 1\'b0;\n'
        '    assign uecc_error = 1\'b0;\n'
        if stype == 'ecc_sram' else ''
    )

    return (
        f'// AUTO-GENERATED STUB — delta-cycle fold (C-model)\n'
        f'// {name}: {depth} x {data_bits}b synchronous SRAM\n'
        f'module {name} (\n{port_decls}\n);\n'
        f'    reg [{data_bits-1}:0] mem [0:{depth-1}];\n'
        f'    reg [{q_bits-1}:0]   q_r;\n'
        f'    always @(posedge clk) begin\n'
        f'        if (!cen && wen)  mem[a] <= d;\n'
        f'        if (!cen)         q_r   <= mem[a];\n'
        f'    end\n'
        f'    assign {q_name} = q_r;\n'
        f'{ecc_assigns}'
        f'endmodule\n'
    )

def generate_all(mlir_path: Path, out_dir: Path) -> dict[str, Path]:
    """Scan mlir_path, generate stubs for all SRAM modules, return {name: stub_path}."""
    text = mlir_path.read_text(errors='replace')
    modules = parse_modules(text)
    out_dir.mkdir(exist_ok=True)
    stubs = {}
    for name, ports in modules.items():
        stype = classify_sram(name, ports)
        if stype:
            stub_v = out_dir / f'{name}.v'
            stub_v.write_text(generate_stub(name, ports, stype))
            stubs[name] = stub_v
            print(f'  Generated: {stub_v.name}')
    return stubs

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Usage: generate_sram_stubs.py <hw_out.mlir> <stubs_dir>')
        sys.exit(1)
    stubs = generate_all(Path(sys.argv[1]), Path(sys.argv[2]))
    print(f'Generated {len(stubs)} stubs.')
