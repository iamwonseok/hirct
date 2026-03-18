# Hybrid Tiering Report

> 작성일: 2026-02-23
> 기준 파일: `config/hybrid-tier-modules.yaml`
> 근거: `docs/report/phase-3-release/hybrid-cross-validation-baseline.md`

## 분류 기준 표

| Tier | 점수/특성 | 주 위험 요인 | 게이트 시작점 |
|---|---|---|---|
| Tier-1 | `score <= 8`, 조합 위주 | 낮은 상태공간, 낮은 메모리 의존 | `hybrid-module-gate` |
| Tier-2 | `8 < score <= 25`, 레지스터/핸드셰이크 포함 | 상태 전이 증가, 인터페이스 경계 | `hybrid-module-gate` 이후 `hybrid-subsystem-gate` |
| Tier-3 | `score > 25`, Queue/FIFO/TileLink 중심 | 타이밍 민감, SoC 파급도 큼 | `hybrid-module-gate` + 원인 고정 후 승격 |

## 각 Tier 대표 모듈

- Tier-1: `secded_hamming_enc_d10_p5`, `secded_hamming_dec_d10_p5`, `secded_hamming_enc_d12_p6`
- Tier-2: `Fadu_K2_S5_LevelGateway`, `Fadu_K2_S5_AsyncResetRegVec_w1_i0`, `Fadu_K2_S5_Repeater`
- Tier-3: `Fadu_K2_S5_Queue_11`, `Fadu_K2_S5_Queue_152`, `Fadu_K2_S5_TLBuffer`, `Fadu_K2_S5_TLFragmenter`

## 분류 근거 요약

- `make verify` 최신 결과에서 `Fadu_K2_S5_Queue_11`은 `io_deq_bits` mismatch가 즉시 재현되어 Tier-3 실패 대표군으로 유지
- `output/report.json`/`output/verify-report.json` 수치 기준:
  - 전체 `1604` 파일 중 `gen-model pass 1121`
  - 검증 대상 `1121` 중 `pass 133 / fail 224 / skip 764`
- 따라서 현재는 Tier-1/2 우선 통과 경로 확보 후 Tier-3 승격을 단계적으로 진행하는 전략이 타당

## KPI (현재 기준선)

| KPI | 기준값 | 출처 |
|---|---|---|
| 생성 전체 파일 수 | `1604` | `output/report.json` |
| gen-model pass | `1121` | `output/report.json` |
| verify pass/fail/skip | `133 / 224 / 764` | `output/verify-report.json` |
| Queue_11 재현 명령 exit code | `2` | `hirct-verify ...Queue_11... --seeds 3 --cycles 100` |

## 현재 상태 (ready/blocked)

- Tier-1: **ready**
  - 이유: 저위험 대표군이 존재하고 module gate 기준 정의가 명확함
- Tier-2: **ready**
  - 이유: 승격 규칙(모듈 -> 서브시스템)이 YAML에 구체화됨
- Tier-3: **blocked**
  - 이유: `Fadu_K2_S5_Queue_11` 재현 실패(exit `2`)가 고정 패턴으로 남아 있어 상위 승격 조건 미충족

## 다음 승격 액션

1. Tier-1 대표군으로 `hybrid-module-gate` 통과율을 먼저 고정
2. Tier-2 대표군을 subsystem 후보로 묶어 `hybrid-subsystem-gate` 안정성 측정
3. Tier-3는 Queue/FIFO mismatch 원인 제거 후 동일 seed/cycle 기준 재측정
4. Top 승격은 `hybrid-top-gate`에서 회귀 0 또는 allowlist-only 조건 충족 시 수행
