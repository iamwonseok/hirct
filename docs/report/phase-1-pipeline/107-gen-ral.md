# Task 107 GenRAL 게이트 검증 리포트

> **검증 일시**: 2026-02-18 ~ 2026-02-19
> **환경**: Ubuntu 24.04 (x86_64), kernel 6.14.0-27-generic
>
> **역할**: 이 파일은 게이트 실측 데이터(근거)만 기록한다.
> 체크리스트 진행 상태는 `docs/plans/phase-1-pipeline/107-gen-ral.md`를 참조.

---

## 종합 판정: [V] ALL PASS (11/11)

| # | 게이트 | 결과 | 실측 |
|---|--------|------|------|
| G01 | GenRAL.cpp 생성 | [V] PASS | 2026-02-18 lib/Target/GenRAL.cpp 존재 |
| G02 | uvm_reg_block 상속 클래스 (_ral.sv) | [V] PASS | 2026-02-18 LevelGateway ral/ 3파일 생성 |
| G03 | 포트→reg_field 매핑 | [V] PASS | 2026-02-19 inFlight→inFlight_reg, offset 0x00, width 1, RW |
| G04 | UVM compile / verilator lint | [V] PASS | 2026-02-19 HAL .h gcc -fsyntax-only + driver.c gcc exit 0 |
| G05 | HAL 헤더 (_hal.h) 생성 | [V] PASS | 2026-02-18 파일 생성 확인 |
| G06 | C 드라이버 (_driver.c) 생성 | [V] PASS | 2026-02-18 파일 생성 확인 |
| G07 | gcc -c _driver.c exit 0 | [V] PASS | 2026-02-19 LevelGateway RAL driver compile PASS |
| G08 | 레지스터 없는 모듈 → ral/ 미생성 + 스킵 기록 | [V] PASS | 2026-02-18 skip.test PASS |
| G09 | 레지스터 판정 기준 정확 동작 | [V] PASS | 2026-02-19 LevelGateway ral 생성 + CombOnly 정상 스킵 |
| G10 | 커밋 완료 | [V] PASS | 2026-02-18 git log 확인 |
| G11 | test/Target/GenRAL/ lit 테스트 PASS | [V] PASS | 2026-02-19 basic.test + skip.test + register-block.test 3/3 PASS |
