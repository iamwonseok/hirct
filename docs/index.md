# HIRCT — HDL IR Compiler & Tools

HIRCT is an automation pipeline that transforms **SystemVerilog/Verilog RTL**
into **CIRCT IR** and generates 8 types of artifacts for verification, documentation,
and integration.

## Features

HIRCT provides two CLI tools:

- **hirct-gen** — Reads RTL, lowers to MLIR via `circt-verilog`, analyzes the IR,
  and emits artifacts through 8 specialized emitters.
- **hirct-verify** — Runs cycle-accurate equivalence checking between RTL and
  the generated C++ model.

### Emitter Types

| Emitter | Output | Description |
|---------|--------|-------------|
| `gen-model` | C++ behavioral model | Cycle-accurate C++ model of the RTL |
| `gen-tb` | Verilator testbench | Auto-generated testbench for verification |
| `gen-dpic` | DPI-C bridge | SystemVerilog DPI-C interface layer |
| `gen-wrapper` | SV wrapper | Top-level SystemVerilog wrapper |
| `gen-format` | Formatter metadata | Signal format / width information |
| `gen-doc` | Markdown documentation | Per-module port and register docs |
| `gen-ral` | UVM RAL model | Register Abstraction Layer for UVM |
| `gen-cocotb` | cocotb testbench | Python-based cocotb test skeleton |

## Current Status

HIRCT is in **Phase 3 — Integration & Release**.

- **1,603** RTL files processed
- **1,230** successfully lowered to MLIR (76.7%)
- **1,120** C++ models generated (gen-model pass)
- **132** modules pass equivalence verification

## Quick Links

- [Quick Start](quickstart.md) — Get running in 3 steps
- [Architecture](architecture.md) — Pipeline and source layout
- [Module Status](module-status.md) — Detailed traversal and verification statistics
