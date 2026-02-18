# HIRCT 실행 계획 총괄

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** hirct-gen / hirct-verify가 모든 RTL 입력에서 올바른 산출물을 생성하고, 전체 순회 테스트를 통과하여 프로덕션 품질로 배포한다.

**Architecture:** 기능별 구현(Phase 1) → 전체 순회 테스트(Phase 2) → 배포(Phase 3). 입력 크기(단일/블록/Top)는 Phase가 아니라 Phase 2의 테스트 파라미터.

**Tech Stack:** CIRCT, Verilator 5.020, hirct-gen/hirct-verify (C++17), VCS V-2023.12-SP2-7

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

## 2. hirct-gen 산출물 (8종)

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
  └─ 205 테스트 자동화 (make test-all)

Phase 3: 통합 및 배포 (5일) ───────────────────────────
  ├─ 301 VCS DPI-C co-simulation
  ├─ 302 Documentation (mkdocs)
  └─ 303 프로덕션 패키징
```

**Phase 1A 완료 게이트** (Phase 1B/2 진입 조건):
- `hirct-gen input.v` → cmodel/ + tb/ + doc/ + meta.json + Makefile 생성
- `hirct-verify input.v` → 10시드 x 1000cyc PASS (LevelGateway + RVCExpander)
- `make check-hirct` → lit 단위 테스트 PASS
- `g++ -c output/.../cmodel/*.cpp` → 컴파일 성공
- `make check-hirct-unit` → gtest C++ API 단위 테스트 PASS

**Agent-in-the-Loop 체크포인트** (상세: `phase-1-pipeline/README.md` §Agent-in-the-Loop 체크포인트):

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

### 4.3 Phase 2 실행 전략 (Rule-Based Triage + LLM 보조)

> Phase 2는 `executing-plans` 스킬로 배치 실행한다.
> 전체 순회 후 rule-based triage 도구로 실패를 자동 분류하고, 체크포인트에서 사람이 개입한다.

**배치 구성**:

```
Batch A: Task 201 + 202 (순회)    → Checkpoint CP-2A (사람 리뷰)
Batch B: Task 203 (검증)           → Checkpoint CP-2B (사람 리뷰)
Batch C: Task 204 + 206 (분류)    → Checkpoint CP-2C (사람 리뷰: XFAIL 최종 승인)
Batch D: Task 205 (자동화)        → 자동 리뷰만
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

### 4.4 Phase 3 실행 전략

> Phase 3는 `executing-plans` 스킬로 순차 실행한다.

**배치 구성**: Task 301 → 302 → 303 (순차)

**Agent-in-the-Loop 체크포인트 (Phase 3)**:

| 체크포인트 | 시점 | 자동 Gate | 수동 Gate |
|-----------|------|----------|----------|
| CP-3A: VCS 완료 | Task 301 후 | VCS 10seed×1000cyc PASS | 3자 비교 결과 리뷰 |
| CP-3B: 배포 준비 | Task 303 후 | 클린 환경 Quick Start 성공 | 최종 배포 승인 |

**실측 파싱 성공률**: 1,597개 .v 파일 중 **590개(36%)** 가 단일 파일 모드(`circt-verilog <file>.v`)에서 MLIR 변환 성공.
나머지 64%의 실패 원인은 `unknown module` (외부 모듈 의존성)이며, filelist 모드(`-f`)에서 해소 예상.
(근거: `docs/plans/risk-validation-results.md` §2)

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

```
CMake + Ninja ─── 바이너리 빌드 + lit 테스트 구성 (순수 C++17, LLVM/MLIR API 미링크)
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

> **Note**: 이 저장소에는 C++ 소스 코드가 존재하지 않는다. 모든 파일을 **처음부터 신규 작성**한다.
> Task 100(Bootstrap)에서 기본 스켈레톤을 생성하고, 각 태스크에서 기능을 완성한다.

| hirct-gen 기능 | C++ 소스 | 생성 태스크 | Phase | 상태 |
|---|---|---|---|---|
| CirctRunner (외부 프로세스 래퍼) | lib/Support/CirctRunner.cpp | Task 100 (Bootstrap) | **1A** | **신규** |
| ModuleAnalyzer | lib/Analysis/ModuleAnalyzer.cpp | Task 100 (Bootstrap) | **1A** | **신규** |
| gen-model | lib/Target/GenModel.cpp | Task 100 (스켈레톤) + Task 101 (완성) | **1A** | **신규** |
| gen-tb | lib/Target/GenTB.cpp | Task 102 | **1A** | **신규** |
| gen-doc | lib/Target/GenDoc.cpp | Task 106 | **1A** | **신규** |
| verify (드라이버 생성) | lib/Target/GenVerify.cpp | Task 109 | **1A** | **신규** |
| Makefile 생성 | lib/Target/GenMakefile.cpp | Task 100 (스켈레톤) + Task 110 (완성) | **1A** | **신규** |
| CLI (hirct-gen) | tools/hirct-gen/main.cpp | Task 100 (스켈레톤) + Task 111 (완성) | **1A** | **신규** |
| CLI (hirct-verify) | tools/hirct-verify/main.cpp | Task 100 (스켈레톤) + Task 109 (완성) | **1A** | **신규** |
| gen-dpic | lib/Target/GenDPIC.cpp | Task 103 | **1B** | **신규** |
| gen-wrapper | lib/Target/GenWrapper.cpp | Task 104 | **1B** | **신규** |
| gen-format | lib/Target/GenFormat.cpp | Task 105 | **1B** | **신규** |
| gen-ral | lib/Target/GenRAL.cpp | Task 107 | **1B** | **신규** |
| gen-cocotb | lib/Target/GenCocotb.cpp | Task 108 | **1B** | **신규** |

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
hirct-gen input.v             # 8종 산출물 생성
hirct-verify input.v          # 자동 등가성 검증
make check-hirct              # lit 단위 테스트
make check-hirct-unit         # gtest 단위 테스트

# Phase 2
make test-all                 # 전체 테스트 (메인 진입점)
make test-traversal           # ~1,600 .v 전체 순회

# Phase 3
make docs                     # mkdocs 문서 사이트
```

**운영 매뉴얼 링크**:
- Phase 1A quickstart: `docs/plans/phase-1-pipeline/README.md`의 "Phase 1A Getting Started"
- 테스트 자동화/로컬 실행: `docs/plans/phase-2-testing/205-test-automation.md`

**Phase별 진입/완료 조건 요약**:

| Phase | 진입 조건 | 완료 게이트 |
|-------|----------|------------|
| 0 | 없음 | `make setup` exit 0, 외부 도구 pre-test PASS |
| 1A | Phase 0 완료 | hirct-gen/verify 동작 + lit/gtest PASS |
| 1B | Phase 1A 완료 | 모든 emitter LevelGateway 산출물 생성 |
| 2 | Phase 1A 완료 | `make test-all` exit 0, XFAIL 제외 100% PASS |
| 3 | Phase 2 완료 | 클린 환경 Quick Start 3단계 성공 |

**배포 필수 파일** (Phase 0에서 stub 생성, Phase 3에서 완성):
- `LICENSE` — Apache License 2.0 with LLVM Exceptions (상용 사용 제약 없음)
- `README.md` — Quick Start + 프로젝트 소개
- `CONTRIBUTING.md` — 기여 가이드

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
