# GenModel IR 입력 조건 및 Cycle-Accurate CModel 설계 브레인스토밍

> 작성일: 2026-03-06
> 목적: 다음 세션에서 이어서 진행할 두 가지 설계 주제의 컨텍스트와 미결 사항을 정리한다.

---

## 세션 배경

이번 세션에서 `2026-03-06-hirct-output-spec-survey.md`를 리뷰하면서 현재 문서에 없는 두 가지 핵심 스펙 공백을 확인했다:

1. **CIRCT pipeline 각 단계별 IR 변화 비교** — 어떤 IR 수준이어야 GenModel이 C++ 코드를 생성할 수 있는가
2. **Cycle-accurate CModel에 필요한 IR 필수 정보 명세** — 진입 조건, 멀티 클럭/리셋, DAG 순회 알고리즘

코드 (`hirct/lib/Target/GenModel.cpp`, `hirct/lib/Analysis/IRAnalysis.cpp`, `hirct/lib/Target/EmitExpr.cpp`)를 직접 읽어서 현재 구현된 것과 없는 것을 실제로 확인했다.

---

## 현재 코드에서 확인한 실제 상태

### IR 진입 조건 (현재)

`GenModel::emit()`에서 하는 유일한 진입 체크:
```cpp
// llhd.prb / llhd.drv 잔존 여부만 검사
// ProcessOp/WaitOp/HaltOp/YieldOp/CombinationalOp은 허용 (GenModel이 직접 처리)
if (residual_llhd_ops > 0) → skip (경고 출력 후 반환)
```

**문제**: 공식 precondition 명세 없음. 이 단일 체크 외에는 검증 없음.

### 멀티 클럭 (현재 구현됨)

`IRAnalysis.cpp`의 `trace_clock_to_port()`:
```cpp
// seq.firreg / seq.compreg의 clock 오퍼랜드를 BlockArgument까지 역추적
// seq.ToClockOp 투과, 첫 번째 operand 따라가기 (fallback)
```

**설계 결함**: clock이 internal mux/gate를 통해 오는 경우 `getOperand(0)` fallback이 틀릴 수 있음.
**현재 상태**: `is_multi_clock` 플래그로 `step_<clock>()` 분리 생성은 되어 있음 (KL 미해소 아님).

### DAG 순회 (현재 구현됨)

`EmitExpr.cpp`의 `emit_op_expr()`:
- **지원 op**: `comb.*` 전체 + `hw.ConstantOp`, `hw.BitcastOp`, `hw.ArrayCreateOp`, `hw.ArrayGetOp`, `hw.AggregateConstantOp`
- **미지원 op** → `"\x01"` sentinel 반환 → 상위에서 처리 또는 스킵
- **`hw.array_inject` 없음** → KL-16 원인

### 클럭/리셋 식별 (현재 구현됨, 부분적 결함)

`IRAnalysis.cpp`:
```cpp
// is_clock_port(): 이름 휴리스틱 (clk, clock, pclk, _clk suffix 등)
// is_reset_port(): 이름 휴리스틱 (rst, reset, rst_n 등)
// is_active_low_reset(): 이름 suffix _n / n 으로만 판단
```

**설계 결함**: `seq.firreg`의 reset 오퍼랜드 극성을 IR에서 직접 읽지 않고 이름 suffix만 사용.
올바른 방법: `FirRegOp::getResetValue()`가 0이면 active-high reset, 초기값이면 active-low 등 IR 기반 판단.

---

## 두 주제 상세

### 주제 1: CIRCT Pipeline 단계별 IR 변화 비교

**목표**: 각 pass 통과 후 IR이 어떻게 바뀌는지, GenModel 진입 가능 여부가 언제 결정되는지 문서화.

현재 파이프라인:
```
import (Moore) → moore-to-core → llhd-to-core
→ [HIRCT pipeline]
  sim_cleanup → unroll_process_loops → RemoveControlFlow → Canonicalizer
  → process_flatten → process_deseq → signal_lowering → CSE → Canonicalizer
→ GenModel (emitter)
```

**확인해야 할 것**:
- 각 pass 전후 `--mlir-print-ir-after-all`로 IR dump 비교
- `llhd.process` → `hw/comb/seq`로 완전히 변환되는 시점
- `seq.firreg` vs `seq.compreg` 잔존 조건
- 멀티 클럭 모듈에서 clock 오퍼랜드 trace 경로
- **CIRCT 오픈소스에 이미 있는 것**: `ExportSystemC`(SystemC 런타임 의존), `Arc`(JIT, 소스 없음) — HIRCT와 목적이 다름

**오픈소스 참조 후보**:
- **OpenTitan** (`github.com/lowRISC/opentitan`): RISC-V SoC RTL. CIRCT 기반 툴체인 사용. `hw.module` 계층 구조, 클럭 도메인 패턴 참조 가능.
- **Caliptra** (`github.com/chipsalliance/caliptra-rtl`): RISC-V RoT. SystemVerilog 기반. DPI-C 사용 패턴 참조 가능.
- **CIRCT Arc dialect**: `arc.define` / `arc.call` 구조. 상태 전이 함수 추출 패턴이 GenModel `step()` 설계와 유사.

### 주제 2: Cycle-Accurate CModel에 필요한 IR 필수 정보 명세

**목표**: 문서로 정의된 공식 IR 진입 조건 + 필수 정보 체크리스트.

**현재 확인된 필수 정보** (`IRAnalysis.cpp` 코드 기반):

| 정보 | IR 소스 | 현재 구현 | 문제/개선 필요 |
|------|---------|----------|--------------|
| 포트 목록/방향/비트폭 | `hw.module` args | ○ 완성 | — |
| 클럭 포트 식별 | 이름 휴리스틱 + `seq.to_clock` | ○ 동작 | `seq.to_clock` 없으면 이름만 의존 |
| 리셋 포트 식별 + 극성 | 이름 suffix | △ 불완전 | IR 기반 극성 판단 필요 |
| 레지스터 (이름/비트폭/리셋값) | `seq.firreg`, `seq.compreg` | ○ 완성 | — |
| 클럭 도메인 → 레지스터 매핑 | `trace_clock_to_port()` | △ fallback 위험 | internal mux 경로 처리 미완 |
| 메모리 (depth/width/RW포트) | `seq.firmem.*` | ○ 완성 | — |
| 조합 로직 DAG | `comb.*` SSA | △ 일부 미지원 | `hw.array_inject` KL-16 |
| 서브모듈 인스턴스 + 연결 | `hw.instance` | ○ 완성 | 위상 정렬 (Kahn's algorithm) |
| 넓은 비트 (65+) | `iN (N>64)` | [X] truncate | KL-3 WideInt pass 미구현 |
| 잔존 llhd.process | lowering 미완 | △ skip 처리 | KL-5, KL-10, KL-11 |

**설계해야 할 것** (현재 없는 것):

1. **공식 IR Precondition Checklist** — GenModel 진입 전 validate 단계:
   - 잔존 LLHD op 종류 검사 (현재: prb/drv만, 개선: 모든 LLHD op 분류)
   - `iN (N>64)` 타입 포트/레지스터 존재 여부 체크
   - `hw.array_inject` 존재 여부 체크

2. **Clock domain trace 알고리즘 개선** — `getOperand(0)` fallback 제거:
   - `seq.ToClockOp` → `BlockArgument`까지 strict하게 추적
   - internal clock gate (`comb.and` 등) 통과 시 처리 방식 결정

3. **Reset 극성 판단 IR 기반 전환**:
   - `FirRegOp::getReset()`의 ooerand value 분석
   - `FirRegOp::getResetValue()` 확인으로 active-low/high 판단

4. **`emit_op_expr()` 미지원 op 처리 전략**:
   - sentinel `"\x01"` 반환 → 상위 에러 처리 통일
   - `hw.array_inject` 코드 생성 추가 (KL-16 해소)
   - WideInt (65+비트) 표현 전략 결정 (KL-3)

---

## 미결 질문 (다음 세션에서 결정할 것)

### Q1: 두 주제 중 우선순위
- **A**: 주제 1 먼저 — pipeline 단계별 IR dump 실험/문서화
- **B**: 주제 2 먼저 — precondition checklist + 알고리즘 명세 설계 문서
- **C**: 병렬 — 탐색(주제 1)과 설계(주제 2) 동시 진행

> **상태 (2026-03-08)**: `2026-03-08-hirct-product-strategy.md`에서 상위 전략 수준으로 논의됨. GenModel은 사내 시뮬레이터 연동 목적 유지, CXXRTL을 중간 단계로 활용하는 전략 채택. IR 스펙 작업은 전략 확정 후 재개.

### Q2: 주제 1 접근 방식
- `--mlir-print-ir-after-all` 실험으로 직접 dump 비교?
- CIRCT 소스코드 (Arc/ExportSystemC) 읽기?
- OpenTitan/Caliptra RTL로 실제 파이프라인 통과 테스트?

> **상태 (2026-03-08)**: `2026-03-08-hirct-product-strategy.md`에서 상위 전략 수준으로 논의됨. IR 스펙 작업은 전략 확정 후 재개.

### Q3: 주제 2 산출물 형태
- 설계 문서 (`docs/plans/2026-03-06-genmodel-ir-precondition.md` — 다음 세션 생성 예정) 작성?
- 코드로 바로 구현 (`IRAnalysis.cpp` 개선 + 새 precondition 체크)?
- 두 가지 모두?

> **상태 (2026-03-08)**: `2026-03-08-hirct-product-strategy.md`에서 상위 전략 수준으로 논의됨. IR 스펙 작업은 전략 확정 후 재개.

### Q4: CIRCT 오픈소스 참조 범위
- Arc dialect의 `arc.define`/`state-flow` 구조를 GenModel `step()` 설계에 반영할 것인가?
- ExportSystemC의 `EmissionPrinter` 패턴을 `emit_op_expr()` 구조에 참고할 것인가?

> **상태 (2026-03-08)**: `2026-03-08-hirct-product-strategy.md`에서 상위 전략 수준으로 논의됨. IR 스펙 작업은 전략 확정 후 재개.

---

## 다음 세션 실행 프롬프트

> **참고 (2026-03-08)**: 아래 프롬프트를 실행하기 전에 `2026-03-08-hirct-product-strategy.md`의 미결 결정(H-1~H-4)이 확정되었는지 먼저 확인한다.

```
@docs/plans/2026-03-06-genmodel-ir-spec-brainstorm.md 를 기반으로 두 가지 설계 주제를 계속 진행해줘.

목표: HIRCT GenModel의 IR 입력 조건과 cycle-accurate CModel 생성을 위한 필수 IR 정보를
      공식 설계 문서로 만들고, 필요시 코드 개선까지 진행한다.

참조 파일:
- @hirct/lib/Target/GenModel.cpp (현재 구현, 1,925줄)
- @hirct/lib/Analysis/IRAnalysis.cpp (분석 유틸리티)
- @hirct/lib/Target/EmitExpr.cpp (DAG → C++ 표현식 변환)
- @hirct/lib/Transforms/Pipeline.cpp (lowering pipeline)
- @docs/plans/2026-03-06-hirct-output-spec-survey.md (갱신된 스펙 문서)

우선 미결 질문 Q1~Q4에 대해 /brainstorm 진행 후 설계 방향을 결정한다.
```

---

## 현재 세션 완료 사항

- [x] `2026-03-06-hirct-output-spec-survey.md` 구조 리뷰
- [x] 스펙 공백 2가지 확인 (pipeline IR 비교, CModel 필수 정보)
- [x] 코드 기반 현재 구현 상태 실측 (`GenModel.cpp`, `IRAnalysis.cpp`, `EmitExpr.cpp`)
- [x] CIRCT 참조 가능 선례 후보 확인 (Arc, ExportSystemC, OpenTitan, Caliptra)
- [x] `2026-03-06-hirct-output-spec-survey.md` 전체 리뉴얼 계획 수립

## 남은 작업

- [ ] **[다음 세션]** Q1~Q4 결정 후 두 주제 브레인스토밍/설계
- [ ] **[다음 세션]** `2026-03-06-hirct-output-spec-survey.md` 전체 리뉴얼 실행
