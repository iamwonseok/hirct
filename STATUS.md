# HIRCT 프로젝트 상태 (As-Is vs To-Be)

> **목적**: 현재 리포지토리에 실제로 존재하는 것과, Phase 0~3 완료 후 목표 상태를 명확히 분리합니다.
> **최종 갱신**: 2026-02-18 (Phase 0 완료)

---

## As-Is: 현재 존재하는 것

| 분류 | 경로 | 설명 |
|------|------|------|
| 제안서 | `docs/proposal/` | 프로젝트 제안서 (v6.0) |
| 실행 계획 | `docs/plans/` | Phase 0~3 태스크별 상세 계획 (45+ 문서) |
| 게이트 리포트 | `docs/report/phase-0-setup/` | Phase 0 게이트 실측 근거 (001/002/003) |
| RTL 소스 | `rtl/` | Verilog 소스 1,597개 `.v` 파일 — **git 미추적** (`.gitignore`) |
| 오케스트레이션 | `Makefile` | make setup/build/lint/clean (Phase 0 산출물) |
| 환경 스크립트 | `utils/setup-env.sh` | 도구 검증 + 환경변수 설정 (Phase 0 산출물) |
| 도구 버전 SSOT | `tool-versions.env` | pinned + min 버전 레지스트리 (Phase 0 산출물) |
| lit 테스트 설정 | `test/lit.cfg.py` | 단위 테스트 스켈레톤 (Phase 0 산출물, Phase 1에서 테스트 추가) |
| lit 통합 설정 | `integration_test/lit.cfg.py` | 통합 테스트 스켈레톤 (Phase 0 산출물, Phase 2에서 테스트 추가) |
| C/C++ 포맷 | `.clang-format`, `.clang-tidy` | LLVM 기반 스타일 설정 (Phase 0 산출물) |
| pre-commit 설정 | `.pre-commit-config.yaml` | emoji-guard, clang-format, shellcheck, verible, black |
| XFAIL SSOT | `known-limitations.md` | 테이블 헤더만 존재 (Phase 2에서 등록 예정) |
| Python 의존성 | `requirements.txt` | black, flake8, mypy, lit, pre-commit |
| Python 선택 의존성 | `requirements-optional.txt` | cocotb, mkdocs |
| 라이선스 | `LICENSE` | Apache-2.0 with LLVM Exceptions |
| 프로젝트 가이드 | `README.md` | 진입점 문서 |
| 상태 문서 | `STATUS.md` | 이 파일 |

## As-Is: 현재 존재하지 않는 것

> 아래 항목은 Phase 1~3에서 순차적으로 생성됩니다. 계획 문서에서 이들을 참조하더라도 **현재 구현되어 있지 않습니다.**

| 분류 | 경로 | 생성 시점 |
|------|------|----------|
| C++ 헤더 | `include/hirct/` | Phase 1 (Task 100 Bootstrap) |
| C++ 소스 | `lib/Analysis/`, `lib/Target/` | Phase 1 (Task 100~109) |
| CLI 도구 | `tools/hirct-gen/`, `tools/hirct-verify/` | Phase 1 (Task 100) |
| 빌드 시스템 | `CMakeLists.txt` | Phase 1 (Task 100 Bootstrap) |
| gtest 테스트 | `unittests/` | Phase 1 (Task 100+) |
| 생성 산출물 | `output/` | Phase 1+ (hirct-gen 실행 시) |
| 설정 파일 | `config/` | Phase 2 (순회 테스트용) |
| Python 유틸리티 | `utils/generate-report.py`, `utils/triage-failures.py` | Phase 2 |
| 문서 사이트 | mkdocs 빌드 | Phase 3 |
| `scripts/` 디렉토리 | **존재하지 않으며, 생성 계획도 없음** | — |

---

## To-Be: Phase 0~3 완료 후 목표 구조

Phase 0~3이 모두 완료된 후의 최종 프로젝트 구조는 아래 문서를 참조하십시오:

> **[docs/plans/reference-commands-and-structure.md](docs/plans/reference-commands-and-structure.md)**
> 이 문서는 최종 목표 구조이며, 현재 리포지토리에 없는 경로가 포함됩니다.

---

## Phase 진행 상태

| Phase | 기간(계획) | 상태 | 핵심 산출물 |
|-------|----------|------|-----------|
| **Phase 0** | 2일 | **완료** | 도구 설치, 빌드 인프라, 컨벤션 점검 |
| **Phase 1A** | 16일 | 미시작 | Core pipeline (gen-model, gen-tb, gen-doc, verify) |
| **Phase 1B** | 12.5일 | 미시작 | Remaining emitters (gen-dpic, gen-wrapper, gen-format, gen-ral, gen-cocotb) |
| **Phase 2** | 12일 | 미시작 | ~1,600 .v 전체 순회 테스트 |
| **Phase 3** | 5일 | 미시작 | VCS co-sim, mkdocs, 패키징 |
