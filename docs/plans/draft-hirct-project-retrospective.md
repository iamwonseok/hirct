# HIRCT 프로젝트 전체 회고: 처음부터 다시 만든다면

> **작성일**: 2026-03-10
> **범위**: Phase 0 ~ Phase 4-F + CXXRTL PoC (2026-02-17 ~ 2026-03-10)
> **목적**: 프로젝트를 처음부터 다시 만든다고 가정하고, 목표/목적/기대 결과물/시도/실패/극복/현재 이슈를 상세히 기술한다. 사내 발표 자료, 신규 팀원 온보딩, 후속 프로젝트 설계의 참조 문서로 사용한다.

---

## 1. 프로젝트 정의

### 1.1 이름

**HIRCT** — HDL IR Compiler & Tools

### 1.2 한 줄 정의

SystemVerilog/Verilog RTL → CIRCT MLIR IR → 9종 자동화 산출물 생성 파이프라인

### 1.3 핵심 비전

칩 전체 RTL을 C/C++ 모델로 **자동 변환**하여, 사내 제품 시뮬레이터에 연결하고, FW 선행 개발을 가능하게 한다.

---

## 2. 왜 이 프로젝트가 필요한가

### 2.1 사내 문제

사내에서 Verilog 모듈(`v2p_tbl_stage1`, `v2p_tbl_stage2` 등)을 **수동으로 C 모델로 변환**하고 있다. 수동 변환은:

- 시간 소모가 크다 (모듈당 수일~수주)
- 오류 가능성이 높다 (비트 연산, 타이밍 불일치)
- RTL 변경 시마다 재작업이 필요하다
- 유지보수 부담이 누적된다

### 2.2 업계 Pain Point

CHIPS Alliance의 Caliptra `sw-emulator`가 대표적 사례다. RTL 동작을 Rust로 수동 구현한 주변장치 모델로, "RTL → C 모델" 수동 변환은 업계 전반의 pain point이다.

### 2.3 기존 도구로는 왜 안 되는가

| 도구 | 유형 | 왜 안 되는가 |
|------|------|-------------|
| **Verilator** | RTL → C++ 시뮬레이터 | 모듈 계층 flatten, 이름 해시화, libverilated 런타임 필수. 사내 시뮬레이터에 모듈 단위로 연결 불가 |
| **ARM CycleModels** | RTL → SystemC | ARM 라이선스 필요, 비공개 |
| **Yosys CXXRTL** | RTL → C++ | 모듈 계층 보존되지만, cxxrtl 런타임 헤더 의존, 커스텀 최적화 불가 |
| **CIRCT arcilator** | CIRCT IR → C++ (JIT) | cf.br/cf.cond_br 잔존 시 실패. FSM 패턴에서 범용 NO-GO |
| **Synopsys/Cadence/Siemens** | 스펙 → 모델 | RTL이 아닌 별도 스펙(IP-XACT, SystemRDL)이 입력. 스펙 없으면 사용 불가 |

### 2.4 HIRCT가 채우는 공백

**"RTL에서 사내 시뮬레이터용 경량 C/C++ 모델 자동 생성"을 하는 도구는 상용/오픈소스 모두 없다.**

HIRCT의 차별점:

| 항목 | Verilator | HIRCT GenModel |
|------|-----------|----------------|
| 런타임 의존 | libverilated 필수 | 없음 (`cstdint`만) |
| 모듈 계층 | flatten | 보존 |
| 이름 보존 | 변형/해시화 | 원본 그대로 |
| 서브모듈 단위 교체 | 어려움 | 자연스러움 |
| 증분 빌드 | 전체 재빌드 | 변경 모듈만 |
| 코드 가독성 | 불가 | 가능 |
| 사내 API 적응 | 래퍼 수동 작성 | 코드 생성 템플릿 수정 |

---

## 3. 목표한 결과물

### 3.1 9종 산출물

| # | Emitter | 산출물 | 소비자 | 목적 |
|---|---------|--------|--------|------|
| 1 | **GenModel** | `cmodel/*.h` + `*.cpp` | FW팀, 검증팀 | RTL과 비트/사이클 정확한 standalone C++17 모델 |
| 2 | **GenVerify** | `verify/verify_*.cpp` | CI, 검증팀 | GenModel vs Verilator RTL 매 사이클 자동 비교 |
| 3 | **GenFuncModel** | `func_model/*.h` + `*.c` | 사내 C sim 팀 | 사람이 읽을 수 있는 pure C 기능 모델 |
| 4 | **GenTB** | `tb/*.sv` | 검증팀 | SV 테스트벤치 (interface + 클럭/리셋) |
| 5 | **GenDPIC** | `dpi/*.h` + `*.cpp` + `*.sv` | 검증팀 (VCS/Questa) | DPI-C 래퍼 (CModel을 RTL sim에서 호출) |
| 6 | **GenRAL** | `ral/*_ral.sv` + `*_hal.h` + `*_drv.c` | FW팀, 검증팀 | UVM RAL + C HAL + 드라이버 뼈대 |
| 7 | **GenDoc** | `doc/*.md` | 전체 팀 | 포트맵, 계층, 레지스터 요약 + Programmer's Guide |
| 8 | **GenCocotb** | `cocotb/test_*.py` | 검증팀 | cocotb Python 테스트 스캐폴드 |
| 9 | **GenFormat** | `rtl/*.v` | 전체 팀 | IR 기반 RTL 리포매팅 |

### 3.2 도구

| 도구 | 용도 |
|------|------|
| `hirct-gen input.v` | 9종 산출물 생성 |
| `hirct-verify input.v` | 자동 등가성 검증 (10seed x 1000cyc) |
| `hirct-gen -f filelist.f --top Top` | 다중 파일 + Top 지정 |
| `hirct-gen input.v --only model,tb` | 선택적 산출물 생성 |

### 3.3 기술 스택

| 구성 요소 | 역할 |
|----------|------|
| CIRCT (라이브러리 링크) | Verilog → MLIR IR 변환, hw/comb/seq dialect |
| LLVM/MLIR C++ API | IR 순회, 분석, 커스텀 pass |
| Verilator 5.020 | RTL reference 시뮬레이터 (GenVerify) |
| VCS V-2023.12-SP2-7 | DPI-C co-simulation, cross-validation |
| C++17 | hirct-gen/hirct-verify 구현 |
| CMake + Ninja | 빌드 시스템 |
| lit (LLVM) | 테스트 오케스트레이션 |

---

## 4. 실행 타임라인: 무엇을 시도했고, 무엇이 실패했고, 어떻게 극복했는가

### Phase 0: 환경 구성 (2026-02-17 ~ 02-18)

**목표**: 도구 설치, 빌드 인프라, 코딩 컨벤션 확립

**결과**: 29/29 게이트 ALL PASS

**시도한 것**:
- CIRCT/LLVM 빌드 환경 구축
- Verilator, VCS, ncsim 경로 검증
- `utils/setup-env.sh` 멱등성 보장
- clang-format, shellcheck 등 lint 도구 설정

**실패/이슈**: 없음. 가장 순조로운 Phase.

---

### Phase 1A: Core Pipeline (2026-02-18 ~ 02-19)

**목표**: hirct-gen/hirct-verify가 동작하는 Walking Skeleton 구축

**결과**: lit 28/28 PASS, gtest 2/2 PASS

**아키텍처 (Phase 1 당시)**:
```
.v → fork(circt-verilog) → MLIR 텍스트 stdout
   → ModuleAnalyzer (정규식 82+개) → 자체 구조체
   → GenModel/GenTB/... → 산출물 파일
```

**시도한 것**:
- `CirctRunner`: circt-verilog를 외부 프로세스로 호출하는 래퍼
- `ModuleAnalyzer`: MLIR 텍스트를 정규식으로 파싱 (1,812줄)
- 8종 emitter 순차 구현 (gen-model → gen-tb → gen-dpic → ...)
- LevelGateway, RVCExpander 2개 모듈로 end-to-end 검증

**실패한 것**:
1. **BUG-1: eval_comb() 타이밍 오류** — register-to-output 경로에서 eval_comb 1회만 호출 → 이전 사이클 값 출력. `step()` 내 eval_comb() 2회 호출(pre/post register update)로 수정.
2. **BUG-2: SSA regex 파싱 실패** — RVCExpander에서 SSA 이름 파싱 실패 → 정규식 패턴 확장.
3. **BUG-3: extract_bit_width 로직 오류** — 비트폭 계산 오류 → 로직 수정.

**극복 방법**: 3건 모두 개별 fix 커밋으로 해결. `feature/hirct-phase1-bugfix-gate` 브랜치로 일괄 병합.

**교훈**: **정규식 파싱은 새로운 IR 패턴마다 깨진다.** 이것이 Phase 4 전면 전환의 씨앗이 되었다.

---

### Phase 1B: Remaining Emitters (2026-02-19)

**목표**: 나머지 5개 emitter (gen-dpic, gen-wrapper, gen-format, gen-ral, gen-cocotb) 완성

**결과**: 8종 emitter 전부 동작, 다양성 게이트 4/5

**시도한 것**:
- 103(dpic) → 104(wrapper) → 105(format) → 107(ral) → 108(cocotb) 순차 구현
- 각 emitter 완료 시 spec reviewer + code quality reviewer 자동 리뷰

**실패한 것**:
- MultiModule 테스트 XFAIL (hw.instance 계층 출력 구조 미완성)
- hw.param 파라미터 축 미검증 (Phase 2 이관)

**극복 방법**: XFAIL로 등록, Phase 2에서 대규모 순회 시 확인하기로 연기.

---

### Phase 2: 전체 순회 테스트 (2026-02-21 ~ 02-23)

**목표**: ~1,600개 .v 파일 전체 순회 + 자동 검증 + 실패 분류

**결과**:

| 지표 | 값 |
|------|-----|
| 전체 파일 | 1,604 |
| MLIR 변환 성공 | 1,231 (76.7%) |
| MLIR 실패 | 373 (unknown module 98.3%) |
| gen-model 성공 | 1,121 |
| verify 시도 | 1,121 모듈 |
| verify PASS | 133 |
| verify FAIL | 224 |
| verify SKIP | 764 |

**시도한 것**:
- Batch A (순회) → B (검증) → C (분류) → D (파싱 개선) → E (자동화)
- Rule-based triage 도구로 324건 자동 분류
- `-y` 라이브러리 경로 보강으로 unknown module 해소 시도

**실패한 것**:
1. **MLIR 파싱 성공률 36.7%** — 단일 파일 모드에서 의존 모듈 미해결 (unknown module 98.3%)
2. **verify PASS 12.1%** — 다수 모듈이 SKIP (MODMISSING 633건) 또는 컴파일 에러
3. **task-batch-c 병합 후 async-reset SSA 깨짐** — 병합 시 SSA 매핑이 덮어쓰여짐

**극복 방법**:
- MLIR 파싱: Phase 4에서 `importVerilog()` in-process + SourceMgr 다중 파일 로드로 전면 해결
- verify: known-limitations.md + XFAIL 관리로 CI Green 유지
- SSA 깨짐: `fix: preserve async-reset SSA handling after task-batch-c merge`

**교훈**: **외부 프로세스 + 텍스트 파싱 아키텍처의 근본 한계가 확인되었다.** 이것이 Phase 4 전환 결정의 직접적 원인이 되었다.

---

### Phase 3: 통합 및 배포 (2026-02-23 ~ 02-24)

**목표**: VCS DPI-C co-simulation, mkdocs 문서, 프로덕션 패키징

**결과**:

| 항목 | 결과 |
|------|------|
| VCS co-sim | LevelGateway 10/10 PASS, Queue_11 FAIL (XFAIL) |
| mkdocs | `make docs` exit 0, `site/index.html` 생성 |
| 패키징 | README.md, Dockerfile, SECURITY.md, CHANGELOG.md |
| Quick Start | 3단계 검증 완료 |

**시도한 것**:
- Task 301: VCS DPI-C co-simulation (LevelGateway, Queue_11)
- Task 302: mkdocs 문서 사이트
- Task 303: 프로덕션 패키징 (README, Dockerfile)
- Task 304: VCS/ncsim multi-level cross-validation

**실패한 것**:
1. **Queue_11 FAIL** — GenModel 음수 상수 비트폭 미정규화 + FIFO 메모리 인덱싱 버그. `io_deq_bits` mismatch.
2. **CLINT FAIL** — CModel TileLink 타이머 카운터 로직 버그.
3. **SoC gate SKIP** — RocketTile/CoreIPSubsystem CModel 미생성 (hw.instance 깊이 제한).
4. **IUS 15.1 호환성** — `-static-libstdc++` + `LD_PRELOAD` workaround 필요.

**극복 방법**:
- Queue_11: 음수 상수 정규화 수정 (`normalize negative constants to unsigned bit-width`)
- CLINT/SoC: XFAIL 처리. IP-top/SoC 게이트는 backlog으로 분리.
- IUS: workaround 적용 후 ncsim 10/10 PASS 확인.

**교훈**: **3-모델 교차 리뷰(GPT/OPUS/Sonnet)를 수행하여 blind spot 발견.** ncsim co-sim은 scope creep으로 판정, backlog 분리가 유효했다.

---

### Phase 4: CIRCT 내장 아키텍처 전환 (2026-02-28 ~ 03-10)

**목표**: 외부 프로세스(CirctRunner) + 정규식 파싱(ModuleAnalyzer 82+개) → CIRCT 라이브러리 직접 링크 + MLIR API 순회로 **전면 교체**

**전환 동기** (Phase 1~3에서 드러난 한계):
1. 계층적 클럭 전파 경로 분석 불가 → old_ssa 타이밍 문제 며칠째 미해결
2. MLIR 파싱 성공률 36.7% (unknown module 98.3%)
3. 정규식 82개+ 유지보수 부담

**새 아키텍처**:
```
.v → importVerilog() (in-process) → mlir::ModuleOp (인메모리 IR)
   → HIRCT lowering pipeline (in-process)
   → Emitter가 mlir::Operation을 직접 순회
```

**단계별 진행**:

| 단계 | 내용 | 결과 |
|------|------|------|
| **A: 빌드 인프라** | CMake + CIRCT/MLIR/LLVM 링크 | cmake + ninja exit 0 |
| **B: IR 분석 계층** | VerilogLoader + IRAnalysis (PortView, RegisterView, ClockDomainMap) | gtest PASS |
| **C: GenModel 전면 재작성** | hw::HWModuleOp 직접 순회, 1,900줄, MLIR 단일 경로 | LevelGateway eval_comb + step 검증 PASS |
| **D: 나머지 Emitter 전환** | 전 emitter IRAnalysis 단일 경로, analyzer_ 분기 0건 | lit 전체 PASS |
| **E: CLI 전환** | VerilogLoader + SymbolTable | 완료 |
| **F: 검증 + 레거시 삭제** | CirctRunner(281줄) + ModuleAnalyzer(1,808줄) 삭제, ~2,647줄 제거 | lit 57/57, gtest 2/2, integration FAIL 0 |

**커스텀 MLIR Pass 4종** (Phase 4-B 병행):

| Pass | 줄 수 | 효과 |
|------|-------|------|
| HirctSimCleanup | — | sim display process DCE |
| HirctUnrollProcessLoops | 401줄 | 180/266 llhd.process 해소 |
| HirctProcessFlatten V2 | — | 다이아몬드 패턴 O(n), wait-dest-args 지원 |
| HirctSignalLowering | — | llhd.sig/prb/drv → hw/comb/seq |

**시도한 것**:
- Arc PoC (arcilator 경로 평가) → CONDITIONAL GO
- verilator -E 전처리 (`--preprocess` CLI)
- `--run-pass <name>` CLI (pass별 lit 테스트)
- `--timing` 프로파일링 CLI

**실패한 것**:
1. **arcilator 범용 경로 NO-GO** — cf.br/cf.cond_br 잔존이 blocker. FSM 패턴 모듈에서 전부 실패.
2. **ProcessDeseq V1 실패** — intermediate reset block 처리 미흡 → V2에서 clone + merge_args fix.
3. **signal-lowering segfault** — 방어 코드 추가로 해결.
4. **verilator-preprocess-pipeline 병합 후 롤백** — soft reset 후 재병합.
5. **Makefile/환경 설정 다수 오류** — PROJECT 미export, HIRCT_GEN 경로 오타, 0바이트 입력 허위 pass.

**극복 방법**:
- arcilator: 커스텀 pass 4종으로 llhd.process를 hw/comb/seq로 낮추는 자체 경로 구축
- 환경 오류: 사후 수정 (Gate 3: Build/Config Smoke 필요성의 근거)
- 병합 회귀: 사후 수정 (Gate 4: Merge Impact Analysis 필요성의 근거)

**Phase 2 재실행 결과 (커스텀 pass 적용 후)**:

| 지표 | Baseline (v1) | V2 | 변화 |
|------|:---:|:---:|------|
| pass / pass_with_warnings | 3 / 0 | 0 / 6 | uart, gpio, wdt, ptimer 추가 통과 |
| GenModel-specific failures | 4 | 0 | 전부 해소 |
| 총 실행 시간 | 357.8s | 27.9s | **-92.2%** |

**교훈**: **CIRCT 라이브러리 직접 링크가 프로젝트의 game changer였다.** 파싱 성공률, 실행 속도, 코드 유지보수 모두 극적으로 개선. 단, LLHD lowering의 불완전성은 커스텀 pass로 자체 해소해야 했다.

---

### CXXRTL PoC (2026-03-08 ~ 03-10)

**목표**: Yosys CXXRTL이 GenModel을 대체/보완할 수 있는지 확인

**결과**:

| 대상 | CXXRTL 변환 | GenModel 비교 | 판정 |
|------|-----------|-------------|------|
| UART | PASS | PRDATA/INTR 5/5 시드 0 mismatch | **Conditional Go** |
| ncs_cmd_v2p_blk_swap | PASS (동적 for-loop 전처리 필요) | **BLOCKED** (CIRCT llhd.drv 타입 불일치) | Conditional Go (CXXRTL 단독) |

**시도한 것**:
- UART preprocessed.v (21,041줄, 22모듈) → Yosys CXXRTL → C++ 모델
- GenModel과 동일 테스트 프레임워크로 비교
- NC (Incisive/Xcelium) 빌드 환경 구축
- ncs_cmd_v2p_blk_swap CXXRTL standalone 검증

**실패한 것**:
1. **hw.array_inject 미지원** — bcm57 FIFO RAM stub 한계 → PRDATA 85% 일치, INTR 251 mismatch
2. **flatten_block 버그** — CondBranchOp true/false가 모두 wait_block인 경우 미처리
3. **CompRegOp forward reference** — val map에 없는 SSA → 빈 문자열 반환
4. **ncs_cmd_v2p_blk_swap GenModel 생성 실패** — CIRCT llhd.drv 타입 불일치

**극복 방법**:
- hw.array_inject: 지원 추가 → PRDATA/INTR 100% 일치로 개선
- flatten_block: 해당 케이스 처리 추가
- CompRegOp: deferred_regs 2단계 emit
- v2p: CXXRTL standalone으로 우회, GenModel 비교는 Phase 1 후속 과제

**교훈**: **CXXRTL은 단기 활용 도구로 유효하다.** 모듈 계층 보존, 이름 보존, standalone C++ 등 GenModel과 유사한 접근. 다만 cxxrtl 런타임 헤더 의존성과 커스텀 최적화 불가가 장기 한계.

---

## 5. 현재 이슈 (2026-03-10 기준)

### 5.1 Known Limitations (해소 필요)

| KL | 심각도 | 이슈 | 영향 |
|----|--------|------|------|
| **KL-3** | Medium | 64비트 초과 신호 truncate (`uint64_t`) | smbus, axi_x2p 정확성 문제 |
| **KL-5** | Medium | llhd.prb/drv 잔존 모듈 스킵 | uart_top 전체 sub-module GenModel 미생성 |
| **KL-10** | Medium | FSM ceq 기반 process 미처리 | FSM sub-module GenModel 스킵 |
| **KL-13** | High | FSMAnalysis ICmpPredicate::ceq 미지원 | uart 전 sub-module func_model 미생성 |
| **KL-14** | High | llhd.drv 동적 배열 인덱스 타입 불일치 | ncs_cmd_v2p_blk_swap hirct-gen exit 1 |
| **KL-15** | High | `--pp-comments` define 주석 파서 에러 | packet_router, top_v2p importVerilog 실패 |
| **KL-16** | Medium | hw.array_inject → g++ 컴파일 실패 | 동적 배열 쓰기 모듈 컴파일 불가 |
| **KL-18** | Medium | `--llhd-mem2reg` segfault (CIRCT upstream) | Stage 2 pass 제외로 우회 중 |

### 5.2 Open Decisions (미결정)

| ID | 주제 | 영향 |
|----|------|------|
| **H-1** | 사내 시뮬레이터 인터페이스 (pure C vs SystemC vs 커스텀) | GenModel/FuncModel 래퍼 설계에 영향 |
| **H-2** | CXXRTL PoC 범위 및 일정 | 단기 자동화 전략 |
| **H-3** | GenFuncModel SystemC TLM 전환 여부 | SystemC 의존성 도입 여부 |
| **H-4** | GenModel IR 스펙 작업 재개 시점 | IR precondition 공식화 일정 |
| **G-1** | DPI Wrapper 출력 갱신 타이밍 | false mismatch 허용 범위 |

### 5.3 실측 수치 (최종)

| 지표 | 값 |
|------|-----|
| lit 테스트 | 57/57 PASS |
| gtest | 2/2 PASS |
| integration FAIL | 0 |
| 코드 삭제량 (Phase 4-F) | ~2,647줄 (CirctRunner + ModuleAnalyzer) |
| fc6161 GenModel survey V3 | GenModel-specific failures 0 |
| fc6161 실행 시간 | 27.9s (baseline 357.8s 대비 92.2% 감소) |
| CXXRTL UART 비교 | PRDATA/INTR 5/5 시드 0 mismatch |

---

## 6. 처음부터 다시 만든다면: 핵심 교훈

### 6.1 아키텍처 결정

| # | 교훈 | 근거 |
|---|------|------|
| 1 | **CIRCT 라이브러리를 Day 1부터 직접 링크하라** | Phase 1~3의 외부 프로세스 + 정규식 82개는 Phase 4에서 전면 폐기. 처음부터 MLIR API를 사용했으면 ~3주 절약 |
| 2 | **LLHD lowering의 불완전성을 전제하라** | CIRCT `populateLlhdToCorePipeline`이 모든 패턴을 변환하지 못한다. 커스텀 pass가 반드시 필요. 계획 단계에서 pass 커버리지를 실측해야 |
| 3 | **Verilator는 대체가 아닌 보완 도구로 활용하라** | GenVerify의 reference로 활용. verilator -E 전처리도 유용 |
| 4 | **CXXRTL을 단기 브릿지로 활용하라** | 모듈 계층 보존 + standalone C++ + 이름 보존. 사내 시뮬레이터 PoC를 즉시 확보 가능 |

### 6.2 프로세스

| # | 교훈 | 근거 |
|---|------|------|
| 5 | **IR Op 분포를 구현 전에 실측(census)하라** | 26건 blocker 중 13건(50%)이 미처리 Op 조합에서 발생. 대표 RTL 3~5개의 Op census로 예방 가능 |
| 6 | **CIRCT upstream 기능은 사전 smoke 필수** | 4건의 upstream 한계가 "사용 시점"에서야 발견. 계획 단계에서 대표 RTL 1건으로 실행 확인 |
| 7 | **병합 전 diff 기반 영향 분석을 수행하라** | 3건의 병합 후 회귀. GenModel 내부 상태(val map, SSA naming) 변경 시 대표 모듈 re-verify |
| 8 | **plan-readiness-check를 mandatory gate로 강제하라** | 3건의 계획-구현 gap. 계획이 stale한 상태에서 구현 시작 → 세션 시간 소진 |

### 6.3 Phase 설계 (다시 만든다면)

```
Phase 0: 환경 구성 (2일)
  └─ CIRCT 라이브러리 빌드 + CMake 링크부터 시작 (외부 프로세스 경로 없음)

Phase 1: IR 분석 + Core Pipeline (10일)
  ├─ VerilogLoader (importVerilog in-process) + IRAnalysis API
  ├─ 커스텀 LLHD lowering pass 먼저 구현
  ├─ GenModel (MLIR API 직접 순회) + GenVerify
  └─ 대표 RTL 3~5개로 Op census + upstream smoke

Phase 2: 나머지 Emitter + 전체 순회 (10일)
  ├─ GenTB, GenDPIC, GenRAL, GenDoc, GenCocotb, GenFormat, GenFuncModel
  ├─ ~1,600 .v 전체 순회 + triage
  └─ XFAIL 관리 + known-limitations.md

Phase 3: 통합 및 배포 (5일)
  ├─ VCS/ncsim cross-validation
  ├─ mkdocs + 패키징
  └─ CXXRTL PoC (단기 브릿지)

Phase 4: 고도화 (ongoing)
  ├─ KL 해소 (WideInt, FSM ceq, array_inject 등)
  ├─ 4축 산출물 전략 실행
  └─ 사내 시뮬레이터 연동
```

**예상 절약**: Phase 1~3에서 정규식 파싱 아키텍처 구축 + Phase 4에서 전면 폐기까지 소요된 ~3주를 절약. 전체 일정 약 30% 단축 가능.

---

## 7. 4축 전략 (향후 방향)

| 축 | 핵심 산출물 | 차별 포인트 |
|----|-----------|-----------|
| **축 1: 레지스터 맵 + SMOKE + Guide** | GenRAL, GenDoc | RTL에서 레지스터 주소/필드/접근속성 자동 역추출. 기존 도구는 전부 별도 스펙 입력 |
| **축 2: 테스트 인터페이스** | GenTB, GenCocotb, GenDPIC | 포트/프로토콜 분석으로 cocotb/UVM/DPI-C 자동 생성 |
| **축 3: FuncModel** | GenFuncModel | pure C 본체 + 환경별 래퍼 (SystemC/DPI-C/QEMU/standalone) |
| **축 4: Cycle-Accurate 모델** | GenModel, GenVerify | 사내 시뮬레이터용 투명한 모듈 단위 C++ 모델 |

---

## 부록 A: 전체 커밋 패턴 요약

| 패턴 | 건수 | 주요 사례 |
|------|------|----------|
| GenModel 타이밍/SSA/비트폭 수정 | 10+ | eval_comb 순서, val-map miss, negative constant, width parsing |
| LLHD Process 파이프라인 수정 | 5+ | ProcessFlatten, ProcessDeseq V2, UnrollProcessLoops |
| 인프라/빌드 수정 | 5+ | CirctRunner safety, CirctRunner 삭제, CMake CIRCT 링크 |
| 클럭 도메인 수정 | 3+ | is_clock_port 휴리스틱 → IR 기반 전환, multi-clock step |
| 병합 후 회귀 수정 | 2 | task-batch-c SSA, verilator-preprocess 롤백 |

## 부록 B: Phase 완료 게이트 요약

| Phase | 완료일 | 핵심 게이트 | 상태 |
|-------|--------|-------------|------|
| 0 | 2026-02-18 | 29/29 게이트, make setup exit 0 | 완료 |
| 1A | 2026-02-19 | lit 28/28, gtest 2/2, verify 10seed x 1000cyc | 완료 |
| 1B | 2026-02-19 | 8종 emitter, 다양성 4/5 | 완료 |
| 2 | 2026-02-23 | make test-all exit 0, 133P/224F/764S | 완료 |
| 3 | 2026-02-23 | VCS LG 10/10, mkdocs, Quick Start, v0.1.0 | 완료 |
| 4-A | 2026-03-04 | cmake + ninja exit 0 | 완료 |
| 4-B | 2026-03-04 | gtest PASS, IRAnalysis API | 완료 |
| 4-C | 2026-03-05 | GenModel 1,900줄 MLIR 단일 경로 | 완료 |
| 4-D | 2026-03-05 | 전 emitter MLIR 단일 경로 | 완료 |
| 4-F | 2026-03-05 | lit 57/57, gtest 2/2, CirctRunner/ModuleAnalyzer 삭제 | 완료 |

## 부록 C: 참조 문서

| 문서 | 위치 |
|------|------|
| 제안서 | `docs/proposal/001-hirct-automation-framework.md` |
| 실행 계획 총괄 | `docs/plans/summary.md` |
| 기술 규약 | `docs/plans/hirct-convention.md` |
| 미합의 사항 | `docs/plans/open-decisions.md` |
| Known Limitations | `docs/plans/known-limitations.md` |
| CIRCT 내장 설계 | `docs/plans/2026-02-28-circt-embedding-design.md` |
| 제품 전략 | `docs/plans/2026-03-08-hirct-product-strategy.md` |
| Blocker RCA | `docs/plans/2026-03-10-blocker-root-cause-analysis.md` |
| 산출물 스펙 | `docs/plans/2026-03-06-hirct-output-spec-survey.md` |
