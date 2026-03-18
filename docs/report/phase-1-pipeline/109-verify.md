# Task 109 Verify 게이트 검증 리포트

> **검증 일시**: 2026-02-18 ~ 2026-02-19
> **환경**: Ubuntu 24.04 (x86_64), kernel 6.14.0-27-generic
>
> **역할**: 이 파일은 게이트 실측 데이터(근거)만 기록한다.
> 체크리스트 진행 상태는 `docs/plans/phase-1-pipeline/109-verify.md`를 참조.

---

## 종합 판정: [V] ALL PASS (8/8)

| # | 게이트 | 결과 | 실측 |
|---|--------|------|------|
| G01 | verify_\<module\>.cpp 자동 생성 | [V] PASS | 2026-02-18 LevelGateway, SimpleAnd 확인 |
| G02 | Verilator RTL 모델 빌드 | [V] PASS | 2026-02-19 BUG-3 수정 후 SimpleAnd.v PASS |
| G03 | 비교 드라이버 빌드 | [V] PASS | 2026-02-19 BUG-2 수정 후 SimpleAnd.v PASS |
| G04 | 10 seeds × 1000 cyc 실행 | [V] PASS | 2026-02-19 SimpleAnd.v 10×1000 PASS |
| G05 | LevelGateway 자동 드라이버 PASS | [V] PASS | 2026-02-19 10시드×1000사이클 PASS (async reset 수정 후) |
| G06 | GenVerify.cpp emitter 구현 | [V] PASS | 2026-02-18 lib/Target/GenVerify.cpp 존재 |
| G07 | test/Target/GenVerify/ lit 테스트 PASS | [V] PASS | 2026-02-18 basic.test PASS |
| G08 | 커밋 완료 | [V] PASS | 2026-02-18 git log 확인 |

### 참고 사항

- RVCExpander verify는 Phase 2 스모크 대상으로 이관 (seq.firreg 복합 케이스)
- `known-limitations.md`에 XFAIL 등록
