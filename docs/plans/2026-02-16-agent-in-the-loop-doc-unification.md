# HIRCT Agent-in-the-Loop 문서 통합 계획

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** GPT/Claude 두 리뷰에서 발견된 모든 문서 간 불일치(12건)를 한 번의 원자적 편집으로 해소하고, 전 Phase Agent-in-the-Loop 전략을 완성하여 subagent-driven-development로 즉시 실행 가능한 계획서 세트를 만든다.

**Architecture:** 문서 편집 전용 계획. 12개 기존 .md 파일의 특정 텍스트를 교체/추가하고 1개 신규 .md를 생성한다. 각 편집은 old_string → new_string으로 특정되며, grep으로 교차 참조 일관성을 검증한다. 코드 변경 없음.

**Tech Stack:** Markdown, grep (검증)

---

## Task 1: Proposal 불일치 수정 (G2: verify-decisions, G3: 출력 경로)

**Files:**
- Modify: `docs/proposal/001-hirct-automation-framework.md:143-144` (verify-decisions)
- Modify: `docs/proposal/001-hirct-automation-framework.md:84-88` (출력 경로)

### Step 1: verify-decisions 텍스트 교체 (2분)

**old_string:**
```
- **verify-decisions**: 의사결정 문서(`open-decisions.md`)의 RESOLVED 항목은 자동 검증 스크립트로 반영 여부를 확인.
```

**new_string:**
```
- **verify-decisions**: 의사결정 문서(`open-decisions.md`)의 RESOLVED 항목은 `test/`(lit) 및 `unittests/`(gtest)의 테스트 PASS로 반영을 증명한다. 별도 검증 스크립트는 사용하지 않는다.
```

### Step 2: 출력 경로 원칙 명확화 (3분)

**old_string:**
```
- **원본 RTL 불변**: `rtl/` 디렉토리는 절대 수정하지 않는다.
- **소스 트리 미러링**: `rtl/<path>/<file>.v` → `output/<path>/<file>/`
- **파일명 기준**: 디렉토리명은 소스 파일명(확장자 제외). 모듈명이 아님.
- **per-module Makefile**: 각 모듈 디렉토리에 자동 생성된 Makefile로 `make test` 실행.
- **재귀 make test**: 어느 디렉토리에서든 `make test` → 그 하위 전체 테스트.
```

**new_string:**
```
- **원본 RTL 불변**: `rtl/` 디렉토리는 절대 수정하지 않는다.
- **출력 경로 규칙 (두 가지 모드)**:
  - **기본 CLI** (`hirct-gen input.v`): `output/<filename>/` — 입력 경로에 독립. 어디서든 동일한 출력 구조.
  - **순회 모드** (`make generate`, `config/generate.f` 기반): 소스 트리 미러링 적용 `rtl/<path>/<file>.v` → `output/<path>/<file>/`
- **파일명 기준**: 디렉토리명은 소스 파일명(확장자 제외). 모듈명이 아님.
- **per-module Makefile**: 각 모듈 디렉토리에 자동 생성된 Makefile로 `make test` 실행.
- **재귀 make test**: 어느 디렉토리에서든 `make test` → 그 하위 전체 테스트.
```

### Step 3: 검증 (1분)

**Run:**
```bash
grep -n "소스 트리 미러링" docs/proposal/001-hirct-automation-framework.md
grep -n "verify-decisions" docs/proposal/001-hirct-automation-framework.md
```

**Expect:**
- 미러링: "순회 모드" 컨텍스트 안에서만 언급
- verify-decisions: "test/(lit) 및 unittests/(gtest)" 문구 존재

### Step 4: 커밋

```bash
git add docs/proposal/001-hirct-automation-framework.md
git commit -m "docs(proposal): fix verify-decisions language, clarify output path modes (G2, G3)"
```

---

## Task 2: open-decisions에 combinational loop 결정 추가 (G4)

**Files:**
- Modify: `docs/plans/open-decisions.md` (A-8 추가 + 요약 테이블 건수 갱신)

### Step 1: A-7 뒤에 A-8 추가 (3분)

`docs/plans/open-decisions.md`에서 `## B. 네이밍 / 구조 정리 (6건)` 바로 앞에 다음을 삽입:

```markdown
### A-8. Combinational loop 처리 레벨 — RESOLVED (2026-02-16)

**결정**: **ERROR** (hard fail). WARN이 아니라 ERROR + meta.json emitter `"fail"`.

- 감지 시: `ERROR: combinational loop detected: %a -> %b -> ... -> %a`
- meta.json: `"combinational_loop": true`, 해당 emitter `"fail"`
- 모듈은 `known-limitations.md`에 `combinational_loop` 카테고리로 등록 가능

**근거**: WARN + 계속 진행은 미정렬 op이 잘못된 순서로 emit되어 "컴파일은 되지만 시뮬레이션 불일치"를 허용한다. 이는 verify 단계에서야 실패가 발견되어 원인 추적이 어렵다. ERROR로 즉시 표면화하여 `hirct-convention.md` §5 실패 분류 체계의 `fail` 정의와 일관성을 유지한다.

**반영 대상**: `hirct-convention.md` §2.10.1, `100-bootstrap.md` Step 4 Kahn's Algorithm
**검증 grep**: `grep -n "combinational.*loop.*ERROR\|combinational_loop.*true" docs/plans/hirct-convention.md docs/plans/phase-1-pipeline/100-bootstrap.md`

---
```

### Step 2: 요약 테이블 건수 갱신 (1분)

**old_string:**
```
| A. 아키텍처 / 설계 결정 | 7 | 전체 RESOLVED |
```
**new_string:**
```
| A. 아키텍처 / 설계 결정 | 8 | 전체 RESOLVED |
```

그리고:

**old_string:**
```
|| **합계** | **25** | **전체 RESOLVED** |
```
**new_string:**
```
|| **합계** | **26** | **전체 RESOLVED** |
```

### Step 3: 검증 + 커밋

**Run:** `grep -c "RESOLVED" docs/plans/open-decisions.md && grep "A-8" docs/plans/open-decisions.md`
**Expect:** RESOLVED 26+, A-8 존재

```bash
git add docs/plans/open-decisions.md
git commit -m "docs(open-decisions): add A-8 combinational loop ERROR policy (G4)"
```

---

## Task 3: hirct-convention 통일 (G4 확인 + G6 meta.json + XFAIL-lit)

**Files:**
- Modify: `docs/plans/hirct-convention.md` (meta.json 확장 키 + XFAIL-lit 섹션)

### Step 1: convention loop 정책이 이미 ERROR인지 확인 (1분)

**Run:** `grep -n "ERROR.*combinational\|WARN.*combinational" docs/plans/hirct-convention.md`
**Expect:** §2.10.1은 이미 ERROR. (convention이 권위 문서이므로 수정 불필요)

### Step 2: meta.json §4.5 뒤에 §4.6 확장 선택 키 추가 (3분)

`## 5. 실패 분류 체계` 바로 앞에 삽입:

```markdown
### 4.6 meta.json 확장 선택 키 (Agent Triage 지원)

Agent 자동 분류(Phase 2 `206-agent-triage.md`)를 지원하기 위한 선택 키:

| 키 | 타입 | 설명 | 생성 조건 |
|----|------|------|----------|
| `unsupported_ops` | string[] | 미지원 op 목록 | GenModel fail 시 |
| `combinational_loop` | bool | 조합 루프 감지 여부 | Kahn 정렬 실패 시 |
| `elapsed_ms` | int | 총 처리 시간 (ms) | 항상 |
| `timing` | object | 단계별 소요 시간 (`parse_ms`, `analyze_ms`, `emit_ms`) | `--verbose` 또는 항상 |
| `tool_versions` | object | `{"circt": "5e760efa9", "verilator": "5.020"}` | 항상 |
| `stderr_tail` | string | 실패 시 stderr 마지막 5줄 | fail 시 |

**reason 필드 표준 접두사** (규칙 기반 triage 정확도를 위해):

| 접두사 | 예시 | 분류 매핑 |
|--------|------|----------|
| `unsupported op:` | `"unsupported op: seq.firmem"` | `unsupported_op` |
| `timeout:` | `"timeout: circt-verilog (60s)"` | `timeout` |
| `parse error:` | `"parse error: unknown module 'foo'"` | `parse_error` |
| `combinational loop:` | `"combinational loop: %a -> %b -> %a"` | `combinational_loop` |
| `inout port:` | `"inout ports not supported"` | `inout_port` |
| `multi clock:` | `"multiple clock domains (2)"` | `multi_clock` |
| `wide signal:` | `"65+ bit signal: %data (128 bits)"` | `wide_signal` |
```

### Step 3: §5 뒤에 §5.1 XFAIL-lit 연동 추가 (3분)

`## 6. 마이크로 스텝 템플릿` 바로 앞에 삽입:

```markdown
### 5.1 lit XFAIL 연동 메커니즘

`known-limitations.md`의 Markdown 테이블을 lit에서 참조하여 XFAIL 판정하는 방법:

**파싱 로직** (`utils/parse_known_limitations.py`):
1. `known-limitations.md`의 `| Path | Category | Reason | Date |` 테이블을 파싱
2. `Path` 열을 set으로 수집 → `xfail_paths`
3. `Category` 열로 분류 가능 (unsupported_op, multi_module, parse_error, timeout 등)

**lit.cfg.py 연동**:
1. `lit.cfg.py`가 시작 시 `parse_known_limitations.load_xfail_paths()` 호출
2. 각 테스트의 입력 파일 경로(`%s`)가 `xfail_paths`에 포함되면 `xfail=True` 표기
3. XFAIL 모듈이 통과(XPASS)하면 WARN 출력 (CI Green 유지, 사람이 XFAIL 항목 재검토)

**구현 위치**:
- `utils/parse_known_limitations.py` — Markdown 파서 (Phase 2 Task 205에서 구현)
- `test/lit.cfg.py` — 단위 테스트용 (Phase 0 Task 001에서 스켈레톤)
- `integration_test/lit.cfg.py` — 통합 테스트용
```

### Step 4: 검증 + 커밋

**Run:** `grep -n "4.6\|5.1\|unsupported_ops\|reason.*표준" docs/plans/hirct-convention.md`
**Expect:** 4건 모두 존재

```bash
git add docs/plans/hirct-convention.md
git commit -m "docs(convention): add meta.json triage keys, reason prefixes, XFAIL-lit mechanism (G4, G6)"
```

---

## Task 4: 100-bootstrap.md 정합 (G4 loop + gtest + pipe)

**Files:**
- Modify: `docs/plans/phase-1-pipeline/100-bootstrap.md`

### Step 1: Kahn's Algorithm loop WARN → ERROR (2분)

**old_string:**
```
Combinational Loop 처리:
- 감지 시: stderr에 "WARN: combinational loop detected: %a → %b → ... → %a"
- meta.json에 "combinational_loop": true 기록
- 미정렬 op은 원본 순서대로 결과 목록 끝에 추가 (빌드는 계속 진행)
- Phase 2에서 실제 발견 시 XFAIL 등록 가능
```
**new_string:**
```
Combinational Loop 처리:
- 감지 시: stderr에 "ERROR: combinational loop detected: %a → %b → ... → %a"
- meta.json에 "combinational_loop": true, 해당 emitter "fail" 기록
- 모듈 처리 중단 (hard fail — 미정렬 op의 잘못된 emit 방지)
- known-limitations.md에 combinational_loop 카테고리로 등록 가능
- (근거: open-decisions.md A-8, hirct-convention.md §2.10.1)
```

### Step 2: gtest FetchContent 추가 (2분)

Step 3 CMakeLists.txt 코드 블록 뒤에 다음을 추가:

```markdown
**gtest 통합** (unittests/ 빌드에 필요):

\`\`\`cmake
# CMakeLists.txt에 추가
include(FetchContent)
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0)
FetchContent_MakeAvailable(googletest)
enable_testing()
add_subdirectory(unittests)
\`\`\`

> **Note**: `FetchContent`는 CMake 3.14+에서 사용 가능. 프로젝트 최소 요구 버전 3.20이므로 충분.
```

### Step 3: CirctRunner pipe 방식 결정 추가 (2분)

Step 3.5 구현 고려사항 뒤에 추가:

```markdown
**결정: 임시 파일 방식 채택**

대용량 MLIR 출력(수 MB)에서 pipe 버퍼(64KB) deadlock을 방지하기 위해 stdout을 임시 파일로 리다이렉트:

1. `mkstemp()`로 임시 파일 생성
2. `fork()` + `exec()` 시 stdout fd를 임시 파일 fd로 `dup2()`
3. 자식 프로세스 완료(`waitpid`) 후 임시 파일 전체를 `std::string`으로 읽기
4. 임시 파일 삭제 (`unlink`)
5. stderr는 별도 pipe로 읽기 (stderr는 진단만 포함하여 64KB 미만)

비동기 읽기(`select`/`poll`)보다 구현이 단순하고, MLIR 출력 크기에 제한이 없다.
```

### Step 4: 검증 + 커밋

**Run:** `grep -n "ERROR.*combinational\|FetchContent\|임시 파일 방식" docs/plans/phase-1-pipeline/100-bootstrap.md`
**Expect:** 3건 모두 존재, WARN 0건

```bash
git add docs/plans/phase-1-pipeline/100-bootstrap.md
git commit -m "docs(bootstrap): unify loop to ERROR, add gtest FetchContent, decide pipe strategy"
```

---

## Task 5: 101-gen-model.md op 목록 정합 (G5)

**Files:**
- Modify: `docs/plans/phase-1-pipeline/101-gen-model.md:16-17`

### Step 1: 상단 op 목록 교체 (2분)

**old_string:**
```
- 지원 대상 operation: comb.and/or/xor/mux/icmp/extract/concat/shl/shru, seq.firreg, hw.array_get
- 미지원: hw.instance (계층) — CIRCT flatten pass로 해소, 그 외 발견되는 op
```
**new_string:**
```
- **Phase 1A 지원 (24개 op)**: comb.and/or/xor/mux/icmp/extract/concat/shl/shru/add/sub/mul/shrs/parity/replicate, seq.firreg/compreg/to_clock(무시), hw.constant/output/array_get/array_create/array_inject/aggregate_constant/bitcast (실측 근거: `risk-validation-results.md` §2)
- **Phase 1A 미지원 → Error + XFAIL**: seq.firmem/firmem.read_port, llhd.sig/prb/process
- **hw.instance**: CIRCT flatten pass로 해소, 실패 시 Error + 진단 메시지 (open-decisions A-3)
```

### Step 2: 검증 + 커밋

**Run:** `grep "Phase 1A 지원" docs/plans/phase-1-pipeline/101-gen-model.md`
**Expect:** "24개 op" 문구 존재

```bash
git add docs/plans/phase-1-pipeline/101-gen-model.md
git commit -m "docs(gen-model): align op list header with mapping table and risk-validation (G5)"
```

---

## Task 6: summary.md 전략 추가 (W1 + CP2 + Phase 0)

**Files:**
- Modify: `docs/plans/summary.md`

### Step 1: CP2 시점 표현 통일 (1분)

**old_string:**
```
| CP2: Core Pipeline 관통 | Task 109 후 | verify PASS + lit PASS + gtest PASS | C++ 코드 품질 + 아키텍처 리뷰 |
```
**new_string:**
```
| CP2: Core Pipeline 관통 | Batch 2+3 완료 후 (Task 101+110+111+109) | verify PASS + lit PASS + gtest PASS | C++ 코드 품질 + 아키텍처 리뷰 |
```

### Step 2: Phase 1B/2/3 전략 섹션 추가 (5분)

`**Phase 1B는 Phase 2 초반과 병행 가능**` 줄 뒤에 §4.2, §4.3, §4.4 삽입.

내용은 summary.md에 3개 서브섹션으로 삽입하며, 각각 Phase 1B(순차 실행 + 자동 리뷰), Phase 2(rule-based triage + CP-2A/2B/2C), Phase 3(순차 실행 + CP-3A/3B)의 배치 구성과 체크포인트 테이블을 포함한다.

(전체 내용은 기존 계획의 Task 6 Step 2 참조 — 여기서는 삽입 위치만 특정)

### Step 3: Phase 0에 브랜치 생성 추가 (1분)

Phase 0 트리에 `└─ 브랜치 생성 (detached HEAD 해소)` 추가.

### Step 4: 검증 + 커밋

**Run:** `grep -n "4.2 Phase 1B\|4.3 Phase 2\|4.4 Phase 3\|CP-2A\|CP-3A\|Batch 2+3" docs/plans/summary.md`
**Expect:** 모든 항목 존재

```bash
git add docs/plans/summary.md
git commit -m "docs(summary): add Phase 1B/2/3 agent strategies, fix CP2, add Phase 0 branch (W1)"
```

---

## Task 7: phase-1-pipeline/README.md (W3 Batch 2 + 1B 전략)

**Files:**
- Modify: `docs/plans/phase-1-pipeline/README.md`

### Step 1: Batch 2 순서 명확화 (2분)

**old_string:**
```
**배치**: Task 110+111+101 → Task 102+106+109 순차 실행.
```
**new_string:**
```
**배치**: Task 110 → 111 → 101 순차 실행 (서브에이전트 1개씩. 110+111은 출력 구조/CLI이므로 순서 의존성이 있어 동시가 아닌 순차), 이후 Task 102 → 106 → 109 순차 실행.
```

### Step 2: Phase 1B Agent-in-the-Loop 섹션 추가 (3분)

`**이 체크포인트를 통과해야 Phase 1B 및 Phase 2를 시작할 수 있다.**` 뒤에 Phase 1B 실행 전략 섹션 삽입 (실행 순서, 리뷰 절차, 완료 게이트).

### Step 3: 검증 + 커밋

```bash
git add docs/plans/phase-1-pipeline/README.md
git commit -m "docs(phase-1): clarify Batch 2 sequential order, add Phase 1B agent strategy (W3)"
```

---

## Task 8: reference-commands-and-structure.md 확장

**Files:**
- Modify: `docs/plans/reference-commands-and-structure.md`

### Step 1: CIRCT_VERSION을 디렉토리 트리에 추가 (1분)

`.gitignore` 줄 뒤에 `├── CIRCT_VERSION` 추가.

### Step 2: make triage 타겟 추가 (1분)

Makefile 타겟 테이블 `make report` 행 뒤에 `make triage` 행 추가.

### Step 3: meta.json §8에 확장 선택 키 추가 (2분)

convention §4.6와 동일한 확장 키 테이블을 §8에 추가.

### Step 4: §13 lit.cfg.py 템플릿 추가 (3분)

test/lit.cfg.py 및 integration_test/lit.cfg.py 템플릿 코드 포함.

### Step 5: 검증 + 커밋

```bash
git add docs/plans/reference-commands-and-structure.md
git commit -m "docs(reference): add CIRCT_VERSION, make triage, meta triage keys, lit.cfg.py templates"
```

---

## Task 9: Phase 2/3 README + 204 Agent 정의

**Files:**
- Modify: `docs/plans/phase-2-testing/README.md` (Task 206 + CP)
- Modify: `docs/plans/phase-2-testing/204-failure-analysis.md` (Agent 정의)
- Modify: `docs/plans/phase-3-release/README.md` (CP)

### Step 1: Phase 2 README에 Task 206 + 체크포인트 추가 (3분)

### Step 2: 204에 Agent 정의 블록 추가 (2분)

### Step 3: Phase 3 README에 체크포인트 추가 (2분)

### Step 4: 검증 + 커밋

```bash
git add docs/plans/phase-2-testing/README.md docs/plans/phase-2-testing/204-failure-analysis.md docs/plans/phase-3-release/README.md
git commit -m "docs(phase-2,3): add agent checkpoints, Task 206, Agent definition (W1, G1)"
```

---

## Task 10: 206-agent-triage.md 신규 작성

**Files:**
- Create: `docs/plans/phase-2-testing/206-agent-triage.md`

### Step 1: 전체 파일 작성 (10분)

포함 내용:
- Agent 역할 + 데이터 플로우 (mermaid)
- 분류 규칙 테이블 (7개 카테고리)
- 자동 PR 권한 (허용/조건부/금지)
- 증거 첨부 필수 항목
- utils/triage-failures.py 입출력 사양
- make triage Makefile 타겟
- 게이트 체크리스트

### Step 2: 검증 + 커밋

```bash
git add docs/plans/phase-2-testing/206-agent-triage.md
git commit -m "docs(phase-2): add 206-agent-triage spec (G1)"
```

---

## Task 11: risk-validation-results.md TODO 스텁

**Files:**
- Modify: `docs/plans/risk-validation-results.md`

### Step 1: §4 flatten + §5 filelist 스텁 추가 (3분)

`## 변경 이력` 앞에 TODO 섹션 2개 삽입. Phase 0 Pre-test에서 실측 후 결과 기입.

### Step 2: 커밋

```bash
git add docs/plans/risk-validation-results.md
git commit -m "docs(risk-validation): add flatten and filelist validation TODO stubs"
```

---

## Task 12: 교차 참조 최종 검증

**Files:** 없음 (읽기 전용)

### Step 1: combinational loop 일관성

**Run:** `rg -n "combinational.*loop" docs/ --glob "*.md" | grep -i "warn\|error"`
**Expect:** 전부 ERROR, WARN 0건

### Step 2: verify-decisions 일관성

**Run:** `rg -n "verify-decisions" docs/ --glob "*.md"`
**Expect:** "test/(lit)" 문구만, "자동 검증 스크립트" 0건

### Step 3: 출력 경로 일관성

**Run:** `rg -n "소스 트리 미러링" docs/ --glob "*.md"`
**Expect:** "순회 모드" 또는 "make generate" 컨텍스트에서만 언급

### Step 4: Phase별 CP 완전성

**Run:** `rg -n "CP-?[123]" docs/plans/ --glob "*.md" | sort`
**Expect:** Phase 1(CP1,CP2,CP3), Phase 2(CP-2A,2B,2C), Phase 3(CP-3A,3B) 모두 존재

### Step 5: Agent 권한 SSOT

**Run:** `rg -n "절대 금지.*rtl\|자동 PR 권한\|Agent 정의" docs/ --glob "*.md"`
**Expect:** 206이 SSOT, 204는 206 참조

### Step 6: meta.json 스키마 정합

**Run:** `rg -n "unsupported_ops" docs/plans/ --glob "*.md"`
**Expect:** convention §4.6 + reference §8 + 206에 존재

---

## 해소된 갭 매핑

| 갭 ID | 출처 | 해소 Task | 편집 파일 |
|--------|------|----------|----------|
| GPT-G1 | Agent 정의 부재 | Task 9, 10 | 204, 206(신규) |
| GPT-G2 | verify-decisions | Task 1 | proposal |
| GPT-G3 | 출력 경로 | Task 1 | proposal |
| GPT-G4 | loop WARN/ERROR | Task 2, 3, 4 | open-decisions, convention, bootstrap |
| GPT-G5 | op 목록 | Task 5 | 101-gen-model |
| GPT-G6 | meta.json 확장 | Task 3, 8 | convention, reference |
| Claude-W1 | Phase 1B/2/3 전략 | Task 6, 7, 9 | summary, phase READMEs |
| Claude-W3 | Batch 2 순서 | Task 7 | phase-1 README |
| Claude-G1 | gtest | Task 4 | bootstrap |
| Claude-G2 | lit.cfg.py | Task 8 | reference |
| Claude-G3 | pipe deadlock | Task 4 | bootstrap |
| Claude-G4 | flatten 검증 | Task 11 | risk-validation |
| Claude-G6 | CIRCT_VERSION | Task 8 | reference |
| Claude-R3 | filelist 검증 | Task 11 | risk-validation |
| Claude-R4 | XFAIL-lit 연동 | Task 3 | convention |
| GPT-추가 | make triage 타겟 | Task 8 | reference |
| GPT-추가 | reason 표준 접두사 | Task 3 | convention |
| GPT-추가 | failure-classification | Task 10 | 206 |
