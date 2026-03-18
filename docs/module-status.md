# Module Status

Summary of HIRCT traversal and verification results across 1,603 RTL files.

## Traversal Results

| Metric | Count | Rate |
|--------|-------|------|
| Total RTL files | 1,603 | — |
| MLIR success | 1,230 | 76.7% |
| MLIR fail | 373 | 23.3% |

### Per-Emitter Pass Rates

| Emitter | Pass | Fail | Rate |
|---------|------|------|------|
| gen-model | 1,120 | 483 | 69.9% |
| gen-tb | 1,230 | 373 | 76.7% |
| gen-dpic | 1,085 | 518 | 67.7% |
| gen-wrapper | 1,230 | 373 | 76.7% |
| gen-format | 1,230 | 373 | 76.7% |
| gen-doc | 1,230 | 373 | 76.7% |
| gen-ral | 527 | 373 | 32.9% |
| gen-cocotb | 1,230 | 373 | 76.7% |

## Verification Results (10 seeds × 1000 cycles)

From 1,094 gen-model=pass modules (Task B):

> **Note:** gen-model traversal pass 1,120건은 MLIR→C++ 코드 생성 성공을 의미.
> verify 대상 1,094건은 Verilator 컴파일까지 성공한 모듈만 포함 (차이 26건은 Verilator 호환성 문제).

| Category | Count | Rate |
|----------|-------|------|
| **PASS** | 132 | 12.1% |
| **FAIL** | 192 | 17.6% |
| **SKIP** | 770 | 70.4% |

### SKIP Breakdown (770)

| Cause | Count | Description |
|-------|-------|-------------|
| MODMISSING | 607 | Sub-module RTL not in Verilator include path |
| Undeclared SSA variable | 131 | GenModel SSA variable scoping bug |
| C++ compile error | 21 | GenModel codegen issues (type mismatch, wide-bit) |
| Verilator warning/error | 10 | WIDTHEXPAND, NEEDTIMINGOPT strict mode |
| Other | 1 | Top-module not found |

### FAIL Patterns (192)

| Pattern | Count |
|---------|-------|
| Queue (FIFO) | 59 |
| secded_hamming (ECC) | 38 |
| TileLink/Bus | 19 |
| Divider | 8 |
| Other | 68 |

## VCS Co-simulation (Phase 3)

Three-way comparison: RTL vs C++ model via both Verilator and VCS.

| Module | Verilator | VCS | Verdict |
|--------|-----------|-----|---------|
| Fadu_K2_S5_LevelGateway | **PASS** (10/10) | **PASS** (10/10) | Model correct |
| Fadu_K2_S5_Queue_11 | **FAIL** (0/10) | **FAIL** (0/10) | hirct-gen codegen bug |

Queue_11 failure confirmed as hirct-gen bug (FIFO wrap logic), not a Verilator artifact.

## Known Limitations

See [known-limitations.md](https://github.com/iamwonseok/llvm-cpp-model/blob/main/known-limitations.md)
for the full XFAIL registry including SRAM macros, external IP dependencies,
and LLHD dialect modules.
