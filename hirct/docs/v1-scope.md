# hirct-gen v1 Exporter Scope

> **목적**: v1 export 경로(C model + SystemC wrapper)에서 되는 것과 안 되는 것을 과장 없이 고정한다.
> **기준일**: 2026-04-13 (M4 closure sync)
> **Revision Note**: commit-specific `HEAD` 값은 빠르게 stale 될 수 있으므로, 이 문서는 현재 검증된 v1 contract를 기준으로 유지한다.
> **SSOT 관계**: 이 문서가 v1 scope의 **detailed contract**(SSOT)다. `known-limitations.md` KL-20은 요약 포인터로, 상세는 여기를 참조한다.

---

## v1 Supported

같은 표 안에 있어도 **아래 “Export 경로”는 C model·SystemC wrapper 생성이 실제로 보장되는 범위**이고, **“SemanticModel” 소절은 `%hirct-semantic`으로만 검증되는 모델 표현 능력**이다. 후자는 `--export-cmodel` / `--export-systemc-wrapper` 근거가 아니다. multi-clock·>64-bit I/O의 export 쪽 제약은 `Unsupported` 표를 따른다.

### Export 경로 (C model + SystemC wrapper)

| 항목 | 범위 | 테스트 근거 |
|------|------|-------------|
| **입력 형식 (positive path)** | Arc MLIR (`.mlir`) 입력으로 C model export가 동작한다 | `test/Tools/hirct-gen/export-cmodel.test` (CombOnly.mlir, CounterArc.mlir) — test-backed |
| **입력 형식 (`.mlir` only guard)** | `--export-cmodel`은 `.mlir` 입력만 허용한다. `.v` 입력 시 error exit. 이 제약은 `main.cpp` code guard로 강제되지만 dedicated negative lit는 없다 | `main.cpp` L1000–1002 (`!is_mlir_input` 거부) — code-guard-backed |
| **semantic scope (single target export)** | `--export-cmodel`은 단일 모듈을 선택하여 export한다. `--top`으로 지정하거나 마지막 `hw.module`을 선택한다. hierarchy-preserving export는 이 범위 밖이다 | `export-cmodel.test`: happy-path는 single-module `.mlir` 사용 (test-backed). `--top` fallback(마지막 `hw.module` 선택)과 multi-module `.mlir`에서의 단일 선택은 `main.cpp` code path에서 확인 가능하나 dedicated lit 없음 (code-path-backed) |
| **invalid `--top` rejection** | 존재하지 않는 모듈 이름 → 명시적 error exit (silent fallback 없음) | `--export-cmodel`와 함께일 때: `export-cmodel.test` CHECK-BAD-TOP — test-backed. `--export-cmodel` 없이 동일 lookup을 타는 경로는 `main.cpp`와 동일 메시지이나 dedicated negative lit 없음 — code-path-backed |
| **C model export** | `--export-cmodel` → `<Module>.h` + `<Module>.cpp` (`<Module>_state`, `_eval_comb`, `_eval_<clock>`; `_initialize`는 `CModelEmitter.cpp`에서 항상 생성되나 lit CHECK에서 직접 검증 없음 — code-path-backed) | `export-cmodel.test` CHECK-COMB-H, CHECK-COMB-CPP, CHECK-CLK-H, CHECK-CLK-CPP, CHECK-MEM-H, CHECK-MEM-CPP; `export-cmodel-aggregate.test` CHECK-H, CHECK-CPP + `c++ -fsyntax-only` compile proof |
| **SystemC wrapper export** | `--export-systemc-wrapper` → `<Module>_sc_wrapper.h` + `<Module>_sc_wrapper.cpp` | `export-systemc-wrapper.test` CHECK-WH, CHECK-WI |
| **single-clock wrapper** | `clock_method()` — 단일 `sc_in_clk` 포트 기준 posedge 트리거 | `export-systemc-wrapper.test` CHECK-WH: `sc_in_clk` |
| **<=64-bit wrapper I/O** | 1-bit 포트는 `bool`, 그 외 2..64-bit 포트는 `sc_dt::sc_uint<8\|16\|32\|64>` bucket으로 매핑된다 | `SystemCWrapperEmitter.cpp:scPortType()`; `export-systemc-wrapper.test` (happy-path 구조만 CHECK, 개별 width 타입 CHECK 없음) |
| **combinational module** | clock 없는 모듈도 C model export 가능 (eval_comb만 생성) | `export-cmodel.test` CHECK-COMB-H |

### SemanticModel (`%hirct-semantic`, export 경로와 별개)

| 항목 | 범위 | 테스트 근거 |
|------|------|-------------|
| **multi-clock semantic model** | `SemanticModel.clockDomains`에 복수 클럭 반영 (덤프상 `clocks:` 줄에 나열) | `test/SemanticModel/multi-clock.test`: `%hirct-semantic %s --module DualClk` (export 플래그 없음) |
| **wide-port semantic model** | `SemanticModel`이 width > 64 입력 포트를 인식·덤프 | `test/SemanticModel/wide-port.test`: `%hirct-semantic %s --module WideMux`, `i512` `din` → `width=512` (export 플래그 없음) |

---

### Export 경로 — Hierarchical (M3, `--export-cmodel`)

| 항목 | 범위 | 테스트 근거 |
|------|------|-------------|
| **hierarchical multi-module export** | `--export-cmodel` 경로에서 hierarchy traversal + per-module artifact emission. leaf-first emit, dedup, cycle reject, parent-child integration, topo ordering 지원 | `multi-module.test` (passing); `export-cmodel-hierarchy.test` Batch 1·2·3 — test-backed |
| **instance topological sort** | Kahn's algorithm 기반 topo sort를 codegen eval_comb/eval_clock 호출 순서에 통합. 직접 def-use 기반 DAG edge만 감지 | `instance-topo-sort.test` (passing); `export-cmodel-hierarchy.test` CHECK-B3-ORD-CPP — test-backed |
| **instance output cross-reference** | staged binding map으로 child eval_comb 후 출력을 local에 bind하여 cross-module reference 해결 | `instance-crossref.test` (passing); `export-cmodel-hierarchy.test` CHECK-B3-XREF — test-backed |
| **hierarchy-preserving export** | flatten 없이 parent/child를 개별 artifact로 export하고 계층 구조 유지. parent header가 child header를 include하고 child state를 embed | `export-cmodel-hierarchy.test` Batch 1·2·3; compile-check passing — test-backed |

### Export 경로 — Top-Level SystemC Wrapper Integration (M4)

| 항목 | 범위 | 테스트 근거 |
|------|------|-------------|
| **hierarchical root wrapper** | `--export-systemc-wrapper`는 hierarchical C model export의 root 모듈에 대해서도 wrapper를 생성한다. wrapper는 root의 C model API(`_initialize`, `_eval_comb`, `_eval_{clock}`)를 호출하며, child module은 C model backend artifact로 유지된다 | `export-hierarchy-wrapper.test` CHECK-M4-WH, CHECK-M4-WI — test-backed |
| **comb-only hierarchical root wrapper** | clock이 없는 hierarchical root에도 wrapper 생성 가능 (eval_method만 존재) | `export-hierarchy-wrapper.test` CHECK-M4-COMB-WH; `export-cmodel-hierarchy.test` CHECK-HIER-WRAP-H — test-backed |
| **top-only wrapper policy** | wrapper는 root(final integration top)에만 생성된다. child module별 wrapper는 생성하지 않는다 | `export-hierarchy-wrapper.test` `test ! -f Adder8_sc_wrapper.h` — test-backed |

---

## v1 Unsupported / Deferred

| 항목 | 현재 상태 | 참조 |
|------|-----------|------|
| **wide cross-module binding** | hierarchical export에서 scalar port만 지원. wide port(>64-bit)의 cross-module binding은 `_word`/`_words` API variant를 통해야 하며 M3 scope에서 deferred | M3 plan Task 7·10; `renderExpr`의 `/* wide_port */0` fallback 동작과 일관 |
| **indirect comb-chain dependency** | instance 간 직접 def-use만 topo edge로 형성. parent의 `comb.*` 연산 체인을 경유하는 간접 의존은 감지하지 않으며 순서를 보장하지 않음 | M3 plan Task 4; `IRAnalysis.cpp` edge 판별이 직접 연결만 감지 |
| **multi-clock hierarchy** | hierarchical export는 single-clock 대상만 지원. multi-clock hierarchy는 deferred | M3 plan Task 9 scope 제한 |
| **sequential cut** | instance ordering에서 registered output(seq.compreg)을 통한 sequential cut 분석 미지원. IR-level SSA def-use만으로 edge를 판별하며, port-level timing attribute 도입 후 지원 예정 | M3 plan Task 4 Sequential Cut 섹션 |
| **multi-clock wrapper** | `--export-systemc-wrapper`는 multi-clock 모듈에 대해 명시적 error로 거부. `getWrapperV1UnsupportedReason()`이 `clockDomains.size() > 1`이면 reject | `export-systemc-wrapper.test` CHECK-MC-ERR (test-backed); KL-19·KL-20 |
| **wide wrapper I/O (>64-bit)** | `--export-systemc-wrapper`는 >64-bit 포트가 있는 모듈에 대해 명시적 error로 거부. `getWrapperV1UnsupportedReason()`이 `port.width > 64`이면 reject. `scPortType()`에는 `sc_biguint<N>` 분기가 있으나 v1 guard가 먼저 차단하므로 현재 범위에서는 검증되지 않은 dead branch다 | `export-systemc-wrapper.test` CHECK-WP-ERR (test-backed); KL-19·KL-20; `SystemCWrapperEmitter.cpp:scPortType()` |

---

## CLI Contract

```
hirct-gen [options] <input>

--export-cmodel
  `.mlir` 입력 필수. `.v` 입력 시 error.
  이 제약은 현재 `main.cpp` code guard로 존재하며 dedicated negative lit는 아직 없다.
  single-top export: --top <M> 또는 마지막 hw.module.
  출력: <outputDir>/<Module>.h, <Module>.cpp

--export-systemc-wrapper
  --export-cmodel 필수 (없으면 error).
  이 제약은 현재 `main.cpp` code guard로 존재하며 dedicated negative lit는 아직 없다.
  hierarchical root(top)에 대해서도 wrapper 생성 가능.
  child module별 wrapper는 생성하지 않고, root wrapper가 root C model API를 호출한다.
  v1 범위: single-clock, <=64-bit I/O.
  출력: <outputDir>/<Module>_sc_wrapper.h, <Module>_sc_wrapper.cpp

--top <module>
  존재하지 않는 모듈 이름 시 error exit (silent fallback 없음).
  `--export-cmodel`와 함께 사용할 때의 invalid `--top`는 `export-cmodel.test`(CHECK-BAD-TOP)로 검증된다.
  그 외 플래그 조합에서도 `main.cpp`에서 동일하게 거부되나, 그에 대한 dedicated negative lit는 아직 없다 (code-path-backed).
```

---

## 테스트-문서 연결 요약

| 문서 주장 | 근거 | evidence class |
|-----------|------|----------------|
| 입력 형식 — `.mlir` positive path | `export-cmodel.test` (CombOnly.mlir, CounterArc.mlir) | test-backed |
| 입력 형식 — `.mlir` only / `.v` reject | `main.cpp` L1000–1002 code guard; dedicated negative lit 없음 | code-guard-backed |
| semantic scope — single-module happy path | `export-cmodel.test` happy-path (single-module `.mlir` 사용) | test-backed |
| semantic scope — `--top` fallback, multi-module single selection | `main.cpp` target selection code path; dedicated lit 없음 | code-path-backed |
| invalid `--top` rejection (`--export-cmodel` 경로) | `--export-cmodel` + 존재하지 않는 `--top` → error exit | test-backed |
| invalid `--top` rejection (기타 경로) | `main.cpp` L1079–1084 동일 거부 로직; dedicated negative lit 없음 | code-path-backed |
| C model export (`_state`, `_eval_comb`, `_eval_<clock>`) | `export-cmodel.test` CHECK-COMB-H/CPP, CHECK-CLK-H/CPP, CHECK-MEM-H/CPP; `export-cmodel-aggregate.test` CHECK-H/CPP + compile proof | test-backed |
| `_initialize` in API | `CModelEmitter.cpp` 항상 생성; lit CHECK에서 `_initialize` 직접 검증 없음 | code-path-backed |
| SystemC wrapper export | `export-systemc-wrapper.test` | test-backed |
| single-clock wrapper | `export-systemc-wrapper.test` CHECK-WH | test-backed |
| <=64-bit wrapper I/O | `SystemCWrapperEmitter.cpp:scPortType()`; `export-systemc-wrapper.test` (happy-path 구조만 CHECK, 개별 width 타입 CHECK 없음) | code-guard-backed |
| combinational module | `export-cmodel.test` CHECK-COMB-H | test-backed |
| multi-clock SemanticModel | `multi-clock.test` (`%hirct-semantic`, **모델 전용**) | test-backed |
| wide-port SemanticModel | `wide-port.test` (`%hirct-semantic`, **모델 전용**) | test-backed |
| hierarchical multi-module export | `multi-module.test` (passing); `export-cmodel-hierarchy.test` Batch 1·2·3 | test-backed |
| instance topological sort | `instance-topo-sort.test` (passing); `export-cmodel-hierarchy.test` CHECK-B3-ORD-CPP | test-backed |
| instance output cross-reference | `instance-crossref.test` (passing); `export-cmodel-hierarchy.test` CHECK-B3-XREF | test-backed |
| multi-clock wrapper 거부 | `export-systemc-wrapper.test` CHECK-MC-ERR | test-backed |
| wide wrapper I/O 거부 | `export-systemc-wrapper.test` CHECK-WP-ERR | test-backed |
| wrapper requires cmodel | `main.cpp` L994–997 code guard; dedicated negative lit 없음 | code-guard-backed |
