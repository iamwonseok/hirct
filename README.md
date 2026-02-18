# HIRCT — HDL Intermediate Representation Compiler & Tools

> **현재 상태: Phase 0 완료 — Phase 1 대기**
> 빌드 인프라(Makefile, setup-env.sh, lit 설정, 린터)가 구성되었습니다.
> C++ 소스와 CMakeLists.txt는 Phase 1 Bootstrap(Task 100)에서 작성됩니다.

SystemVerilog/Verilog RTL을 입력받아, 컴파일러 인프라(Slang, CIRCT)를 통해 생성된 IR을 기반으로 8종 자동화 산출물을 생성하는 LLVM 기반 통합 파이프라인.

**핵심 원칙**: RTL as Single Source of Truth (SSOT) — RTL 코드가 유일한 진실 공급원이며, 모든 파생 산출물이 자동 생성되어 데이터 불일치를 원천 차단합니다.

---

## 문서 읽기 순서 (SSOT)

이 프로젝트를 이해하려면 다음 순서로 문서를 읽으십시오:

| 순서 | 문서 | 역할 |
|------|------|------|
| 1 | [Proposal](docs/proposal/001-hirct-automation-framework.md) | 프로젝트 제안서 — 배경, 아키텍처, 산출물, 기대 효과 |
| 2 | **[Roadmap](docs/plans/summary.md)** | **총괄 실행 계획 (Roadmap SSOT)** — Phase별 태스크, CLI, 빌드, Agent-in-the-Loop 체크포인트 |
| 3 | [Convention](docs/plans/hirct-convention.md) | 기술 규약 — 검증 방법론, 클럭/리셋, 비트폭, 테스트 규약 |
| 4 | [Reference](docs/plans/reference-commands-and-structure.md) | 최종 목표 구조 — Phase 0~3 완료 후의 디렉토리 트리, Make 타겟, 스키마 |
| 5 | [Open Decisions](docs/plans/open-decisions.md) | 의사결정 추적 (전원 RESOLVED) |
| 6 | [Risk Validation](docs/plans/risk-validation-results.md) | 실측 파싱 성공률 — 1,597개 .v 파일 circt-verilog 테스트 결과 |

> **Agent-in-the-Loop**: Roadmap(`summary.md`) §4.1~§4.4에 Phase별 체크포인트(CP1~CP-3B)가 정의되어 있습니다. 각 체크포인트에서 자동 Gate와 수동 Gate를 통해 개발자가 개입합니다.

---

## 빠른 시작

### 1. 문서 환경 설정

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt                # 필수: black, flake8, mypy, lit
pip install -r requirements-optional.txt       # 선택: cocotb, mkdocs (Phase 3)
```

### 2. 현재 상태 확인

현재 리포지토리의 실제 파일 구성과 Phase 0~3 완료 후 목표 구조를 비교하려면 [STATUS.md](STATUS.md)를 참조하십시오.

---

## 기술 스택

| 계층 | 기술 | 비고 |
|------|------|------|
| Frontend | Slang (SystemVerilog Parser) | `circt-verilog` 내장 |
| Middle-end | LLVM/MLIR, CIRCT | hw/comb/seq/arc dialect |
| Backend | C++17 | `hirct-gen`, `hirct-verify` (자체 개발, **LLVM/MLIR API 미링크**) |
| Verification | Verilator 5.020, VCS | VCS는 Phase 3 |
| Build | CMake + Ninja (바이너리), Make (오케스트레이션), lit (테스트) | |
| Documentation | Markdown, mkdocs | mkdocs는 Phase 3 |
| Python | 3.10+ | 유틸리티 전용 (lit, 리포트 생성, triage) |

> **CIRCT 연동 방식**: HIRCT는 `circt-verilog`와 `circt-opt`를 **외부 프로세스**로 호출합니다(`CirctRunner`). MLIR 라이브러리를 직접 링크하지 않습니다.

---

## 정책

- **Bash 스크립트 금지**: 오케스트레이션은 Makefile + lit + Python으로 수행. 유일한 예외: `utils/setup-env.sh` (환경 부트스트랩, idempotent)
- **원본 RTL 불변**: `rtl/` 디렉토리는 절대 수정하지 않음
- **라이선스**: Apache License v2.0 with LLVM Exceptions ([LICENSE](LICENSE))

---

## 프로젝트 팩트 시트 (PROJECT_CONTEXT)

> LLM 또는 외부 도구가 이 리포지토리를 분석할 때 참고할 요약 정보입니다.

| 항목 | 값 |
|------|------|
| **프로젝트명** | HIRCT (HDL Intermediate Representation Compiler & Tools) |
| **성격** | CIRCT 기반 SoC 설계/검증 자동화 프레임워크 |
| **현재 단계** | Phase 0 완료 — 빌드 인프라 구성됨, C++ 코드는 Phase 1에서 작성 |
| **목표** | RTL → CIRCT IR → 8종 산출물 자동 생성 + 자동 등가성 검증 |
| **라이선스** | Apache License v2.0 with LLVM Exceptions |
| **계획된 기술 스택** | C++17, Python 3.10+, CMake, Ninja, lit, Verilator |
| **CIRCT 연동** | 외부 프로세스 호출 (LLVM/MLIR API 미링크) |
| **Shell 스크립트** | 금지 (예외: `utils/setup-env.sh` 1개) |
| **JS/TS 파일** | 프로젝트 코드 아님 (`.cursor/` IDE 설정 전용) |
| **핵심 용어** | EDA, IR, MLIR, RTL, SSOT, XFAIL, CIRCT |
| **XFAIL 관리** | [known-limitations.md](known-limitations.md) (Phase 2에서 등록) |

---

## 디렉토리 구조 (현재)

```
llvm-cpp-model/          ← 리포지토리명 (프로젝트명: HIRCT)
├── docs/
│   ├── proposal/           # 프로젝트 제안서
│   ├── plans/              # 실행 계획 (Phase 0~3 태스크별 문서)
│   └── report/             # 게이트 검증 리포트 (실측 근거)
├── rtl/                    # Verilog 소스 (1,597 .v 파일, .gitignore)
│   ├── lib/                #   IP 라이브러리 (async_bridge, fifo, ecc, ...)
│   └── plat/               #   플랫폼 RTL (src/ 하위)
├── utils/
│   └── setup-env.sh        # 환경 부트스트랩 (유일 허용 .sh, 멱등성)
├── test/
│   └── lit.cfg.py          # lit 단위 테스트 설정 (Phase 1에서 테스트 추가)
├── integration_test/
│   └── lit.cfg.py          # lit 통합 테스트 설정 (Phase 2에서 테스트 추가)
├── Makefile                # 오케스트레이션 (setup, build, lint, clean)
├── .clang-format           # C/C++ 포맷 (LLVM 기반)
├── .clang-tidy             # C/C++ 정적 분석
├── .pre-commit-config.yaml # pre-commit 훅 설정
├── tool-versions.env       # 도구 버전 SSOT (pinned + min)
├── known-limitations.md    # XFAIL SSOT (현재 비어 있음 — Phase 2에서 등록)
├── requirements.txt        # Python 필수 의존성
├── requirements-optional.txt  # Python 선택 의존성
├── LICENSE                 # Apache-2.0 with LLVM Exceptions
├── STATUS.md               # 현재 상태 vs 목표 상태
└── README.md               # 이 파일
```

> `rtl/` 디렉토리는 `.gitignore`에 등록되어 git에서 추적하지 않습니다. 개발/테스트용 예제이며 실제 환경에서는 별도 경로에 위치합니다.

> Phase 0~3 완료 후의 최종 디렉토리 구조는 [reference-commands-and-structure.md](docs/plans/reference-commands-and-structure.md)를 참조하십시오.
