# Task 001: 도구 설치 + 환경 검증 + 빌드 인프라 생성

> **목표**: 모든 Phase(0~3)에 필요한 외부 도구를 설치·검증하고, 빌드 인프라 파일을 생성한다
> **예상 시간**: 1일
> **산출물**: `utils/setup-env.sh`, `Makefile` (make setup/build/lint/clean 타겟), `.clang-format`, `test/lit.cfg.py`
> **원칙**: 패키지 매니저 우선 + **멱등성(Idempotency)** — 여러 번 실행해도 안전
> **정책 예외**: `utils/setup-env.sh`는 프로젝트에서 유일하게 허용된 `.sh` 파일 (환경 부트스트랩 용도)
>
> **비대화형 실행 정책**: `setup-env.sh`는 비대화형(non-interactive) 환경에서도 안전하게 실행되어야 한다.
> - 모든 `apt` 명령에 `DEBIAN_FRONTEND=noninteractive` + `-y` 플래그 필수
> - `sudo` 불가 시: 사전 설치 요구사항 문서 참조 또는 Docker 경로 안내 (에러 메시지에 포함)
> - CI 최소 경로: `make setup` 스킵 → `make build` + `make test-all`만 실행 (도구가 이미 설치된 환경)
>
> **주의**: 이 태스크에서는 **외부 도구만 검증**한다. hirct-gen/hirct-verify 바이너리 빌드는
> C++ 소스 코드가 필요하며, Phase 1 Bootstrap(Task 100)에서 수행한다.

---

## 지원 환경 (Supported Platforms)

| 항목 | 요구사항 |
|------|---------|
| **OS** | Ubuntu 22.04 LTS+ (x86_64) |
| **패키지 매니저** | apt (Debian 계열) |
| **아키텍처** | x86_64 (arm64는 미검증) |
| **커널** | Linux 5.15+ |
| **셸** | bash 5.0+ (setup-env.sh 실행용) |

> **Note**: macOS, Windows(WSL2 포함)는 공식 지원하지 않는다. Docker 컨테이너 사용을 권장한다.

---

## 왜 Phase 0에서 전부 검증하는가

Phase 1~3에서 "도구 없음"으로 실패하면 Phase 0로 되돌아와야 한다. 선행 검증 비용(수 분)이 되돌림 비용(수 시간)보다 압도적으로 작으므로, **모든 도구를 Phase 0에서 한 번에 검증**한다.

실패 시 영향:

```
도구 실패              → 영향받는 Phase/Task
─────────────────────────────────────────────
circt-verilog          → Phase 1 전체 (MLIR 생성 불가)
g++ (C++17)            → Phase 1 전체 (cmodel 컴파일 불가)
verilator              → Phase 1 (109-verify), Phase 2 전체
cmake + ninja          → Phase 1 전체 (hirct-gen 빌드 불가)
python3 (3.10+)        → Phase 2 전체 (테스트 러너), Phase 3 (mkdocs)
jq                     → Phase 2 (리포트 게이트 검증)
vcs                    → Phase 3 (301 VCS co-sim)
clang-format           → Phase 0 (003-lint), Phase 1~2 (make lint)
verible                → Phase 0 (003-lint), Phase 1~2 (SV lint)
mkdocs                 → Phase 3 (302-documentation)
```

---

## 도구 목록 (전체)

### 필수 도구 (Mandatory) — 실패 시 즉시 중단

> **CIRCT/LLVM 버전 고정**: 재현 가능한 빌드를 위해 검증된 CIRCT 커밋을 사용한다.
> 현재 검증 기준: **CIRCT `main` @ 2026-02-01 이후** (Slang 통합, `circt-verilog` 포함 빌드).
> 구체적 커밋 해시는 첫 빌드 성공 시 `tool-versions.env`의 `CIRCT_COMMIT` 변수로 고정한다.

| 도구 | 최소 버전 | 용도 | 설치 방법 | 검증 명령 |
|------|----------|------|----------|----------|
| **circt-verilog** | CIRCT main (위 Note 참조) | Verilog → MLIR 변환 | 로컬 빌드 경로 검증 | `circt-verilog --version` |
| **g++** | 11+ (C++17) | cmodel 컴파일, verify 드라이버 빌드 | `apt install g++` | `g++ --version` |
| **verilator** | 5.020 | RTL → C++ 시뮬레이션 모델 | `apt` 또는 소스 빌드 | `verilator --version` |
| **cmake** | 3.20+ | hirct-gen 빌드 시스템 | `apt install cmake` | `cmake --version` |
| **ninja** | 1.10+ | 빌드 백엔드 | `apt install ninja-build` | `ninja --version` |
| **python3** | 3.10+ | 테스트 러너, 리포트, 검증 스크립트 | 시스템 패키지 | `python3 --version` |
| **GNU Make** | 4.0+ | 오케스트레이션 (make test-all 등) | 시스템 기본 | `make --version` |
| **git** | 2.30+ | 버전 관리 | 시스템 기본 | `git --version` |

### 필수 린터 (Mandatory) — 미설치 시 설치 후 계속

> **Note**: 아래 도구들이 시스템에 미설치일 수 있다. `setup-env.sh`가 자동 설치를 시도한다.
> 설치 실패 시 **경고를 출력하고 계속 진행**하되, `make lint` 타겟에서 해당 도구를 스킵한다.
> Phase 1 시작 전까지 설치를 완료해야 한다.

| 도구 | 용도 | 설치 방법 | 검증 명령 |
|------|------|----------|----------|
| **clang-format** | C/C++ 포맷팅 | `sudo apt install -y clang-format-15` 또는 `sudo apt install -y clang-format` | `clang-format --version` |
| **clang-tidy** | C/C++ 정적 분석 | `sudo apt install -y clang-tidy-15` 또는 `sudo apt install -y clang-tidy` | `clang-tidy --version` |
| **verible** | Verilog/SV 포맷팅·린트 | `curl -fsSL https://github.com/chipsalliance/verible/releases/latest/download/verible-$(uname -s)-$(uname -m).tar.gz \| tar xz -C /usr/local/bin --strip-components=2` 또는 수동 다운로드 | `verible-verilog-lint --version` |
| **lit** | LLVM 테스트 러너 | `pip install lit` (requirements.txt에 포함) | `lit --version` |

### Python 패키지 (pip) — requirements.txt로 관리

**필수** (`requirements.txt` — `pip install -r requirements.txt`):

| 패키지 | 최소 버전 | 용도 | Phase | 검증 명령 |
|--------|----------|------|-------|----------|
| **black** | 24.0 | Python 포매터 | 0~2 | `black --version` |
| **flake8** | 7.0 | Python 린터 | 0~2 | `flake8 --version` |
| **mypy** | 1.8 | Python 타입 체크 | 0~2 | `mypy --version` |

**선택** (`requirements-optional.txt` — `pip install -r requirements-optional.txt`):

| 패키지 | 최소 버전 | 용도 | Phase | 검증 명령 |
|--------|----------|------|-------|----------|
| **cocotb** | 1.9 | Python 테스트벤치 | 2+ | `python3 -c "import cocotb"` |
| **mkdocs** | 1.6 | 문서 사이트 생성 | 3 | `mkdocs --version` |
| **mkdocs-material** | 9.5 | mkdocs 테마 | 3 | (mkdocs와 함께) |

### 선택 시스템 도구 (Optional) — 실패 시 경고만 출력

| 도구 | 용도 | Phase | 설치 방법 | 검증 명령 |
|------|------|-------|----------|----------|
| **vcs** | VCS co-simulation (1차 게이트) | 3 | 경로 검증만 | `vcs -ID` |
| **ncsim** | Cadence co-simulation | 3 | 경로 검증만 | `ncsim -version` |
| **jq** | JSON 리포트 게이트 검증 | 2 | `apt install jq` | `jq --version` |
| **shellcheck** | Shell 린트 (inline recipe 검증) | 0 | `apt install shellcheck` | `shellcheck --version` |
| **checkmake** | Makefile 린터 | 0 | `go install .../checkmake@latest` | `checkmake --version` |

### LLVM/CIRCT 빌드 의존성 (빌드 환경)

| 도구 | 용도 | 검증 명령 |
|------|------|----------|
| **llvm-config** | LLVM 빌드 경로 | `llvm-config --version` |
| **circt-opt** | CIRCT flatten pass 등 | `circt-opt --version` |

---

## 주요 작업

### 0단계: 작업 브랜치 생성 (detached HEAD 해소)

> **현재 상태**: 이 worktree는 detached HEAD (`9cb42a1`) 상태다.
> 작업을 시작하기 전에 브랜치를 생성하고 checkout해야 한다.

**브랜치 전략 (최소 규칙)**:

| Phase | 브랜치 이름 | 기준 |
|-------|------------|------|
| Phase 0 | `feature/hirct-phase0` | 현재 HEAD에서 분기 |
| Phase 1A | `feature/hirct-phase1a` | Phase 0 완료 커밋에서 분기 (또는 Phase 0 브랜치 연장) |
| Phase 1B | Phase 1A 브랜치 연장 (코드 변경 순차이므로 별도 불필요) | — |
| Phase 2 | Phase 1 브랜치 연장 또는 `feature/hirct-phase2` | — |
| Phase 3 | Phase 2 브랜치 연장 또는 `feature/hirct-phase3` | — |

**원칙**:
- Phase 간 전환 시 기존 브랜치를 연장하거나 새 브랜치를 분기하되, **main에는 Phase 3 완료 후에만 머지**.
- 워크트리 분리가 필요하면 `using-git-worktrees` 스킬로 별도 worktree 생성.

```bash
git checkout -b feature/hirct-phase0
```

### 1단계: 필수 도구 설치·검증 (실패 시 즉시 중단)
- g++, cmake, ninja, python3, make, git 버전 확인
- CIRCT/LLVM 빌드 경로 검증 (circt-verilog, circt-opt)
- Verilator 5.020 설치·검증
- 린터 설치·검증 (clang-format, clang-tidy, verible)

### 2단계: 환경변수 설정 + 빌드 인프라 파일 생성

**필수 환경변수** (setup-env.sh가 자동 탐지하거나 사용자가 수동 설정):

```bash
# CIRCT/LLVM 빌드 경로 (필수)
export CIRCT_BUILD="$HOME/circt/build"                  # CIRCT 빌드 루트
export MLIR_DIR="$CIRCT_BUILD/lib/cmake/mlir"           # CMake 자동 참조
export LLVM_DIR="$CIRCT_BUILD/lib/cmake/llvm"           # CMake 자동 참조

# PATH 확장 (circt-verilog, circt-opt 등)
export PATH="$CIRCT_BUILD/bin:$PATH"
```

**탐지 로직**: `setup-env.sh`는 아래 순서로 `CIRCT_BUILD`를 탐지한다:
1. 환경변수 `CIRCT_BUILD`가 이미 설정되어 있으면 사용
2. `$HOME/circt/build`가 존재하면 사용
3. 위 모두 실패 → Error: `CIRCT_BUILD 환경변수를 설정하세요`

**빌드 인프라 파일 생성** (Phase 0에서 생성, Phase 1에서 확장):
- `utils/setup-env.sh` — 도구 검증 + 환경변수 설정 (위 탐지 로직 구현)
- 루트 `Makefile` 초기 버전 작성 (make setup, make build, make lint, make clean 타겟)
- `.clang-format` — LLVM/CIRCT 표준 스타일 기반 C/C++ 포맷 설정
- `test/lit.cfg.py` — lit 테스트 기본 설정 (Phase 1에서 테스트 추가)
- `integration_test/lit.cfg.py` — 통합 테스트 기본 설정

> **Note**: `make build` 타겟은 Phase 0에서 생성하지만, 실제 빌드 대상(CMakeLists.txt, C++ 소스)은
> Phase 1 Bootstrap(Task 100)에서 작성된다. Phase 0의 `make build`는 "CMakeLists.txt 부재 시 안내 메시지 출력"으로 구현한다.

### 3단계: Python 패키지 설치 (requirements.txt)
- `pip install -r requirements.txt` → black, flake8, mypy (필수 린터)
- `pip install -r requirements-optional.txt` → cocotb, mkdocs (선택, 실패 시 경고)
- venv 사용 권장: `python3 -m venv .venv && source .venv/bin/activate`

### 4단계: 선택 시스템 도구 설치·검증 (실패 시 경고 후 계속)
- VCS/ncsim: 경로 검증 (`vcs -ID`)
- jq: `apt install jq`
- shellcheck, checkmake

### 5단계: Pre-test (외부 도구 체인 스모크 테스트)

> **Note**: hirct-gen 바이너리는 아직 존재하지 않으므로, 외부 도구만 검증한다.
> hirct-gen 파이프라인 스모크 테스트는 Phase 1 Bootstrap(Task 100) 완료 후 수행한다.

- `circt-verilog rtl/.../Fadu_K2_S5_LevelGateway.v` → MLIR 변환 성공 확인
- `verilator --cc rtl/.../Fadu_K2_S5_LevelGateway.v` → Verilator RTL 모델 빌드 확인
- `g++ -std=c++17` → C++17 컴파일 능력 확인 (간단한 테스트 파일)
- `python3 -c "import json, pathlib, subprocess, concurrent.futures; print('OK')"` → Python 모듈 확인
- `make setup` 타겟 동작 확인

### 6단계: 환경 요약 출력
- 모든 도구의 버전을 표로 출력
- 선택 도구 중 미설치 항목 경고 목록 출력
- Phase별 "사용 가능/불가능" 매트릭스 출력

---

## Pre-test 상세

Phase 0에서 아래 **외부 도구 스모크 테스트**를 통과해야 후속 Phase 진입이 허용된다:

> **Note**: hirct-gen 바이너리가 아직 존재하지 않으므로 (Phase 1 Task 100에서 작성),
> Phase 0 Pre-test는 외부 도구 체인만 검증한다.

```
Pre-test 1: MLIR 생성
  circt-verilog rtl/.../Fadu_K2_S5_LevelGateway.v → .mlir 파일 생성

Pre-test 2: Verilator RTL 모델 빌드
  verilator --cc rtl/.../Fadu_K2_S5_LevelGateway.v → obj_dir/ 생성

Pre-test 3: C++ 컴파일 능력
  g++ -std=c++17 -c <테스트 파일> → exit 0

Pre-test 4: Python 러너 의존성
  python3 -c "import json, pathlib, subprocess, concurrent.futures" → exit 0

Pre-test 5 (선택): VCS 접근
  vcs -ID → 버전 출력 (실패 시 Phase 3 일부 스킵 경고)
```

**Pre-test 실패 시 처리**:
- Pre-test 1 실패: **Phase 1 진입 불가** → CIRCT 빌드 확인 필수
- Pre-test 2 실패: **Phase 1 진입 불가** → Verilator 설치 확인 필수
- Pre-test 3 실패: **Phase 1 진입 불가** → g++ 설치 확인 필수
- Pre-test 4 실패: Phase 2 테스트 러너 사용 불가 → Python 설치 필수
- Pre-test 5 실패: Phase 3 VCS co-sim 스킵 경고

---

## 게이트 (완료 기준)

### 필수 게이트: 외부 도구 버전 확인 (10/10 PASS)

> 근거: [게이트 검증 리포트](../../report/phase-0-setup/001-setup-env.md)

- [x] `circt-verilog --version` → 버전 출력 — G01: LLVM 23.0.0git
- [x] `g++ --version` → 11 이상 — G02: 13.3.0
- [x] `verilator --version` → 5.020 이상 — G03: 5.020
- [x] `cmake --version` → 3.20 이상 — G04: 3.28.3
- [x] `ninja --version` → 1.10 이상 — G05: 1.11.1
- [x] `python3 --version` → 3.10 이상 — G06: 3.12.3
- [x] `make --version` → GNU Make 4.0 이상 — G07: 4.3
- [x] `clang-format --version` → 버전 출력 — G08: 18.1.3
- [x] `clang-tidy --version` → 버전 출력 — G09: 18.1.3
- [x] `verible-verilog-lint --version` → 버전 출력 — G10: 0.0-3824

### Pre-test 게이트: 외부 도구 동작 확인 (4/4 PASS)
- [x] Pre-test 1: circt-verilog MLIR 생성 성공 — G11: 784 bytes
- [x] Pre-test 2: Verilator RTL 모델 빌드 성공 — G12: 22,698 bytes
- [x] Pre-test 3: g++ C++17 컴파일 성공 — G13
- [x] Pre-test 4: Python 모듈 import 성공 — G14

### 빌드 인프라 게이트 (7/7 PASS)
- [x] `utils/setup-env.sh` 생성 + 실행 시 exit 0 — G15
- [x] 루트 `Makefile` 생성 (make setup, make build, make lint, make clean 타겟) — G16
- [x] `.clang-format` 생성 — G17
- [x] `test/lit.cfg.py` 생성 — G18
- [x] `integration_test/lit.cfg.py` 생성 — G19
- [x] `CIRCT_BUILD` 환경변수 설정 확인 — G20
- [x] `tool-versions.env`에 CIRCT 커밋 해시·날짜 기록 (`CIRCT_COMMIT`, `CIRCT_DATE`) — G21

### CIRCT 버전 고정 (tool-versions.env SSOT)

검증 완료된 CIRCT 커밋을 `tool-versions.env`의 `CIRCT_COMMIT` / `CIRCT_DATE` 변수에 기록한다:

```bash
# tool-versions.env (수동 편집 — SSOT)
CIRCT_COMMIT="5e760efa95e0e2b6a98339d656345818b70d416f"
CIRCT_DATE="2026-02-17"
```

`setup-env.sh`가 `tool-versions.env`를 source하여 현재 CIRCT 빌드 커밋과 비교:
- 일치 → PASS
- 불일치 → `WARN: CIRCT version mismatch: expected <expected>, found <actual>` 경고 출력 (중단하지 않음)
- `tool-versions.env` 미존재 → FAIL (SSOT 파일 필수)

### 선택 게이트 (5/5 PASS)
- [x] `vcs -ID` → 버전 출력 (Phase 3 VCS co-sim) — G22: V-2023.12-SP2-7
- [x] `jq --version` → 버전 출력 (Phase 2 리포트 검증) — G23: 1.7
- [x] `black --version` → 버전 출력 (Python 포맷) — G24: 26.1.0
- [x] `mkdocs --version` → 버전 출력 (Phase 3 문서) — G25: 1.6.1
- [x] `shellcheck --version` → 버전 출력 (Shell lint) — G26: 0.9.0

### 종합 (2/2 PASS)
- [x] `make setup` → exit 0 (전체 설치·검증·pre-test 완료) — G27
- [x] 환경 요약표 출력 (도구별 버전 + Phase별 가용성) — G28
