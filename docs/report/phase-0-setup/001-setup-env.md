# Phase 0 게이트 검증 리포트

> **검증 일시**: 2026-02-18 (최종)
> **브랜치**: `feature/hirct-phase0`
> **환경**: Ubuntu 24.04 (x86_64), kernel 6.14.0-27-generic
>
> **역할**: 이 파일은 게이트 실측 데이터(근거)만 기록한다.
> 체크리스트 진행 상태는 `docs/plans/phase-0-setup/` 각 태스크 문서를 참조.

---

## 종합 판정: [V] ALL PASS (29/29)

| 범주 | PASS | FAIL | SKIP | 합계 |
|------|------|------|------|------|
| 필수: 도구 버전 | 10 | 0 | 0 | 10 |
| 필수: Pre-test | 4 | 0 | 0 | 4 |
| 필수: 빌드 인프라 | 7 | 0 | 0 | 7 |
| 선택 게이트 | 5 | 0 | 0 | 5 |
| 종합 게이트 | 3 | 0 | 0 | 3 |
| **합계** | **29** | **0** | **0** | **29** |

---

## 필수 게이트: 외부 도구 버전 확인

| # | 게이트 | 결과 | Detected | Pinned | 최소 요구 |
|---|--------|------|----------|--------|----------|
| G01 | `circt-verilog --version` | [V] PASS | LLVM 23.0.0git | `5e760efa95e0` | 버전 출력 |
| G02 | `g++ --version` | [V] PASS | 13.3.0 | 13.3.0 | >= 11 |
| G03 | `verilator --version` | [V] PASS | 5.020 | 5.020 | >= 5.020 |
| G04 | `cmake --version` | [V] PASS | 3.28.3 | 3.28.3 | >= 3.20 |
| G05 | `ninja --version` | [V] PASS | 1.11.1 | 1.11.1 | >= 1.10 |
| G06 | `python3 --version` | [V] PASS | 3.12.3 | 3.12.3 | >= 3.10 |
| G07 | `make --version` | [V] PASS | GNU Make 4.3 | 4.3 | >= 4.0 |
| G08 | `clang-format --version` | [V] PASS | 18.1.3 | 18.1.3 | 버전 출력 |
| G09 | `clang-tidy --version` | [V] PASS | 18.1.3 | 18.1.3 | 버전 출력 |
| G10 | `verible-verilog-lint --version` | [V] PASS | 0.0-3824 | 0.0-3824-g27b6347f | 버전 출력 |

## Pre-test 게이트: 외부 도구 동작 확인

| # | 게이트 | 결과 | 비고 |
|---|--------|------|------|
| G11 | circt-verilog MLIR 생성 | [V] PASS | 784 bytes (`hw.module @Fadu_K2_S5_LevelGateway`) |
| G12 | Verilator RTL 모델 빌드 | [V] PASS | `VFadu_K2_S5_LevelGateway__ALL.a` (22,698 bytes) |
| G13 | g++ C++17 컴파일 | [V] PASS | test_compile.cpp exit 0 |
| G14 | Python 모듈 import | [V] PASS | json, pathlib, subprocess, concurrent.futures OK |

## 빌드 인프라 게이트

| # | 게이트 | 결과 | 비고 |
|---|--------|------|------|
| G15 | `utils/setup-env.sh` 존재 + 실행 exit 0 | [V] PASS | executable, exit 0 |
| G16 | 루트 `Makefile` (4 타겟) | [V] PASS | setup/build/lint/clean 전부 존재 |
| G17 | `.clang-format` 존재 | [V] PASS | LLVM BasedOnStyle, ColumnLimit 80 |
| G18 | `test/lit.cfg.py` 존재 | [V] PASS | 스켈레톤 (suffixes: .mlir, .test) |
| G19 | `integration_test/lit.cfg.py` 존재 | [V] PASS | 스켈레톤 (suffixes: .v, .f, .test) |
| G20 | `CIRCT_BUILD` 환경변수 탐지 | [V] PASS | `$HOME/circt/build` 자동 탐지 |
| G21 | `tool-versions.env` CIRCT 커밋 기록 | [V] PASS | `5e760efa95e0` pinned, commit match |

### 추가 인프라 파일

| 파일 | 결과 | 비고 |
|------|------|------|
| `tool-versions.env` | [V] 존재 | 전체 도구 SSOT (pinned + min + license) |
| `requirements.txt` | [V] 존재 | lit, black, flake8, mypy |
| `requirements-optional.txt` | [V] 존재 | cocotb, mkdocs, mkdocs-material |
| `.gitignore` | [V] 적합 | build/, output/, site/, .venv/, obj_dir/ |
| `known-limitations.md` | [V] 존재 | 빈 XFAIL 테이블 |

## 선택 게이트

| # | 게이트 | 결과 | 비고 |
|---|--------|------|------|
| G22 | `vcs -ID` | [V] PASS | V-2023.12-SP2-7 (`-full64` 필수) |
| G23 | `jq --version` | [V] PASS | 1.7 |
| G24 | `black --version` | [V] PASS | 26.1.0 (venv) |
| G25 | `mkdocs --version` | [V] PASS | 1.6.1 (venv) |
| G26 | `shellcheck --version` | [V] PASS | 0.9.0 |

## 종합 게이트

| # | 게이트 | 결과 | 비고 |
|---|--------|------|------|
| G27 | `make setup` exit 0 | [V] PASS | 0건 WARN, 0건 FAIL |
| G28 | 환경 요약표 출력 | [V] PASS | 22개 도구 전부 PASS |
| G-extra | 작업 브랜치 | [V] PASS | `feature/hirct-phase0` |

---

## EDA 도구 설정

| 도구 | 경로 | 버전 | 비고 |
|------|------|------|------|
| VCS | `/tools/synopsys/vcs/V-2023.12-SP2-7` | V-2023.12-SP2-7 | `-full64` 필수 (커널 6.14 호환) |
| ncsim | `/tools/cadence/INCISIVE151` | 15.10-s010 | 정상 동작 |
| Verdi | `/tools/synopsys/verdi/V-2023.12-SP2-7` | V-2023.12-SP2-7 | VCS와 동일 버전 |
| 라이선스 | `27020@fdn37` | — | `tool-versions.env`에 기록 |

## RTL 파일 복구 경위

- **초기 상태**: `rtl/` 디렉토리 구조만 존재, `.v` 파일 0개
- **원인**: 과거 커밋 `66ec6fc`에서 RTL 파일이 git에서 untrack 처리되었으나, 워킹 디렉토리 파일이 유실됨
- **복구**: `git checkout 66ec6fc -- rtl/` + `git reset HEAD -- rtl/` 로 1,597개 .v 파일 untracked 복구
- **검증**: LevelGateway.v MLIR 변환 + Verilator 빌드 모두 성공

---

## 커밋 이력

| 커밋 | 내용 |
|------|------|
| `5ce7024` | feat(phase0): complete Phase 0 environment setup (Task 001) |
