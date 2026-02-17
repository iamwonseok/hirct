# HIRCT Agent-in-the-Loop 실행 전 갭 해소 계획서

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Q1~Q4 결정 확정 + 7개 교차 참조 갭 해소 → subagent-driven-development로 Phase 0 즉시 착수 가능한 문서 세트 완성.

**Architecture:** 문서 편집 전용 계획. 기존 .md 파일의 특정 텍스트를 교체/추가한다. 코드 변경 없음.

**Tech Stack:** Markdown, grep (검증)

**선행 조건:** `2026-02-16-agent-in-the-loop-doc-unification.md`의 12개 Task 반영 완료 상태.

---

## 결정 요약 (Q1~Q4)

### Q1: output/** lint 정책 — **Phase별 분리 (B→A 단계 전환)**

**결정:** Phase 1은 Policy B(output/** lint 제외), Phase 2 후반에 Policy A(output/** lint 포함)로 전환.

**근거:**
- Phase 1은 모든 C++을 신규 작성하는 greenfield 단계. 생성 코드의 lint 패턴이 아직 확립되지 않음.
- open-decisions C-1의 "전환 조건"(반복 실패, 생산성 저해)이 Phase 1에서 거의 확실히 발생할 것 — C-1의 폴백 조항을 Phase 시작부터 적용하는 것이 현실적.
- Phase 2 후반에 전체 순회 후 lint 패턴이 안정화되면 Policy A 전환.
- emitter별 산출물 테스트(verilator --lint-only, g++ -c 등)는 Phase 1부터 실행 — lint의 부재가 품질 공백은 아님.

**영향 파일:** open-decisions.md C-1, hirct-convention.md §4.4 이후, reference-commands.md

**Agent Gate 해석:** Phase 1의 `make lint`는 `src/**` + `tools/**`만 대상. Phase 2 후반부터 `output/**` 포함.

---

### Q2: meta.json MLIR 실패 사유 — **최상위 `reason` 필드 공식화**

**결정:** meta.json에 최상위 `reason` 필드를 공식 스키마로 승격.

```json
{
  "path": "rtl/.../foo.v",
  "top": "FooModule",
  "mlir": "fail",
  "reason": "parse error: unknown module 'Bar'",
  "emitters": {}
}
```

**규칙:**
- `mlir: "pass"` → `reason` 생략 또는 빈 문자열
- `mlir: "fail"` → `reason` 필수. reason 접두사 표준(convention §4.6)을 동일 적용:

| 접두사 | 예시 | triage 분류 |
|--------|------|------------|
| `parse error:` | `"parse error: unknown module 'Bar'"` | `parse_error` |
| `timeout:` | `"timeout: circt-verilog (60s)"` | `timeout` |
| `multiple modules:` | `"multiple modules: 3 found, --top required"` | `multi_module` |
| `flatten error:` | `"flatten error: hw-flatten-modules failed"` | `flatten_error` |

**근거:**
- 이미 reference-commands §10, convention §4.3.2(timeout)에서 암묵적으로 `"mlir": "fail"` + reason 패턴 사용 중 — 공식화만 하면 됨.
- emitter-level reason과 MLIR-level reason을 구분하므로 triage 자동 분류의 1차 분기가 `mlir` 필드 하나로 가능.
- `stages` 구조 도입은 과설계 — MLIR 단계와 emitter 단계 2분법이면 충분.

**영향 파일:** hirct-convention.md §4.5, reference-commands.md §8, 206-agent-triage.md §2

---

### Q3: triage 결과 파일 — **`output/triage-report.json` 공식 산출물로 추가**

**결정:** `utils/triage-failures.py`가 stdout(사람 읽기용) + `output/triage-report.json`(기계 읽기용) 모두 생성.

```json
{
  "generated_at": "2026-03-01T10:00:00Z",
  "total_failures": 150,
  "categories": {
    "parse_error": { "count": 80, "top_reasons": ["unknown module (65)", "syntax error (15)"] },
    "unsupported_op": { "count": 30, "top_reasons": ["seq.firmem (25)", "llhd.sig (5)"] },
    "verify_mismatch": { "count": 5, "modules": ["ModuleA", "ModuleB"] }
  },
  "known_limitations_candidates": [
    { "path": "rtl/.../foo.v", "category": "parse_error", "reason": "unknown module 'Bar'" }
  ]
}
```

**근거:**
- CP-2C 리뷰 시 "이전 triage와 비교"가 가능해짐 (반복 순회 → 개선 추적).
- mkdocs 인덱스나 다른 도구에서 참조 가능한 기계 판독 형식 필요.
- stdout 출력은 사람 요약용으로 유지 — 파일은 상세 데이터 보존용.

**영향 파일:** 206-agent-triage.md §5, reference-commands.md §3.1(make 타겟), reference-commands.md §4.2(전역 산출물)

---

### Q4: multi-file `--timescale` — **기본 강제, 사용자 오버라이드 가능**

**결정:** multi-file 모드(`-f filelist.f`)에서 `--timescale`을 **항상 전달**. 기본값 `1ns/1ps`, CLI 옵션으로 오버라이드 가능.

```bash
# 기본 (1ns/1ps 자동 적용)
hirct-gen -f filelist.f --top CoreIP

# 사용자 오버라이드
hirct-gen -f filelist.f --top CoreIP --timescale 1ps/1ps
```

**CirctRunner 내부 변환:**
```
hirct-gen -f filelist.f --top CoreIP
  ↓ filelist 파싱 (hirct-gen 자체)
  ↓ circt-verilog --timescale=1ns/1ps --top=CoreIP file1.v file2.v file3.v
```

**근거:**
- risk-validation §5: multi-file에서 timescale 미지정 시 "timescale 정의 혼재" 에러 발생 확인.
- `--timescale`은 circt-verilog 레벨 가드레일 — hirct-gen이 무조건 전달해야 안전.
- 단일 파일 모드에서는 불필요 (파일 자체의 timescale 선언 사용).
- 기본값 `1ns/1ps`는 업계 가장 일반적인 설정.

**영향 파일:** hirct-convention.md §0(또는 신규 §0.1), 111-cli.md, 100-bootstrap.md CirctRunner 인터페이스, reference-commands.md §2.1

---

## 추가 갭 해소 (GAP-A ~ GAP-G)

### GAP-A: Phase 1 README C++ 소스 매핑 CirctRunner 누락

**편집:** `phase-1-pipeline/README.md` C++ 소스 매핑 테이블에 CirctRunner 행 추가 + `lib/Support/` 디렉토리 인지.

### GAP-B: `utils/` 디렉토리 트리 불완전

**편집:** `reference-commands.md` §1 디렉토리 트리 + 부록 A 테이블에 `triage-failures.py`, `parse_known_limitations.py` 추가.

### GAP-C: `circt-verilog -f` 미지원 → CLI/CirctRunner 설계 반영

**편집:** `111-cli.md`에 filelist→multi-file 변환 로직 섹션 추가, `100-bootstrap.md` CirctRunner에 `runCirctVerilogMulti()` API 추가.

### GAP-D: conventions.md / 003-convention-check.md 리다이렉트 삭제

**편집:** 두 파일 삭제 (SSOT 원칙).

### GAP-E: Phase 0 `make build` 의미 명확화

**편집:** `reference-commands.md` §1.1 전제 조건 테이블과 §3.1 make 타겟 테이블에서 Phase 0 `make build`의 동작을 "CMakeLists.txt 부재 시 안내 메시지 출력"으로 명시.

### GAP-F: Git 브랜치 전략 최소 정의

**편집:** `001-setup-env.md` 0단계에 브랜치 이름 규칙 추가 + summary.md Phase 0 트리에 연동.

### GAP-G: `seq.compreg` 지원 범위 명시

**편집:** `101-gen-model.md` op 매핑 테이블에서 `seq.compreg` 처리를 `seq.firreg`와 동등하게 명시.

---

## Task 1: open-decisions C-1 경정 (Q1)

**Files:** `docs/plans/open-decisions.md`

### Step 1: C-1 결정 텍스트 교체

**old_string:**
```
**결정**: **1차 정책은 A(생성 코드도 소스/tools와 동일 규칙으로 lint 포함)** + **AUTO-GENERATED 헤더 필수**.

단, 아래 "전환 조건"을 만족할 정도로 개발 생산성을 저해하면 2차 정책(B)을 도입할 수 있다.
```

**new_string:**
```
**결정**: **Phase별 단계 전환** + **AUTO-GENERATED 헤더 필수**.

- **Phase 1**: Policy B — `output/**`를 `make lint`에서 제외. emitter 개발 생산성 우선.
- **Phase 2 후반**: Policy A로 전환 — `output/**`를 lint에 포함. 실패 시 생성기 수정.
- **전환 후 반복 저해 시**: `output/**`에 한해 완화 규칙 도입(허용 예외 목록 고정).

> **경정 사유 (2026-02-17)**: 원래 "A 먼저, 필요 시 B 폴백"이었으나, Phase 1이 전체 C++을 신규 작성하는 greenfield 단계여서 생성 코드 lint 패턴이 확립되지 않은 상태에서 Policy A는 전환 조건을 거의 확실히 충족한다. 003-coding-convention.md의 실행 계획과 일치시켰다.
```

### Step 2: 검증

```bash
rg -n "Phase별 단계 전환|Policy B.*Phase 1|Policy A.*Phase 2" docs/plans/open-decisions.md
```

**Expect:** C-1 섹션에서 3건 모두 존재

### Step 3: 커밋

```bash
git add docs/plans/open-decisions.md
git commit -m "docs(open-decisions): amend C-1 lint policy to phased B→A transition (Q1)"
```

---

## Task 2: meta.json 최상위 `reason` 공식화 (Q2)

**Files:**
- `docs/plans/hirct-convention.md` (§4.5 meta.json 최소 스키마)
- `docs/plans/reference-commands-and-structure.md` (§8 meta.json 스키마)
- `docs/plans/phase-2-testing/206-agent-triage.md` (§2 분류 규칙)

### Step 1: convention §4.5 최소 스키마에 `reason` 추가

`docs/plans/hirct-convention.md`에서 §4.5 최소 스키마 JSON 예시 뒤의 필드 설명 테이블을 교체:

**old_string:**
```
**필드 설명**:

| 필드 | 설명 |
|------|------|
| `path` | 입력 RTL 파일의 상대 경로 |
| `top` | 최상위 모듈 이름 |
| `mlir` | CIRCT MLIR 변환 결과 (`pass` / `fail`) |
| `emitters.<name>.result` | 각 emitter 결과 (`pass` / `fail` / `skipped`) |
| `emitters.<name>.reason` | 실패 또는 스킵 사유 (빈 문자열 = 성공) |
```

**new_string:**
```
**필드 설명**:

| 필드 | 설명 |
|------|------|
| `path` | 입력 RTL 파일의 상대 경로 |
| `top` | 최상위 모듈 이름 |
| `mlir` | CIRCT MLIR 변환 결과 (`pass` / `fail`) |
| `reason` | MLIR 변환 실패 사유 (`mlir: "fail"` 시 필수, `pass` 시 생략). §4.6 reason 접두사 표준 적용. |
| `emitters.<name>.result` | 각 emitter 결과 (`pass` / `fail` / `skipped`) |
| `emitters.<name>.reason` | 실패 또는 스킵 사유 (빈 문자열 = 성공) |

**`reason` (최상위) vs `emitters.<name>.reason` 구분**:
- 최상위 `reason`: MLIR 변환 단계 실패 사유 (파싱/타임아웃/다중모듈 등). `mlir: "fail"` 시에만 존재.
- `emitters.<name>.reason`: 개별 emitter 실패 사유. MLIR은 성공했지만 특정 emitter가 실패한 경우.
- triage 1차 분기: `mlir` 필드 → `"fail"`: 최상위 `reason` 확인 / `"pass"`: `emitters` 순회.
```

### Step 2: convention §4.5 최소 스키마 JSON에 `reason` 필드 추가

**old_string:**
```json
{
  "path": "rtl/.../foo.v",
  "top": "ModuleName",
  "mlir": "pass | fail",
  "emitters": {
```

**new_string:**
```json
{
  "path": "rtl/.../foo.v",
  "top": "ModuleName",
  "mlir": "pass | fail",
  "reason": "",
  "emitters": {
```

### Step 3: reference-commands §8 필수 키 테이블에 `reason` 추가

`docs/plans/reference-commands-and-structure.md`에서 §8 필수 키 테이블:

**old_string:**
```
| `mlir` | string | MLIR 변환 결과 (`"pass"` \| `"fail"`) |
| `emitters.<name>.result` | string | emitter 실행 결과 (`"pass"` \| `"fail"` \| `"skipped"`) |
```

**new_string:**
```
| `mlir` | string | MLIR 변환 결과 (`"pass"` \| `"fail"`) |
| `reason` | string | MLIR 실패 사유 (`mlir: "fail"` 시 필수, `"pass"` 시 생략). reason 접두사 표준 적용 (`parse error:`, `timeout:`, `multiple modules:`, `flatten error:`) |
| `emitters.<name>.result` | string | emitter 실행 결과 (`"pass"` \| `"fail"` \| `"skipped"`) |
```

### Step 4: reference-commands §8 JSON 예시에 `reason` 추가

**old_string:**
```json
  "top": "LevelGateway",
  "mlir": "pass",
  "emitters": {
```

**new_string:**
```json
  "top": "LevelGateway",
  "mlir": "pass",
  "reason": "",
  "emitters": {
```

### Step 5: 206-agent-triage §2 분류 규칙에서 `reason` 참조 위치 명확화

**old_string:**
```
| `parse_error` | `mlir="fail"` 또는 reason starts with `"parse error:"` | `known-limitations.md`에 등록 |
```

**new_string:**
```
| `parse_error` | `mlir="fail"` + 최상위 `reason` starts with `"parse error:"` | `known-limitations.md`에 등록 |
```

### Step 6: 검증 + 커밋

```bash
rg -n '"reason".*MLIR\|최상위.*reason\|reason.*필수' docs/plans/hirct-convention.md docs/plans/reference-commands-and-structure.md
```

```bash
git add docs/plans/hirct-convention.md docs/plans/reference-commands-and-structure.md docs/plans/phase-2-testing/206-agent-triage.md
git commit -m "docs: formalize top-level reason field in meta.json schema (Q2)"
```

---

## Task 3: triage 결과 파일 공식화 (Q3)

**Files:**
- `docs/plans/phase-2-testing/206-agent-triage.md` (§5 출력)
- `docs/plans/reference-commands-and-structure.md` (§3.1 make 타겟, §4.2 전역 산출물)

### Step 1: 206 §5 출력에 `output/triage-report.json` 추가

**old_string:**
```
### 출력

| 출력 | 형식 | 설명 |
|------|------|------|
| 분류 요약 | stdout (텍스트) | 카테고리별 개수, 대표 모듈 |
| known-limitations 후보 | stdout (TSV) | `Path\tCategory\tReason\tDate` 형식 |
| PR 대상 파일 목록 | stdout (JSON) | `{"files": [...], "reason": "..."}` |
```

**new_string:**
```
### 출력

| 출력 | 형식 | 경로 | 설명 |
|------|------|------|------|
| 분류 요약 | stdout (텍스트) | — | 카테고리별 개수, 대표 모듈 (사람 읽기용) |
| triage 리포트 | JSON | `output/triage-report.json` | 기계 판독 상세 분류 (CP-2C 추적용) |
| known-limitations 후보 | stdout (TSV) | — | `Path\tCategory\tReason\tDate` 형식 |
| PR 대상 파일 목록 | stdout (JSON) | — | `{"files": [...], "reason": "..."}` |

**`output/triage-report.json` 스키마**:

```json
{
  "generated_at": "2026-03-01T10:00:00Z",
  "source": {
    "report": "output/report.json",
    "verify_report": "output/verify-report.json"
  },
  "total_failures": 150,
  "categories": {
    "parse_error": { "count": 80, "top_reasons": ["unknown module (65)", "syntax error (15)"] },
    "unsupported_op": { "count": 30, "top_reasons": ["seq.firmem (25)", "llhd.sig (5)"] },
    "verify_mismatch": { "count": 5, "modules": ["ModuleA", "ModuleB"] },
    "timeout": { "count": 10 },
    "combinational_loop": { "count": 3 },
    "inout_port": { "count": 12 },
    "multi_clock": { "count": 8 },
    "wide_signal": { "count": 2 }
  },
  "known_limitations_candidates": [
    { "path": "rtl/.../foo.v", "category": "parse_error", "reason": "unknown module 'Bar'" }
  ],
  "phase1_feedback": [
    { "module": "ModuleA", "category": "verify_mismatch", "target_task": "101-gen-model", "seed": 42, "cycle": 347 }
  ]
}
```
```

### Step 2: reference-commands §3.1 make triage 행 출력 열 갱신

**old_string:**
```
| `make triage` | `python3 utils/triage-failures.py` | `output/report.json`, `output/**/meta.json` | triage 분류 리포트 | non-zero | 2+ |
```

**new_string:**
```
| `make triage` | `python3 utils/triage-failures.py` | `output/report.json`, `output/**/meta.json` | `output/triage-report.json` + stdout 요약 | non-zero | 2+ |
```

### Step 3: reference-commands §4.2 전역 산출물에 triage-report 추가

`known-limitations.md` 행 뒤에 삽입:

```markdown
| `triage-report.json` | `output/triage-report.json` | 2 | 자동 실패 분류 리포트 (make triage) |
```

### Step 4: 206 §5 CLI에 `--output` 옵션 추가

**old_string:**
```
python3 utils/triage-failures.py \
    --meta-dir output/ \
    --report output/report.json \
    --verify-report output/verify-report.json
```

**new_string:**
```
python3 utils/triage-failures.py \
    --meta-dir output/ \
    --report output/report.json \
    --verify-report output/verify-report.json \
    --output output/triage-report.json
```

### Step 5: 검증 + 커밋

```bash
rg -n "triage-report.json" docs/plans/phase-2-testing/206-agent-triage.md docs/plans/reference-commands-and-structure.md
```

```bash
git add docs/plans/phase-2-testing/206-agent-triage.md docs/plans/reference-commands-and-structure.md
git commit -m "docs: add output/triage-report.json as official artifact (Q3)"
```

---

## Task 4: timescale 정책 + filelist 변환 로직 (Q4 + GAP-C)

**Files:**
- `docs/plans/hirct-convention.md` (§0 뒤에 §0.1 신규)
- `docs/plans/phase-1-pipeline/111-cli.md` (filelist 섹션)
- `docs/plans/phase-1-pipeline/100-bootstrap.md` (CirctRunner 인터페이스)
- `docs/plans/reference-commands-and-structure.md` (§2.1 CLI)

### Step 1: convention에 §0.1 multi-file 정책 추가

`docs/plans/hirct-convention.md`에서 `## 1. 검증 방법론` 바로 앞에 삽입:

```markdown
### 0.1 multi-file 모드 정책 (filelist → circt-verilog 변환)

**실측 사실**: `circt-verilog`는 `-f filelist.f` 옵션을 **지원하지 않는다** (CIRCT `5e760efa9` 기준).
대신 여러 입력 파일을 인자로 나열하는 multi-file 모드를 지원한다.

**변환 규칙**: hirct-gen이 filelist를 자체 파싱하여 circt-verilog 인자로 변환한다.

```
hirct-gen -f filelist.f --top CoreIP [--timescale 1ns/1ps]
  ↓ hirct-gen이 filelist.f 파싱 (주석 제거, +incdir+ 처리)
  ↓ CirctRunner::runCirctVerilogMulti(files, top, timescale)
  ↓ circt-verilog --timescale=1ns/1ps --top=CoreIP file1.v file2.v ...
```

**`--timescale` 정책**:

| 모드 | timescale 전달 | 기본값 |
|------|---------------|--------|
| 단일 파일 (`hirct-gen input.v`) | 전달하지 않음 (파일 자체 선언 사용) | — |
| multi-file (`hirct-gen -f ... --top ...`) | **항상 전달** | `1ns/1ps` |
| 사용자 오버라이드 | `--timescale <value>` | — |

**근거**: multi-file 입력에서 timescale 정의가 섞인 RTL을 다루면 `circt-verilog`가 "timescale 충돌" 에러를 발생시킨다. `--timescale`을 명시하면 모든 파일에 동일 기준을 강제하여 에러를 방지한다. (실측: `risk-validation-results.md` §5)
```

### Step 2: 111-cli.md에 filelist 변환 로직 섹션 추가

`docs/plans/phase-1-pipeline/111-cli.md`에서 `**filelist.f 형식**` 블록 뒤에 추가:

```markdown
**filelist → circt-verilog 변환** (hirct-gen 내부):

> `circt-verilog`는 `-f filelist.f`를 지원하지 않으므로, hirct-gen이 filelist를 자체 파싱하여 multi-file 인자로 변환한다. (실측: `risk-validation-results.md` §5)

1. filelist.f 파싱: 주석(`//`) 제거, `+incdir+<path>` 수집, 파일 경로 목록 추출
2. `--timescale` 기본값 주입: multi-file 모드에서 기본 `1ns/1ps` (사용자 오버라이드 가능)
3. CirctRunner 호출: `circt-verilog --timescale=<ts> --top=<top> file1.v file2.v ...`

**`--timescale` 옵션**:
```bash
hirct-gen -f filelist.f --top CoreIP                      # 기본 1ns/1ps
hirct-gen -f filelist.f --top CoreIP --timescale 1ps/1ps  # 오버라이드
```
```

### Step 3: 100-bootstrap.md CirctRunner에 multi-file API 추가

`docs/plans/phase-1-pipeline/100-bootstrap.md`에서 CirctRunner.h 인터페이스의 `runCirctOpt` 메서드 뒤에 추가:

**old_string:**
```cpp
    /// circt-opt 호출: MLIR → 변환된 MLIR
    RunResult runCirctOpt(const std::string& mlirContent,
                          const std::vector<std::string>& passes);
```

**new_string:**
```cpp
    /// circt-opt 호출: MLIR → 변환된 MLIR
    RunResult runCirctOpt(const std::string& mlirContent,
                          const std::vector<std::string>& passes);

    /// circt-verilog multi-file 호출: 여러 .v → MLIR
    /// circt-verilog는 -f 미지원 → hirct-gen이 filelist를 파싱하여 인자로 전달
    RunResult runCirctVerilogMulti(const std::vector<std::string>& inputPaths,
                                   const std::string& topModule,
                                   const std::string& timescale = "1ns/1ps");
```

### Step 4: reference-commands §2.1 hirct-gen CLI에 --timescale 추가

`docs/plans/reference-commands-and-structure.md`에서 §2.1 hirct-gen 테이블에 행 추가:

**old_string:**
```
| `hirct-gen --help` | 사용법 출력 | 1 |
```

**new_string:**
```
| `hirct-gen -f filelist.f --top Top --timescale 1ps/1ps` | timescale 오버라이드 | 1 |
| `hirct-gen --help` | 사용법 출력 | 1 |
```

### Step 5: 검증 + 커밋

```bash
rg -n "timescale\|runCirctVerilogMulti\|filelist.*파싱" docs/plans/hirct-convention.md docs/plans/phase-1-pipeline/111-cli.md docs/plans/phase-1-pipeline/100-bootstrap.md docs/plans/reference-commands-and-structure.md
```

```bash
git add docs/plans/hirct-convention.md docs/plans/phase-1-pipeline/111-cli.md docs/plans/phase-1-pipeline/100-bootstrap.md docs/plans/reference-commands-and-structure.md
git commit -m "docs: add timescale policy, filelist→multi-file conversion, CirctRunner multi API (Q4+GAP-C)"
```

---

## Task 5: Phase 1 README CirctRunner 누락 (GAP-A)

**Files:** `docs/plans/phase-1-pipeline/README.md`

### Step 1: C++ 소스 매핑 테이블에 CirctRunner + lib/Support/ 추가

**old_string:**
```
| ModuleAnalyzer | lib/Analysis/ModuleAnalyzer.cpp | Task 100 (Bootstrap) | **신규** |
| gen-model | lib/Target/GenModel.cpp | Task 100 (스켈레톤) + Task 101 (완성) | **신규** |
```

**new_string:**
```
| ModuleAnalyzer | lib/Analysis/ModuleAnalyzer.cpp | Task 100 (Bootstrap) | **신규** |
| CirctRunner | lib/Support/CirctRunner.cpp | Task 100 (Bootstrap) | **신규** |
| gen-model | lib/Target/GenModel.cpp | Task 100 (스켈레톤) + Task 101 (완성) | **신규** |
```

### Step 2: 검증 + 커밋

```bash
rg -n "CirctRunner\|lib/Support" docs/plans/phase-1-pipeline/README.md
```

```bash
git add docs/plans/phase-1-pipeline/README.md
git commit -m "docs(phase-1): add CirctRunner to C++ source mapping table (GAP-A)"
```

---

## Task 6: utils/ 디렉토리 트리 완성 (GAP-B)

**Files:** `docs/plans/reference-commands-and-structure.md`

### Step 1: §1 디렉토리 트리 utils/ 항목 확장

**old_string:**
```
├── utils/
│   ├── setup-env.sh                     # 유일 허용 .sh (멱등성 필수)
│   └── generate-report.py               # lit xunit XML + meta.json → JSON 리포트
```

**new_string:**
```
├── utils/
│   ├── setup-env.sh                     # 유일 허용 .sh (멱등성 필수)
│   ├── generate-report.py               # lit xunit XML + meta.json → JSON 리포트
│   ├── triage-failures.py               # 자동 실패 분류 (Phase 2 Task 206)
│   └── parse_known_limitations.py       # known-limitations.md 파서 (lit XFAIL 연동)
```

### Step 2: 부록 A 테이블에 두 파일 추가

`utils/generate-report.py` 행 뒤에:

```markdown
| `utils/triage-failures.py` | 자동 실패 분류 + triage 리포트 | Phase 2 Task 206 |
| `utils/parse_known_limitations.py` | known-limitations.md 파서 (lit XFAIL 연동) | Phase 2 Task 205 |
```

### Step 3: 검증 + 커밋

```bash
rg -n "triage-failures\|parse_known_limitations" docs/plans/reference-commands-and-structure.md
```

```bash
git add docs/plans/reference-commands-and-structure.md
git commit -m "docs(reference): add triage-failures.py and parse_known_limitations.py to utils/ (GAP-B)"
```

---

## Task 7: 리다이렉트 파일 삭제 (GAP-D)

**Files:**
- Delete: `docs/plans/conventions.md`
- Delete: `docs/plans/phase-0-setup/003-convention-check.md`

### Step 1: 삭제

```bash
git rm docs/plans/conventions.md docs/plans/phase-0-setup/003-convention-check.md
git commit -m "docs: remove redirect stubs (conventions.md, 003-convention-check.md) per SSOT (GAP-D)"
```

---

## Task 8: Phase 0 make build 의미 명확화 (GAP-E)

**Files:** `docs/plans/reference-commands-and-structure.md`

### Step 1: §1.1 전제 조건 테이블에 Phase 구분 추가

**old_string:**
```
| **hirct-gen/hirct-verify 바이너리** | `make build` (CMake + Ninja) | Phase 0 |
```

**new_string:**
```
| **hirct-gen/hirct-verify 바이너리** | `make build` (CMake + Ninja). **Phase 0**: C++ 소스 미존재이므로 안내 메시지만 출력. **Phase 1+**: Task 100 이후 실제 빌드 | Phase 0 (스켈레톤) → Phase 1+ (실제) |
```

### Step 2: §3.1 make build 행에 Phase 0 동작 명시

**old_string:**
```
| `make build` | `cmake -B build -G Ninja && ninja -C build` | CIRCT/LLVM 경로 | `build/` 바이너리 | non-zero | 0 |
```

**new_string:**
```
| `make build` | `cmake -B build -G Ninja && ninja -C build` | CIRCT/LLVM 경로 | `build/` 바이너리. Phase 0에서는 CMakeLists.txt 부재 → 안내 메시지 출력 (exit 0) | non-zero | 0 (스켈레톤) → 1+ (실제) |
```

### Step 3: 검증 + 커밋

```bash
rg -n "Phase 0.*스켈레톤\|안내 메시지.*출력" docs/plans/reference-commands-and-structure.md
```

```bash
git add docs/plans/reference-commands-and-structure.md
git commit -m "docs(reference): clarify Phase 0 make build behavior (GAP-E)"
```

---

## Task 9: Git 브랜치 전략 최소 정의 (GAP-F)

**Files:** `docs/plans/phase-0-setup/001-setup-env.md`

### Step 1: 0단계 브랜치 생성에 전략 추가

**old_string:**
```
### 0단계: 작업 브랜치 생성 (detached HEAD 해소)

> **현재 상태**: 이 worktree는 detached HEAD (`9cb42a1`) 상태다.
> 작업을 시작하기 전에 브랜치를 생성하고 checkout해야 한다.

```bash
git checkout -b feature/hirct-phase0
```
```

**new_string:**
```
### 0단계: 작업 브랜치 생성 (detached HEAD 해소)

> **현재 상태**: 이 worktree는 detached HEAD 상태다.
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
```

### Step 2: 검증 + 커밋

```bash
rg -n "브랜치 전략\|feature/hirct" docs/plans/phase-0-setup/001-setup-env.md
```

```bash
git add docs/plans/phase-0-setup/001-setup-env.md
git commit -m "docs(setup-env): add minimal git branch strategy (GAP-F)"
```

---

## Task 10: seq.compreg 지원 범위 명시 (GAP-G)

**Files:** `docs/plans/phase-1-pipeline/101-gen-model.md`

### Step 1: op 매핑 테이블에서 seq.compreg 동등 처리 확인

`101-gen-model.md`의 "Phase 1A 지원 (24개 op)" 줄에서 이미 `seq.firreg/compreg`로 포함되어 있음을 확인.
op 매핑 테이블 내 `seq.firreg` 행에 명시적 노트 추가:

**old_string (매핑 테이블 내):**

> (101-gen-model.md에서 `seq.firreg` 행을 찾아 `seq.compreg`와 동등 처리를 명시)

이 편집은 101의 실제 매핑 테이블을 읽어야 정확한 old_string을 특정할 수 있으므로, 실행 시점에 `rg -n "seq.firreg" docs/plans/phase-1-pipeline/101-gen-model.md`로 위치를 확인한 뒤 편집한다.

**추가할 내용**: `seq.compreg`는 `seq.firreg`와 동일하게 처리 (리셋 방식만 다름: firreg=async, compreg=sync). 실측(590파일)에서 `seq.compreg`는 미출현이나 지원은 완비.

### Step 2: 검증 + 커밋

```bash
rg -n "seq.compreg" docs/plans/phase-1-pipeline/101-gen-model.md
```

```bash
git add docs/plans/phase-1-pipeline/101-gen-model.md
git commit -m "docs(gen-model): explicitly document seq.compreg as equivalent to seq.firreg (GAP-G)"
```

---

## Task 11: 교차 참조 최종 검증

**Files:** 없음 (읽기 전용)

### Step 1: Q1 — lint 정책 일관성

```bash
rg -n "Policy [AB]" docs/ --glob "*.md"
```

**Expect:** open-decisions C-1과 003-coding-convention.md가 동일한 "B→A 단계 전환" 정책

### Step 2: Q2 — meta.json reason 일관성

```bash
rg -n '"reason".*MLIR\|최상위.*reason\|mlir.*reason' docs/plans/ --glob "*.md"
```

**Expect:** convention §4.5, reference §8에 최상위 reason 정의 존재

### Step 3: Q3 — triage-report.json 참조 완전성

```bash
rg -n "triage-report.json" docs/ --glob "*.md"
```

**Expect:** 206, reference(3.1 + 4.2) 에서 참조

### Step 4: Q4 — timescale 정책 참조 완전성

```bash
rg -n "timescale" docs/plans/ --glob "*.md" | grep -v "risk-validation\|002-tools"
```

**Expect:** convention §0.1, 111-cli, 100-bootstrap, reference §2.1에 존재

### Step 5: GAP 해소 확인

```bash
rg -n "CirctRunner" docs/plans/phase-1-pipeline/README.md
rg -n "triage-failures\|parse_known_limitations" docs/plans/reference-commands-and-structure.md
rg -n "Phase 0.*스켈레톤\|안내 메시지" docs/plans/reference-commands-and-structure.md
rg -n "브랜치 전략" docs/plans/phase-0-setup/001-setup-env.md
```

---

## 해소 매트릭스

| ID | 출처 | 해소 Task | 편집 파일 | 상태 |
|----|------|----------|----------|------|
| Q1 | lint 정책 불일치 | Task 1 | open-decisions.md | — |
| Q2 | meta.json reason 미정의 | Task 2 | convention, reference, 206 | — |
| Q3 | triage 결과 파일 미확정 | Task 3 | 206, reference | — |
| Q4 | timescale 정책 미확정 | Task 4 | convention, 111, 100, reference | — |
| GAP-A | CirctRunner 누락 | Task 5 | phase-1 README | — |
| GAP-B | utils/ 트리 불완전 | Task 6 | reference | — |
| GAP-C | filelist 변환 미반영 | Task 4 (병합) | convention, 111, 100 | — |
| GAP-D | 리다이렉트 잔존 | Task 7 | conventions, 003-check (삭제) | — |
| GAP-E | make build 의미 혼동 | Task 8 | reference | — |
| GAP-F | 브랜치 전략 미정의 | Task 9 | 001-setup-env | — |
| GAP-G | seq.compreg 범위 | Task 10 | 101-gen-model | — |

---

## 실행 후 상태 (Definition of Done)

이 계획서의 11개 Task가 모두 완료되면:

1. **Q1~Q4 결정이 문서에 반영** — Agent Gate의 의미가 흔들리지 않음.
2. **meta.json 스키마가 triage-ready** — `mlir` 단계와 `emitters` 단계 실패 사유가 구분됨.
3. **triage 산출물이 추적 가능** — `output/triage-report.json`으로 CP-2C 반복 비교 가능.
4. **multi-file 파이프라인이 문서화** — CirctRunner, CLI, convention에 timescale/filelist 변환 정책 일관.
5. **모든 교차 참조가 검증 통과** — Task 11의 grep 검증 전부 PASS.
6. **SSOT 중복 파일 제거** — conventions.md, 003-convention-check.md 삭제.
7. **Phase 0 즉시 착수 가능** — `subagent-driven-development` 스킬로 001-setup-env.md부터 실행.

---

## 변경 이력

| 날짜 | 내용 |
|------|------|
| 2026-02-17 | 신규 작성: Q1~Q4 결정 + GAP-A~G 해소 계획 |
