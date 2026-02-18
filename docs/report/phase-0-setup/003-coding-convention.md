# Task 003 게이트 검증 리포트: 코딩 컨벤션 점검

> **검증 일시**: 2026-02-18
> **브랜치**: `feature/hirct-phase0`
> **환경**: Ubuntu 24.04 (x86_64), kernel 6.14.0-27-generic
>
> **역할**: Task 003 검증 항목의 실측 데이터를 기록한다.
> Task 001에서 생성된 파일(`.clang-format`, `Makefile`)의 근거는 [Task 001 리포트](001-setup-env.md) 참조.

---

## 종합 판정: [V] 5/7 PASS (2건 N/A — Phase 1/2 범위)

---

## .clang-format 검증

> Task 001 G17에서 생성 확인됨.

| 항목 | 설정 값 | cpp.md 컨벤션 부합 |
|------|---------|-------------------|
| `BasedOnStyle` | LLVM | [V] CIRCT 스타일 기반 |
| `ColumnLimit` | 80 | [V] |
| `IndentWidth` | 2 | [V] |
| `UseTab` | Never | [V] |
| `BreakBeforeBraces` | Attach | [V] |
| `AllowShortFunctionsOnASingleLine` | All | [V] |

## verible Verilog 린트 설정

> Task 001 G10에서 verible 설치 확인됨.

| 항목 | 결과 | 실측 |
|------|------|------|
| verible 설치 | [V] PASS | 0.0-3824-g27b6347f |
| `make lint-sv` 타겟 존재 | [V] PASS | Makefile `lint-sv`, `HAVE_VERIBLE` 자동 감지 |
| verible rule 파일 (.rules.verible_lint) | — | 기본 룰 사용 중 (커스텀 룰 미생성) |
| SV 파일 대상 lint 실행 | N/A | 현재 SV 파일 없음 (Phase 1에서 생성) |

## shellcheck Shell 린트 설정

> Task 001 G26에서 shellcheck 설치 확인됨.

| 항목 | 결과 | 실측 |
|------|------|------|
| shellcheck 설치 | [V] PASS | 0.9.0 |
| `make lint-sh` 서브타겟 존재 | [V] PASS | Makefile `lint-sh`, `HAVE_SHELLCHECK` 자동 감지 |
| `make lint-sh` → `utils/setup-env.sh` 통과 | [V] PASS | exit 0, 경고 0건 |

## make lint 서브타겟별 상세

| 서브타겟 | 결과 | 비고 |
|---------|------|------|
| `lint-cpp` | [V] PASS (skip) | C/C++ 파일 없음 → graceful skip |
| `lint-sv` | [V] PASS (skip) | SV 파일 없음 → graceful skip |
| `lint-py` | [V] PASS (skip) | Python 파일 없음 → graceful skip |
| `lint-sh` | [V] PASS | `utils/setup-env.sh` 검사 완료, 경고 0건 |
| 전체 `make lint` | [V] exit 0 | 4개 서브타겟 정상 동작 |

> **Note**: C++/SV/Python은 lint 대상 소스 파일이 없어 graceful skip.
> Phase 1에서 파일 생성 후 실제 lint 동작을 재검증해야 한다.

## Makefile lint (checkmake)

| 항목 | 상태 | 비고 |
|------|------|------|
| checkmake 도입 | Phase 1+ 이관 | 새 도구 설치 필요, Phase 0 범위 외 |

## 생성 코드 lint 정책

| 정책 | 상태 | 비고 |
|------|------|------|
| Policy B: `output/**` lint 제외 | N/A | Phase 1 범위 |
| Policy A: `output/**` lint 포함 | N/A | Phase 2 범위 |
