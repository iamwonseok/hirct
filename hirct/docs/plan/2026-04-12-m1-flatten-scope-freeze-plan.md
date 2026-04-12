# M1 Flatten-First Exporter Scope Freeze Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 이미 flatten/lower된 single-top MLIR 입력 또는 명시적 pipeline을 거친 입력을 기준으로, 현재 exporter 능력과 남은 hardening 범위를 분리해 `M1` 범위를 고정한다.

**Architecture:** `M1`은 새 architecture를 도입하는 단계가 아니라 flatten-first exporter를 “신뢰 가능한 현재 기반”으로 굳히는 단계다. 이미 닫힌 reset/wide/array/non-port hardening은 증거를 모아 고정하고, 아직 남은 gap은 artifact contract, CLI contract, regression coverage 기준으로 다시 분류한다.

**Tech Stack:** Markdown, `llvm-lit` or equivalent lit runner, generated `lit.site.cfg.py` from a configured build tree, `hirct-gen`, direct export tests, existing legacy `GenModel` regressions.

---

## Working Rules

- `M1`은 flattened single-top exporter만 다룬다.
- 입력은 이미 flatten/lower된 MLIR이거나, 필요 시 사용자가 명시적으로 pipeline을 지정한 경우로 한정한다.
- hierarchy-preserving 방향성은 문서에서 언급만 하고 구현 계획은 `M3`로 넘긴다.
- direct export acceptance와 legacy hardening background를 구분한다.
- wide, array, non-port reset 관련 regression은 “현재 기반이 어디까지 단단한지”를 보여주는 간접 hardening background로 쓴다.
- 새 bugfix가 필요해 보여도 이 문서에서는 설계와 분류만 한다.

## Task 0: Verify Test Runner Preflight

**Files:**
- Read: local toolchain state

**Step 1: Check whether `llvm-lit` is available**

Run: `llvm-lit --version`

Expected: version prints, or the missing runner is recorded as a preflight blocker.

**Step 2: Check fallback runner if needed**

Run: `python3 -m lit --version`

Expected: version prints, or fallback is also missing and later verification steps are marked blocked.

**Step 3: Locate generated lit harness**

Run: `rg -n "hirct_gen_path|hirct_obj_root|filecheck_path" <chosen-build-test-lit-site-cfg>`

Expected: generated `lit.site.cfg.py` exists and exposes `%hirct-gen` substitution inputs.

**Step 4: Record chosen lit runner**

Expected: later verification steps use `<chosen-lit-runner>` consistently together with one configured build-tree lit entrypoint, or the milestone is marked verification-blocked.

## Scope

**In scope**
- flatten-first small module export baseline
- current artifact contract
- current CLI contract
- hardening 완료/미완료 분리
- `M1` acceptance 기준 정의

**Out of scope**
- module composition contract
- hierarchical artifact generation
- host ABI
- top-level wrapper integration redesign

## Task 1: Read M1 Boundary

**Files:**
- Read: `hirct/hirct/docs/plan/2026-04-12-v1-export-roadmap.md`
- Read: `hirct/hirct/docs/v1-scope.md`

**Step 1: Locate M1 section in roadmap**

Run: `rg -n "## M1\\.|flatten-first|small module" hirct/hirct/docs/plan/2026-04-12-v1-export-roadmap.md`

Expected: M1 objective is visible.

**Step 2: Read current truthful contract**

Run: `rg -n "C model export|SystemC wrapper export|Unsupported|CLI Contract" hirct/hirct/docs/v1-scope.md`

Expected: current baseline rows to freeze are visible.

## Task 2: Inventory Core Export Entry Points

**Files:**
- Read: `hirct/hirct/tools/hirct-gen/main.cpp`
- Read: `hirct/hirct/lib/Target/CModelEmitter.cpp`
- Read: `hirct/hirct/lib/Target/SystemCWrapperEmitter.cpp`

**Step 1: Locate C model export entry**

Run: `rg -n "export_cmodel|CModelEmitter|emit\\(" hirct/hirct/tools/hirct-gen/main.cpp hirct/hirct/lib/Target/CModelEmitter.cpp`

Expected: current artifact generation path is visible.

**Step 2: Locate wrapper export entry**

Run: `rg -n "export-systemc-wrapper|getWrapperV1UnsupportedReason" hirct/hirct/tools/hirct-gen/main.cpp hirct/hirct/lib/Target/SystemCWrapperEmitter.cpp`

Expected: wrapper generation path and guards are visible.

## Task 3: Freeze C Model Artifact Contract

**Files:**
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel-aggregate.test`
- Read: `hirct/hirct/lib/Target/CModelEmitter.cpp`
- Modify: `hirct/hirct/docs/plan/2026-04-12-m1-flatten-scope-freeze-plan.md`

**Step 1: Identify generated file pair**

Run: `rg -n "CHECK-COMB-H|CHECK-CLK-H|\\.h|\\.cpp" hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`

Expected: file-level artifact evidence is visible.

**Step 2: Identify required exported members**

Run: `rg -n "_initialize|_eval_comb|_eval_|typedef struct .*_state" hirct/hirct/lib/Target/CModelEmitter.cpp`

Expected: current externally visible generated API shape is visible.

**Step 3: Write the M1 artifact contract section**

Modify: this file.

Expected: M1 artifact contract names what must exist and what is intentionally not promised yet.

## Task 4: Freeze Wrapper Artifact Contract

**Files:**
- Read: `hirct/hirct/test/Tools/hirct-gen/export-systemc-wrapper.test`
- Read: `hirct/hirct/lib/Target/SystemCWrapperEmitter.cpp`
- Modify: this file

**Step 1: Identify generated wrapper file pair**

Run: `rg -n "CHECK-WH|CHECK-WI|_sc_wrapper" hirct/hirct/test/Tools/hirct-gen/export-systemc-wrapper.test`

Expected: wrapper artifact pair is visible.

**Step 2: Identify required emitted wrapper structure**

Run: `rg -n "SC_MODULE|clock_method|eval_method|sc_in|sc_out" hirct/hirct/lib/Target/SystemCWrapperEmitter.cpp`

Expected: current wrapper API shape is visible.

**Step 3: Write the M1 wrapper contract section**

Modify: this file.

Expected: wrapper scope is frozen to current v1 limits.

## Task 5: Freeze CLI Contract

**Files:**
- Read: `hirct/hirct/tools/hirct-gen/main.cpp`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-systemc-wrapper.test`
- Modify: this file

**Step 1: Locate `--export-cmodel` contract points**

Run: `rg -n -- "--export-cmodel|outputDir|top" hirct/hirct/tools/hirct-gen/main.cpp`

Expected: C model CLI behavior is visible.

**Step 2: Locate `--export-systemc-wrapper` contract points**

Run: `rg -n -- "--export-systemc-wrapper|export-cmodel|required|unsupported" hirct/hirct/tools/hirct-gen/main.cpp`

Expected: wrapper CLI behavior is visible.

**Step 3: Add exact CLI promises to the plan**

Modify: this file.

Expected: M1 does not promise any CLI behavior not already enforced.

## Task 6: Inventory Already-Hardened Register Semantics

**Files:**
- Read: `hirct/hirct/test/Target/GenModel/wide-constant-crash.test`
- Read: `hirct/hirct/test/Target/GenModel/wide-reg-multi-clock.test`
- Read: `hirct/hirct/test/Target/GenModel/async-reset-wide-reg.test`
- Read: `hirct/hirct/test/Target/GenModel/async-reset-wide-reg-multi-clock.test`
- Read: `hirct/hirct/test/Target/GenModel/sync-reset-narrow-reg.test`
- Read: `hirct/hirct/test/Target/GenModel/sync-reset-narrow-reg-multi-clock.test`
- Read: `hirct/hirct/test/Target/GenModel/non-port-reset-sig.test`
- Read: `hirct/hirct/test/Target/GenModel/non-port-reset-sig-multi-clock.test`
- Read: `hirct/hirct/test/Target/GenModel/sync-reset-array-reg.test`
- Read: `hirct/hirct/test/Target/GenModel/sync-reset-array-reg-multi-clock.test`
- Read: `hirct/hirct/test/Target/GenModel/sync-reset-array-reg-agg-const.test`

**Step 1: Group wide-register regression proofs**

Run: `rg -n "wide|reset|domain_step|async" hirct/hirct/test/Target/GenModel/wide-constant-crash.test hirct/hirct/test/Target/GenModel/wide-reg-multi-clock.test hirct/hirct/test/Target/GenModel/async-reset-wide-reg.test hirct/hirct/test/Target/GenModel/async-reset-wide-reg-multi-clock.test`

Expected: wide-register hardening evidence is visible.

**Step 2: Group narrow-register regression proofs**

Run: `rg -n "reset|next_|if \\(" hirct/hirct/test/Target/GenModel/sync-reset-narrow-reg.test hirct/hirct/test/Target/GenModel/sync-reset-narrow-reg-multi-clock.test`

Expected: narrow-register reset evidence is visible.

**Step 3: Group non-port reset proofs**

Run: `rg -n "rst_sig_|reset" hirct/hirct/test/Target/GenModel/non-port-reset-sig.test hirct/hirct/test/Target/GenModel/non-port-reset-sig-multi-clock.test`

Expected: non-port reset evidence is visible.

**Step 4: Group array reset proofs**

Run: `rg -n "array|aggregate|reset" hirct/hirct/test/Target/GenModel/sync-reset-array-reg.test hirct/hirct/test/Target/GenModel/sync-reset-array-reg-multi-clock.test hirct/hirct/test/Target/GenModel/sync-reset-array-reg-agg-const.test`

Expected: array-reset evidence is visible.

## Task 7: Separate Closed Hardening From Open Hardening

**Files:**
- Modify: this file

**Step 1: Create a `Closed In M1` list**

Expected: already-covered hardening items are listed with test references.

**Step 2: Create an `Open In M1` list**

Expected: only current flatten-first reliability gaps remain.

**Step 3: Create an `Out Of M1` list**

Expected: composition, hierarchy, host ABI, and top integration are moved out.

## Task 8: Define M1 Acceptance Fixture Set

**Files:**
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-systemc-wrapper.test`
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel-aggregate.test`
- Modify: this file

**Step 1: Pick minimum positive export fixtures**

Expected: at least one comb-only and one clocked export fixture are named.

**Step 2: Pick minimum negative wrapper fixtures**

Expected: multi-clock and wide-port reject fixtures are named.

**Step 3: Pick one generated-code compile proof**

Expected: direct `--export-cmodel` compile proof is called out explicitly, preferably `export-cmodel-aggregate.test`.

## Task 9: Define M1 Exit Conditions

**Files:**
- Modify: this file

**Step 1: Write the M1 done statement**

Expected: “small flattened exporter is trustworthy” is expressed as objective criteria.

**Step 2: Write the M1 not-done statement**

Expected: unresolved issues that block moving to M2 are explicit.

## Task 10: Run Focused Verification

**Files:**
- Run against referenced M1 tests

**Step 1: Run export C model tests**

Run: `<chosen-lit-runner> -sv <chosen-build-test-root-or-lit-site-cfg> hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`

Expected: PASS, or the run is skipped because the preflight blocker was recorded.

**Step 2: Run direct export compile proof**

Run: `<chosen-lit-runner> -sv <chosen-build-test-root-or-lit-site-cfg> hirct/hirct/test/Tools/hirct-gen/export-cmodel-aggregate.test`

Expected: PASS, or the run is skipped because the preflight blocker was recorded.

**Step 3: Run wrapper export tests**

Run: `<chosen-lit-runner> -sv <chosen-build-test-root-or-lit-site-cfg> hirct/hirct/test/Tools/hirct-gen/export-systemc-wrapper.test`

Expected: PASS, or the run is skipped because the preflight blocker was recorded.

**Step 4: Run core hardening regressions**

Run: `<chosen-lit-runner> -sv <chosen-build-test-root-or-lit-site-cfg> hirct/hirct/test/Target/GenModel/wide-constant-crash.test hirct/hirct/test/Target/GenModel/wide-reg-multi-clock.test hirct/hirct/test/Target/GenModel/async-reset-wide-reg.test hirct/hirct/test/Target/GenModel/async-reset-wide-reg-multi-clock.test hirct/hirct/test/Target/GenModel/sync-reset-narrow-reg.test hirct/hirct/test/Target/GenModel/non-port-reset-sig.test hirct/hirct/test/Target/GenModel/sync-reset-array-reg-agg-const.test`

Expected: PASS for current hardening background, or the run is skipped because the preflight blocker was recorded. These are legacy `--only model` proofs, not direct export acceptance.

## Conditional Closure Rule

- lit runner가 없거나 generated `lit.site.cfg.py`가 없는 source-tree 단독 상태이면 `M1 verification-blocked`로 표기한다.
- 이 경우 문서/계약 freeze 자체는 진행할 수 있지만, milestone closure는 `조건부`로만 선언한다.
- configured build-tree lit harness 확보 후 Task 10을 별도 verification batch로 다시 실행해야 `M1 closed`를 선언할 수 있다.

## Completion Criteria

- M1 artifact contract is documented for C model and wrapper separately
- current CLI promises are frozen
- direct export acceptance fixtures and indirect hardening background are separated
- already-closed hardening items and remaining gaps are separated
- acceptance fixture set is explicit
- `M2+` work is not mixed into the M1 document
