# hirct-gen v1 Exporter Scope

> **목적**: v1 export 경로(C model + SystemC wrapper)에서 되는 것과 안 되는 것을 과장 없이 고정한다.
> **기준일**: 2026-04-11 (HEAD: e9a4b85)
> **SSOT 관계**: `known-limitations.md` KL-20과 정합. 아래 내용이 KL-20보다 상세하다.

---

## v1 Supported

| 항목 | 범위 | 테스트 근거 |
|------|------|-------------|
| **입력 형식** | Arc MLIR (`.mlir`) — CIRCT Arc dialect 기반 lowered IR | `test/Tools/hirct-gen/export-cmodel.test` (CombOnly.mlir, CounterArc.mlir) |
| **semantic scope** | single-top / flattened module — `--top`으로 지정하거나 마지막 `hw.module` 자동 선택 | `export-cmodel.test` CHECK-BAD-TOP; `main.cpp:1009-1020` |
| **C model export** | `--export-cmodel` → `<Module>.h` + `<Module>.cpp` (state struct + eval_comb + eval_\<clock\> + initialize) | `export-cmodel.test` CHECK-COMB-H, CHECK-CLK-H |
| **SystemC wrapper export** | `--export-systemc-wrapper` → `<Module>_sc_wrapper.h` + `<Module>_sc_wrapper.cpp` | `export-systemc-wrapper.test` CHECK-WH, CHECK-WI |
| **single-clock wrapper** | `clock_method()` — 단일 `sc_in_clk` 포트 기준 posedge 트리거 | `export-systemc-wrapper.test` CHECK-WH: `sc_in_clk` |
| **<=64-bit wrapper I/O** | `sc_in<sc_uint<N>>` / `sc_out<sc_uint<N>>` — setter/getter 1:1 매핑 | `SystemCWrapperEmitter.cpp:scPortType()` + `eval_method()` |
| **invalid --top rejection** | `--top MissingTop` → 명시적 error exit | `export-cmodel.test` CHECK-BAD-TOP |
| **combinational module** | clock 없는 모듈도 C model export 가능 (eval_comb만 생성) | `export-cmodel.test` CHECK-COMB-H |
| **multi-clock semantic model** | `SemanticModel`은 multi-clock 인식 (clockDomains 복수) | `test/SemanticModel/multi-clock.test` (DualClk) |
| **wide-port semantic model** | `SemanticModel`은 width > 64 포트 인식 | `test/SemanticModel/wide-port.test` (WideMux, i512) |

---

## v1 Unsupported / Deferred

| 항목 | 현재 상태 | 참조 |
|------|-----------|------|
| **hierarchical multi-module export** | XFAIL — `--export-cmodel`은 single-top만 처리. 서브모듈 인스턴스 composition은 legacy `--only model` 경로에서 부분 지원되나 export 경로에서는 미지원 | `test/Target/GenModel/multi-module.test` (XFAIL) |
| **instance topological sort** | XFAIL — 복수 인스턴스 간 평가 순서 결정 미구현 | `test/Target/GenModel/instance-topo-sort.test` (XFAIL) |
| **instance output cross-reference** | XFAIL — 인스턴스 출력을 후속 조합 로직 입력으로 연결 미구현 | `test/Target/GenModel/instance-crossref.test` (XFAIL) |
| **multi-clock wrapper** | `--export-systemc-wrapper`는 multi-clock 모듈에 대해 명시적 error로 거부. `getWrapperV1UnsupportedReason()`이 `clockDomains.size() > 1`이면 reject | KL-20; CLI guard in `main.cpp` |
| **wide wrapper I/O (>64-bit)** | `--export-systemc-wrapper`는 >64-bit 포트가 있는 모듈에 대해 명시적 error로 거부. `getWrapperV1UnsupportedReason()`이 `port.width > 64`이면 reject | KL-20; CLI guard in `main.cpp` |
| **arbitrary hierarchy preservation** | flatten 전제. non-flattened hierarchy를 그대로 보존하는 export 경로 없음 | KL-20 |

---

## CLI Contract

```
hirct-gen [options] <input>

--export-cmodel
  .mlir 입력 필수. .v 입력 시 error.
  single-top export: --top <M> 또는 마지막 hw.module.
  출력: <outputDir>/<Module>.h, <Module>.cpp

--export-systemc-wrapper
  --export-cmodel 필수 (없으면 error).
  v1 범위: single-clock, <=64-bit I/O.
  출력: <outputDir>/<Module>_sc_wrapper.h, <Module>_sc_wrapper.cpp

--top <module>
  존재하지 않는 모듈 이름 시 error exit (silent fallback 없음).
```

---

## 테스트-문서 연결 요약

| 문서 주장 | 근거 테스트 | 상태 |
|-----------|-------------|------|
| C model export 동작 | `export-cmodel.test` | PASS |
| SystemC wrapper export 동작 | `export-systemc-wrapper.test` | PASS |
| invalid --top rejection | `export-cmodel.test` CHECK-BAD-TOP | PASS |
| multi-module hierarchical export 미지원 | `multi-module.test` | XFAIL |
| instance topo sort 미지원 | `instance-topo-sort.test` | XFAIL |
| instance cross-ref 미지원 | `instance-crossref.test` | XFAIL |
| multi-clock SemanticModel 인식 | `multi-clock.test` | PASS (모델 레벨) |
| wide-port SemanticModel 인식 | `wide-port.test` | PASS (모델 레벨) |
| multi-clock wrapper 명시적 거부 | `export-systemc-wrapper.test` CHECK-MC-ERR | PASS (CLI error exit) |
| wide wrapper I/O 명시적 거부 | `export-systemc-wrapper.test` CHECK-WP-ERR | PASS (CLI error exit) |
