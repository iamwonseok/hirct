# HIRCT — HDL Intermediate Representation Compiler & Tools

![Status](https://img.shields.io/badge/status-Phase%203%20(Integration%20%26%20Release)-blue)
![License](https://img.shields.io/badge/license-Apache--2.0%20with%20LLVM%20Exceptions-green)
![C++17](https://img.shields.io/badge/C%2B%2B-17-brightgreen)
![Python](https://img.shields.io/badge/Python-3.10%2B-yellow)

HIRCT is an LLVM/CIRCT-based automation pipeline that takes SystemVerilog/Verilog RTL
as input and produces 8 types of design artifacts automatically. By treating RTL as the
**Single Source of Truth (SSOT)**, all downstream outputs stay in sync with the hardware
design — eliminating manual data duplication and inconsistency.

---

## Quick Start

```bash
# 1. Setup environment
source utils/setup-env.sh        # bash
# (zsh: bash utils/setup-env.sh && source .venv/bin/activate)

# 2. Build
make build

# 3. Generate all 8 artifacts for a module
build/bin/hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v \
    --lib-dir rtl/lib/stubs
# → output/Fadu_K2_S5_LevelGateway/ with 8 artifact directories
```

---

## Generated Artifacts

| Emitter | Output Dir | Description |
|---------|-----------|-------------|
| gen-model | `cmodel/` | C++ cycle-accurate behavioral model |
| gen-tb | `tb/` | SystemVerilog testbench |
| gen-dpic | `dpi/` | DPI-C interface for VCS co-simulation |
| gen-wrapper | `wrapper/` | SystemVerilog interface wrapper |
| gen-format | `rtl/` | IR-based RTL formatter |
| gen-doc | `doc/` | Hardware documentation (Markdown) |
| gen-ral | `ral/` | UVM RAL model + HAL + C driver |
| gen-cocotb | `cocotb/` | Python cocotb testbench |

---

## Verification

```bash
# Equivalence check: C++ model vs RTL (10 seeds × 1000 cycles)
build/bin/hirct-verify rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v \
    --lib-dir rtl/lib/stubs --seeds 10 --cycles 1000

# Full test suite (lit 44/44, gtest 2/2, traversal 1604/1121)
make test-all

# Documentation site
make docs
```

---

## Architecture

```
RTL (.v/.sv)
    │
    ▼
circt-verilog          ← External process (Slang frontend)
    │
    ▼
MLIR (.mlir)           ← hw / comb / seq / arc dialects
    │
    ▼
ModuleAnalyzer         ← HIRCT C++ (port/signal/op extraction)
    │
    ├─► GenModel       → C++ cycle-accurate model
    ├─► GenTestbench   → SV testbench
    ├─► GenDpiC        → DPI-C VCS wrapper
    ├─► GenWrapper     → SV interface wrapper
    ├─► GenFormat      → IR-based RTL formatter
    ├─► GenDoc         → HW documentation
    ├─► GenRal         → UVM RAL + HAL + C driver
    └─► GenCocotb      → Python testbench
```

HIRCT invokes `circt-verilog` and `circt-opt` as **external processes** via `CirctRunner`.
No LLVM/MLIR libraries are linked directly — the C++ codebase is a standalone MLIR text
parser and code generator built with standard C++17.

---

## Tech Stack

| Layer | Technology | Notes |
|-------|-----------|-------|
| Frontend | Slang (SystemVerilog parser) | Via `circt-verilog` |
| Middle-end | LLVM/MLIR, CIRCT | hw/comb/seq/arc dialects |
| Backend | C++17 | `hirct-gen`, `hirct-verify` |
| Verification | Verilator 5.020, VCS | VCS for DPI-C co-simulation |
| Build | CMake + Ninja, Make, lit | Binary build + orchestration + tests |
| Documentation | Markdown, mkdocs-material | `make docs` → `site/` |
| Python | 3.10+ | Utilities only (lit, reports, triage) |

---

## Documentation

- **Documentation site**: `make docs` builds to `site/` (mkdocs-material)
- **Plans & roadmap**: [`docs/plans/summary.md`](docs/plans/summary.md)
- **Proposal**: [`docs/proposal/001-hirct-automation-framework.md`](docs/proposal/001-hirct-automation-framework.md)
- **Convention**: [`docs/plans/hirct-convention.md`](docs/plans/hirct-convention.md)
- **Known limitations**: [`known-limitations.md`](hirct/known-limitations.md)

---

## Project Structure

```
llvm-cpp-model/
├── include/hirct/         # C++ headers (Analysis/, Target/)
├── lib/                   # C++ implementation
├── tools/                 # CLI entry points (hirct-gen, hirct-verify)
├── test/                  # lit/FileCheck unit tests
├── unittests/             # gtest unit tests
├── integration_test/      # E2E integration tests
├── rtl/                   # Verilog source (not tracked in git)
├── utils/                 # setup-env.sh, generate-report.py
├── docs/                  # Plans, proposals, reports
├── vcs-cosim/             # VCS DPI-C co-simulation artifacts
└── Makefile               # Orchestration (setup/build/test/lint/docs)
```

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup, code style, and branch strategy.

---

## License

[Apache License 2.0 with LLVM Exceptions](LICENSE)

Copyright 2026 HIRCT Contributors.
