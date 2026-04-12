# M1 Flatten-First Exporter Scope Freeze Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 이미 flatten/lower된 single-top MLIR 입력 또는 명시적 pipeline을 거친 입력을 기준으로, 현재 exporter 능력과 남은 hardening 범위를 분리해 `M1` 범위를 고정한다.

**Architecture:** `M1`은 새 architecture를 도입하는 단계가 아니라 flatten-first exporter를 “신뢰 가능한 현재 기반”으로 굳히는 단계다. 이미 닫힌 reset/wide/array/non-port hardening은 증거를 모아 고정하고, 아직 남은 gap은 artifact contract, CLI contract, regression coverage 기준으로 다시 분류한다.

**Tech Stack:** Markdown, `llvm-lit` or equivalent lit runner, generated `lit.site.cfg.py` from a configured build tree, `hirct-gen`, direct export tests, existing legacy `GenModel` regressions.

---

## Working Rules

- `M1`은 flattened single-top exporter만 다룬다.
- 입력은 이미 flatten/lower된 MLIR이거나, 필요 시 사용자가 명시적으로 pipeline을 지정한 경우로 한정한다.
- hierarchy-preserving 방향성은 문서에서 언급만 하고 구현 계획은 `M3`로 넘긴다.
- direct export acceptance와 indirect hardening background를 구분한다.
- wide, array, non-port reset 관련 regression은 “현재 기반이 어디까지 단단한지”를 보여주는 간접 hardening background로 쓴다.
- 새 bugfix가 필요해 보여도 이 문서에서는 설계와 분류만 한다.

## Task 0: Verify Test Runner Preflight

**Files:**
- Read: local toolchain state

**Step 1: Check whether `llvm-lit` is available**

Run: `llvm-lit --version`

Expected: version prints, or the missing runner is recorded as a preflight blocker.

**Step 2: Check fallback runner if needed**

Run: `python3 -m lit --version`

Expected: version prints, or fallback is also missing and later verification steps are marked blocked.

**Step 3: Locate generated lit harness**

Run: `rg -n "hirct_gen_path|hirct_obj_root|filecheck_path" <chosen-build-test-lit-site-cfg>`

Expected: generated `lit.site.cfg.py` exists and exposes `%hirct-gen` substitution inputs.

**Step 4: Record chosen lit runner**

Expected: later verification steps use `<chosen-lit-runner>` consistently together with one configured build-tree lit entrypoint, or the milestone is marked verification-blocked.

## Scope

**In scope**
- flatten-first small module export baseline
- current artifact contract
- current CLI contract
- hardening 완료/미완료 분리
- `M1` acceptance 기준 정의

**Out of scope**
- module composition contract
- hierarchical artifact generation
- host ABI
- top-level wrapper integration redesign

## Task 1: Read M1 Boundary

**Files:**
- Read: `hirct/hirct/docs/plan/2026-04-12-v1-export-roadmap.md`
- Read: `hirct/hirct/docs/v1-scope.md`

**Step 1: Locate M1 section in roadmap**

Run: `rg -n "## M1\\.|flatten-first|small module" hirct/hirct/docs/plan/2026-04-12-v1-export-roadmap.md`

Expected: M1 objective is visible.

**Step 2: Read current truthful contract**

Run: `rg -n "C model export|SystemC wrapper export|Unsupported|CLI Contract" hirct/hirct/docs/v1-scope.md`

Expected: current baseline rows to freeze are visible.

## Task 2: Inventory Core Export Entry Points

**Files:**
- Read: `hirct/hirct/tools/hirct-gen/main.cpp`
- Read: `hirct/hirct/lib/Target/CModelEmitter.cpp`
- Read: `hirct/hirct/lib/Target/SystemCWrapperEmitter.cpp`

**Step 1: Locate C model export entry**

Run: `rg -n "export_cmodel|CModelEmitter|emit\\(" hirct/hirct/tools/hirct-gen/main.cpp hirct/hirct/lib/Target/CModelEmitter.cpp`

Expected: current artifact generation path is visible.

**Step 2: Locate wrapper export entry**

Run: `rg -n "export-systemc-wrapper|getWrapperV1UnsupportedReason" hirct/hirct/tools/hirct-gen/main.cpp hirct/hirct/lib/Target/SystemCWrapperEmitter.cpp`

Expected: wrapper generation path and guards are visible.

## Task 3: Freeze C Model Artifact Contract

**Files:**
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel-aggregate.test`
- Read: `hirct/hirct/lib/Target/CModelEmitter.cpp`
- Modify: `hirct/hirct/docs/plan/2026-04-12-m1-flatten-scope-freeze-plan.md`

**Step 1: Identify generated file pair**

Run: `rg -n "CHECK-COMB-H|CHECK-CLK-H|\\.h|\\.cpp" hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`

Expected: file-level artifact evidence is visible.

**Step 2: Identify required exported members**

Run: `rg -n "_initialize|_eval_comb|_eval_|typedef struct .*_state" hirct/hirct/lib/Target/CModelEmitter.cpp`

Expected: current externally visible generated API shape is visible.

**Step 3: Write the M1 artifact contract section**

Modify: this file.

Expected: M1 artifact contract names what must exist and what is intentionally not promised yet.

## M1 C Model Artifact Contract (Frozen)

> 이 섹션은 현재 코드와 테스트가 실제로 보장하는 범위를 기록한다.
> 입력은 이미 flatten/lower된 single-top MLIR이다. M1은 자동 flatten을 포함하지 않는다.

### Generated File Pair

| Artifact | Path Pattern |
|---|---|
| Header | `{outputDir}/{moduleName}.h` |
| Implementation | `{outputDir}/{moduleName}.cpp` |

### State Struct

```
typedef struct {Mod}_state { ... } {Mod}_state;
```

| Member Pattern | Condition | Evidence |
|---|---|---|
| `input_{port}` | per narrow input port | **code-path-backed** — CModelEmitter `emitHeader` always generates; no lit CHECK directly verifies individual input port struct members |
| `input_{port}[N]` | per wide input port (`uint64_t` word array) | **code-path-backed** — CModelEmitter `emitHeader` always generates; no lit CHECK directly verifies individual wide input word-array members |
| `output_{port}` | per narrow scalar output port | **code-path-backed** — CModelEmitter `emitHeader` always generates; no lit CHECK directly verifies individual scalar output port struct members |
| `output_{port}[numElements]` | per aggregate output port | test-backed (`export-cmodel-aggregate.test CHECK-H`) |
| state variables | per register | **code-path-backed** — CModelEmitter `emitHeader` generates per register; `CHECK-CLK-H` verifies struct existence but not individual state variable names |
| aggregate state arrays | per array register | test-backed (`CHECK-MEM-H`) |
| `memory_0[N]` | per `firrtl.mem` | test-backed (`CHECK-MEM-H`: `memory_0[16]`) |

### Required API — Always Generated

| Function Signature | Purpose | Evidence Class |
|---|---|---|
| `void {Mod}_initialize({Mod}_state *s);` | zero-init all state | **code-path-backed** — CModelEmitter always emits; no lit CHECK |
| `void {Mod}_eval_comb({Mod}_state *s);` | evaluate combinational logic | **test-backed** — `CHECK-COMB-H`, `CHECK-COMB-CPP` |

### Required API — Per Clock Domain

| Function Signature | Purpose | Evidence Class |
|---|---|---|
| `void {Mod}_eval_{clockDomain}({Mod}_state *s);` | advance one clock domain | **test-backed** — `CHECK-CLK-H`, `CHECK-CLK-CPP`, `CHECK-MEM-H`, `CHECK-MEM-CPP` |

### Required API — Per Input Port

| Function Signature | Condition | Evidence Class |
|---|---|---|
| `void {Mod}_set_{port}({Mod}_state *s, {type} v);` | narrow input | **code-path-backed** |
| `void {Mod}_set_{port}_word({Mod}_state *s, size_t idx, uint64_t v);` | wide input | **code-path-backed** |
| `void {Mod}_set_{port}_words({Mod}_state *s, const uint64_t *src, size_t count);` | wide input | **code-path-backed** |

### Required API — Per Output Port

| Function Signature | Condition | Evidence Class |
|---|---|---|
| `{type} {Mod}_get_{port}(const {Mod}_state *s);` | narrow scalar output | **code-path-backed** |
| `void {Mod}_get_{port}(const {Mod}_state *s, {elemType} *dst, size_t count);` | aggregate output | **code-path-backed** — getter 자체의 lit CHECK 없음; `export-cmodel-aggregate.test CHECK-CPP`는 eval 경로의 `memcpy` 갱신만 검증 |
| `uint64_t {Mod}_get_{port}_word(const {Mod}_state *s, size_t idx);` | wide output | **code-path-backed** |
| `size_t {Mod}_get_{port}_word_count(void);` | wide output | **code-path-backed** |

### Test Evidence References

| Test File | CHECK Prefix | Covers |
|---|---|---|
| `export-cmodel.test` | `CHECK-COMB-H` | `CombOnly_state`, `eval_comb` (combinational) |
| `export-cmodel.test` | `CHECK-COMB-CPP` | `CombOnly_eval_comb` |
| `export-cmodel.test` | `CHECK-CLK-H` | `CounterArc_state`, `eval_comb` (clocked) |
| `export-cmodel.test` | `CHECK-CLK-CPP` | `CounterArc_eval_comb` |
| `export-cmodel.test` | `CHECK-MEM-H` | `FirmemBasic_state`, `memory_0[16]`, `eval_clock` (memory) |
| `export-cmodel.test` | `CHECK-MEM-CPP` | `FirmemBasic_eval_clock`, `memory_0[` |
| `export-cmodel.test` | `CHECK-BAD-TOP` | error on missing `--top` |
| `export-cmodel-aggregate.test` | `CHECK-H` | `output_q[4]` (array output, not scalar) |
| `export-cmodel-aggregate.test` | `CHECK-CPP` | `memcpy(s->output_q, s->arr_reg, sizeof(s->output_q))` |
| `export-cmodel-aggregate.test` | *(compile proof)* | `c++ -fsyntax-only` on generated code |

### M1이 약속하지 않는 것 (Explicitly NOT Promised)

| Item | Reason |
|---|---|
| `_initialize` 의미론 정확성 | lit CHECK 없음; code-path-backed만 |
| Wide input/output API 의미론 정확성 | 전용 lit CHECK 없음; code-path-backed만 |
| 개별 width-bucket 매핑 정확성 | lit CHECK 없음 |
| unsupported op 의미론 | `/* unsupported:... */0`으로 emit — 컴파일은 되지만 의미가 틀릴 수 있음 |
| `validateModuleModel` export path 호출 | export path에서 호출되지 않음 |
| 자동 flatten/lower | M1 입력은 이미 flatten/lower된 MLIR; auto-flatten은 scope 밖 |

## Task 4: Freeze Wrapper Artifact Contract

**Files:**
- Read: `hirct/hirct/test/Tools/hirct-gen/export-systemc-wrapper.test`
- Read: `hirct/hirct/lib/Target/SystemCWrapperEmitter.cpp`
- Modify: this file

**Step 1: Identify generated wrapper file pair**

Run: `rg -n "CHECK-WH|CHECK-WI|_sc_wrapper" hirct/hirct/test/Tools/hirct-gen/export-systemc-wrapper.test`

Expected: wrapper artifact pair is visible.

**Step 2: Identify required emitted wrapper structure**

Run: `rg -n "SC_MODULE|clock_method|eval_method|sc_in|sc_out" hirct/hirct/lib/Target/SystemCWrapperEmitter.cpp`

Expected: current wrapper API shape is visible.

**Step 3: Write the M1 wrapper contract section**

Modify: this file.

Expected: wrapper scope is frozen to current v1 limits.

## M1 Wrapper Artifact Contract (Frozen)

> 이 섹션은 현재 코드와 테스트가 실제로 보장하는 SystemC wrapper 범위를 기록한다.
> 입력은 이미 flatten/lower된 single-top MLIR이다. M1은 자동 flatten을 포함하지 않는다.
> Wrapper는 C model을 전제로 한다 — `--export-systemc-wrapper`는 항상 `--export-cmodel`을 먼저 요구한다.

### Generated File Pair

| Artifact | Path Pattern |
|---|---|
| Wrapper Header | `{outputDir}/{moduleName}_sc_wrapper.h` |
| Wrapper Implementation | `{outputDir}/{moduleName}_sc_wrapper.cpp` |

### Wrapper Header Structure (`SC_MODULE`)

```
#include "{moduleName}.h"
#include <systemc>

SC_MODULE({moduleName}_sc_wrapper) {
    sc_in_clk {clockPortName};            // if module has clock
    sc_in<{scPortType}> {portName};       // per non-clock input port
    sc_out<{scPortType}> {portName};      // per output port
    {moduleName}_state state_;            // embeds C model state
    void eval_method();
    void clock_method();                  // if module has clock
    {wrapperName}(sc_module_name name);
};
```

| Member | Condition | Evidence Class |
|---|---|---|
| `sc_in_clk {clockPortName}` | module has clock | **test-backed** — `CHECK-WH: sc_in_clk` |
| `sc_in<{scPortType}> {portName}` | per non-clock input | **code-path-backed** — `CHECK-WH`에 `sc_in<>` CHECK 없음 |
| `sc_out<{scPortType}> {portName}` | per output port | **code-path-backed** — `CHECK-WH`에 `sc_out<>` CHECK 없음 |
| `{moduleName}_state state_` | always | **code-path-backed** — `CHECK-WH`에 `state_` CHECK 없음 |
| `eval_method()` | always | **test-backed** — `CHECK-WI: eval_method` |
| `clock_method()` | module has clock | **test-backed** — `CHECK-WI: clock_method` |
| Constructor | always | **test-backed** — `CHECK-WH: SC_MODULE(CounterArc_sc_wrapper)` |

### SC Port Type Mapping (`scPortType`)

| Port Width | SC Type | Evidence Class |
|---|---|---|
| width <= 1 | `bool` | **code-path-backed** — `SystemCWrapperEmitter.cpp:32-44` |
| width <= 8 | `sc_dt::sc_uint<8>` | **code-path-backed** |
| width <= 16 | `sc_dt::sc_uint<16>` | **code-path-backed** |
| width <= 32 | `sc_dt::sc_uint<32>` | **code-path-backed** |
| width <= 64 | `sc_dt::sc_uint<64>` | **code-path-backed** |
| width > 64 | `sc_dt::sc_biguint<N>` | **dead branch** — v1 guard가 wide port를 먼저 reject |

> 개별 width-bucket 매핑에 대한 lit CHECK는 없다. Happy-path 테스트(`CounterArc`)는 구조만 검증한다.

### Wrapper Implementation — Constructor

| Behavior | Condition | Evidence Class |
|---|---|---|
| `_initialize(&state_)` 호출 | always | **code-path-backed** — `CHECK-WI`에 `_initialize` CHECK 없음 |
| `SC_METHOD(clock_method)` sensitive to `{clockPort}.pos()` | module has clock | **code-path-backed** — `CHECK-WI`에 `SC_METHOD`/sensitivity CHECK 없음 |
| `SC_METHOD(eval_method)` sensitive to all non-clock inputs | always | **code-path-backed** — `CHECK-WI`에 `SC_METHOD`/sensitivity CHECK 없음 |

### Wrapper Implementation — `eval_method()`

| Step | Description | Evidence Class |
|---|---|---|
| 1 | SC input ports → `_set_{port}` calls | **code-path-backed** |
| 2 | `_eval_comb(&state_)` | **code-path-backed** — `CHECK-WI`는 `eval_method` 존재만 검증; 내부 `eval_comb` 호출 CHECK 없음 |
| 3 | Write outputs back to SC output ports | **code-path-backed** |

### Wrapper Implementation — `clock_method()`

| Step | Description | Evidence Class |
|---|---|---|
| 1 | Read all SC ports (including clock) → `_set_{port}` calls | **code-path-backed** |
| 2 | `_eval_{cd}(&state_)` per clock domain | **code-path-backed** — `CHECK-WI`는 `clock_method` 존재만 검증; 내부 `eval_{cd}` 호출 CHECK 없음 |
| 3 | `_eval_comb(&state_)` | **code-path-backed** |
| 4 | Write outputs back to SC output ports | **code-path-backed** |

### v1 Guards (`getWrapperV1UnsupportedReason`)

| Condition | Behavior | Evidence Class |
|---|---|---|
| `clockDomains.size() > 1` | reject — error message | **test-backed** — `CHECK-MC-ERR` |
| Any port `width > 64` | reject — error message | **test-backed** — `CHECK-WP-ERR` |

### Test Evidence References

| Test File | CHECK Prefix | Covers |
|---|---|---|
| `export-systemc-wrapper.test` | `CHECK-WH` | `SC_MODULE`, `CounterArc_sc_wrapper`, `sc_in_clk` (3 CHECK lines만) |
| `export-systemc-wrapper.test` | `CHECK-WI` | `eval_method`, `clock_method` (2 CHECK lines만) |
| `export-systemc-wrapper.test` | `CHECK-MC-ERR` | multi-clock rejection error message |
| `export-systemc-wrapper.test` | `CHECK-WP-ERR` | wide port rejection error message |

### M1이 약속하지 않는 것 — Wrapper (Explicitly NOT Promised)

| Item | Reason |
|---|---|
| `sc_biguint<N>` wrapper 동작 | v1 guard가 wide port를 먼저 reject — dead branch |
| 개별 width-bucket SC 타입 정확성 | lit CHECK 없음; code-path-backed만 |
| Multi-clock wrapper 동작 | v1 guard가 먼저 reject |
| Aggregate/array port wrapper 동작 | v1 guard가 wide port를 reject; aggregate 전용 경로 없음 |
| `eval_method`/`clock_method` 내 port read/write 정확성 | 구조 CHECK만; 의미론 검증 없음 |
| 자동 flatten/lower | M1 입력은 이미 flatten/lower된 MLIR; auto-flatten은 scope 밖 |

## Task 5: Freeze CLI Contract

**Files:**
- Read: `hirct/hirct/tools/hirct-gen/main.cpp`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-systemc-wrapper.test`
- Modify: this file

**Step 1: Locate `--export-cmodel` contract points**

Run: `rg -n -- "--export-cmodel|outputDir|top" hirct/hirct/tools/hirct-gen/main.cpp`

Expected: C model CLI behavior is visible.

**Step 2: Locate `--export-systemc-wrapper` contract points**

Run: `rg -n -- "--export-systemc-wrapper|export-cmodel|required|unsupported" hirct/hirct/tools/hirct-gen/main.cpp`

Expected: wrapper CLI behavior is visible.

**Step 3: Add exact CLI promises to the plan**

Modify: this file.

Expected: M1 does not promise any CLI behavior not already enforced.

---

## M1 CLI Contract (Frozen)

> Frozen 2026-04-12. Documents **current** behavior only — no aspiration.
> Input to `--export-cmodel` is already flatten/lowered MLIR; M1 does NOT auto-flatten.

### Flag Behavior

| Flag | Behavior | Evidence |
|------|----------|----------|
| `--export-cmodel` | Requires `.mlir` input; creates output dir via `mkdir_p`; finds `hw.module` ops; emits C model artifacts; prints success message; returns 0. Does NOT enter the general pipeline (no meta.json, no `--only`). | **test-backed**: `export-cmodel.test` (happy path, `CHECK-BAD-TOP`) |
| `--export-systemc-wrapper` | Must be paired with `--export-cmodel`; after C model written, runs `getWrapperV1UnsupportedReason()` gate then emits SystemC wrapper artifacts. | **test-backed**: `export-systemc-wrapper.test` |
| `--top <module>` | If non-empty: `SymbolTable::lookup` → error if not found. If empty: selects last `hw.module` in walk order. | **test-backed** (with `--export-cmodel`): `CHECK-BAD-TOP` in `export-cmodel.test`. **code-path-backed** (empty → last module). |
| `-o <dir>` | Output directory for all artifacts. Default: `"output"`. | **test-backed**: exercised in `export-cmodel.test` |

### Guards / Error Conditions

| Condition | Behavior | Evidence |
|-----------|----------|----------|
| Non-`.mlir` input + `--export-cmodel` | Error and exit | **code-guard-backed** (L1000-1002 in main.cpp); no dedicated negative lit |
| No `hw.module` in input | Error and exit | **code-guard-backed** (L1013-1016 in main.cpp) |
| `--top <name>` not found | Error: module not found | **test-backed**: `CHECK-BAD-TOP` |
| `--export-systemc-wrapper` without `--export-cmodel` | Error and exit | **code-guard-backed** (L994-996 in main.cpp); no dedicated negative lit |
| `getWrapperV1UnsupportedReason()` fails | Error with specific message (multi-clock or >64-bit port) | **test-backed**: `CHECK-MC-ERR`, `CHECK-WP-ERR` in `export-systemc-wrapper.test`; code anchor: L1049-1053 in main.cpp |

### NOT Promised by M1

These behaviors exist in code but lack dedicated lit coverage. They are **observed**, not **contracted**:

- `.v` input rejection for `--export-cmodel` — code-guard-backed only
- Standalone `--export-systemc-wrapper` rejection — code-guard-backed only
- `--top` without `--export-cmodel` error path — code-path-backed only (L1079-1084)
- Multi-module `.mlir` last-module selection — code-path-backed only
- No auto-flatten in `--export-cmodel` path — input must already be flatten/lowered MLIR
- No pipeline integration (meta.json, `--only`, etc.) when `--export-cmodel` is active

---

## Task 6: Inventory Already-Hardened Register Semantics

**Files:**
- Read: `hirct/hirct/test/Target/GenModel/wide-constant-crash.test`
- Read: `hirct/hirct/test/Target/GenModel/wide-reg-multi-clock.test`
- Read: `hirct/hirct/test/Target/GenModel/async-reset-wide-reg.test`
- Read: `hirct/hirct/test/Target/GenModel/async-reset-wide-reg-multi-clock.test`
- Read: `hirct/hirct/test/Target/GenModel/sync-reset-narrow-reg.test`
- Read: `hirct/hirct/test/Target/GenModel/sync-reset-narrow-reg-multi-clock.test`
- Read: `hirct/hirct/test/Target/GenModel/non-port-reset-sig.test`
- Read: `hirct/hirct/test/Target/GenModel/non-port-reset-sig-multi-clock.test`
- Read: `hirct/hirct/test/Target/GenModel/sync-reset-array-reg.test`
- Read: `hirct/hirct/test/Target/GenModel/sync-reset-array-reg-multi-clock.test`
- Read: `hirct/hirct/test/Target/GenModel/sync-reset-array-reg-agg-const.test`

**Step 1: Group wide-register regression proofs**

Run: `rg -n "wide|reset|domain_step|async" hirct/hirct/test/Target/GenModel/wide-constant-crash.test hirct/hirct/test/Target/GenModel/wide-reg-multi-clock.test hirct/hirct/test/Target/GenModel/async-reset-wide-reg.test hirct/hirct/test/Target/GenModel/async-reset-wide-reg-multi-clock.test`

Expected: wide-register hardening evidence is visible.

**Step 2: Group narrow-register regression proofs**

Run: `rg -n "reset|next_|if \\(" hirct/hirct/test/Target/GenModel/sync-reset-narrow-reg.test hirct/hirct/test/Target/GenModel/sync-reset-narrow-reg-multi-clock.test`

Expected: narrow-register reset evidence is visible.

**Step 3: Group non-port reset proofs**

Run: `rg -n "rst_sig_|reset" hirct/hirct/test/Target/GenModel/non-port-reset-sig.test hirct/hirct/test/Target/GenModel/non-port-reset-sig-multi-clock.test`

Expected: non-port reset evidence is visible.

**Step 4: Group array reset proofs**

Run: `rg -n "array|aggregate|reset" hirct/hirct/test/Target/GenModel/sync-reset-array-reg.test hirct/hirct/test/Target/GenModel/sync-reset-array-reg-multi-clock.test hirct/hirct/test/Target/GenModel/sync-reset-array-reg-agg-const.test`

Expected: array-reset evidence is visible.

## Task 7: Separate Closed Hardening From Open Hardening

**Files:**
- Modify: this file

**Step 1: Create a `Closed In M1` list**

Expected: already-covered hardening items are listed with test references.

**Step 2: Create an `Open In M1` list**

Expected: only current flatten-first reliability gaps remain.

**Step 3: Create an `Out Of M1` list**

Expected: composition, hierarchy, host ABI, and top integration are moved out.

### Closed In M1

Items with test-backed evidence in M1. Two categories: **direct export acceptance**
(tests that exercise the `--export-cmodel` / `--export-systemc-wrapper` export path) and **indirect hardening
background** (legacy `--only model` tests that cover register-level code generation semantics,
including reset, wide registers, and arrays — not the direct export entry point, so they add
background confidence without standing in for acceptance coverage).

#### Direct export acceptance

| # | Item | Test reference |
|---|------|----------------|
| 1 | C model export (combinational) | `export-cmodel.test` CHECK-COMB-* |
| 2 | C model export (clocked) | `export-cmodel.test` CHECK-CLK-* |
| 3 | C model export (memory) | `export-cmodel.test` CHECK-MEM-* |
| 4 | C model export (aggregate output) | `export-cmodel-aggregate.test` + compile proof |
| 5 | Invalid `--top` rejection | `export-cmodel.test` CHECK-BAD-TOP |
| 6 | SystemC wrapper (single-clock, <=64-bit) | `export-systemc-wrapper.test` CHECK-WH, CHECK-WI |
| 7 | Multi-clock wrapper rejection | `export-systemc-wrapper.test` CHECK-MC-ERR |
| 8 | Wide port wrapper rejection | `export-systemc-wrapper.test` CHECK-WP-ERR |

#### Indirect hardening background

| # | Item | Test reference |
|---|------|----------------|
| 9 | Wide register word-array storage | `wide-constant-crash.test`, `wide-reg-multi-clock.test` (2 files) |
| 10 | Async/sync reset for wide registers | `async-reset-wide-reg.test`, `async-reset-wide-reg-multi-clock.test` (2 files) |
| 11 | Sync reset for narrow registers | `sync-reset-narrow-reg.test`, `sync-reset-narrow-reg-multi-clock.test` (2 files) |
| 12 | Non-port reset signal handling | `non-port-reset-sig.test`, `non-port-reset-sig-multi-clock.test` (2 files) |
| 13 | Array register reset | `sync-reset-array-reg.test`, `sync-reset-array-reg-multi-clock.test`, `sync-reset-array-reg-agg-const.test` (3 files) |

**Total Closed: 13 items** (8 direct export acceptance, 5 indirect hardening categories covering 11 individual GenModel test files — see Task 10 Step 4 for the canonical file list)

### Open In M1

Current flatten-first reliability gaps — code-path-backed or code-guard-backed only,
no dedicated lit test.

| # | Item | Evidence level |
|---|------|----------------|
| 1 | `.v` input rejection | code-guard-backed, no dedicated negative lit |
| 2 | Standalone `--export-systemc-wrapper` rejection | code-guard-backed, no negative lit |
| 3 | `--top` without `--export-cmodel` | code-path-backed, no lit |
| 4 | Multi-module `.mlir` last-module selection | code-path-backed, no lit |
| 5 | `_initialize` semantics | code-path-backed only |
| 6 | Wide input/output API semantics | code-path-backed only |
| 7 | Individual SC port type mapping | code-path-backed only |
| 8 | eval_method/clock_method internal execution correctness | code-path-backed only |

**Total Open: 8 items**

### Out Of M1

Items requiring new design or implementation — pushed to M2+ or a separate batch.

| # | Item | Rationale |
|---|------|-----------|
| 1 | Module composition contract | New design needed |
| 2 | Hierarchical artifact generation / hierarchy-preserving export | New design needed |
| 3 | Host ABI concerns | New design needed |
| 4 | Top-level wrapper integration redesign | New design needed |
| 5 | Multi-clock wrapper SUPPORT (beyond current v1 guard rejection) | New implementation needed |
| 6 | Wide port wrapper SUPPORT (beyond current v1 guard rejection) | New implementation needed |
| 7 | `sc_biguint<N>` dead branch activation | New implementation needed |
| 8 | Auto-flatten pipeline in CLI | New implementation needed |
| 9 | Pipeline integration (meta.json, --only) when export is active | New implementation needed |

**Total Out: 9 items**

---

## Task 8: Define M1 Acceptance Fixture Set

**Files:**
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-systemc-wrapper.test`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel-aggregate.test`
- Modify: this file

**Step 1: Pick minimum positive export fixtures**

Expected: at least one comb-only and one clocked export fixture are named.

**Step 2: Pick minimum negative wrapper fixtures**

Expected: multi-clock and wide-port reject fixtures are named.

**Step 3: Pick one generated-code compile proof**

Expected: direct `--export-cmodel` compile proof is called out explicitly, preferably `export-cmodel-aggregate.test`.

## M1 Acceptance Fixture Set

### Minimum Positive Export Fixtures

| # | Fixture | What it proves | Test file |
|---|---------|----------------|-----------|
| 1 | Combinational-only C model export | export works without clock | `export-cmodel.test` (CombOnly.mlir) |
| 2 | Clocked C model export | export works with clock domain | `export-cmodel.test` (CounterArc.mlir) |
| 3 | Memory C model export | export works with seq.firmem | `export-cmodel.test` (FirmemBasic.mlir) |
| 4 | Aggregate output C model export | array output + compile proof | `export-cmodel-aggregate.test` (AggMod) |
| 5 | SystemC wrapper (single-clock, <=64-bit) | wrapper generation works | `export-systemc-wrapper.test` (CounterArc.mlir) |

### Minimum Negative Fixtures

| # | Fixture | What it proves | Test file |
|---|---------|----------------|-----------|
| 1 | Invalid --top rejection | non-existent module → error | `export-cmodel.test` CHECK-BAD-TOP |
| 2 | Multi-clock wrapper rejection | multi-clock → explicit error | `export-systemc-wrapper.test` CHECK-MC-ERR |
| 3 | Wide port wrapper rejection | >64-bit port → explicit error | `export-systemc-wrapper.test` CHECK-WP-ERR |

### Generated-Code Compile Proof

| # | Fixture | What it proves | Test file |
|---|---------|----------------|-----------|
| 1 | `c++ -fsyntax-only` on AggMod | generated C++ compiles cleanly | `export-cmodel-aggregate.test` |

### Indirect Hardening Background Fixtures (NOT acceptance — context only)

| # | Category | Count | Test files |
|---|----------|-------|-----------|
| 1 | Wide register | 4 | `wide-constant-crash.test`, `wide-reg-multi-clock.test`, `async-reset-wide-reg.test`, `async-reset-wide-reg-multi-clock.test` |
| 2 | Narrow register | 2 | `sync-reset-narrow-reg.test`, `sync-reset-narrow-reg-multi-clock.test` |
| 3 | Non-port reset | 2 | `non-port-reset-sig.test`, `non-port-reset-sig-multi-clock.test` |
| 4 | Array reset | 3 | `sync-reset-array-reg.test`, `sync-reset-array-reg-multi-clock.test`, `sync-reset-array-reg-agg-const.test` |

> Direct export acceptance와 indirect hardening background는 다른 표로 분리된다.
> Direct export acceptance는 `--export-cmodel` / `--export-systemc-wrapper` 경로를 검증한다.
> Indirect hardening background는 legacy `--only model` 회귀 테스트로 register-level 코드 생성 의미(reset, wide, array 등)를 다루며, direct export와는 다른 진입점을 통한 배경 신뢰도를 보완한다.
> M1 acceptance 판정은 positive + negative + compile proof (direct export) 기준이다.
> Indirect hardening은 모델 기반 신뢰도를 보여주지만 M1 acceptance의 필수 조건은 아니다.
> 단, `M1 closed` exit에는 indirect hardening 전부 PASS가 포함된다 (M1 Done Statement 조건 5 참조).

---

## Task 9: Define M1 Exit Conditions

**Files:**
- Modify: this file

**Step 1: Write the M1 done statement**

Expected: “small flattened exporter is trustworthy” is expressed as objective criteria.

**Step 2: Write the M1 not-done statement**

Expected: unresolved issues that block moving to M2 are explicit.

## M1 Exit Conditions

### M1 Done Statement

M1 is done when ALL of the following are true:

1. **C model artifact contract is frozen** — `M1 C Model Artifact Contract (Frozen)` section exists with complete API shape and evidence classes
2. **Wrapper artifact contract is frozen** — `M1 Wrapper Artifact Contract (Frozen)` section exists with structure, guards, and evidence classes
3. **CLI contract is frozen** — `M1 CLI Contract (Frozen)` section exists with flag behavior, guards, and NOT promised items
4. **Direct export acceptance fixtures ALL PASS** — `export-cmodel.test`, `export-cmodel-aggregate.test`, `export-systemc-wrapper.test` all pass under configured build-tree lit harness
5. **Indirect hardening background regressions ALL PASS** — all 11 GenModel test files pass under configured build-tree lit harness. These are **context, not blocking** criteria — see Conditional Closure Rule below.
   - Wide register: 4 files
   - Narrow register: 2 files
   - Non-port reset: 2 files
   - Array reset: 3 files
   - Canonical file list: Task 10 Step 4
6. **Closed/Open/Out classification is complete** — all items classified with no ambiguous items
7. **No M2+ work mixed into M1 document** — Out Of M1 items are clearly deferred

### M1 Not-Done Statement

M1 is NOT done if ANY of the following:

1. **Any direct export acceptance fixture FAILS** — blocks M1 closure
2. **Artifact contracts are missing or incomplete** — no frozen contract sections
3. **CLI contract is missing or incomplete**
4. **Closed/Open/Out classification has ambiguous items**
5. **M2+ work is mixed into M1** — new capabilities, hierarchy support, etc.

### Conditional Closure Rule

- If lit runner is unavailable or `lit.site.cfg.py` is not generated (source-tree-only state): mark `M1 verification-blocked`
- In this case, document/contract freeze can proceed but milestone closure is conditional only
- After configured build-tree lit harness is available, re-run Task 10 as a separate verification batch to declare `M1 closed`

### M1 Closure Verdicts

| Verdict | Meaning |
|---------|---------|
| `M1 closed` | All exit conditions met, all tests pass |
| `M1 verification-blocked` | Contracts frozen, but lit verification incomplete |
| `M1 still open` | One or more exit conditions not met |

## Task 10: Run Focused Verification

Before any step, define `LLVM_LIT` and `BUILD_TEST_ROOT` from **Task 0** preflight results.

- `LLVM_LIT` — Task 0 Step 4의 `<chosen-lit-runner>`에 해당한다.
  - Task 0 Step 1(`llvm-lit --version`)이 성공했으면 그 바이너리의 **절대 경로**를 넣는다.
  - 실패 시 Task 0 Step 2(`python3 -m lit`)를 폴백으로 쓸 수 있으나, `python3 -m lit`은 공백이 포함된 복합 명령이므로 이 변수에 넣지 않는다. 그 경우 아래 명령에서 `"$LLVM_LIT"`을 `python3 -m lit`으로 직접 치환해서 실행한다.
- `BUILD_TEST_ROOT` — Task 0 Step 3에서 확인한 generated `lit.site.cfg.py`가 위치한 디렉터리의 **절대 경로** (예: `<build-tree>/test`).

```bash
export LLVM_LIT="/absolute/path/to/llvm-lit"
export BUILD_TEST_ROOT="/absolute/path/to/build/test"
```

All commands below use `--filter` with `"$BUILD_TEST_ROOT"` as the lit suite path. This keeps discovery inside the build tree and avoids the "warning in tests" that occurs when mixing `lit.site.cfg.py` with source-tree test file paths.

**Files:**
- Run against referenced M1 tests

**Step 1: Run export C model tests**

Run:

```
"$LLVM_LIT" -sv --filter="export-cmodel.test" "$BUILD_TEST_ROOT"
```

Expected: PASS, or the run is skipped because the preflight blocker was recorded.

**Step 2: Run direct export compile proof**

Run:

```
"$LLVM_LIT" -sv --filter="export-cmodel-aggregate.test" "$BUILD_TEST_ROOT"
```

Expected: PASS, or the run is skipped because the preflight blocker was recorded.

**Step 3: Run wrapper export tests**

Run:

```
"$LLVM_LIT" -sv --filter="export-systemc-wrapper.test" "$BUILD_TEST_ROOT"
```

Expected: PASS, or the run is skipped because the preflight blocker was recorded.

**Step 4: Run core hardening regressions**

Run:

```
"$LLVM_LIT" -sv --filter="wide-constant-crash|wide-reg-multi-clock|async-reset-wide-reg|sync-reset-narrow-reg|non-port-reset-sig|sync-reset-array-reg" "$BUILD_TEST_ROOT"
```

Expected: PASS for all 11 indirect hardening background tests, or the run is skipped because the preflight blocker was recorded. These are legacy `--only model` proofs, not direct export acceptance.

## Conditional Closure Rule

- lit runner가 없거나 generated `lit.site.cfg.py`가 없는 source-tree 단독 상태이면 `M1 verification-blocked`로 표기한다.
- 이 경우 문서/계약 freeze 자체는 진행할 수 있지만, milestone closure는 `조건부`로만 선언한다.
- configured build-tree lit harness 확보 후 Task 10을 별도 verification batch로 다시 실행해야 `M1 closed`를 선언할 수 있다.

## Completion Criteria

- M1 artifact contract is documented for C model and wrapper separately
- current CLI promises are frozen
- direct export acceptance fixtures and indirect hardening background are separated
- already-closed hardening items and remaining gaps are separated
- acceptance fixture set is explicit
- `M2+` work is not mixed into the M1 document
