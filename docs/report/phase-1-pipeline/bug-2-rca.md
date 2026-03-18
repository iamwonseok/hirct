# BUG-2: Comparison Driver Build 실패 RCA

> **발견 일시**: 2026-02-19
> **해결 일시**: 2026-02-19
> **영향 범위**: hirct-verify 파이프라인 (G03)

## 증상

hirct-verify 실행 시 비교 드라이버(`verify_<module>.cpp`) 빌드 단계에서 컴파일 오류 발생.

## 근본 원인

GenVerify가 생성하는 verify 드라이버 코드에서 C++ model 헤더 include 경로가 올바르지 않거나, 생성된 코드의 타입 불일치가 있었음.

- 관련 파일: `lib/Target/GenVerify.cpp`
- 영향: verify 드라이버가 RTL model 클래스 헤더를 찾지 못하거나, 포트 타입 캐스팅 문제

## 수정 내용

GenVerify의 드라이버 코드 생성 로직 수정:
- include 경로 정정 (`../cmodel/<module>.h`)
- 출력 포트 비교 시 `static_cast<uint64_t>()` 적용으로 타입 일치

## 검증

- SimpleAnd.v verify: PASS (10 seeds × 1000 cycles)
- LevelGateway verify: PASS (10 seeds × 1000 cycles)
- `make check-hirct` regression: ALL PASS

## 참조

- 109-verify.md G03 게이트
- 111-cli.md G03 게이트
