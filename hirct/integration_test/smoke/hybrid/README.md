# Hybrid Smoke Tests

Hybrid(A->B) 교차검증 smoke 테스트를 위한 디렉토리이다.

- 목적: Module -> Subsystem -> Top 게이트를 동일 seed/cycle 정책으로 점검
- `module-gate-red.test`: Queue/FIFO 의미론 불일치 RED 재현용 XFAIL 테스트
- `matrix-smoke.test`: `hybrid-verify-matrix.py` CLI 스켈레톤(dry-run/JSON) 검증
- 실행: `make hybrid-module-gate` (또는 `make check-hirct-integration`)
- 현재 기대결과: `hybrid-module-gate` 기준 `Passed 1`, `Expectedly Failed 1`
