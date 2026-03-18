# Architecture

## Pipeline

HIRCT processes RTL through a linear pipeline:

```
RTL (.v/.sv)
  → circt-verilog (Slang frontend → MLIR lowering)
  → MLIR (hw/comb/seq/sv dialects)
  → ModuleAnalyzer (port/register/instance extraction)
  → 8 Emitters (gen-model, gen-tb, gen-dpic, gen-wrapper, gen-format, gen-doc, gen-ral, gen-cocotb)
  → Output artifacts per module
```

Multi-module files are handled via `MultiModuleContext`, which resolves
`hw.instance` cross-references and composes sub-module models.

## Source Layout

```
include/hirct/
├── Analysis/       # ModuleAnalyzer, MultiModuleContext headers
└── Target/         # Emitter headers (GenModel, GenTb, etc.)

lib/
├── Analysis/       # ModuleAnalyzer implementation
└── Target/         # Emitter implementations

tools/
├── hirct-gen/      # CLI entry point (main.cpp)
└── hirct-verify/   # Equivalence checker (main.cpp)

test/               # lit/FileCheck functional tests
unittests/          # gtest unit tests
integration_test/   # E2E smoke tests (lit)
utils/              # setup-env.sh, generate-report.py, triage-failures.py
```

## Build System

| Layer | Tool | Purpose |
|-------|------|---------|
| Binary build | CMake + Ninja | Compiles hirct-gen, hirct-verify |
| Orchestration | GNU Make | `make build`, `make generate`, `make test-all` |
| Testing | LLVM lit + FileCheck | Functional tests with XFAIL support |
| Unit testing | GoogleTest | C++ unit tests |
| Verification | Verilator / VCS | RTL-vs-model equivalence checking |

## Key Design Decisions

- **Single Source of Truth**: RTL is the only authoritative source.
  All artifacts are derived, never manually maintained.
- **XFAIL management**: Known failures tracked in `known-limitations.md`.
  CI stays green; XPASS triggers a warning.
- **No shell scripts**: Orchestration uses Makefiles exclusively.
  Only `utils/setup-env.sh` is permitted (idempotent bootstrap).
