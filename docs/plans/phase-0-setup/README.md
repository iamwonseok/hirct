# Phase 0: 환경 구성

> **예상 기간**: 2일
> **진입 조건**: 없음
> **완료 기준**: 외부 도구 검증 PASS + 빌드 인프라 파일 생성 + 린터 설정 완료

## 목표

코드를 작성하기 전에 환경을 준비하고, 빌드 인프라를 생성하고, 현재 상태를 정확히 파악한다.

> **Note**: hirct-gen/hirct-verify 바이너리는 Phase 0에서 빌드하지 않는다.
> C++ 소스 코드가 아직 존재하지 않으며, Phase 1 Bootstrap(Task 100)에서 처음 작성된다.

## 태스크

- [001-setup-env.md](001-setup-env.md) — 도구 설치 + 환경 검증 + 빌드 인프라 생성 (1일)
- [002-tools-validation.md](002-tools-validation.md) — 외부 도구 체인 validation (0.5일)
- [003-coding-convention.md](003-coding-convention.md) — 코딩 컨벤션 점검 (0.5일)

## 성공 기준

> **판정: 완료** (2026-02-18) — 29/29 게이트 ALL PASS
> 근거: [게이트 검증 리포트](../../report/phase-0-setup/001-setup-env.md)

- [x] `utils/setup-env.sh` 실행 시 모든 필수 외부 도구 확인 (멱등성 보장) — G15, G27
- [x] `CIRCT_BUILD` 환경변수 설정 + `circt-verilog --version` 정상 출력 — G01, G20
- [x] `circt-verilog` → LevelGateway.v MLIR 변환 성공 (외부 도구 동작 확인) — G11
- [x] `verilator --cc` → LevelGateway.v RTL 모델 빌드 성공 (외부 도구 동작 확인) — G12
- [x] `.clang-format` 생성 + `make lint` 타겟 동작 — G17
- [x] 루트 Makefile 동작 (make setup, make lint, make clean) — G16, G27
- [x] `test/lit.cfg.py` 생성 (Phase 1 lit 테스트 기반) — G18
- [x] `integration_test/lit.cfg.py` 생성 — G19
- [x] `known-limitations.md` 존재 (빈 테이블) — 파일 존재 확인
- [x] 작업 브랜치 생성 완료 (detached HEAD 해소) — G-extra
