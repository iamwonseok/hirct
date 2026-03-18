# Arc PoC Phase A 최종 리포트

> **Date:** 2026-03-04
> **Status:** **COMPLETE** — Core/Extended/전수 조사 완료, Go/No-Go 판정 확정, 후속 구현 계획 작성 (plan-readiness-check Go)
> **Goal:** arcilator pipeline, verilator -E 전처리, Arc MLIR C++ 매핑 가능성을 UART+GPIO로 검증하여 Arc 기반 CModel 전환의 Go/No-Go 판정
> **CIRCT Version:** arcilator 5e760efa9, circt-verilog b73e3dc94, LLVM 23.0.0git
> **verilator:** 5.044

## Core Stage 결과

| 실험 | 결과 | 비고 |
|------|------|------|
| 1. arcilator UART | **FAIL** | async reset (174/174 firreg) → preproc 실패. sync 변환 후에도 cf.br/cf.cond_br (14개) → arc-conv 실패 |
| 2. verilator -E UART | **PASS** | 18,406줄 전처리 (exit 0) → circt-verilog → 2,381줄 MLIR (exit 0). timescale 미보존 (불필요) |
| 3. Arc MLIR 구조 분석 | **CONDITIONAL** | Arc dialect 미생성. HW/Comb/Seq에서 직접 분석: 포트이름 PASS, 비트폭 PASS, state이름 PASS, 클럭도메인 PARTIAL, arc.model/offset FAIL |

**Core 판정: CONDITIONAL GO** — arcilator 전체 경로 불가, 그러나 HW/Comb/Seq에서 직접 C++ 매핑 가능성 확인

## Extended Stage 결과

### L3 확장 (arcilator — async→sync 변환 후 테스트)

| 타겟 | GenModel 상태 | async reset | cf.br | arcilator 결과 | 의미 |
|------|---------------|-------------|-------|----------------|------|
| UART | unresolved_process | 174 | **YES** (14) | **FAIL** (arc-conv) | cf.br가 진짜 blocker |
| GPIO | unresolved_process + flatten_cycle | 52 | **YES** | **FAIL** (arc-conv) | 동일 패턴 |
| WDT | unresolved_drive | 8 | NO | **PASS** (state-alloc, 154B) | 단순 구조 = 성공 |
| PTIMER | unresolved_drive | 83 | NO | **PASS** (state-alloc, 1,533B) | 단순 구조 = 성공 |

**핵심 발견:** arcilator 실패의 실제 blocker는 async reset가 아닌 `cf.br`/`cf.cond_br` 제어 흐름.
- `unresolved_drive` 타겟 (WDT, PTIMER): cf.br 없음 → arcilator 호환
- `unresolved_process` 타겟 (UART, GPIO): cf.br 존재 → arcilator 비호환
- WDT Arc MLIR에서 `arc.model`, `arc.alloc_state` (byte offset 포함), `arc.clock_domain` 등 C++ 매핑에 필요한 정보 완전 보존 확인

### L1 확장 (verilator -E)

| 타겟 | hirct-gen 실패 이유 | verilator -E 결과 | 의미 |
|------|-------------------|------------------|------|
| mpm_bram (+define+) | +define+ 미파싱 | **PASS** (exit 0, 1,898줄) | L1-a 해소 |
| p0_pcie_ctl (-y) | -y 미파싱 | **FAIL** (exit 1, 119K줄 생성, include 162 에러) | L1-a 미해소 (IP 내부 include 경로 누락) |
| caliptra (timescale) | timescale | **PASS** (exit 0, 207,905줄) | L1-d 해소 |

**핵심 발견:** verilator -E는 +define+, timescale 문제 완전 해결. -y 경로 문제는 IP 내부 include 경로가 filelist에 명시되어야 해결.

### L2 확인

| 타겟 | hirct-gen 결과 | verilator -E → circt-verilog 결과 | 비고 |
|------|---------------|----------------------------------|------|
| i2cm (concat_ref) | **FAIL** (`moore.concat_ref` legalize 실패) | **PASS** (exit 0, 6,926줄 MLIR) | llhd op 96개 잔존, moore op 0 |

**핵심 발견:** verilator -E 전처리가 L2(concat_ref) 우회에도 효과적! 다만 LLHD 잔존 op(llhd.prb/drv) 처리를 위한 추가 lowering 필요.

## Go/No-Go 최종 판정

- [x] arcilator 경로: **PARTIAL** — cf.br 없는 단순 모듈(WDT, PTIMER)에서만 성공. 범용 경로로는 NO-GO
- [x] verilator -E 전처리: **GO** — L1(+define+, timescale), L2(concat_ref) 문제를 광범위하게 해결
- [x] Arc MLIR C++ 매핑: **CONDITIONAL GO** — Arc dialect 도달 시 매핑 정보 완전 보존 (WDT/PTIMER 실증). HW/Comb/Seq에서도 직접 매핑 가능
- [x] **최종: CONDITIONAL GO**

## llhd.process 전수 조사 (58 filelist, 13 분석 성공)

### 패턴 분포 (총 266개 llhd.process)

| 패턴 | 개수 | 비율 | 설명 | 상태 |
|------|------|------|------|------|
| LOOP+BITWISE | 142 | 53% | `for(i<N)` 비트별 연산 (shru/shl) | **미해결 — 루프 언롤링 필요** |
| SIGNAL_DRIVE | 66 | 25% | 클럭+신호 드라이브 → seq.firreg | deseq가 처리 |
| LOOP+ARRAY | 38 | 14% | `for(i<N)` 배열 초기화/접근 (hw.array_inject) | **미해결 — 루프 언롤링 필요** |
| NO_CF | 17 | 6% | cf.br 없는 단순 프로세스 | 이미 처리됨 |
| LOOP+OTHER (오분류) | 3 | 1% | 실제는 FSM/CONDITIONAL (comb.icmp ceq 상태 비교) | deseq가 처리 |

### 루프 특성 (LOOP+BITWISE + LOOP+ARRAY = 180개)

- **루프 술어**: 전부 `slt` (signed less than). `ult` 는 FSM 오분류 3건뿐
- **유도 변수**: 전부 `i32`
- **증분**: 전부 +1
- **바운드 분포**: N=1~73 (최대 smbus N=73). 전부 1024 미만 정적 상수
- **중첩 루프**: 일부 모듈에서 2중 루프 존재 (예: smbus `N=32,N=12`)

### 근본 원인 확정

`llhd.process` 내부의 `cf.br`/`cf.cond_br`를 생성하는 **유일한 미해결 원인**은 Verilog `for` 루프:

```
for (i = 0; i < N; i = i + 1) begin
  result[i] = f(inputs[i]);      // LOOP+BITWISE (53%)
  mem[i] <= reset_value;          // LOOP+ARRAY (14%)
end
```

CIRCT `--llhd-unroll-loops` pass가 이미 동일한 루프 언롤링 로직을 보유하나, `CombinationalOp`에서만 동작. 우리 루프는 `ProcessOp` 내부에 있어 스캔 대상에서 제외됨.

### LOOP+OTHER 3건 상세 분석

| 모듈 | 프로세스 | 실제 패턴 | 오분류 원인 |
|------|---------|-----------|------------|
| axi_x2p1 #1 | `result=i3, i1` | **FSM**: `comb.icmp ceq %arb_state, %c0_i3` (AXI arbiter 상태 전이) | 프로세스 외부 `comb.icmp ult` 검출 |
| axi_x2p1 #11 | `result=i1, i1` | **CONDITIONAL**: `comb.icmp eq %state, %c18_i6` (APB write 상태 분기) | 동일 |
| axi_x2p2 #29 | `result=i1, i1` | **CONDITIONAL**: axi_x2p1 #11과 동일 구조 (x2p 변종) | 동일 |

3건 모두 루프가 아닌 FSM/조건 분기. deseq가 처리할 수 있는 패턴이며, verilator -E 경로에서만 발생 (hirct-gen 경로에서는 deseq가 이미 처리).

## 다음 단계: ProcessOp 루프 언롤링 Pass 설계

### 설계 근거

1. **fc6161 전수 조사**: 266개 llhd.process 중 미해결 180개가 전부 정적 바운드 for 루프
2. **CIRCT 인프라 재사용**: `UnrollLoops.cpp`의 `Loop::match()` + `Loop::unroll()` 로직이 `slt`/`ult` 모두 지원, 바운드 < 1024 조건 충족
3. **병목 지점 단일**: `UnrollLoopsPass`가 `CombinationalOp`만 스캔하여 `ProcessOp` 내부 루프를 무시하는 것이 유일한 이유
4. **후속 파이프라인 자동 해소**: 루프 언롤 후 → `llhd-remove-control-flow`(cf.br → mux) → `llhd-deseq`(process → firreg) → arcilator `arc-conv` 통과

### 구현 방안: `HirctUnrollProcessLoops` Pass

```
ProcessOp 내부 → CFGLoopInfo 구성 → Loop::match() → Loop::unroll()
→ cf.br 제거 → 후속 deseq/arc-conv 자동 통과
```

- **입력**: `llhd.process` 내부의 `cf.br`/`cf.cond_br` 루프
- **출력**: 언롤된 직선 코드 (cf.br 없음)
- **바운드 제한**: N < 1024 (CIRCT와 동일, fc6161 최대 N=73)
- **술어 지원**: `slt`, `ult` (전수 조사에서 확인된 두 유형)
- **위치**: `hirct/lib/Transforms/HirctUnrollProcessLoops.cpp`
- **파이프라인 위치**: deseq 이전, HirctProcessFlatten 이전

### 기대 효과

| 지표 | 현재 | 적용 후 |
|------|------|---------|
| arcilator 호환 모듈 | WDT, PTIMER (2개) | +UART, GPIO, fsl_wflow, axi_*, smbus, i2cm, mpm_async_bridge |
| unresolved_process | UART, GPIO 등 다수 | 0 (루프 → 언롤 → deseq → firreg) |
| GenModel 커버리지 | ~65% (Phase 2 조사 기준) | 루프 원인 모듈 추가 해소 |

### 기타 권고 (변경 없음)

1. **verilator -E 전처리 파이프라인 통합**: L1/L2 문제 해소에 독립적으로 유효
2. **arcilator 선별적 활용**: 루프 언롤 후 WDT/PTIMER뿐 아니라 UART/GPIO도 arcilator 호환 예상
3. **async reset**: CIRCT 업스트림 의존 (루프 언롤과 독립)

## Phase A 완료 산출물 + 후속 계획

### 후속 구현 계획 (plan-readiness-check: Go)

- **계획 문서**: 삭제됨 (구현 완료)
- **대상**: `HirctUnrollProcessLoops` pass (5 Tasks, ~200-300줄 C++)
- **효과**: 180/266 llhd.process 해소, arcilator 호환 모듈 대폭 확대
- **실행 방식**: subagent-driven-development 또는 parallel session

### 전체 해결 경로 요약

| 패턴 (개수) | 해결 수단 | 상태 |
|-------------|----------|------|
| LOOP+BITWISE (142) | HirctUnrollProcessLoops (신규) | 계획 완료, 구현 대기 |
| LOOP+ARRAY (38) | HirctUnrollProcessLoops (신규) | 동일 |
| SIGNAL_DRIVE (66) | llhd-deseq (기존 CIRCT) | 이미 작동 |
| NO_CF (17) | HirctProcessFlatten (기존) | 이미 작동 |
| FSM/CONDITIONAL (3) | HirctProcessFlatten (기존) | verilator -E 통합 시 작동 |

## 실험 산출물

| 파일 | 내용 |
|------|------|
| `/tmp/poc-arc/uart_hw.mlir` | UART HW MLIR (hirct-gen --dump-ir, 1,860줄) |
| `/tmp/poc-arc/uart_cv_hw.mlir` | UART HW MLIR (circt-verilog 직접, 2,381줄) |
| `/tmp/poc-arc/uart_preproc.mlir` | UART arcilator preproc 출력 (sync hack, 1,867줄) |
| `/tmp/poc-arc/uart_preprocessed.v` | UART verilator -E 출력 (18,406줄) |
| `/tmp/poc-arc/uart_from_verilator.mlir` | UART verilator -E → circt-verilog MLIR (2,381줄) |
| `/tmp/poc-arc/wdt_arc.mlir` | WDT Arc MLIR (arcilator state-alloc 성공, 744줄) |
| `/tmp/poc-arc/ptimer_arc.mlir` | PTIMER Arc MLIR (arcilator state-alloc 성공, 7,824줄) |
| `/tmp/poc-arc/analysis-report.md` | Arc MLIR 구조 분석 상세 결과 |
| `/tmp/poc-arc/i2cm_hw.mlir` | i2cm verilator -E → circt-verilog MLIR (6,926줄, concat_ref 우회) |
