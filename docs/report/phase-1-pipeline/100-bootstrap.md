# Task 100 Bootstrap 게이트 검증 리포트

> **검증 일시**: 2026-02-18 ~ 2026-02-19
> **환경**: Ubuntu 24.04 (x86_64), kernel 6.14.0-27-generic
>
> **역할**: 이 파일은 게이트 실측 데이터(근거)만 기록한다.
> 체크리스트 진행 상태는 `docs/plans/phase-1-pipeline/100-bootstrap.md`를 참조.

---

## 종합 판정: [V] ALL PASS (15/15)

| # | 게이트 | 결과 | 실측 |
|---|--------|------|------|
| G01 | `cmake -B build -G Ninja && ninja -C build` 빌드 성공 | [V] PASS | 2026-02-18 exit 0 |
| G02 | `hirct-gen --help` exit 0 | [V] PASS | 2026-02-18 exit 0 |
| G03 | `hirct-verify --help` exit 0 | [V] PASS | 2026-02-18 exit 0 |
| G04 | `hirct-gen test/fixtures/LevelGateway.mlir` → cmodel/ 존재 | [V] PASS | 2026-02-18 cmodel/*.h + *.cpp 생성 확인 |
| G05 | `g++ -std=c++17 -c cmodel/*.cpp` 컴파일 성공 | [V] PASS | 2026-02-18 exit 0 |
| G06 | `make build` exit 0 | [V] PASS | 2026-02-18 exit 0 |
| G07 | `include/hirct/Analysis/ModuleAnalyzer.h` 존재 | [V] PASS | 2026-02-18 파일 존재 |
| G08 | `include/hirct/Support/CirctRunner.h` 존재 | [V] PASS | 2026-02-18 파일 존재 |
| G09 | `lib/Analysis/ModuleAnalyzer.cpp` 존재 | [V] PASS | 2026-02-18 파일 존재 |
| G10 | `lib/Support/CirctRunner.cpp` 존재 | [V] PASS | 2026-02-18 파일 존재 |
| G11 | `tools/hirct-gen/main.cpp` 존재 | [V] PASS | 2026-02-18 파일 존재 |
| G12 | `tools/hirct-verify/main.cpp` 존재 | [V] PASS | 2026-02-18 파일 존재 |
| G13 | `test/fixtures/LevelGateway.mlir` 존재 | [V] PASS | 2026-02-18 784 bytes |
| G14 | `unittests/Analysis/ModuleAnalyzerTest.cpp` 존재 + PASS | [V] PASS | 2026-02-19 gtest 1/1 PASS |
| G15 | `output/.../meta.json` 존재 | [V] PASS | 2026-02-18 10종 emitter 모두 "pass" |
