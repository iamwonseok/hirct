# Task 203: 자동 검증 (hirct-verify 전체)

> **목표**: GenModel 성공한 모듈에 대해 hirct-verify 10시드×1000cyc 실행, 결과 수집
> **예상 시간**: 2일
> **산출물**: `lit integration_test/hirct-verify/`, `output/verify-report.json`
> **피드백 루프**: mismatch → Phase 1 (101-gen-model 또는 109-verify)

---

## 목표

201-file-traversal의 report.json에서 GenModel 성공 모듈 목록을 추출하고, `hirct-verify <file>.v`로 10시드×1000사이클 검증을 실행한다. 모듈별·시드별 PASS/FAIL을 기록하고, mismatch 시 Phase 1으로 되돌린다.

## 주요 작업

- report.json에서 GenModel 성공 모듈 추출
- hirct-verify CLI 사용: `hirct-verify input.v` (10 seeds × 1000 cycles)
- `lit integration_test/hirct-verify/` 또는 `make test-traversal`: 검증 대상 전체 순회
- per-module per-seed 결과를 output/verify-report.json에 기록
- 리포트 생성 경로: lit xunit XML + meta.json → `utils/generate-report.py` → verify-report.json
- mismatch 패턴 수집 (cycle, seed, RTL vs Model 값)
- FAIL 모듈 목록 추출 (Phase 1 되돌림용)

## 게이트 (완료 기준)

- [ ] GenModel 성공 모듈 전체에 대해 10시드 x 1000cyc 검증 완료
- [ ] `output/verify-report.json` 생성, 필수 키 포함:
  ```json
  {
    "total_modules": N,
    "pass": N,
    "fail": N,
    "modules": [{"name": "...", "seeds": [{"seed": 42, "result": "pass|fail", "cycles": 1000}]}]
  }
  ```
- [ ] **100% PASS** (XFAIL 제외). XFAIL 모듈은 `known-limitations.md`에 사유와 함께 문서화. CI는 항상 Green 상태 유지.
- [ ] FAIL 모듈 목록 → Phase 1 이슈로 전달
