# Phase 3: 통합 및 배포

> **예상 기간**: 5일
> **진입 조건**: Phase 2 완료 (make test-all PASS)
> **완료 기준**: VCS co-sim PASS + make docs 동작 + README 완성

## 목표

외부 도구 연동과 사용자 배포 준비를 완료한다.

## 태스크

- [301-vcs-cosimulation.md](301-vcs-cosimulation.md) — VCS DPI-C co-simulation (2일)
- [302-documentation.md](302-documentation.md) — mkdocs 문서화 (1일)
- [303-production-packaging.md](303-production-packaging.md) — 프로덕션 패키징 (2일)

## Phase 3 Agent-in-the-Loop 체크포인트

| 체크포인트 | 시점 | 자동 Gate | 수동 Gate |
|-----------|------|----------|----------|
| CP-3A: VCS 완료 | Task 301 후 | VCS 10seed×1000cyc PASS | 3자 비교 결과 리뷰 |
| CP-3B: 배포 준비 | Task 303 후 | 클린 환경 Quick Start 성공 | 최종 배포 승인 |

> Phase 3는 `executing-plans` 스킬로 순차 실행한다 (summary.md §4.4 참조).

## 성공 기준

- [ ] VCS DPI-C lock-step 10시드×1000cyc PASS
- [ ] make docs 성공
- [ ] 신규 환경에서 setup-env.sh → make build → hirct-gen input.v 동작
