# hirct-gen v1 Exporter Scope

> **목적**: v1 export 경로(C model + SystemC wrapper)에서 되는 것과 안 되는 것을 과장 없이 고정한다.
> **기준일**: 2026-04-12
> **HEAD**: `8c60a73` (`test(CModelEmitter): cover aggregate-constant array resets`)
> **SSOT 관계**: `known-limitations.md` KL-20과 정합. 아래 내용이 KL-20보다 상세하다.

---

## v1 Supported

같은 표 안에 있어도 **아래 “Export 경로”는 C model·SystemC wrapper 생성이 실제로 보장되는 범위**이고, **“SemanticModel” 소절은 `%hirct-semantic`으로만 검증되는 모델 표현 능력**이다. 후자는 `--export-cmodel` / `--export-systemc-wrapper` 근거가 아니다. multi-clock·>64-bit I/O의 export 쪽 제약은 `Unsupported` 표를 따른다.

### Export 경로 (C model + SystemC wrapper)

| 항목 | 범위 | 테스트 근거 |
|------|------|-------------|
| **입력 형식 (positive path)** | Arc MLIR (`.mlir`) 입력으로 C model export가 동작한다 | `test/Tools/hirct-gen/export-cmodel.test` (CombOnly.mlir, CounterArc.mlir) — test-backed |
| **입력 형식 (`.mlir` only guard)** | `--export-cmodel`은 `.mlir` 입력만 허용한다. `.v` 입력 시 error exit. 이 제약은 `main.cpp` code guard로 강제되지만 dedicated negative lit는 없다 | `main.cpp` L999–1002 code guard — code-guard-backed |
| **semantic scope (single target export)** | `--export-cmodel`은 단일 모듈을 선택하여 export한다. `--top`으로 지정하거나 마지막 `hw.module`을 선택한다. hierarchy-preserving export는 이 범위 밖이다 | `export-cmodel.test`: happy-path는 single-module `.mlir` 사용 (test-backed). `--top` fallback(마지막 `hw.module` 선택)과 multi-module `.mlir`에서의 단일 선택은 `main.cpp` code path에서 확인 가능하나 dedicated lit 없음 (code-path-backed) |
| **invalid `--top` rejection** | `--top MissingTop` → 명시적 error exit | `export-cmodel.test` CHECK-BAD-TOP — test-backed |
| **C model export** | `--export-cmodel` → `<Module>.h` + `<Module>.cpp` (`<Module>_state`, `_initialize`, `_eval_comb`, `_eval_<clock>`) | `export-cmodel.test` CHECK-COMB-H, CHECK-CLK-H; `export-cmodel-aggregate.test` |
| **SystemC wrapper export** | `--export-systemc-wrapper` → `<Module>_sc_wrapper.h` + `<Module>_sc_wrapper.cpp` | `export-systemc-wrapper.test` CHECK-WH, CHECK-WI |
| **single-clock wrapper** | `clock_method()` — 단일 `sc_in_clk` 포트 기준 posedge 트리거 | `export-systemc-wrapper.test` CHECK-WH: `sc_in_clk` |
| **<=64-bit wrapper I/O** | 1-bit 포트는 `bool`, 그 외 2..64-bit 포트는 `sc_dt::sc_uint<8|16|32|64>` bucket으로 매핑된다 | `SystemCWrapperEmitter.cpp:scPortType()`; `export-systemc-wrapper.test` |
| **combinational module** | clock 없는 모듈도 C model export 가능 (eval_comb만 생성) | `export-cmodel.test` CHECK-COMB-H |

### SemanticModel (`%hirct-semantic`, export 경로와 별개)

| 항목 | 범위 | 테스트 근거 |
|------|------|-------------|
| **multi-clock semantic model** | `SemanticModel.clockDomains`에 복수 클럭 반영 (덤프상 `clocks:` 줄에 나열) | `test/SemanticModel/multi-clock.test`: `%hirct-semantic %s --module DualClk` (export 플래그 없음) |
| **wide-port semantic model** | `SemanticModel`이 width > 64 입력 포트를 인식·덤프 | `test/SemanticModel/wide-port.test`: `%hirct-semantic %s --module WideMux`, `i512` `din` → `width=512` (export 플래그 없음) |

---

## v1 Unsupported / Deferred

| 항목 | 현재 상태 | 참조 |
|------|-----------|------|
| **hierarchical multi-module export** | 현재 `--export-cmodel`은 선택된 단일 모듈만 export한다. 계층 미지원의 직접 근거는 아직 export-path negative fixture가 아니라 legacy `--only model` XFAIL에 주로 의존한다 | `test/Target/GenModel/multi-module.test` (legacy XFAIL, indirect evidence) |
| **instance topological sort** | legacy `--only model` 경로에서 XFAIL — 복수 인스턴스 간 평가 순서 결정 미구현. export-path direct negative fixture는 아직 없음 | `test/Target/GenModel/instance-topo-sort.test` (legacy XFAIL, indirect evidence) |
| **instance output cross-reference** | legacy `--only model` 경로에서 XFAIL — 인스턴스 출력을 후속 조합 로직 입력으로 연결 미구현. export-path direct negative fixture는 아직 없음 | `test/Target/GenModel/instance-crossref.test` (legacy XFAIL, indirect evidence) |
| **multi-clock wrapper** | `--export-systemc-wrapper`는 multi-clock 모듈에 대해 명시적 error로 거부. `getWrapperV1UnsupportedReason()`이 `clockDomains.size() > 1`이면 reject | KL-20; CLI guard in `main.cpp` |
| **wide wrapper I/O (>64-bit)** | `--export-systemc-wrapper`는 >64-bit 포트가 있는 모듈에 대해 명시적 error로 거부. `getWrapperV1UnsupportedReason()`이 `port.width > 64`이면 reject. `scPortType()`에는 `sc_biguint<N>` 분기가 있으나 v1 guard가 먼저 차단하므로 현재 범위에서는 검증되지 않은 dead branch다 | KL-20; CLI guard in `main.cpp`; `SystemCWrapperEmitter.cpp:scPortType()` |
| **arbitrary hierarchy preservation** | flatten 전제. non-flattened hierarchy를 그대로 보존하는 export 경로 없음 | KL-20 |

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
  v1 범위: single-clock, <=64-bit I/O.
  출력: <outputDir>/<Module>_sc_wrapper.h, <Module>_sc_wrapper.cpp

--top <module>
  존재하지 않는 모듈 이름 시 error exit (silent fallback 없음).
```

---

## 테스트-문서 연결 요약

| 문서 주장 | 근거 | evidence class |
|-----------|------|----------------|
| 입력 형식 — `.mlir` positive path | `export-cmodel.test` (CombOnly.mlir, CounterArc.mlir) | test-backed |
| 입력 형식 — `.mlir` only / `.v` reject | `main.cpp` L999–1002 code guard; dedicated negative lit 없음 | code-guard-backed |
| semantic scope — single-module happy path | `export-cmodel.test` happy-path (single-module `.mlir` 사용) | test-backed |
| semantic scope — `--top` fallback, multi-module single selection | `main.cpp` target selection code path; dedicated lit 없음 | code-path-backed |
| invalid `--top` rejection | `export-cmodel.test` CHECK-BAD-TOP | test-backed |
| C model export | `export-cmodel.test`; `export-cmodel-aggregate.test` | test-backed |
| `_initialize` in API | `CModelEmitter.cpp` 항상 생성; lit CHECK에서 `_initialize` 직접 검증 없음 | code-guard-backed |
| SystemC wrapper export | `export-systemc-wrapper.test` | test-backed |
| single-clock wrapper | `export-systemc-wrapper.test` CHECK-WH | test-backed |
| <=64-bit wrapper I/O | `SystemCWrapperEmitter.cpp:scPortType()`; `export-systemc-wrapper.test` (happy-path, 개별 width CHECK 없음) | code-guard-backed |
| combinational module | `export-cmodel.test` CHECK-COMB-H | test-backed |
| multi-clock SemanticModel | `multi-clock.test` (`%hirct-semantic`, **모델 전용**) | test-backed |
| wide-port SemanticModel | `wide-port.test` (`%hirct-semantic`, **모델 전용**) | test-backed |
| hierarchical multi-module export 미지원 | `multi-module.test` (legacy XFAIL) | indirect-legacy-backed |
| instance topo sort 미지원 | `instance-topo-sort.test` (legacy XFAIL) | indirect-legacy-backed |
| instance cross-ref 미지원 | `instance-crossref.test` (legacy XFAIL) | indirect-legacy-backed |
| multi-clock wrapper 거부 | `export-systemc-wrapper.test` CHECK-MC-ERR | test-backed |
| wide wrapper I/O 거부 | `export-systemc-wrapper.test` CHECK-WP-ERR | test-backed |
| `.mlir` 입력 강제 | `main.cpp` code guard; dedicated negative lit 없음 | code-guard-backed |
| wrapper requires cmodel | `main.cpp` code guard; dedicated negative lit 없음 | code-guard-backed |
