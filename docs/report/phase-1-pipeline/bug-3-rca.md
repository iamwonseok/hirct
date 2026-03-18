# BUG-3: Verilator RTL Model Build 실패 RCA

> **발견 일시**: 2026-02-19
> **해결 일시**: 2026-02-19
> **영향 범위**: hirct-verify 파이프라인 (G02)

## 증상

hirct-verify 실행 시 Verilator RTL 모델 빌드 단계에서 실패. `make test-verify` 타겟이 Verilator 컴파일 에러로 중단.

## 근본 원인

GenMakefile이 생성하는 Makefile에서 Verilator 호출 옵션이 올바르지 않거나, RTL 소스 경로가 절대 경로로 변환되지 않아 빌드 디렉토리에서 RTL 파일을 찾지 못했음.

- 관련 파일: `lib/Target/GenMakefile.cpp`, `tools/hirct-verify/main.cpp`
- 영향: Verilator가 RTL 소스를 찾지 못해 모델 빌드 실패

## 수정 내용

- `to_absolute_path()` 적용으로 RTL 소스 경로를 절대 경로로 변환
- GenMakefile에서 Verilator include 경로 및 소스 경로 정정

## 검증

- SimpleAnd.v Verilator 빌드: PASS
- SimpleAnd.v verify: PASS (10 seeds × 1000 cycles)
- `make check-hirct` regression: ALL PASS

## 참조

- 109-verify.md G02 게이트
