# Task 101 GenModel 게이트 검증 리포트

> **검증 일시**: 2026-02-18 ~ 2026-02-19
> **환경**: Ubuntu 24.04 (x86_64), kernel 6.14.0-27-generic
>
> **역할**: 이 파일은 게이트 실측 데이터(근거)만 기록한다.
> 체크리스트 진행 상태는 `docs/plans/phase-1-pipeline/101-gen-model.md`를 참조.

---

## 종합 판정: [V] ALL PASS (6/6)

| # | 게이트 | 결과 | 실측 |
|---|--------|------|------|
| G01 | hirct-gen LevelGateway.mlir → cmodel/*.h + *.cpp 존재 | [V] PASS | 2026-02-18 파일 생성 확인 |
| G02 | g++ -std=c++17 -c cmodel/*.cpp exit 0 | [V] PASS | 2026-02-18 컴파일 성공 |
| G03 | LevelGateway 모든 IR op 포함 | [V] PASS | 2026-02-18 10종 emitter 모두 "pass" |
| G04 | RVCExpander 모든 IR op 포함 | [V] PASS | 2026-02-19 hirct-gen + g++ 컴파일 성공 |
| G05 | hw.instance CIRCT flatten → Error 진단 확인 | [V] PASS | 2026-02-18 확인 |
| G06 | test/Target/GenModel/ lit 테스트 PASS | [V] PASS | 2026-02-19 4/4 PASS (combinational + unsupported-op + wide-signal + multi-module) |
