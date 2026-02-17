# Phase 2: 전체 순회 테스트

> **예상 기간**: 12일 (Phase 1 되돌림 포함)
> **진입 조건**: Phase 1 완료 (모든 gen-xxx가 최소 1개 모듈에서 동작)
> **완료 기준**: 지원 모듈 전체 PASS + make test-all 동작

## 목표

Phase 1에서 구현한 기능을 전체 RTL(~1,600 파일)에 자동 순회 적용하여 실패를 발견하고 수정한다.

## 태스크

- [201-file-traversal.md](201-file-traversal.md) — 개별 파일 순회 + 리포트 (3일)
 - [202-top-traversal.md](202-top-traversal.md) — Top 순회 (filelist 기반) (2일)
- [203-auto-verification.md](203-auto-verification.md) — 자동 검증 (hirct-verify 전체) (2일)
- [204-failure-analysis.md](204-failure-analysis.md) — 실패 분석 + Phase 1 되돌림 (2일)
- [205-test-automation.md](205-test-automation.md) — make test-all + CI (1일)
- [206-agent-triage.md](206-agent-triage.md) — 자동 실패 분류 + LLM 보조 패치 (1일)

## 피드백 루프

실패 원인별 되돌림:
- 미지원 IR 연산 → 101-gen-model
- emitter 버그 → 해당 1xx
- 검증 드라이버 문제 → 109-verify
- DPI-C 문제 → 103-gen-dpic

## Phase 2 Agent-in-the-Loop 체크포인트

| 체크포인트 | 시점 | 자동 Gate | 수동 Gate |
|-----------|------|----------|----------|
| CP-2A: 순회 완료 | Task 201+202 후 | `report.json` 생성 | 성공률/실패 분포 확인 |
| CP-2B: 자동 검증 완료 | Task 203 후 | `verify-report.json` 생성 | mismatch 패턴 리뷰 |
| CP-2C: 분류 완료 | Task 204+206 후 | triage 리포트 생성 | XFAIL 리스트 최종 승인 |

> 상세 Agent 스펙(권한/금지사항/분류 규칙): `206-agent-triage.md` 참조

## 성공 기준

- [ ] 전체 .v 파일 순회 리포트 생성
- [ ] 지원 모듈 hirct-verify 10시드×1000cyc 전체 PASS
- [ ] Top 산출물 생성 성공
- [ ] make test-all 단일 명령 동작
- [ ] `make test-traversal` 동작 (lit integration_test/traversal/)
