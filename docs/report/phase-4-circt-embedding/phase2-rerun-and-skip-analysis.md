# Phase 2 재실행 + SKIP 모듈 재분석 리포트

> **Date:** 2026-03-04
> **Status:** 실행 완료
> **선행**: Arc PoC Phase A (#1 HirctUnrollProcessLoops + #2 verilator -E 완료)
> **빌드**: ninja exit 0, lit 55/55 PASS (100%)

## 1. 개요

Arc PoC 후속 작업 #3(Phase 2 재실행)과 #5(SKIP 모듈 재분석)을 병렬 실행하여,
커스텀 MLIR pass 4종 + verilator -E 전처리가 fc6161 전수 대상에 미치는 효과를 측정.

## 2. Task #3 — Phase 2 재실행 (GenModel Survey V2)

### 2.1 실행 환경

- **도구**: `survey-errors.py` (기존 인프라 재사용)
- **대상**: `survey-targets.txt` 56개 RTL filelist
- **hirct-gen**: `hirct/build/bin/hirct-gen` (커스텀 pass 4종 내장)
- **파이프라인**: SimCleanup → UnrollProcessLoops → ProcessFlatten → SignalLowering → CSE → Canonicalize
- **출력**: `examples/fc6161/pt_plat/survey-results-v2/`

### 2.2 카테고리별 비교

| 카테고리 | Baseline (v1) | V2 (현재) | 변화 | 의미 |
|----------|:---:|:---:|:---:|------|
| pass | 3 | 0 | -3 | pass_with_warnings로 재분류 |
| pass_with_warnings | 0 | **6** | **+6** | uart, gpio, wdt, ptimer, mpm_util, ets_con |
| unresolved_process | 2 | **0** | **-2** | 루프 언롤링으로 해소 (uart, gpio) |
| unresolved_drive | 2 | **0** | **-2** | SignalLowering으로 해소 (wdt, ptimer) |
| lowering_fail | 5 | 6 | +1 | axi_2x3: timeout→lowering_fail |
| unknown_module | 24 | 26 | +2 | 에러 경로 변경 (mpm_bram, inpsytech) |
| timescale_error | 9 | 11 | +2 | 에러 경로 변경 (mpm_bus, subsystem) |
| other | 10 | 6 | -4 | 더 정확한 분류 |
| timeout | 1 | **0** | **-1** | axi_2x3 더 이상 타임아웃 없음 |
| emit_fail | 0 | ~~1~~ → **0** | ~~+1~~ → 0 | fsl_wflow: ProcessDeseq V2로 해소 (V3) |

### 2.3 주요 개선 사항

**루프 언롤링 효과 (UnrollProcessLoops)**

| 타겟 | Baseline | V2 | 원인 |
|------|----------|-----|------|
| uart | unresolved_process (exit 0) | **pass_with_warnings** (exit 0) | llhd.process 내 for 루프 → 언롤 → cf.br 제거 |
| gpio | unresolved_process (exit 0) | **pass_with_warnings** (exit 0) | 동일 |

**SignalLowering 효과**

| 타겟 | Baseline | V2 | 원인 |
|------|----------|-----|------|
| wdt | unresolved_drive (exit 0) | **pass_with_warnings** (exit 0) | llhd.sig/drv/prb → hw wire 치환 |
| ptimer | unresolved_drive (exit 0) | **pass_with_warnings** (exit 0) | 동일 |

**타임아웃 해소**

| 타겟 | Baseline | V2 | 원인 |
|------|----------|-----|------|
| axi_2x3 | timeout (301.6s) | lowering_fail (0.9s) | 파이프라인 효율화로 타임아웃 제거 |

**총 실행 시간**: 357.8s → 27.9s (**92.2% 감소**, axi_2x3 타임아웃 제거 효과)

### 2.4 uart 산출물 생성 확인

uart_top은 V2에서 산출물 생성 성공 (exit 0):

| 항목 | 값 |
|------|-----|
| 총 서브모듈 | 39개 |
| GenModel 생성 | 37/39 (DW_apb_uart_bcm57, bcm57_0 제외) |
| GenModel 스킵 사유 | residual llhd.prb/drv 2개 (CDC 메모리 모듈) |
| val map miss 경고 | comb.and/or operand 참조 실패 (bcm57 의존) |

### 2.5 fsl_wflow 리그레션 분석 → V3에서 해소

| 항목 | Baseline | V2 | V3 |
|------|----------|-----|-----|
| 상태 | pass (exit 0) | emit_fail (exit 1) | **pass_with_warnings** (exit 0) |
| 원인 | - | fsl_wflow_fifo_reg_* 29개 서브모듈에 잔존 llhd.prb/drv 2개씩 | ProcessDeseq V2: intermediate block 처리 |
| 근본 원인 | 기존 파이프라인은 잔존 LLHD op을 묵인 | 새 파이프라인이 정확히 감지 후 거부 | UnrollProcessLoops가 리셋 경로 for 루프를 언롤 → ^bb3→^bb4→^bb1 구조 생성 → ProcessDeseq가 ^bb4를 intermediate reset block으로 인식 |
| 판정 | - | 탐지 개선 | **해결 완료** |

V2에서 fsl_wflow_fifo_reg의 emit_fail 원인은 ProcessDeseq pass가 리셋 경로의 intermediate block (언롤된 배열 초기화)을 처리하지 못해 skip한 것.
V3에서 두 가지 수정으로 해결:
1. intermediate block의 ops를 `rst_mapping`에 clone하여 리셋값 추출
2. `merge_args`에서 `block_conds`에 없는 predecessor(intermediate block)를 skip하여 null Condition segfault 방지

### 2.6 GenModel-specific 실패 변화

| 지표 | Baseline | V2 |
|------|:---:|:---:|
| GenModel-specific failures | 4 | ~~1~~ → **0** |
| → unresolved_process | 2 | 0 |
| → unresolved_drive | 2 | 0 |
| → emit_fail | 0 | ~~1~~ → 0 |

## 3. Task #5 — SKIP 모듈 재분석 (verilator -E → circt-verilog)

### 3.1 실행 환경

- **도구**: `analyze-skip-modules.py` (신규 작성)
- **대상**: Baseline에서 non-pass인 53개 타겟
- **파이프라인**: verilator -E → circt-verilog --ir-hw → MLIR 패턴 분석
- **verilator**: 5.044
- **circt-verilog**: 5e760efa9
- **출력**: `examples/fc6161/pt_plat/skip-analysis-results/`

### 3.2 총괄 결과

| 지표 | 값 | 비율 |
|------|---:|---:|
| 전체 SKIP 타겟 | 53 | 100% |
| verilator -E 성공 | 37 | 69.8% |
| circt-verilog 성공 | 6 | 11.3% |
| MLIR hw.module 보유 | 6 | 11.3% |
| 총 hw.module | 252 | - |
| 총 llhd.process | 176 | - |

### 3.3 Baseline 카테고리별 verilator -E 효과

| Baseline 카테고리 | 대상 수 | V-E 성공 | CV 성공 | 비고 |
|------------------|:---:|:---:|:---:|------|
| unknown_module | 24 | 18 | **0** | 매크로 해소되나 모듈 정의 여전히 누락 |
| other | 10 | 4 | **0** | include/경로 문제 일부 해소 |
| timescale_error | 9 | 5 | **0** | timescale 해소되나 다른 에러 노출 |
| lowering_fail | 5 | 5 | **4** | concat_ref 우회 → MLIR 생성 성공 |
| unresolved_process | 2 | 2 | **2** | hirct-gen 없이도 MLIR 직접 생성 |
| unresolved_drive | 2 | 2 | **0** | 잔존 매크로 미해소 |
| timeout | 1 | 1 | **0** | 전처리 성공하나 CV에서 실패 |

### 3.4 MLIR 생성 성공 타겟 (6개)

| 타겟 | Baseline | MLIR Lines | hw.module | llhd.process | cf.br | cf.cond_br |
|------|----------|---:|---:|---:|---:|---:|
| uart | unresolved_process | 2,381 | 42 | 31 | 66 | 150 |
| gpio | unresolved_process | 1,376 | 10 | 14 | 51 | 28 |
| **i2cm** | **lowering_fail** | **6,926** | **48** | **19** | **34** | **37** |
| **smbus** | **lowering_fail** | **18,523** | **76** | **42** | **132** | **130** |
| **axi_x2p1** | **lowering_fail** | **3,145** | **38** | **35** | **90** | **96** |
| **axi_x2p2** | **lowering_fail** | **3,264** | **38** | **35** | **90** | **96** |

**신규 해소 4개** (lowering_fail → MLIR 생성): verilator -E가 `moore.concat_ref` legalize 실패를 우회.

### 3.5 실패 원인 분류

#### verilator -E 실패 (16/53)

| 원인 | 건수 | 대표 타겟 |
|------|:---:|----------|
| include 파일 누락 | 8 | ssd_ctrl_top_wrapper, pt_ncs_*, pt_ldpc_dec |
| -y 경로 미존재 | 2 | p0_pcie_ctl (Synopsys DW 라이브러리) |
| +define+ 처리 오류 | 3 | ddr_phy.syn, ddr_phy.dep, inpsytech |
| IP 내부 경로 문제 | 3 | p0_pcie_ide, pcie_phy, pt_hcs |

#### circt-verilog 실패 (31/37 V-E 성공 중)

| 원인 | 건수 | 설명 |
|------|:---:|------|
| 잔존 매크로 (`` `MACRO ``) | 12 | verilator -E가 +define+ 없이 호출되어 미확장 |
| unknown_module | 10 | 전처리로 해소 불가 (모듈 정의 자체가 누락) |
| 구문 오류 | 5 | SystemVerilog 특수 구문 (`$disable_warnings` 등) |
| 기타 | 4 | 재정의, 포트 불일치 등 |

### 3.6 잔존 llhd.process 패턴 분석 (176개)

| 타겟 | llhd.process | 추정 패턴 | 해결 수단 |
|------|---:|----------|----------|
| smbus | 42 | LOOP+BITWISE/ARRAY (i2c 레지스터) | UnrollProcessLoops |
| axi_x2p1 | 35 | FSM + LOOP (AXI-APB 브릿지) | UnrollProcessLoops + ProcessFlatten |
| axi_x2p2 | 35 | 동일 (x2p 변종) | 동일 |
| uart | 31 | LOOP+BITWISE (bcm57 메모리) | UnrollProcessLoops |
| i2cm | 19 | LOOP+BITWISE (sync 모듈) | UnrollProcessLoops |
| gpio | 14 | LOOP+BITWISE (debounce) | UnrollProcessLoops |

대부분 기존 분석 (Arc PoC `arc-poc-results.md` §llhd.process 전수 조사)에서 확인된 패턴과 일치.
hirct-gen 파이프라인의 UnrollProcessLoops가 이미 처리하는 패턴이므로, `--preprocess verilator` 경로에서도 동일하게 적용 가능.

## 4. 종합 비교

### 4.1 hirct-gen 파이프라인 (Task #3)

```
[v1 Baseline — 2026-03-01]
 56 targets: 3 pass, 2 UP, 2 UD, 5 LF, 24 UM, 9 TS, 10 OT, 1 TO
 GenModel-specific failures: 4
 Total time: 357.8s

[v2 Current — 2026-03-04]
 56 targets: 6 pass_w_warn, 6 LF, 26 UM, 11 TS, 6 OT, 1 EF
 GenModel-specific failures: 1 (emit_fail)
 Total time: 27.9s
```

### 4.2 verilator -E 경로 (Task #5)

```
[신규 MLIR 접근 가능 타겟]
 기존: uart, gpio (2, unresolved_process)
 추가: i2cm, smbus, axi_x2p1, axi_x2p2 (4, lowering_fail → concat_ref 우회)
 합계: 6 타겟, 252 hw.module, 176 llhd.process
```

### 4.3 커버리지 변화 요약

| 지표 | Phase 2 Baseline | 현재 (hirct-gen) | 현재 (verilator -E) |
|------|:---:|:---:|:---:|
| exit 0 타겟 | 7/56 (12.5%) | 6/56 (10.7%) | 6/53 MLIR 생성 |
| GenModel 실패 | 4건 | **0건** | N/A |
| 전수 시간 | 357.8s | **27.9s** | 27.6s |
| MLIR 접근 가능 모듈 | - | - | 252 hw.module |

## 5. 후속 작업

| # | 작업 | 우선순위 | 의존 |
|---|------|:---:|------|
| 6 | ~~LLHD 잔존 op 처리 — fsl_wflow_fifo_reg llhd.prb/drv~~ | ~~HIGH~~ | **완료** — ProcessDeseq V2로 해소 |
| 4 | arcilator 종단 검증 — i2cm/smbus verilator -E → arcilator | MED | #5 완료 |
| 7 | Phase 2 전수 순회 (1,600 .v) — verilator -E 통합 경로 | MED | #6 완료 |
| - | circt-verilog 실패 31건 개선 — filelist +define+ 자동 전달 | LOW | 별도 조사 |
| - | uart val map miss 해소 — bcm57 CDC 모듈 완전 lowering | LOW | SignalLowering 확장 |

## 6. 산출물

| 파일 | 설명 |
|------|------|
| `examples/fc6161/pt_plat/survey-results-v2/` | Task #3 전수 재실행 결과 |
| `examples/fc6161/pt_plat/survey-results-v2/error-taxonomy.json` | V2 분류 JSON |
| `examples/fc6161/pt_plat/survey-results-v2/error-taxonomy-summary.txt` | V2 요약 |
| `examples/fc6161/pt_plat/skip-analysis-results/` | Task #5 SKIP 분석 결과 |
| `examples/fc6161/pt_plat/skip-analysis-results/skip-analysis.json` | SKIP 분석 JSON |
| `examples/fc6161/pt_plat/skip-analysis-results/skip-analysis-summary.txt` | SKIP 분석 요약 |
| `examples/fc6161/pt_plat/utils/analyze-skip-modules.py` | Task #5 분석 스크립트 |
