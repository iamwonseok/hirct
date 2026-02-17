# HIRCT 레퍼런스: 디렉터리 구조 · 명령어 · Make 타겟

> **주의: 이 문서는 Phase 0~3 완료 후의 최종 목표 구조입니다.**
> 현재 리포지토리에는 이 문서에 기술된 경로(include/, lib/, tools/, CMakeLists.txt, Makefile 등)가
> 아직 존재하지 않습니다. 현재 실제 파일 구성은 [STATUS.md](../../STATUS.md)를 참조하십시오.

> **목적**: Phase 0~3 완료 후 프로젝트의 최종 형태를 한 문서로 조망
> **버전**: 2.1 (2026-02-17)

### 변경 이력

| 버전 | 날짜 | 변경 내용 |
|------|------|----------|
| 1.0 | 2026-02-15 | 초안 작성 |
| 2.0 | 2026-02-16 | CIRCT 스타일 디렉터리 전환, lit 기반 테스트 인프라, scripts/ 제거, meta.json·known-limitations 스키마 추가, 마이그레이션 부록 |
| 2.1 | 2026-02-17 | 상단 경고 블록 추가 (현재 상태 vs 목표 상태 혼동 방지), STATUS.md 링크 |

---

## 1. 최종 프로젝트 디렉터리 구조

Phase 0~3이 모두 완료된 후의 전체 트리 (CIRCT 스타일 v2):

```
llvm-cpp-model/
│
├── include/hirct/                       # 공개 헤더
│   ├── Analysis/
│   │   └── ModuleAnalyzer.h             #   IR 분석 (공통 기반)
│   └── Target/
│       ├── GenModel.h                   #   C++ cycle-accurate 모델
│       ├── GenTB.h                      #   SV 테스트벤치
│       ├── GenDPIC.h                    #   DPI-C VCS 래퍼
│       ├── GenWrapper.h                 #   SV Interface 래퍼
│       ├── GenFormat.h                  #   IR 기반 RTL 포매터
│       ├── GenDoc.h                     #   HW Doc + Programmer's Guide
│       ├── GenRAL.h                     #   UVM RAL + HAL + C 드라이버
│       ├── GenCocotb.h                  #   Python testbench
│       ├── GenVerify.h                  #   verify 드라이버 생성
│       └── GenMakefile.h               #   per-module Makefile 생성
│
├── lib/                                 # 라이브러리 구현
│   ├── Analysis/
│   │   ├── ModuleAnalyzer.cpp
│   │   └── CMakeLists.txt
│   ├── Target/
│   │   ├── GenModel.cpp
│   │   ├── GenTB.cpp
│   │   ├── GenDPIC.cpp
│   │   ├── GenWrapper.cpp
│   │   ├── GenFormat.cpp
│   │   ├── GenDoc.cpp
│   │   ├── GenRAL.cpp
│   │   ├── GenCocotb.cpp
│   │   ├── GenVerify.cpp
│   │   ├── GenMakefile.cpp
│   │   └── CMakeLists.txt
│   └── CMakeLists.txt
│
├── tools/                               # CLI 바이너리 (thin main.cpp만)
│   ├── hirct-gen/
│   │   ├── main.cpp                     #   hirct-gen CLI 진입점
│   │   └── CMakeLists.txt
│   └── hirct-verify/
│       ├── main.cpp                     #   hirct-verify CLI 진입점 (별도 바이너리)
│       └── CMakeLists.txt
│
├── test/                                # lit/FileCheck 단위 테스트
│   ├── Target/
│   │   ├── GenModel/
│   │   ├── GenTB/
│   │   ├── GenDPIC/
│   │   ├── GenWrapper/
│   │   ├── GenFormat/
│   │   ├── GenDoc/
│   │   ├── GenRAL/
│   │   ├── GenCocotb/
│   │   ├── GenVerify/
│   │   └── GenMakefile/
│   ├── Analysis/
│   ├── Tools/hirct-gen/
│   ├── lit.cfg.py
│   └── CMakeLists.txt
│
├── unittests/                           # gtest C++ API 테스트
│   ├── Analysis/
│   │   └── ModuleAnalyzerTest.cpp
│   ├── Target/
│   └── CMakeLists.txt
│
├── integration_test/                    # lit E2E 통합 테스트
│   ├── smoke/                           #   대표 모듈 (CI 대상)
│   ├── hirct-gen/
│   ├── hirct-verify/
│   ├── traversal/                       #   Phase 2, ~1,600 .v 파일 (CI 제외)
│   │   └── lit.cfg.py                   #     .v 파일용 커스텀 포맷
│   ├── lit.cfg.py
│   └── CMakeLists.txt
│
├── utils/
│   ├── setup-env.sh                     # 유일 허용 .sh (멱등성 필수)
│   ├── generate-report.py               # lit xunit XML + meta.json → JSON 리포트
│   ├── triage-failures.py               # 자동 실패 분류 (Phase 2 Task 206)
│   └── parse_known_limitations.py       # known-limitations.md 파서 (lit XFAIL 연동)
│
├── config/
│   └── generate.f                       # make generate 대상 filelist
│
├── rtl/                                 # 입력 RTL (읽기 전용, SSOT)
│   └── plat/src/
│       ├── s5/design/*.v
│       ├── s5mc/design/*.v
│       ├── edma/*.v
│       ├── uart/*.v
│       └── ...                          (~1,600 .v 파일)
│
├── output/                              # 생성 산출물 (gitignored)
│   ├── <path>/<file>/                   #   per-module 산출물
│   │   ├── meta.json                    #     항상 생성 (hirct-gen)
│   │   ├── Makefile                     #     자동 생성 (GenMakefile.cpp)
│   │   ├── cmodel/                      #     C++ 모델 (.h + .cpp)
│   │   ├── tb/                          #     SV 테스트벤치 (.sv)
│   │   ├── dpi/                         #     DPI-C 래퍼 (.h + .cpp + .sv)
│   │   ├── wrapper/                     #     SV Interface 래퍼 (.sv)
│   │   ├── rtl/                         #     포맷팅된 RTL (.v)
│   │   ├── doc/                         #     HW Spec + Prog Guide (.md)
│   │   ├── cocotb/                      #     Python testbench (.py)
│   │   ├── ral/                         #     UVM RAL (조건부, _ral.sv + _hal.h + _driver.c)
│   │   └── verify/                      #     검증 산출물 (hirct-verify 시)
│   ├── lit-check.xml                    #   lit test/ 결과
│   ├── lit-integration.xml              #   lit integration_test/smoke/ 결과
│   ├── lit-traversal.xml                #   lit integration_test/traversal/ 결과
│   ├── report.json                      #   전체 RTL 순회 리포트 (Phase 2)
│   └── verify-report.json               #   전체 검증 리포트 (Phase 2)
│
├── build/                               # CMake 빌드 디렉터리
│   └── tools/
│       ├── hirct-gen/hirct-gen          #   빌드된 바이너리
│       └── hirct-verify/hirct-verify    #   빌드된 바이너리
│
├── site/                                # mkdocs 빌드 산출물 (Phase 3)
│   └── index.html
│
├── docs/
│   ├── proposal/
│   │   └── 001-hirct-automation-framework.md
│   └── plans/
│       ├── summary.md                   # 총괄 계획
│       ├── open-decisions.md            # 결정 사항 (RESOLVED = 증거 기반)
│       ├── hirct-convention.md          # 공통 규약 (canonical)
│       ├── conventions.md               # 규약 보조
│       ├── reference-commands-and-structure.md  # 이 문서
│       ├── phase-0-setup/
│       ├── phase-1-pipeline/
│       ├── phase-2-testing/
│       └── phase-3-release/
│
├── known-limitations.md                 # XFAIL SSOT (Markdown 테이블, key=파일 경로)
├── Makefile                             # 루트 오케스트레이션
├── CMakeLists.txt                       # vanilla CMake (cmake/modules/ 없음)
├── .clang-format                        # C/C++ 포맷 설정
├── .clang-tidy                          # C/C++ 린트 설정
├── CIRCT_VERSION                        # CIRCT 빌드 커밋 해시 + 날짜
├── .gitignore                           # build/, output/, site/
├── mkdocs.yml                           # 문서 사이트 설정 (Phase 3)
└── README.md                            # Quick Start + 프로젝트 소개
```

---

## 1.1 전제 조건 (Prerequisites)

아래 환경이 갖춰져야 이 문서의 명령어/타겟이 동작한다.

| 전제 조건 | 설정 방법 | 생성 Phase |
|----------|----------|-----------|
| **CIRCT/LLVM 빌드** | `$CIRCT_BUILD` 환경변수 설정 (001-setup-env.md §2단계 참조) | Phase 0 |
| **hirct-gen/hirct-verify 바이너리** | `make build` (CMake + Ninja). **Phase 0**: C++ 소스 미존재이므로 안내 메시지만 출력. **Phase 1+**: Task 100 이후 실제 빌드 | Phase 0 (스켈레톤) → Phase 1+ (실제) |
| **lit** | `pip install lit` (LLVM lit 테스트 러너) | Phase 0 |
| **config/generate.f** | `find rtl/ -name "*.v" -type f \| sort > config/generate.f` | Phase 2 (201) |
| **output/ 산출물** | `make generate` | Phase 1+ |

**필수 환경변수**:

```bash
export CIRCT_BUILD="$HOME/circt/build"          # CIRCT 빌드 루트
export PATH="$CIRCT_BUILD/bin:$PATH"             # circt-verilog, circt-opt
export MLIR_DIR="$CIRCT_BUILD/lib/cmake/mlir"    # CMake 참조
export LLVM_DIR="$CIRCT_BUILD/lib/cmake/llvm"    # CMake 참조
```

---

## 2. CLI 명령어 세트

### 2.1 hirct-gen (산출물 생성)

| 명령어 | 설명 | Phase |
|--------|------|-------|
| `hirct-gen input.v` | 단일 파일 → 8종 산출물 + meta.json + Makefile 생성 | 1 |
| `hirct-gen input.v -o path/` | 출력 경로 지정 | 1 |
| `hirct-gen input.v --only model,tb` | 선택적 산출물 생성 (쉼표 구분) | 1 |
| `hirct-gen -f filelist.f --top Top` | 다중 파일 + Top 모듈 지정 | 1 |
| `hirct-gen -f filelist.f --top Top --timescale 1ps/1ps` | multi-file timescale 오버라이드 | 1 |
| `hirct-gen --help` | 사용법 출력 | 1 |

**--only 필터 값**: `model`, `tb`, `dpic`, `wrapper`, `format`, `doc`, `ral`, `cocotb`

**--top 규칙**:

| 모드 | 조건 | --top |
|------|------|-------|
| 단일 파일, 모듈 1개 | 자동 지정 | 불필요 |
| 단일 파일, 모듈 2개+ | 미지정 시 Error | **필수** |
| `-f filelist.f` | 항상 | **필수** |

### 2.2 hirct-verify (자동 등가성 검증)

| 명령어 | 설명 | Phase |
|--------|------|-------|
| `hirct-verify input.v` | 10시드 x 1000cyc 자동 검증 | 1 |
| `hirct-verify input.v --seeds 5` | 시드 수 조정 | 1 |
| `hirct-verify input.v --cycles 100` | 사이클 수 조정 | 1 |

**내부 동작 순서**:
1. hirct-gen으로 cmodel 생성 (이미 있으면 스킵)
2. Verilator로 RTL → C++ 시뮬레이션 모델 빌드
3. IR 포트 정보로 `verify_<module>.cpp` 자동 생성
4. 드라이버 빌드 (cmodel + verilator obj 링크)
5. 다중 시드 x N사이클 실행, PASS/FAIL 출력

### 2.3 lit 테스트 명령어

| 명령어 | 설명 | Phase |
|--------|------|-------|
| `lit test/ -v` | emitter 단위 테스트 (FileCheck) | 1+ |
| `lit test/ --xunit-xml-output output/lit-check.xml` | 단위 테스트 + xunit 출력 | 1+ |
| `lit integration_test/smoke/ -v` | 통합 smoke 테스트 | 2+ |
| `lit integration_test/traversal/ -v` | 전체 RTL 순회 테스트 (~1,600 파일) | 2 |
| `lit integration_test/ --timeout 300` | 타임아웃 지정 통합 테스트 | 2+ |

### 2.4 외부 도구 (직접 호출)

| 명령어 | 용도 | Phase |
|--------|------|-------|
| `circt-verilog input.v` | Verilog → MLIR | 1 |
| `circt-opt --hw-flatten-modules input.mlir` | CIRCT 계층 인라인(flatten) pass | 1 |
| `verilator --cc input.v` | RTL → C++ 시뮬레이션 모델 | 1 |
| `verilator --lint-only input.sv` | SV lint (기본 게이트) | 0~2 |
| `verible-verilog-lint input.sv` | SV lint (추가) | 0~2 |
| `vcs -sverilog +incdir+$UVM_HOME/src` | VCS 컴파일 (1차 게이트) | 3 |
| `g++ -std=c++17 -c model.cpp` | C++ 컴파일 확인 | 1~2 |

---

## 3. Makefile 타겟 (전체)

### 3.1 루트 Makefile 타겟

```makefile
LIT_TIMEOUT ?= 300
LIT_JOBS    ?= $(shell nproc)
```

| 타겟 | 명령 | 선행 조건 | 출력 | 실패 시 | Phase |
|------|------|----------|------|---------|-------|
| `make setup` | `./utils/setup-env.sh` | 없음 | 도구 설치 + 버전 테이블 | non-zero | 0 |
| `make build` | `cmake -B build -G Ninja && ninja -C build` | CIRCT/LLVM 경로 | `build/` 바이너리. Phase 0에서는 CMakeLists.txt/C++ 소스 미존재 → 안내 메시지 출력 (exit 0) | non-zero | 0 (스켈레톤) → 1+ (실제) |
| `make lint` | clang-format / verible / black | 소스, 설정 파일 | lint 결과 | non-zero | 0+ |
| `make check-hirct` | `lit test/ --xunit-xml-output output/lit-check.xml` | `make build` | `output/lit-check.xml` | non-zero | 1+ |
| `make check-hirct-unit` | gtest 바이너리 실행 | `make build` | 테스트 결과 | non-zero | 1+ |
| `make generate` | filelist 기반 hirct-gen | `make build`, `config/generate.f` | `output/` 산출물 | non-zero | 1+ |
| `make check-hirct-integration` | `lit integration_test/smoke/ -j $(LIT_JOBS) --timeout $(LIT_TIMEOUT) --xunit-xml-output output/lit-integration.xml` | `make build` | `output/lit-integration.xml` | non-zero | 2+ |
| `make test-all` | check-hirct + unit + integration + report | 위 전체 | 종합 결과 | 첫 실패 시 중단 | 2+ |
| `make test-traversal` | `lit integration_test/traversal/ -j $(LIT_JOBS) --timeout $(LIT_TIMEOUT) --xunit-xml-output output/lit-traversal.xml` | `make generate` | `output/lit-traversal.xml` | non-zero | 2 (CI 제외) |
| `make report` | `python3 utils/generate-report.py` | `output/lit-*.xml`, `output/**/meta.json` | `report.json`, `verify-report.json` | non-zero | 2+ |
| `make triage` | `python3 utils/triage-failures.py --output output/triage-report.json` | `output/report.json`, `output/**/meta.json` | `output/triage-report.json` + stdout 요약 | non-zero | 2+ |
| `make docs` | `mkdocs build` | `mkdocs.yml`, `report.json` | `site/` | non-zero | 3 |
| `make clean` | `rm -rf build/ output/ site/` | 없음 | 삭제 | 항상 성공 | - |

**의존 관계 그래프**:

```
make test-all
  ├── make check-hirct            → lit test/ (FileCheck)
  ├── make check-hirct-unit       → gtest 바이너리
  ├── make check-hirct-integration → lit integration_test/smoke/
  └── make report                 → report.json + verify-report.json

make test-traversal
  └── (depends on) make generate  → ~1,600 .v 전체 순회

make generate
  └── (depends on) make build

make docs
  └── (depends on) make report    → report.json 기반 인덱스 생성
```

### 3.2 per-module Makefile 타겟 (GenMakefile.cpp 자동 생성)

`output/<path>/<module>/Makefile`:

| 타겟 | 설명 | 내부 동작 |
|------|------|----------|
| `make test-compile` | cmodel 컴파일 확인 | `g++ -std=c++17 -c cmodel/*.cpp` |
| `make test-verify` | cmodel vs Verilator 등가성 | verilator 빌드 + verify 드라이버 + 10시드 x 1000cyc |
| `make test-artifacts` | 나머지 산출물 컴파일·lint | verilator lint, vcs lint, `python3 -m py_compile` |
| `make test` | **위 3개 전부** | test-compile + test-verify + test-artifacts |

**SEED 변수**: `make test-verify SEED=42` 로 개별 시드 지정 가능

### 3.3 Phase별 Make 타겟 가용성

| Make 타겟 | Phase 0 | Phase 1 | Phase 2 | Phase 3 |
|-----------|---------|---------|---------|---------|
| `make setup` | **생성** | 유지 | 유지 | 유지 |
| `make build` | **생성** | 유지 | 유지 | 유지 |
| `make lint` | **생성** | 유지 | 유지 | 유지 |
| `make clean` | **생성** | 유지 | 유지 | 유지 |
| `make generate` | - | **생성** | 유지 | 유지 |
| `make check-hirct` | - | **생성** | 유지 | 유지 |
| `make check-hirct-unit` | - | **생성** | 유지 | 유지 |
| `make check-hirct-integration` | - | - | **생성** | 유지 |
| `make test-all` | - | - | **생성** | 유지 |
| `make test-traversal` | - | - | **생성** | 유지 |
| `make report` | - | - | **생성** | 유지 |
| `make docs` | - | - | - | **생성** |
| per-module `make test` | - | **자동 생성** | 유지 | 유지 |

**Phase별 추가 요약**:
- **Phase 0**: setup, build, lint, clean
- **Phase 1**: + generate, check-hirct, check-hirct-unit
- **Phase 2**: + check-hirct-integration, test-all, test-traversal, report
- **Phase 3**: + docs

---

## 4. 산출물 매핑 (hirct-gen 8종 + 검증 + meta.json + Makefile)

### 4.1 per-module 산출물

| 산출물 | 디렉터리 | 파일 | 생성 조건 | 테스트 기준 |
|--------|---------|------|----------|------------|
| meta.json | `.` | `meta.json` | **항상** (부분 실패 시에도) | 파일 존재 + 필수 키 검증 |
| C++ Model | `cmodel/` | `.h` + `.cpp` | 항상 | `g++ -c` + verify 10시드x1000cyc |
| SV Testbench | `tb/` | `.sv` | 항상 | `verilator --lint-only` |
| DPI-C Wrapper | `dpi/` | `.h` + `.cpp` + `.sv` | 항상 | `g++ -c` (C++), `vcs` (SV) |
| SV Wrapper | `wrapper/` | `.sv` | 항상 | `verilator --lint-only` |
| Formatted RTL | `rtl/` | `.v` | 항상 | 원본과 동등성 |
| HW Doc | `doc/` | `.md` | 항상 | `[ -s doc/*.md ]` (비어있지 않음) |
| Cocotb TB | `cocotb/` | `.py` | 항상 | `python3 -m py_compile` |
| UVM RAL | `ral/` | `_ral.sv` + `_hal.h` + `_driver.c` | 레지스터 있을 때 | `vcs -sverilog` |
| Verify Driver | `verify/` | `verify_<module>.cpp` | hirct-verify 시 | 10시드x1000cyc PASS |
| Makefile | `.` | `Makefile` | 항상 | `make test` exit 0 |

### 4.2 전역 산출물 (Phase 2~3)

| 산출물 | 경로 | 생성 Phase | 설명 |
|--------|------|-----------|------|
| `lit-check.xml` | `output/lit-check.xml` | 1+ | lit test/ 단위 테스트 결과 (xunit) |
| `lit-integration.xml` | `output/lit-integration.xml` | 2+ | lit integration_test/smoke/ 결과 (xunit) |
| `lit-traversal.xml` | `output/lit-traversal.xml` | 2 | lit integration_test/traversal/ 결과 (xunit) |
| `report.json` | `output/report.json` | 2 | 전체 RTL 순회 결과 (per-file, per-emitter) |
| `verify-report.json` | `output/verify-report.json` | 2 | 전체 모듈 검증 결과 (per-module, per-seed) |
| `triage-report.json` | `output/triage-report.json` | 2 | 자동 실패 분류 리포트 (make triage) |
| `known-limitations.md` | 루트 | 2 | XFAIL SSOT (Markdown 테이블) |
| `site/` | `site/` | 3 | mkdocs 빌드 결과 |

---

## 5. 검증 시드 표준

모든 hirct-verify 검증에서 사용하는 10개 표준 시드:

```
42, 123, 456, 789, 1024, 2048, 4096, 8192, 16384, 32768
```

기본 사이클 수: **1000** (VCS co-sim도 동일)

---

## 6. report.json 스키마

```json
{
  "total_files": 1600,
  "mlir_success": 1500,
  "mlir_fail": 100,
  "xfail_count": 5,
  "per_emitter": {
    "gen-model": {"pass": 1400, "fail": 100},
    "gen-tb": {"pass": 1500, "fail": 0},
    "gen-dpic": {"pass": 1500, "fail": 0},
    "gen-wrapper": {"pass": 1500, "fail": 0},
    "gen-format": {"pass": 1500, "fail": 0},
    "gen-doc": {"pass": 1500, "fail": 0},
    "gen-ral": {"pass": 50, "fail": 0, "skipped": 1450},
    "gen-cocotb": {"pass": 1500, "fail": 0}
  },
  "files": [
    {
      "path": "rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v",
      "mlir": "pass",
      "xfail": false,
      "emitters": {
        "gen-model": "pass",
        "gen-tb": "pass",
        "gen-ral": "skipped"
      }
    }
  ]
}
```

---

## 7. verify-report.json 스키마

```json
{
  "total_modules": 1400,
  "pass": 1390,
  "fail": 5,
  "xfail": 5,
  "modules": [
    {
      "name": "Fadu_K2_S5_LevelGateway",
      "path": "rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v",
      "result": "pass",
      "seeds": [
        {"seed": 42, "result": "pass", "cycles": 1000},
        {"seed": 123, "result": "pass", "cycles": 1000}
      ]
    }
  ]
}
```

---

## 8. meta.json 스키마

**위치**: `output/<path>/<file>/meta.json`

hirct-gen이 **항상** 생성하는 per-module 메타데이터 파일. 부분 실패 시에도 반드시 생성된다.

```json
{
  "path": "rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v",
  "top": "LevelGateway",
  "mlir": "pass",
  "reason": "",
  "emitters": {
    "gen-model": { "result": "pass", "reason": "" },
    "gen-tb": { "result": "pass", "reason": "" },
    "gen-dpic": { "result": "pass", "reason": "" },
    "gen-wrapper": { "result": "pass", "reason": "" },
    "gen-format": { "result": "pass", "reason": "" },
    "gen-doc": { "result": "pass", "reason": "" },
    "gen-ral": { "result": "skipped", "detection": "none", "reason": "no register indicators" },
    "gen-cocotb": { "result": "pass", "reason": "" }
  }
}
```

**필수 키**:

| 키 | 타입 | 설명 |
|----|------|------|
| `path` | string | 입력 RTL 파일 경로 |
| `top` | string | 분석 대상 최상위 모듈 이름 |
| `mlir` | string | MLIR 변환 결과 (`"pass"` \| `"fail"`) |
| `reason` | string | MLIR 실패 사유 (`mlir: "fail"` 시 **필수**, `"pass"` 시 생략 또는 빈 문자열). reason 접두사 표준 적용 (`parse error:`, `timeout:`, `multiple modules:`, `flatten error:`) |
| `emitters.<name>.result` | string | emitter 실행 결과 (`"pass"` \| `"fail"` \| `"skipped"`) |

**선택 키**:

| 키 | 타입 | 설명 |
|----|------|------|
| `emitters.<name>.reason` | string | 실패·스킵 사유 (빈 문자열 허용) |
| `emitters.gen-ral.detection` | string | GenRAL 전용: 레지스터 감지 방법 |

**GenRAL `detection` 값**:

| 값 | 설명 |
|----|------|
| `"annotation"` | SV 어노테이션 기반 감지 |
| `"ir_pattern"` | CIRCT IR 패턴 매칭 |
| `"port_heuristic"` | 포트 이름 휴리스틱 |
| `"none"` | 레지스터 미감지 → skipped |

**확장 선택 키 (Agent Triage 지원, \`hirct-convention.md\` §4.6 참조)**:

| 키 | 타입 | 설명 |
|----|------|------|
| \`unsupported_ops\` | string[] | 미지원 op 목록 |
| \`combinational_loop\` | bool | 조합 루프 감지 여부 |
| \`elapsed_ms\` | int | 총 처리 시간 |
| \`timing\` | object | 단계별 소요 시간 |
| \`tool_versions\` | object | 도구 버전 |
| \`stderr_tail\` | string | 실패 시 stderr tail |

**실패 정책**:
- `meta.json` 누락 = **infra-error** (인프라 오류로 분류)
- 부분 실패 시에도 항상 생성 (실패한 emitter만 `"fail"` 표기)

---

## 9. known-limitations.md 형식 (XFAIL SSOT)

프로젝트 루트의 `known-limitations.md`는 알려진 제한 사항(XFAIL)의 **단일 진실 공급원**이다.

**형식**:

```markdown
| Path | Category | Reason | Date |
|------|----------|--------|------|
| rtl/plat/src/.../foo.v | unsupported_op | hw.array_create | 2026-02-20 |
| rtl/plat/src/.../bar.v | multi_module | --top required | 2026-02-20 |
```

**규칙**:
- **Key** = 파일 경로 (`Path` 열)
- lit 테스트에서 이 파일을 참조하여 XFAIL 판정
- **XPASS** (예상 실패가 통과) = **WARN** (CI는 Green 유지)
- Category 예시: `unsupported_op`, `multi_module`, `parse_error`, `timeout`

---

## 10. 다중 모듈 .v 실행 규칙

하나의 `.v` 파일에 여러 모듈이 정의된 경우의 처리 정책:

| 조건 | 순회 동작 | meta.json |
|------|----------|-----------|
| .v 파일 내 모듈 1개 | 자동 top 지정 → 정상 실행 | `"top": "ModuleName"` |
| .v 파일 내 모듈 2개+ | **infra-error** | `"mlir": "fail"`, `"reason": "multiple modules, --top required"` |

다중 모듈 파일은 `--top` 플래그 없이는 처리할 수 없으며, 순회 모드에서는 자동으로 infra-error로 분류된다. `known-limitations.md`에 `multi_module` 카테고리로 등록할 수 있다.

---

## 11. Quick Reference Card

```bash
# ── Phase 0: 환경 구성 ──
make setup                              # 전체 도구 설치·검증
make build                              # hirct-gen + hirct-verify 빌드
make lint                               # C++/SV/Python 전체 lint

# ── Phase 1: 개발 중 ──
hirct-gen input.v                       # 전체 산출물 + meta.json 생성
hirct-gen input.v --only model          # C++ 모델만 생성
hirct-verify input.v                    # 자동 등가성 검증
make check-hirct                        # lit 단위 테스트 (FileCheck)
make check-hirct-unit                   # gtest C++ API 테스트
cd output/.../Module && make test       # per-module 테스트

# ── Phase 2: 전체 테스트 ──
make generate                           # 전체 산출물 생성
make check-hirct-integration            # lit 통합 smoke 테스트
make test-all                           # 전체 테스트 (메인 진입점)
make test-traversal                     # ~1,600 .v 전체 순회 (CI 제외)
make report                             # report.json + verify-report.json

# ── Phase 3: 배포 ──
make docs                               # 문서 사이트 빌드
make lint                               # 전체 lint
make test-all                           # 최종 회귀 테스트
make clean                              # build/ + output/ + site/ 삭제
```

---

## 12. utils/generate-report.py 입출력 사양

> **역할**: lit xunit XML + per-module meta.json → 통합 JSON 리포트 변환
> **위치**: `utils/generate-report.py`
> **호출**: `make report` (루트 Makefile)

### 입력

| 입력 | 형식 | 경로 | 설명 |
|------|------|------|------|
| lit xunit XML | JUnit XML | `output/lit-check.xml`, `output/lit-integration.xml`, `output/lit-traversal.xml` | lit `--xunit-xml-output` 산출물 |
| per-module meta.json | JSON (§8 스키마) | `output/<path>/<file>/meta.json` | hirct-gen이 각 모듈에 생성 |

### 출력

| 출력 | 형식 | 경로 | 설명 |
|------|------|------|------|
| report.json | JSON (§6 스키마) | `output/report.json` | 전체 RTL 순회 결과 (per-file, per-emitter 통계) |
| verify-report.json | JSON (§7 스키마) | `output/verify-report.json` | 전체 모듈 검증 결과 (per-module, per-seed) |

### 동작

1. `output/**/meta.json` 파일을 재귀 탐색
2. 각 meta.json에서 `path`, `top`, `mlir`, `emitters` 정보 추출
3. per-emitter 통계 집계 (pass/fail/skipped 카운트)
4. lit xunit XML에서 verify 결과 추출 (있는 경우)
5. `report.json` + `verify-report.json` 생성

### CLI

```bash
python3 utils/generate-report.py \
    --meta-dir output/ \
    --lit-xml output/lit-traversal.xml \
    --output-report output/report.json \
    --output-verify output/verify-report.json
```

### 에러 처리

- meta.json 파싱 실패 → 해당 모듈을 `"mlir": "fail"`, reason: `"meta.json parse error"`로 기록
- lit XML 없음 → verify-report.json 생성 스킵 (report.json만 생성)
- output/ 디렉토리 없음 → Error exit 1

---

## 13. lit.cfg.py 템플릿

### test/lit.cfg.py (단위 테스트용)

\`\`\`python
import os
import lit.formats

config.name = "hirct"
config.test_format = lit.formats.ShTest(True)
config.suffixes = ['.mlir', '.test']
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.hirct_obj_root, 'test')

# 도구 경로 substitution
config.substitutions.append(('%hirct-gen', config.hirct_gen_path))
config.substitutions.append(('%hirct-verify', config.hirct_verify_path))
config.substitutions.append(('%FileCheck', config.filecheck_path))
\`\`\`

### integration_test/lit.cfg.py (통합 테스트용, XFAIL 연동 포함)

\`\`\`python
import os
import sys
import lit.formats

# XFAIL 연동: known-limitations.md 파싱
sys.path.insert(0, os.path.join(config.hirct_src_root, 'utils'))
from parse_known_limitations import load_xfail_paths

config.name = "hirct-integration"
config.test_format = lit.formats.ShTest(True)
config.suffixes = ['.v', '.f', '.test']
config.test_source_root = os.path.dirname(__file__)
config.xfail_paths = load_xfail_paths(
    os.path.join(config.hirct_src_root, 'known-limitations.md'))
\`\`\`

> **Note**: \`config.hirct_gen_path\` 등은 CMake가 \`lit.site.cfg.py\`를 생성할 때 주입한다.
> Phase 0에서 스켈레톤만 생성하고, Phase 1 Bootstrap(Task 100)에서 CMake 연동을 완성한다.

---

## 부록 A. v2 신규 생성 경로

> 이 저장소에는 C++ 소스 코드가 존재하지 않는다.
> 이전 프로토타입(EmitCppModel.cpp 등)의 로직을 참고하되, 모든 파일을 v2 구조로 **처음부터 신규 작성**한다.
> Task 100(Bootstrap)에서 기본 스켈레톤을 생성하고, 각 태스크에서 기능을 완성한다.

| v2 경로 | 역할 | 생성 태스크 |
|---------|------|-----------|
| `CMakeLists.txt` (루트) | CMake 빌드 시스템 | Task 100 (Bootstrap) |
| `lib/Analysis/ModuleAnalyzer.cpp` | IR 분석 (공통 기반) | Task 100 (Bootstrap) |
| `lib/Target/GenModel.cpp` | C++ cycle-accurate 모델 | Task 100 (스켈레톤) + Task 101 (완성) |
| `lib/Target/GenTB.cpp` | SV 테스트벤치 | Task 102 |
| `lib/Target/GenDPIC.cpp` | DPI-C VCS 래퍼 | Task 103 |
| `lib/Target/GenWrapper.cpp` | SV Interface 래퍼 | Task 104 |
| `lib/Target/GenFormat.cpp` | IR 기반 RTL 포매터 | Task 105 |
| `lib/Target/GenDoc.cpp` | HW Doc + Programmer's Guide | Task 106 |
| `lib/Target/GenRAL.cpp` | UVM RAL + HAL + C 드라이버 | Task 107 |
| `lib/Target/GenCocotb.cpp` | Python testbench | Task 108 |
| `lib/Target/GenVerify.cpp` | verify 드라이버 생성 | Task 109 |
| `lib/Target/GenMakefile.cpp` | per-module Makefile 생성 | Task 100 (스켈레톤) + Task 110 (완성) |
| `tools/hirct-gen/main.cpp` | hirct-gen CLI 진입점 | Task 100 (스켈레톤) + Task 111 (완성) |
| `tools/hirct-verify/main.cpp` | hirct-verify CLI 진입점 | Task 100 (스켈레톤) + Task 109 (완성) |
| `utils/setup-env.sh` | 환경 부트스트랩 | Phase 0 Task 001 |
| `utils/generate-report.py` | lit XML + meta.json → JSON 리포트 | Phase 2 Task 205 |
| `utils/triage-failures.py` | 자동 실패 분류 + triage 리포트 | Phase 2 Task 206 |
| `utils/parse_known_limitations.py` | known-limitations.md 파서 (lit XFAIL 연동) | Phase 2 Task 205 |
