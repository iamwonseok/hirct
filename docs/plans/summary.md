# HIRCT 실행 계획 총괄

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** hirct-gen / hirct-verify가 모든 RTL 입력에서 올바른 산출물을 생성하고, 전체 순회 테스트를 통과하여 프로덕션 품질로 배포한다.

**Architecture:** 기능별 구현(Phase 1) → 전체 순회 테스트(Phase 2) → 배포(Phase 3) → **CIRCT 내장 아키텍처 전환(Phase 4)**. Phase 4에서 외부 프로세스 호출 + 정규식 파싱을 CIRCT 라이브러리 직접 링크 + MLIR API 순회로 전면 교체한다.

**Tech Stack:** CIRCT (라이브러리 링크), LLVM/MLIR C++ API, Verilator 5.020, hirct-gen/hirct-verify (C++17), VCS V-2023.12-SP2-7

---

## 1. CLI

```bash
hirct-gen input.v                    # 전체 산출물 생성 (기본)
hirct-gen -f filelist.f              # 다중 파일 / Top 생성
hirct-verify input.v                 # 자동 검증 (10시드 x 1000cyc)
hirct-gen input.v --only model,tb    # 선택적 산출물 생성 (Phase 1 필수)
hirct-gen -f filelist.f --top Top    # 다중 파일 + Top 지정
```

---

## 2. hirct-gen 산출물 (9종)

| hirct-gen | 디렉토리 | 산출물 | 생성 조건 |
|-----------|---------|--------|----------|
| gen-model | `cmodel/` | `.h` + `.cpp` | 항상 |
| gen-tb | `tb/` | `.sv` | 항상 |
| gen-dpic | `dpi/` | `.h` + `.cpp` + `.sv` | 항상 |
| gen-wrapper | `wrapper/` | `.sv` | 항상 |
| gen-format | `rtl/` | `.v` | 항상 |
| gen-doc | `doc/` | `.md` | 항상 |
| gen-ral | `ral/` | `_ral.sv` + `_hal.h` + `_driver.c` | 레지스터 있을 때 |
| gen-cocotb | `cocotb/` | `.py` | 항상 |
| gen-func-model | `func_model/` | `.h` + `.cpp` | FSM 있을 때 |

---

## 3. 출력 구조

입력 경로 독립: 단일 파일 → `output/<filename>/`, filelist+top → `output/<top_name>/`
(`rtl/` 디렉토리는 개발/테스트용 예제. 실제 RTL은 임의 경로에 위치. 상세: `hirct-convention.md` §0)

```
output/<path>/<filename>/
├── Makefile          (자동 생성, 테스트 타겟 포함)
├── <filename>.mlir
├── cmodel/           항상
├── tb/               항상
├── dpi/              항상
├── wrapper/          항상
├── rtl/              항상
├── doc/              항상
├── cocotb/           항상
├── ral/              레지스터 있을 때만
├── func_model/       FSM 있을 때만
└── verify/           hirct-verify 실행 시 생성
    ├── verify_<filename>.cpp   (비교 드라이버, 자동 생성)
    └── obj_dir/                (Verilator 빌드 산출물)
```

각 모듈 디렉토리에 `meta.json`이 항상 생성됨 (hirct-gen이 per-emitter 결과 기록)

**자동 생성 Makefile 테스트 타겟 (per-module):**

| 타겟 | 설명 |
|------|------|
| `make test-compile` | cmodel 컴파일 확인 (g++ -c) |
| `make test-verify` | cmodel vs Verilator 등가성 검증 (10시드 x 1000cyc) |
| `make test-artifacts` | 나머지 산출물 컴파일/lint 확인 |
| `make test` | 위 3개 전부 |

- Top: `output/<path>/top/`
- 루트 `make test-all` → `check-hirct`(lit) + `check-hirct-unit`(gtest) + `check-hirct-integration`(lit smoke) + `report` (CI 메인 진입점)
- `make test-traversal` → ~1,600 .v 전체 순회 (Phase 2 전용, CI 제외)

---

## 4. Phase 로드맵

> **Note**: 예상 기간은 모든 C++ 코드를 처음부터 신규 작성하는 greenfield 기준이다.
> Phase 1을 1A(core pipeline) / 1B(remaining emitters)로 분리하여 핵심 관통을 먼저 안정화한다.

```
Phase 0: 환경 구성 (2일) ──────────────────────────────
  ├─ 001 도구 설치 + 환경 검증 + 빌드 인프라 생성
  ├─ 002 외부 도구 체인 validation
  └─ 003 코딩 컨벤션 점검
  └─ 브랜치 생성 (detached HEAD 해소)

Phase 1A: Core Pipeline (16일) ────────────────────────
  ├─ 100 Bootstrap      CMake 빌드 + ModuleAnalyzer + 스켈레톤  (3일)
  ├─ 110 output-structure  출력 디렉토리 구조                     (1일)
  ├─ 111 cli            hirct-gen / hirct-verify CLI              (1.5일)
  ├─ 101 gen-model      C++ cycle-accurate 모델 생성기            (5일)
  ├─ 102 gen-tb         SV 테스트벤치 골격 생성기                 (1일)
  ├─ 106 gen-doc        HW 문서 + 프로그래머 가이드 생성기        (1.5일)
  └─ 109 verify         자동 검증 (hirct-verify)                   (3일)

Phase 1B: Remaining Emitters (12.5일, Phase 2와 병행 가능) ──
  ├─ 103 gen-dpic       DPI-C VCS 래퍼 생성기                     (2일)
  ├─ 104 gen-wrapper    SV Interface 래퍼 생성기                   (3일)
  ├─ 105 gen-format     IR 기반 RTL 포매터                         (3일)
  ├─ 107 gen-ral        UVM RAL + HAL + C 드라이버 생성기          (3일)
  └─ 108 gen-cocotb     Python testbench 생성기                    (1.5일)

Phase 2: 전체 순회 테스트 (12일, Phase 1B와 병행 가능) ──
  ├─ 201 개별 파일 순회 + 리포트
  ├─ 202 Top 순회 (filelist 기반)
  ├─ 203 자동 검증 (hirct-verify 전체 모듈)
  ├─ 204 실패 분석 + Phase 1 되돌림
  ├─ 205 테스트 자동화 (make test-all)
  ├─ 206 자동 실패 분류 + LLM 보조 패치
  └─ 207 MLIR 파싱 성공률 개선 (`-y` 라이브러리 경로)

Phase 3: 통합 및 배포 (8일) ───────────────────────────
  ├─ 301 VCS DPI-C co-simulation
  ├─ 302 Documentation (mkdocs)
  ├─ 303 프로덕션 패키징
  └─ 304 VCS/ncsim multi-level cross-validation

Phase 4: CIRCT 내장 아키텍처 전환 (14-21일) ──────────
  ├─ A: 빌드 인프라 (CMake + CIRCT/MLIR/LLVM 링크)     (1-2일)
  ├─ B: IR 분석 계층 (VerilogLoader + IRAnalysis)       (3-4일)
  ├─ C: GenModel 전면 재작성 (핵심 병목)                (5-7일)
  ├─ D: 나머지 Emitter 전환                            (3-4일)
  ├─ E: CLI / 오케스트레이션 전환                       (1-2일)
  └─ F: 검증 + 레거시 삭제 + 문서 업데이트              (1-2일)
```

> **Phase 4 전환 동기** (2026-02-28):
> Phase 1-3 완료 후, 실제 IP(UART) 정확성 검증 과정에서 현행 텍스트 파싱 아키텍처의 근본적 한계 확인:
> (1) 계층적 클럭 전파 경로 분석 불가 → old_ssa 타이밍 문제 며칠째 미해결
> (2) MLIR 파싱 성공률 36.7% (unknown module 98.3%)
> (3) 정규식 82개+ 유지보수 부담
> 결정: CIRCT를 라이브러리로 직접 링크하고 MLIR API로 IR을 순회하는 방식으로 전면 전환.
> 설계 문서: `docs/plans/2026-02-28-circt-embedding-design.md`

**Phase 1A 완료 게이트** (Phase 1B/2 진입 조건):
- `hirct-gen input.v` → cmodel/ + tb/ + doc/ + meta.json + Makefile 생성
- `hirct-verify input.v` → 10시드 x 1000cyc PASS (LevelGateway + RVCExpander)
- `make check-hirct` → lit 단위 테스트 PASS
- `g++ -c output/.../cmodel/*.cpp` → 컴파일 성공
- `make check-hirct-unit` → gtest C++ API 단위 테스트 PASS

**Agent-in-the-Loop 체크포인트**:

| 체크포인트 | 시점 | 자동 Gate | 수동 Gate |
|-----------|------|----------|----------|
| CP1: Bootstrap 완료 | Task 100 후 | `make build` exit 0 | CMake/디렉토리 설계 리뷰 |
| CP2: Core Pipeline 관통 | Batch 2+3 완료 후 (Task 101+110+111+109) | verify PASS + lit PASS + gtest PASS | C++ 코드 품질 + 아키텍처 리뷰 |
| CP3: Phase 1A 완료 | Task 102+106 후 | 위 5개 게이트 전부 | Phase 1B/2 진입 판정 + 기술 부채 리뷰 |

**Phase 1B는 Phase 2 초반과 병행 가능** — Phase 2의 전체 순회(Task 201)는 GenModel만으로도 실행 가능하므로, 1B emitter 추가와 순회를 동시 진행할 수 있다.

### 4.2 Phase 1B 실행 전략

> Phase 1B를 `subagent-driven-development` 스킬로 순차 실행한다.
> **원칙**: 같은 브랜치/워크스페이스에서 “수정(코드·문서)” 작업은 서브에이전트 **1개씩 순차 실행**한다 (컨텍스트 오염·파일 충돌 방지).
> **예외**: (1) 읽기 전용 조사/리뷰, (2) 서로 다른 파일을 건드리는 문서 편집은 병렬 가능. 단, **실제 적용(커밋/머지)은 순차**로 정리한다.
> 코드 변경 병렬이 꼭 필요하면 `using-git-worktrees`로 **worktree를 분리**해서 수행한다.

**서브에이전트 Brief(컨텍스트 번들) 템플릿** (각 태스크 시작 시 반드시 포함):

- **Goal**: 1문장 (무엇을 “끝낸 상태”가 성공인지)
- **Scope**: 수정 허용/금지 경로(예: `lib/Target/GenModel.cpp`만, `rtl/` 절대 금지)
- **SSOT**: 반드시 참조할 문서 목록 (`docs/plans/summary.md`, `docs/plans/hirct-convention.md`, `docs/plans/open-decisions.md`, `docs/plans/reference-commands-and-structure.md`, 필요 시 태스크 문서)
- **Gates**: 실행해야 하는 명령과 PASS 기준(예: `make check-hirct` exit 0)
- **Output**: 최종 답변에 포함할 것(변경 요약, 실행한 커맨드/결과, 리스크/추가 TODO)

**실행 순서**: 103(dpic) → 104(wrapper) → 105(format) → 107(ral) → 108(cocotb)

| emitter | 난이도 | 의존 | 비고 |
|---------|--------|------|------|
| 103 gen-dpic | 보통 | GenModel API(do_reset/step/eval_comb) | Phase 3 VCS co-sim 전제 |
| 104 gen-wrapper | 높음 | Trie 접두사 알고리즘 | |
| 105 gen-format | 보통 | CIRCT ExportVerilog | risk-validation §3 참조 |
| 107 gen-ral | 높음 | 레지스터 탐지 | |
| 108 gen-cocotb | 낮음 | 없음 | |

- 각 emitter 완료 시: (1) spec reviewer (2) code quality reviewer 자동 리뷰
- Phase 1B는 별도 사람 체크포인트 없음 — 자동 리뷰만으로 충분 (emitter가 서로 독립)
- Phase 1B 전체 완료 후: **다양성 게이트** 사람 확인 (5축 × 1개+ 모듈 PASS)

> **Phase 1 완료 선언** (2026-02-19):
> Phase 1A/1B 코드 구현 완료. lit 28/28 PASS, gtest 2/2 PASS, lint PASS, compile-check PASS.
> CI 4-gate (lint → gtest → lit → compile-check) 전체 통과.
> 다양성 게이트 4/5 완료 (WideSignal 중형 PASS, RegisterBlock RAL PASS, MultiModule XFAIL 등록).
> 파라미터(hw.param) 축은 Phase 2 이관. Phase 2 진입 차단하는 미해결 항목 없음.
> 근거: [Phase 1 리포트](../report/phase-1-pipeline/)

### 4.3 Phase 2 실행 전략 (Rule-Based Triage + LLM 보조)

> Phase 2는 `executing-plans` 스킬로 배치 실행한다.
> 전체 순회 후 rule-based triage 도구로 실패를 자동 분류하고, 체크포인트에서 사람이 개입한다.

**배치 구성**:

```
Batch A: Task 201 + 202 (순회)    → Checkpoint CP-2A (사람 리뷰)
Batch B: Task 203 (검증)           → Checkpoint CP-2B (사람 리뷰)
Batch C: Task 204 + 206 (분류)    → Checkpoint CP-2C (사람 리뷰: XFAIL 최종 승인)
Batch D: Task 207 (파싱 개선)      → Checkpoint CP-2A 재검증 (성공률/분포)
Batch E: Task 205 (자동화)        → 자동 리뷰만
```

**Agent-in-the-Loop 체크포인트 (Phase 2)**:

| 체크포인트 | 시점 | 자동 Gate | 수동 Gate |
|-----------|------|----------|----------|
| CP-2A: 순회 완료 | Task 201+202 후 | `report.json` 생성 | 성공률/실패 분포 확인 |
| CP-2B: 자동 검증 완료 | Task 203 후 | `verify-report.json` 생성 | mismatch 패턴 리뷰 |
| CP-2C: 분류 완료 | Task 204+206 후 | triage 리포트 생성 | XFAIL 리스트 최종 승인 |

**Triage 자동 조치** (`206-agent-triage.md` 참조):
- `unsupported_op`: `known-limitations.md` 자동 등록 PR
- `emitter_bug`: Phase 1 해당 태스크로 되돌림 이슈 생성
- `verify_mismatch`: 재현 커맨드 + VCD 덤프 첨부
- `parse_error`: `known-limitations.md`에 문서화

> **Phase 2 완료 선언** (2026-02-23):
> Phase 2 전체 순회 + 정확성 검증 + 분류 + 게이트 정렬 완료.
> `make test-all` exit 0 (lit 44 PASS + 1 XFAIL, gtest 2/2, traversal 1604/1597, integration smoke 1 PASS + 1 XFAIL).
> verify 1121 모듈: 133 PASS / 224 FAIL / 764 SKIP. FAIL/SKIP 사유는 known-limitations + closeout 체크리스트에 문서화.
> `make triage` exit 0, triage-report.json 324건 분류. Task 206 게이트 5/5 PASS.
> 회귀 0건. Phase 3 진입 차단하는 미해결 항목 없음 (FAIL/SKIP은 Phase 3 작업 입력으로 귀속됨).

### 4.4 Phase 3 실행 전략

> **Phase 3 완료 선언** (2026-02-23):
> Task 301 VCS DPI-C co-sim: LevelGateway 10seed×1000cyc PASS (VCS V-2023.12-SP2-7), Queue_11 FAIL (hirct-gen 버그 확정).
> Task 302 Documentation: `make docs` exit 0, mkdocs build --strict 성공, `site/index.html` 존재.
> Task 303 Production Packaging: README.md 영문 재작성, Dockerfile, SECURITY.md, CHANGELOG.md 생성.
> Quick Start 3단계 검증: setup → build → hirct-gen → 9종 산출물 확인.
> `make test-all` exit 0 (lit 44 PASS + 1 XFAIL, gtest 2/2, traversal 1604/1597, integration smoke 1 PASS + 1 XFAIL). 회귀 0건.
> Docker build: 네트워크 차단 환경으로 CI에서 별도 검증 필요.
> `git tag v0.1.0`: CP-3B 승인 후 태깅 예정.

> Phase 3는 `executing-plans` 스킬로 순차 실행한다.
>
> **Post-Phase 3 감사** (2026-02-23): 3-모델 교차 리뷰(GPT/OPUS/Sonnet) 수행.
> Queue_11 FAIL 근본 원인(GenModel 음수 상수 비트폭 미정규화)은 v0.1.0 전 필수 수정(Stage A).
> ncsim co-sim은 기존 로드맵에 없으므로 scope creep으로 판정, backlog(Stage C)로 분리.
> Verilator VCD/VCS waveform은 디버깅 인프라로 v0.1.0 후 개선(Stage B).

**배치 구성**: Task 301 → 302 → 303 (순차) → 304 (VCS/ncsim multi-level cross-validation)

> **Task 304 실행 결과** (2026-02-24):
> Module gate: LevelGateway VCS 10/10 + ncsim 10/10 PASS. Queue_11 VCS 1/10 + ncsim 0/10 (XFAIL).
> IP-top gate: CLINT VCS 0/10 FAIL (CModel 로직 버그). TLPLIC/PeripheryBus SKIP.
> SoC gate: SKIP (RocketTile/CoreIPSubsystem CModel 미생성 — hw.instance 깊이 제한).
> VCS vs ncsim diff: PRNG 차이로 per-seed 비교 불가, 전체 PASS/FAIL 판정 일치 확인.
> IUS 15.1 호환: `-static-libstdc++` + `LD_PRELOAD` workaround 적용.
> Waveform: `vcs-cosim/results/vcs/Queue_11_s1.vcd` (VCD, seed=1, 100cyc).
> 리포트: `docs/report/phase-3-release/cross-validation-{module,ip,soc}-gate.md`, `cross-validation-diff-vcs-vs-ncsim.md`

**Agent-in-the-Loop 체크포인트 (Phase 3)**:

| 체크포인트 | 시점 | 자동 Gate | 수동 Gate |
|-----------|------|----------|----------|
| CP-3A: VCS 완료 | Task 301 후 | VCS 10seed×1000cyc PASS | 3자 비교 결과 리뷰 |
| CP-3B: 배포 준비 | Task 303 후 | 클린 환경 Quick Start 성공 | 최종 배포 승인 |

**실측 파싱 성공률 (현재 main 기준 재측정)**: 1,597개 .v 파일 중 **586개(36.7%)** 가 단일 파일 모드(`circt-verilog <file>.v`)에서 MLIR 변환 성공.
MLIR 실패 1,011건 중 **993건(98.3%)** 이 `unknown module`이며, 단일 파일 호출 경로의 의존 모듈 미해결이 주요 병목이다.
(Task 207) `-y <dir>` 라이브러리 경로 보강 + filelist 우선 전략으로 개선한다.

### 4.5 입력 참조 전략 (Manifest-first)

Phase 2/Task 207 기준으로 Verilog 입력 참조는 아래 우선순위를 따른다.

1. **명시적 filelist 직접 사용(권장)**: 프로젝트가 제공하는 `filelist.f`를 SSOT로 사용
2. **빌드 시스템 import(연동)**: Make/Bazel/FuseSoC 등에서 입력/옵션 추출
3. **root scan 초안 생성(fallback)**: 매니페스트 부재 시 `scan`으로 filelist 후보 생성 후 사람 검토
4. **단일 파일 `-y` 보강(호환)**: `hirct-gen single.v`/`make generate` 경로에서 `-y` 자동 주입

> 원칙: 자동화(scan/import)는 보조 수단이며, 최종 재현성 기준은 명시적 manifest(filelist)에 둔다.

---

## 4.1 실행 전략 (Agent-in-the-Loop)

> Phase 1을 `subagent-driven-development` 스킬로 실행한다.
> 각 태스크를 독립 서브에이전트가 구현 → spec reviewer가 게이트 검증 → code quality reviewer가 코드 리뷰.
> 체크포인트에서만 사람이 개입한다.

**배치 구성 (Phase 1A)**:

```
Batch 1: Task 100 (Bootstrap)              → Checkpoint 1 (사람 리뷰)
Batch 2: Task 110 + 111 + 101              → 자동 리뷰만
         (output-structure + CLI + gen-model)
Batch 3: Task 102 + 106 + 109              → Checkpoint 2 + 3 (사람 리뷰)
         (gen-tb + gen-doc + verify)
```

**배치 간 규칙**:
- 각 태스크는 독립 서브에이전트가 실행 (컨텍스트 오염 방지)
- 태스크 완료 시 2단계 자동 리뷰: (1) spec reviewer = 태스크 문서의 게이트 기준, (2) code quality reviewer = C++ 코드 품질
- Checkpoint에서 사람이 자동 Gate 출력 확인 + 수동 Gate 수행
- 사람 리뷰에서 문제 발견 시 해당 태스크로 되돌림

**Phase 1B 실행**: Phase 1B의 5개 emitter(103~108)는 서로 독립이므로, **조사/리뷰는 병렬**로 진행할 수 있다. 다만 코드 변경 적용(커밋/머지)은 **순차 1개씩** 진행한다. Phase 2 초반(Task 201)과도 병행 가능.

---

## 5. 피드백 루프

```
Phase 2 (전체 순회)
       │ 실패 발견
       ▼
  원인 분류:
    미지원 IR 연산 → 101 gen-model
    emitter 버그 → 해당 1xx
    드라이버 문제 → 109 verify
    DPI-C 문제 → 103 gen-dpic
    Verilator 버그 의심 → Phase 3 (301) VCS 교차 검증
       │ 수정 후
       ▼
  Phase 2 재실행
```

---

## 6. 빌드 시스템

> **Phase 4 아키텍처 전환 이후 (2026-02-28~)**:
> CIRCT/MLIR/LLVM 라이브러리를 직접 링크한다. `find_package(CIRCT)` + `find_package(MLIR)` + `find_package(LLVM)` 사용.
> `CIRCT_BUILD` 환경변수 (또는 CMake 캐시)로 CIRCT 빌드 디렉토리를 지정한다.
> 정적 링크(`HIRCT_STATIC`)는 기본 OFF. LLVM과 호환되는 링크 방식 사용.

```
CMake + Ninja ─── 바이너리 빌드 + lit 테스트 구성 (C++17 + CIRCT/MLIR/LLVM 링크)
                  ├── 의존성: find_package(LLVM/MLIR/CIRCT REQUIRED CONFIG)
                  ├── 라이브러리: CIRCTImportVerilog, CIRCTHW, CIRCTComb, CIRCTSeq, CIRCTLLHD, ...
                  ├── 소스: include/hirct/ (헤더), lib/ (구현), tools/ (CLI)
                  ├── lit 통합: configure_file(lit.site.cfg.py.in) → 도구 경로 주입
                  └── 테스트 타겟: ninja check-hirct, check-hirct-unit, check-hirct-integration
Makefile ────── 사용자 진입점 (얇은 래퍼)
                  ├── 프로젝트 루트: build, generate, test-all → 내부적으로 ninja 호출
                  └── output 각 디렉토리: test, test-verify (자동 생성)
lit ──────── 테스트 실행 엔진
                  ├── test/: emitter 단위 (FileCheck)
                  ├── unittests/: C++ API (gtest)
                  └── integration_test/: E2E 파이프라인
```

---

## 7. C++ 소스 매핑

> **Phase 4 아키텍처 전환 (2026-02-28~)**: CirctRunner 삭제, ModuleAnalyzer 정규식 파싱 삭제.
> VerilogLoader (인메모리 importVerilog) + IRAnalysis (MLIR API 순회) 신규 도입.
> 모든 emitter가 `mlir::Operation`을 직접 받아 처리하도록 전면 리팩토링.

| hirct-gen 기능 | C++ 소스 | Phase 4 전환 | 상태 |
|---|---|---|---|
| ~~CirctRunner~~ (외부 프로세스 래퍼) | ~~lib/Support/CirctRunner.cpp~~ | **삭제 완료** (Phase 4-F) → llvm::sys::ExecuteAndWait로 대체 | Phase 4-F |
| VerilogLoader (인메모리 Verilog→MLIR) | lib/Support/VerilogLoader.cpp | **신규** (importVerilog API 사용) | Phase 4-B |
| ~~ModuleAnalyzer~~ (정규식 파싱) | ~~lib/Analysis/ModuleAnalyzer.cpp~~ | **삭제 완료** (Phase 4-F) → IRAnalysis로 대체 | Phase 4-F |
| IRAnalysis (MLIR API 분석 유틸리티) | lib/Analysis/IRAnalysis.cpp | **신규** (PortView, RegisterView, ClockDomainMap 등) | Phase 4-B |
| gen-model | lib/Target/GenModel.cpp | **전면 재작성** (hw::HWModuleOp 직접 순회) — **완료** (1,900줄, MLIR 단일 경로) | Phase 4-C |
| gen-tb | lib/Target/GenTB.cpp | IRAnalysis API로 전환 — **완료** | Phase 4-D |
| gen-doc | lib/Target/GenDoc.cpp | IRAnalysis API로 전환 — **완료** | Phase 4-D |
| verify (드라이버 생성) | lib/Target/GenVerify.cpp | IRAnalysis API로 전환 — **완료** | Phase 4-D |
| Makefile 생성 | lib/Target/GenMakefile.cpp | 최소 변경 (module_name만) — **완료** | Phase 4-D |
| CLI (hirct-gen) | tools/hirct-gen/main.cpp | VerilogLoader + SymbolTable로 전환 | Phase 4-E |
| CLI (hirct-verify) | tools/hirct-verify/main.cpp | VerilogLoader로 전환 | Phase 4-E |
| gen-dpic | lib/Target/GenDPIC.cpp | IRAnalysis API로 전환 (클럭 도메인) — **완료** | Phase 4-D |
| gen-wrapper | lib/Target/GenWrapper.cpp | IRAnalysis API로 전환 — **완료** | Phase 4-D |
| gen-format | lib/Target/GenFormat.cpp | IRAnalysis API로 전환 — **완료** | Phase 4-D |
| gen-ral | lib/Target/GenRAL.cpp | IRAnalysis API로 전환 — **완료** | Phase 4-D |
| gen-cocotb | lib/Target/GenCocotb.cpp | IRAnalysis API로 전환 — **완료** | Phase 4-D |

---

## 8. 핸드오프 Quick Reference

> 이 프로젝트의 계획 문서를 처음 접하는 사람을 위한 1페이지 가이드.

**읽는 순서**:
1. 이 문서 (`summary.md`) — 전체 그림
2. `hirct-convention.md` — 기술 규약
3. Phase별 README.md → 해당 Phase 태스크 문서

**핵심 명령어**:
```bash
# Phase 0
make setup                    # 도구 설치 + 검증
make build                    # hirct-gen/hirct-verify 빌드

# Phase 1
hirct-gen input.v             # 9종 산출물 생성
hirct-verify input.v          # 자동 등가성 검증
make check-hirct              # lit 단위 테스트
make check-hirct-unit         # gtest 단위 테스트

# Phase 2
make test-all                 # 전체 테스트 (메인 진입점)
make test-traversal           # ~1,600 .v 전체 순회

# Phase 3
make docs                     # mkdocs 문서 사이트
```

**운영 매뉴얼**:
- 테스트 실행: `make test-all` (lit + gtest + integration smoke)

**Phase별 진입/완료 조건 요약**:

| Phase | 진입 조건 | 완료 게이트 |
|-------|----------|------------|
| 0 | 없음 | `make setup` exit 0, 외부 도구 pre-test PASS |
| 1A | Phase 0 완료 | hirct-gen/verify 동작 + lit/gtest PASS |
| 1B | Phase 1A 완료 | 모든 emitter LevelGateway 산출물 생성 — **완료** (2026-02-19, lit 28/28, gtest 2/2, 4-gate PASS, 다양성 4/5) |
| 2 | Phase 1A 완료 | `make test-all` exit 0, XFAIL/SKIP 사유 문서화 완료 — **완료** (2026-02-23, 133P/224F/764S, FAIL/SKIP 귀속표 작성) |
| 3 | Phase 2 완료 | 클린 환경 Quick Start 3단계 성공 — **완료** (2026-02-23, VCS LG 10/10 PASS, make docs exit 0, Quick Start 3단계 확인, 배포 파일 완성) |
| 4 | Phase 3 완료 | CirctRunner/ModuleAnalyzer 정규식 전체 삭제, lit 전체 PASS, UART 11/11 PASS, VCS 26/26 PASS |

**배포 필수 파일** (Phase 0에서 stub 생성, Phase 3에서 완성):
- `LICENSE` — Apache License 2.0 with LLVM Exceptions (상용 사용 제약 없음)
- `README.md` — Quick Start + 프로젝트 소개
- `CONTRIBUTING.md` — 기여 가이드

---

## 9. Phase 4 아키텍처 전환: 현재 이슈 → 개선 비교

> 상세: `docs/plans/2026-02-28-circt-embedding-design.md`

### 9.1 파이프라인 비교

```
[Phase 1-3 아키텍처 — 폐기 예정]
  .v → fork(circt-verilog) → MLIR 텍스트 stdout
     → ModuleAnalyzer(regex 82+) → PortInfo/OpInfo/InstanceInfo (자체 구조체)
     → GenModel/GenTB/... (자체 구조체 소비)

[Phase 4 아키텍처 — 전환 대상]
  .v → importVerilog() (in-process) → mlir::ModuleOp (인메모리 IR)
     → circt-opt passes (in-process, 선택)
     → Emitter가 mlir::Operation을 직접 순회
```

### 9.2 이슈별 개선 비교

| # | 이슈 | 현재 상태 | 현행 아키텍처 원인 | MLIR API 내장 후 해결 경로 |
|---|------|----------|------------------|------------------------|
| 1 | **Phase D FIFO TX FAIL** (old_ssa 타이밍) | 며칠째 미해결 | 인스턴스 경계 넘는 클럭 전파 경로 분석 불가. 정규식으로는 use-def chain 추적 불가능 | `FirRegOp::getClk()` → `InstanceOp` 포트 매핑 → use-def chain으로 정확한 클럭 전파 경로 추적. `save_old_ssa()` 재귀 호출 시점 자동 결정 |
| 2 | **eval_comb 인스턴스 순서** (topo sort vs deferred) | 수정 완료 (필요조건) | pre-register로 모든 인스턴스 즉시 emit → topo sort 무시 | MLIR 블록 내 연산 순서가 이미 topological. `InstanceOp` operand→result 관계로 정확한 의존성 그래프 |
| 3 | **MLIR 파싱 성공률 36.7%** | Phase 2 실측 | `circt-verilog` 단일 파일 호출 시 의존 모듈 미해결 (unknown module 98.3%) | `importVerilog()` + `SourceMgr`로 다중 파일 직접 로드. `libDirs` 옵션으로 라이브러리 경로 전달 |
| 4 | **멀티클럭 도메인 step 분리** | 구현됨 (8-depth 제한) | `seq.to_clock` 체인을 텍스트 역추적 (최대 8 depth). 포트 이름 패턴으로 클럭 판별 | `getDefiningOp()` 루프로 depth 무제한 추적. `BlockArgument`까지 도달하여 정확한 포트 식별 |
| 5 | **타입 파싱 실패** (`!hw.array<NxT>`) | 부분 대응 | 정규식으로 복잡한 MLIR 타입 표현 파싱 한계 | `Type::cast<hw::ArrayType>()` → `getSize()`, `getElementType()` 직접 접근 |
| 6 | **정규식 유지보수 부담** (82+ 패턴) | ModuleAnalyzer 1,812줄 | 새 IR 패턴마다 정규식 추가 필요 | `Operation::walk()` + `dyn_cast<OpType>()` — 새 op 추가 시 파서 수정 불필요 |
| 7 | **CIRCT pass 활용 불가** | 외부 프로세스로만 호출 | fork/exec 오버헤드. pass 결과를 텍스트로 다시 파싱해야 함 | `PassManager::run()` 인프로세스 호출. canonicalize, flatten 등 직접 적용 |

### 9.3 Phase 4 체크포인트

| 체크포인트 | 시점 | 자동 Gate | 수동 Gate |
|-----------|------|----------|----------|
| CP-4A | Phase A 완료 (빌드) | cmake + ninja exit 0 | CMake 설계 리뷰 |
| CP-4B | Phase B 완료 (IR 분석) | gtest PASS | IRAnalysis API 리뷰 |
| CP-4C | Phase C 완료 (GenModel) | LevelGateway eval_comb + step 검증 — **완료** (GenModel.cpp 1,900줄, MLIR 단일 경로, use_mlir_/dual-path 0건) | C++ 코드 품질 리뷰 |
| CP-4D | Phase D 완료 (Emitter) | lit 전체 PASS — **완료** (전 emitter MLIR 단일 경로, use_mlir_/analyzer_ 분기 0건) | 산출물 diff 리뷰 |
| CP-4F | Phase F 완료 (검증) | UART 11/11 + VCS 26/26 + lit 전체 — **완료** (lit 57/57, gtest 2/2, integration FAIL 0, CirctRunner/ModuleAnalyzer 0건) | 최종 코드 리뷰 + 레거시 삭제 승인 |

### 9.4 커스텀 MLIR Pass + Arc PoC Phase A (2026-03-04)

Phase 4-B IR 분석 계층 구축과 병행하여 커스텀 MLIR pass 4종 + verilator -E 전처리 + --run-pass CLI 구현 완료.
Arc PoC Phase A에서 arcilator 경로 평가 및 266개 llhd.process 전수 조사 수행, CONDITIONAL GO 판정.

| 작업 | 상태 | 산출물 | 효과 |
|------|------|--------|------|
| HirctSimCleanup | 완료 | `lib/Transforms/HirctSimCleanup.cpp` | sim display process DCE |
| HirctUnrollProcessLoops | 완료 | `lib/Transforms/HirctUnrollProcessLoops.cpp` (401줄) | 180/266 llhd.process 해소 (slt/ult/eq, N<1024) |
| HirctProcessFlatten V2 | 완료 | `lib/Transforms/HirctProcessFlatten.cpp` (토폴로지 알고리즘) | 다이아몬드 패턴 O(n), wait-dest-args 지원 |
| HirctSignalLowering | 완료 | `lib/Transforms/HirctSignalLowering.cpp` | llhd.sig/prb/drv → hw/comb/seq |
| verilator -E 전처리 | 완료 | `lib/Support/VerilatorPreprocessor.cpp` + CLI `--preprocess` | L1(+define+, timescale) + L2(concat_ref) 해소 |
| --run-pass CLI | 완료 | `tools/hirct-gen/main.cpp` (`--run-pass <name>`) | pass별 lit 테스트 가능 |

**파이프라인 순서**: SimCleanup → UnrollProcessLoops → ProcessFlatten → **ProcessDeseq** → SignalLowering → CSE → Canonicalize

**빌드**: ninja exit 0, lit 56/56 PASS (100%)

**근거**: `docs/report/phase-4-circt-embedding/arc-poc-follow-up.md`

### 9.5 Phase 2 재실행 + SKIP 모듈 재분석 (2026-03-04)

커스텀 pass 4종 + verilator -E 적용 후 fc6161 전수 56개 타겟 재측정.
병렬로 53개 SKIP 모듈에 verilator -E → circt-verilog MLIR 생성 테스트 수행.

**Task #3 결과 (GenModel Survey V2)**

| 지표 | Baseline (v1) | V2 | 변화 |
|------|:---:|:---:|------|
| pass / pass_with_warnings | 3 / 0 | 0 / 6 | uart, gpio, wdt, ptimer 추가 통과 |
| unresolved_process | 2 | 0 | 루프 언롤링으로 해소 |
| unresolved_drive | 2 | 0 | SignalLowering으로 해소 |
| GenModel-specific failures | 4 | **0** | fsl_wflow: ProcessDeseq V2로 해소 (V3) |
| timeout | 1 (301.6s) | 0 | axi_2x3 해소 |
| 총 실행 시간 | 357.8s | 27.9s | -92.2% |

**Task #5 결과 (SKIP 모듈 재분석)**

| 지표 | 값 |
|------|---:|
| SKIP 타겟 | 53 |
| verilator -E 성공 | 37 (69.8%) |
| circt-verilog 성공 | 6 (11.3%) |
| 신규 MLIR 접근 가능 | i2cm, smbus, axi_x2p1, axi_x2p2 (4개, 총 200 hw.module) |
| 잔존 llhd.process | 176개 (UnrollProcessLoops 적용 대상) |

**근거**: `docs/report/phase-4-circt-embedding/phase2-rerun-and-skip-analysis.md`

### 9.6 파이프라인 Canonicalize 최적화 + --timing 프로파일링 (2026-03-04)

`load_verilog()` 경로에서 중복 Canonicalize 실행을 제거하고, PassManager 프로파일링 인프라를 추가.

| 작업 | 상태 | 산출물 | 효과 |
|------|------|--------|------|
| `VerilogLoadOptions::canonicalize` 기본값 `false` | 완료 | `VerilogLoader.h` | Canonicalize 7→6회 (HIRCT 파이프라인이 이미 CSE+Canonicalize 포함) |
| `--timing` CLI 옵션 + `enable_timing` API | 완료 | `VerilogLoader.h/cpp` + `main.cpp` | `pm.enableTiming()` 연결, pass별 소요 시간 프로파일링 가능 |
| HIRCT process pass early-exit | 보류 | — | 현재 코드가 이미 빈 worklist→0회 iteration. 프로파일링 결과 확인 후 판단 |

**검토 결론**: CIRCT와 HIRCT의 Deseq/UnrollLoops/RemoveControlFlow pass는 "중복"이 아닌 "fallback" 구조 (CIRCT→CombinationalOp, HIRCT→ProcessOp). pass 제거는 위험. Canonicalize 중복만 확실한 개선.

---

## 변경 이력

| 버전 | 날짜 | 변경 내용 |
|------|------|----------|
| v1 | — | 초기 작성 |
| v2 | 2026-02-16 | §3 meta.json 추가, §6 빌드 시스템 (lit 추가, check-hirct 타겟), §7 소스 경로 CIRCT 스타일 (lib/Target/, lib/Analysis/) 반영, 삭제/통합 확정 |
| v3 | 2026-02-16 | dry-run 반영: §7 C++ 소스 매핑을 "전부 신규 작성"으로 수정, Task 100 (Bootstrap) 추가 반영, 리네이밍/삭제/통합 표현 제거 |
| v4 | 2026-02-16 | 리뷰 반영: §4 Phase 1A/1B 분리 (core pipeline 우선 안정화), §6 빌드 시스템 LLVM/MLIR API 미링크 명시, §7 Phase 열 추가 + CirctRunner 추가, Phase 1A 완료 게이트 추가 |
| v5 | 2026-02-16 | 리뷰 반영: §4 Phase 1A 게이트에 check-hirct-unit 추가, §8 핸드오프 Quick Reference 추가, 배포 필수 파일 명시 |
| v6 | 2026-02-16 | 잔존 5건 해소: LICENSE 확정(Apache 2.0 + LLVM Exception), 토폴로지 정렬(Kahn's + 순환 감지), GenWrapper 접두사 알고리즘(Trie), GenRAL 주소 할당(순차), 성능 예산, hw.module 파라미터(CIRCT elaboration) |
| v7 | 2026-02-16 | 정합성 검토 반영: §4 Agent-in-the-Loop 체크포인트 테이블 추가 (CP1/CP2/CP3), §4.1 실행 전략 섹션 추가 (배치 구성, subagent-driven-development) |
| v8 | 2026-02-19 | Phase 1 완료 선언: plan↔report 동기화, 다양성 게이트 4/5 (WideSignal/MultiModule/RegisterBlock 픽스처 추가), known-limitations.md XFAIL 3건 등록 |
| v9 | 2026-02-22 | Phase 2 로드맵에 Task 206/207 반영, MLIR 파싱 실측치 최신화(586/1,597, unknown module 98.3%), 입력 참조 전략(Manifest-first 4단계) 추가 |
| v10 | 2026-02-23 | Phase 3 감사: 3-모델 교차 리뷰(GPT/OPUS/Sonnet) 통합. Phase 3 완료 확정(산출물 실사 검증). Stage A(v0.1.0 전 필수: GenModel 음수 상수 버그, 문서 정정, 리포트 생성)→B(v0.1.0 후 개선: Verilator VCD, VCS waveform)→C(backlog: ncsim scope creep 분리) 로드맵 확정 |
| v11 | 2026-02-28 | **Phase 4 추가**: CIRCT 내장 아키텍처 전환. 외부 프로세스 호출(CirctRunner) + 정규식 파싱(ModuleAnalyzer 82+ regex) → CIRCT 라이브러리 직접 링크 + MLIR API 순회로 전면 교체. §6 빌드 시스템(CIRCT/MLIR/LLVM 링크), §7 소스 매핑(VerilogLoader/IRAnalysis 신규, CirctRunner/ModuleAnalyzer 삭제), §9 이슈별 개선 비교표 추가. 설계 문서: `2026-02-28-circt-embedding-design.md` |
| v12 | 2026-03-04 | §9.4 추가: Arc PoC Phase A 완료(CONDITIONAL GO) + 커스텀 MLIR pass 4종(SimCleanup, UnrollProcessLoops, ProcessFlatten V2, SignalLowering) + verilator -E 전처리 + --run-pass CLI. lit 55/55 PASS |
| v13 | 2026-03-04 | §9.5 추가: Phase 2 재실행(GenModel-specific failures 4→1, 시간 92.2%↓) + SKIP 53개 모듈 verilator -E 재분석(6개 MLIR 성공, 4개 신규 해소) |
| v14 | 2026-03-04 | §9.5 업데이트: ProcessDeseq V2(intermediate block clone + merge_args fix)로 fsl_wflow emit_fail 해소. GenModel-specific failures 1→0 (V3 survey). lit 56/56 PASS |
| v15 | 2026-03-04 | §9.6 추가: 파이프라인 Canonicalize 최적화(기본값 false, 7→6회) + `--timing` 프로파일링 CLI. HIRCT pass fallback 구조 문서화 |
| v16 | 2026-03-04 | §7/§9.3 갱신: 4-C GenModel 완료(1,900줄, MLIR 단일 경로, dual-path/use_mlir_ 0건), 4-D 전 emitter 완료(IRAnalysis 단일 경로, analyzer_ 분기 0건) |
| v17 | 2026-03-05 | **Phase 4-F 완료**: CirctRunner(281줄)/ModuleAnalyzer(1,808줄) + 관련 테스트 전면 삭제(~2,647줄 제거). hirct-verify를 llvm::sys::ExecuteAndWait로 대체. §7 소스 매핑 "삭제 완료"로 갱신, §9.3 CP-4F 완료 선언. lit 57/57, gtest 2/2, integration FAIL 0 확인 |
