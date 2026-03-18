# Quick Start

Get HIRCT running in 3 steps.

## 1. Environment Setup

```bash
source utils/setup-env.sh
```

For zsh users where `source` may not activate the venv:

```bash
bash utils/setup-env.sh && source .venv/bin/activate
```

This installs CIRCT/MLIR toolchain, Python dependencies, and configures paths.

## 2. Build

```bash
make build
```

This runs `cmake -B build -G Ninja` followed by `ninja -C build`, producing
`build/bin/hirct-gen` and `build/bin/hirct-verify`.

## 3. Generate Artifacts

Run hirct-gen on a single RTL file:

```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --lib-dir rtl/lib/stubs
```

Output appears in `output/` with the following structure:

```
output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/
└── Fadu_K2_S5_LevelGateway/
    ├── Makefile
    ├── meta.json
    ├── cmodel/
    │   ├── Fadu_K2_S5_LevelGateway.cpp    # C++ model (gen-model)
    │   └── Fadu_K2_S5_LevelGateway.h
    ├── tb/
    │   └── Fadu_K2_S5_LevelGateway_tb.sv  # Testbench (gen-tb)
    ├── dpi/
    │   ├── Fadu_K2_S5_LevelGateway_dpi.sv # DPI-C bridge (gen-dpic)
    │   ├── Fadu_K2_S5_LevelGateway_dpi.cpp
    │   └── Fadu_K2_S5_LevelGateway_dpi.h
    ├── wrapper/
    │   └── Fadu_K2_S5_LevelGateway_wrapper.sv
    ├── rtl/
    │   └── Fadu_K2_S5_LevelGateway.v      # Original RTL copy
    ├── doc/
    │   └── Fadu_K2_S5_LevelGateway.md     # Documentation (gen-doc)
    ├── cocotb/
    │   └── test_Fadu_K2_S5_LevelGateway.py
    ├── ral/
    │   ├── Fadu_K2_S5_LevelGateway_ral.sv
    │   ├── Fadu_K2_S5_LevelGateway_hal.h
    │   └── Fadu_K2_S5_LevelGateway_driver.c
    └── verify/
        └── verify_Fadu_K2_S5_LevelGateway.cpp
```

## Verification

After generating artifacts, verify the C++ model against RTL:

```bash
build/bin/hirct-verify rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v \
  --lib-dir rtl/lib/stubs --seeds 10 --cycles 1000
```

## Full Traversal

To process all 1,603 RTL files and generate a report:

```bash
make generate
make report
```

Results are written to `output/report.json`.
