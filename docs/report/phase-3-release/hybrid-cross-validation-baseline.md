# Hybrid Cross-Validation Baseline

> 실행일: 2026-02-23
> 목적: Hybrid(A->B) 게이트 기준선을 최신 실행 로그 기준으로 고정

## 최신 기준선 근거

### 1) `make verify` 실행

```bash
make verify
```

- Exit code: `0`
- 핵심 로그:
  - `=== HIRCT Report Summary ===`
  - `Total modules:   1604`
  - `Report written to: output/report.json`
  - `[verify] Traversal complete in 12.9s`
  - `[verify] Report written to: output/verify-report.json`

### 2) `output/report.json` 핵심 수치

- 기준 시각(`generated_at`): `2026-02-23T14:13:31.804774+00:00`
- `total_files`: `1604`
- `mlir_success`: `1231`
- `mlir_fail`: `373`
- `infra_error`: `0`
- 주요 emitter:
  - `gen-model`: `1121 pass / 483 fail`
  - `gen-verify`: `1091 pass / 513 fail`
  - `gen-dpic`: `1086 pass / 518 fail`

### 3) `output/verify-report.json` 핵심 수치

- 기준 시각(`generated_at`): `2026-02-23T14:13:44.889507+00:00`
- 설정: `seeds=10`, `cycles=1000`
- `total_modules`: `1121`
- `pass`: `133`
- `fail`: `224`
- `skip`: `764`
- 대표 실패 모듈(첫 항목): `Fadu_K2_S5_Queue_11` (`status=fail`)

## 대표 재현 명령 및 관찰

### 명령

```bash
build/bin/hirct-verify rtl/plat/src/s5/design/Fadu_K2_S5_Queue_11.v --lib-dir rtl/lib/stubs --seeds 3 --cycles 100
```

### 결과

- Exit code: `2`
- `seed 1/3`에서 즉시 실패 후 종료
- mismatch 포트: `io_deq_bits`
- 관찰: `cycle 0~9` 구간에서 연속 mismatch가 재현되며 `rtl`/`model` 값 괴리가 고정 패턴으로 나타남
- 판정: Queue 계열 Tier-3 실패 샘플로 재현성 확인됨

## Acceptance Contract (Pilot)

### Module Gate

- 동일 seed/cycle 기준 mismatch `0`
- `hirct-verify` exit code `0`

### Subsystem Gate

- 후보 묶음 PASS 비율 유지
- 실패 패턴 안정성 유지(신규 회귀 없음)

### Top Gate

- SoC smoke mismatch `0`
- 또는 허용 리스트(XFAIL) 항목만 존재
