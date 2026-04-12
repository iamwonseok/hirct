# M0 Truthful Baseline Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** `v1-scope.md`를 현재 코드와 테스트가 실제로 뒷받침하는 범위만 말하는 truthful contract로 고정한다.

**Architecture:** 이 단계는 기능 추가가 아니라 baseline 고정 작업이다. 각 supported / unsupported / CLI claim을 하나씩 읽고, 대응 테스트와 코드 가드를 확인한 뒤, 근거 강도에 따라 문구를 낮추거나 후속 milestone로 이관한다.

**Tech Stack:** Markdown, `rg`, `hirct-gen`, lit runner (PATH `llvm-lit`, `python3 -m lit`, 또는 절대경로 `llvm-lit`), generated `lit.site.cfg.py` from a configured build tree, export tests, unit tests.

---

## Working Rules

- 이 계획은 `M0`만 다룬다.
- 구현 동작은 바꾸지 않는다.
- 한 task는 한 claim 또는 한 claim family만 다룬다.
- mismatch를 발견해도 즉시 구현하지 않는다.
- `v1-scope.md`를 수정해 M0를 닫는 경우, 기준일과 HEAD 식별자도 함께 갱신한다.
- 근거 등급은 아래 넷으로 분리한다.
  - `test-backed`: 문서 row를 직접 지탱하는 1차 근거.
  - `code-guard-backed`: 코드 가드는 있으나 dedicated lit가 없는 항목. 문서에 그 상태를 표시한다.
  - `code-path-backed`: 코드 경로는 확인되지만, 현재 lit가 그 경로를 직접 lock하지 않는 항목.
  - `indirect-legacy-backed`: 현재 export 경로가 아니라 legacy `--only model` 경로에서만 관찰되는 간접 근거. unsupported note에는 쓸 수 있지만, `export-cmodel` 직접 근거처럼 쓰지 않는다.
- transcript는 코드와 테스트만으로 결론을 못 낼 때만 보조 증거로 쓴다.

## Scope

**In scope**
- `hirct/hirct/docs/v1-scope.md`
- `hirct/hirct/known-limitations.md`의 `KL-20`
- `hirct/hirct/tools/hirct-gen/main.cpp`
- `hirct/hirct/lib/Target/CModelEmitter.cpp`
- `hirct/hirct/lib/Target/SystemCWrapperEmitter.cpp`
- export 관련 lit test와 wrapper unit test

**Out of scope**
- hierarchy-preserving export 구현
- wrapper capability 추가
- optimization level 설계
- legacy exporter 재설계

## Task 1: Verify Test Runner Preflight

**Files:**
- Read: local toolchain state

**Step 1: Locate a usable lit runner**

다음 순서로 usable lit runner를 탐색한다:

1. `llvm-lit --version` (PATH 기반)
2. `python3 -m lit --version` (PATH 기반 fallback)
3. generated `lit.site.cfg.py`에 기록된 `llvm_tools_dir` 아래 절대경로 `llvm-lit`

PATH 기반 runner가 없더라도 (3)이 존재하면 usable runner로 간주한다. "PATH에 `llvm-lit`이 없음"은 "verification-blocked"와 동의어가 아니다.

**Step 2: Locate generated lit harness**

Run: `rg -n "hirct_gen_path|hirct_obj_root|filecheck_path" <build>/test/lit.site.cfg.py`

Expected: generated `lit.site.cfg.py`가 존재하고 `%hirct-gen` substitution에 필요한 경로를 노출한다.

**Step 3: Verify configured build tree artifacts**

다음 모두 존재해야 usable harness로 판단한다:
- `<build>/test/lit.site.cfg.py`
- `<build>/bin/hirct-gen` (실행 가능)
- lit runner (Step 1에서 확보)

**Step 4: Record chosen runner and build tree**

Expected: 이후 verification step에서 사용할 (lit runner 절대경로, build-tree test root) 쌍을 명시적으로 기록한다.

**Judgment criteria for `verification-blocked`**: usable lit runner **와** generated harness **둘 다** 확보할 수 없을 때만 `verification-blocked`를 선언한다. PATH에 runner가 없더라도 절대경로 runner + generated harness가 있으면 focused verification은 가능하다.

## Task 2: Snapshot Baseline Documents

**Files:**
- Read: `hirct/hirct/docs/v1-scope.md`
- Read: `hirct/hirct/known-limitations.md`
- Read: `hirct/hirct/docs/plan/2026-04-12-v1-export-roadmap.md`

**Step 1: Read `v1-scope.md` structure**

Run: `rg -n "## v1 Supported|## v1 Unsupported|## CLI Contract|## 테스트-문서 연결 요약" hirct/hirct/docs/v1-scope.md`

Expected: supported, unsupported, CLI, and summary sections are present.

**Step 2: Read `known-limitations.md` KL-20**

Run: `rg -n "KL-20|hirct-gen v1 Exporter Scope" hirct/hirct/known-limitations.md`

Expected: the matching KL-20 section is locatable.

**Step 3: Read roadmap M0 boundary**

Run: `rg -n "## M0\\.|## M1\\." hirct/hirct/docs/plan/2026-04-12-v1-export-roadmap.md`

Expected: M0 intent is visible and bounded from M1.

## Task 3: Verify Input And Export-Target Claims

**Files:**
- Read: `hirct/hirct/docs/v1-scope.md`
- Read: `hirct/hirct/tools/hirct-gen/main.cpp`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`

**Step 1: Locate the input-format row**

Run: `rg -n "\\*\\*입력 형식\\*\\*|\\*\\*semantic scope\\*\\*|invalid --top rejection" hirct/hirct/docs/v1-scope.md`

Expected: the relevant rows are isolated.

**Step 2: Locate export-target selection logic**

Run: `rg -n "buildModuleModel|MissingTop|export_cmodel|export-cmodel|top module not found" hirct/hirct/tools/hirct-gen/main.cpp`

Expected: single selected target behavior and invalid-top rejection are visible.

**Step 3: Locate direct tests**

Run: `rg -n "CombOnly|CounterArc|CHECK-BAD-TOP|MissingTop" hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`

Expected: positive input coverage and invalid-top negative coverage are visible.

**Step 4: Rewrite `semantic scope` wording if needed**

Modify: `hirct/hirct/docs/v1-scope.md` only if needed.

Expected: the row says “single selected export target” and does not overclaim tool-driven flattening.

## Task 4: Verify C Model Export Artifact Claims

**Files:**
- Read: `hirct/hirct/docs/v1-scope.md`
- Read: `hirct/hirct/lib/Target/CModelEmitter.cpp`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel-aggregate.test`

**Step 1: Locate the C model export row**

Run: `rg -n "\\*\\*C model export\\*\\*|combinational module" hirct/hirct/docs/v1-scope.md`

Expected: relevant supported rows are isolated.

**Step 2: Locate direct emitter API shape**

Run: `rg -n "_initialize|_eval_comb|_eval_|typedef struct .*_state|emit\\(" hirct/hirct/lib/Target/CModelEmitter.cpp`

Expected: the actual exported API shape is visible in the emitter.

**Step 3: Locate direct export tests**

Run: `rg -n "CHECK-COMB-H|CHECK-CLK-H|CHECK-COMB-CPP|CHECK-CLK-CPP|CHECK-MEM-H|CHECK-MEM-CPP" hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`

Expected: direct export artifact coverage is visible.

**Step 4: Locate direct compile proof for export path**

Run: `rg -n "AggMod|c\\+\\+ -std=c\\+\\+17 -fsyntax-only" hirct/hirct/test/Tools/hirct-gen/export-cmodel-aggregate.test`

Expected: export-cmodel aggregate output is compile-checked directly.

**Step 5: Rewrite the row if the detail exceeds direct evidence**

Modify: `hirct/hirct/docs/v1-scope.md` only if needed.

Expected: the row is backed by `CModelEmitter.cpp` and export-path tests, not by legacy `--only model`.

## Task 5: Verify Wrapper Artifact And I/O Claims

**Files:**
- Read: `hirct/hirct/docs/v1-scope.md`
- Read: `hirct/hirct/lib/Target/SystemCWrapperEmitter.cpp`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-systemc-wrapper.test`

**Step 1: Locate wrapper rows**

Run: `rg -n "\\*\\*SystemC wrapper export\\*\\*|\\*\\*single-clock wrapper\\*\\*|<=64-bit wrapper I/O" hirct/hirct/docs/v1-scope.md`

Expected: wrapper-related supported rows are isolated.

**Step 2: Locate wrapper artifact and type emission code**

Run: `rg -n "SC_MODULE|clock_method|eval_method|sc_in_clk|scPortType|bool|sc_dt::sc_uint<8>|sc_dt::sc_uint<16>|sc_dt::sc_uint<32>|sc_dt::sc_uint<64>" hirct/hirct/lib/Target/SystemCWrapperEmitter.cpp`

Expected: wrapper structure and width mapping are visible.

**Step 3: Locate direct wrapper tests**

Run: `rg -n "CHECK-WH|CHECK-WI|sc_in_clk|CHECK-MC-ERR|CHECK-WP-ERR" hirct/hirct/test/Tools/hirct-gen/export-systemc-wrapper.test`

Expected: happy-path and negative wrapper coverage are visible.

**Step 4: Rewrite wrapper I/O wording if needed**

Modify: `hirct/hirct/docs/v1-scope.md` only if needed.

Expected: the row mentions `1-bit -> bool`, `2..64-bit -> sc_uint bucket`, and does not imply generic `sc_uint<N>` for all widths.

## Task 6: Verify SemanticModel-Only Claims

**Files:**
- Read: `hirct/hirct/docs/v1-scope.md`
- Read: `hirct/hirct/test/SemanticModel/multi-clock.test`
- Read: `hirct/hirct/test/SemanticModel/wide-port.test`

**Step 1: Locate SemanticModel-only rows**

Run: `rg -n "multi-clock semantic model|wide-port semantic model" hirct/hirct/docs/v1-scope.md`

Expected: model-level rows are isolated.

**Step 2: Locate matching tests**

Run: `rg -n "DualClk|WideMux|i512" hirct/hirct/test/SemanticModel/multi-clock.test hirct/hirct/test/SemanticModel/wide-port.test`

Expected: rows are clearly model-level support, not export-path support.

## Task 7: Verify Unsupported Hierarchy Claims Conservatively

**Files:**
- Read: `hirct/hirct/docs/v1-scope.md`
- Read: `hirct/hirct/test/Target/GenModel/multi-module.test`
- Read: `hirct/hirct/test/Target/GenModel/instance-topo-sort.test`
- Read: `hirct/hirct/test/Target/GenModel/instance-crossref.test`

**Step 1: Locate unsupported hierarchy rows**

Run: `rg -n "hierarchical multi-module export|instance topological sort|instance output cross-reference|arbitrary hierarchy preservation" hirct/hirct/docs/v1-scope.md`

Expected: hierarchy-related unsupported rows are isolated.

**Step 2: Locate the current indirect evidence**

Run: `rg -n "XFAIL|known-limitations|v1-scope" hirct/hirct/test/Target/GenModel/multi-module.test hirct/hirct/test/Target/GenModel/instance-topo-sort.test hirct/hirct/test/Target/GenModel/instance-crossref.test`

Expected: the current evidence is visible as legacy `--only model` XFAIL coverage.

**Step 3: Rewrite unsupported wording if needed**

Modify: `hirct/hirct/docs/v1-scope.md` only if needed.

Expected: rows do not pretend these are direct `--export-cmodel` negative proofs if they are not.

## Task 8: Verify Code-Guard-Only CLI Claims

**Files:**
- Read: `hirct/hirct/docs/v1-scope.md`
- Read: `hirct/hirct/tools/hirct-gen/main.cpp`

**Step 1: Locate CLI contract lines**

Run: `rg -n "^--export-cmodel|^--export-systemc-wrapper|^--top" hirct/hirct/docs/v1-scope.md`

Expected: CLI contract lines are visible.

**Step 2: Locate code guards**

Run: `rg -n "export_systemc_wrapper && !opts.export_cmodel|\\.v|\\.mlir|top module not found" hirct/hirct/tools/hirct-gen/main.cpp`

Expected: code-guard-only claims can be separated from test-backed claims.

**Step 3: Mark claims without dedicated lit coverage**

Modify: `hirct/hirct/docs/v1-scope.md` only if needed.

Expected: code-backed-only CLI claims are labeled as such, not presented as fully lit-locked.

## Task 9: Reconcile SSOT And Rebuild Evidence Table

**Files:**
- Read: `hirct/hirct/known-limitations.md`
- Modify: `hirct/hirct/docs/v1-scope.md`
- Modify: `hirct/hirct/known-limitations.md` only if needed

**Step 1: Compare SSOT wording**

Run: `rg -n "SSOT 관계|KL-20" hirct/hirct/docs/v1-scope.md hirct/hirct/known-limitations.md`

Expected: current wording is visible.

**Step 2: Choose one clear relationship**

Expected: `v1-scope.md` is the detailed contract and `KL-20` is the summary pointer, or the opposite, but not both.

**Step 3: Rebuild the summary table**

Expected: every row in the summary is clearly `test-backed`, `code-guard-backed`, `code-path-backed`, or `indirect-legacy-backed`.

## Task 10: Run Focused Verification

**Files:**
- Run against referenced tests only

Task 1에서 확보한 (lit runner, build-tree test root) 쌍을 사용한다. PATH runner가 없어도 절대경로 runner로 실행 가능하다.

**Step 1: Run core export test**

Run: `<lit-runner> -sv --filter "(export-cmodel|export-cmodel-aggregate|export-systemc-wrapper)" <build-test-root>`

또는 개별 실행:
```
<lit-runner> -sv <build-test-root>/Tools/hirct-gen/export-cmodel.test
```

Expected: 3개 export test 모두 PASS.

**Step 2: Record results**

usable harness가 없어서 실행 불가능한 경우에만 `verification-blocked`를 기록한다. PATH에 runner가 없는 것만으로는 blocked로 기록하지 않는다.

## Task 11: Write Closure Note

**Files:**
- Produce manager summary in report

**Step 1: Classify leftovers**

Expected: every remaining mismatch is tagged as `fix-in-doc`, `code-guard-only`, `M1`, or `separate implementation batch`.

**Step 2: Decide closure**

Expected: final report says either `M0 closed` or `M0 still open` with exact reasons.

## Conditional Closure Rule

- **`M0 verification-blocked`**: usable lit runner(PATH 또는 절대경로)와 generated `lit.site.cfg.py`를 **둘 다** 확보할 수 없을 때만 선언한다. "PATH에 `llvm-lit`이 없음"만으로는 blocked가 아니다 — 절대경로 runner + generated harness가 있으면 focused verification은 가능하다.
- verification-blocked 상태에서도 문서 정합성과 evidence 등급 정리는 진행할 수 있다.
- `M0 closed` 선언에는 focused export-path verification (export-cmodel, export-cmodel-aggregate, export-systemc-wrapper) PASS 결과를 포함해야 한다.

## Completion Criteria

- the file contains exactly one canonical M0 plan
- `v1-scope.md` has no supported claim that exceeds its evidence class
- code-guard-only CLI claims are labeled as such
- hierarchy unsupported wording is conservative about indirect evidence
- focused export-path tests are re-run when tooling is available
