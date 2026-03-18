# Task 104 GenWrapper 게이트 검증 리포트

> **검증 일시**: 2026-02-18 ~ 2026-02-19
> **환경**: Ubuntu 24.04 (x86_64), kernel 6.14.0-27-generic
>
> **역할**: 이 파일은 게이트 실측 데이터(근거)만 기록한다.
> 체크리스트 진행 상태는 `docs/plans/phase-1-pipeline/104-gen-wrapper.md`를 참조.

---

## 종합 판정: [V] ALL PASS (5/5)

| # | 게이트 | 결과 | 실측 |
|---|--------|------|------|
| G01 | hirct-gen LevelGateway.mlir → wrapper/*.sv 존재 | [V] PASS | 2026-02-18 파일 생성 확인 |
| G02 | verilator --lint-only wrapper/*.sv exit 0 | [V] PASS | 2026-02-19 *_wrapper 이름 + dummy bind 추가 후 PASS |
| G03 | interface + modport 포함 확인 | [V] PASS | 2026-02-19 modport dut/tb 포함 확인 |
| G04 | prefix 기반 포트 그룹핑 동작 확인 | [V] PASS | 2026-02-19 LevelGateway io_plic 그룹 확인 |
| G05 | test/Target/GenWrapper/ lit 테스트 PASS | [V] PASS | 2026-02-18 basic.test PASS |
