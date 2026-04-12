# M4 Final Integration Top Wrapper Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 최종 integration top에만 SystemC wrapper를 제공하고, 하위 모듈은 generated C model artifact로 유지하는 최종 export 구조를 완성한다.

**Architecture:** `M4`의 핵심은 wrapper를 시스템 전체에 뿌리는 것이 아니라, `final integration top`에서만 orchestration layer로 쓰는 것이다. backend는 hierarchical C model artifact를 재사용하고, wrapper는 clock/reset 연결과 top-level host integration만 담당한다.

**Tech Stack:** SystemC wrapper generation, hierarchical C model backend, `hirct-gen`, wrapper compile tests, end-to-end integration fixtures.

---

## Working Rules

- `M4`는 top-level integration wrapper만 다룬다.
- 하위 모듈은 wrapper 대상이 아니라 reusable C model artifact로 유지한다.
- wrapper가 backend를 대체하지 않는다.
- top-level policy가 정해지기 전까지 하위 모듈별 wrapper 확장을 하지 않는다.
- multi-clock / wide-I/O wrapper 정책은 `M3` 종료 시점에 명시적으로 결정한다.

## Scope

**In scope**
- final integration top definition
- top-only wrapper policy
- backend reuse contract
- top-level clock/reset policy
- end-to-end acceptance criteria

**Out of scope**
- arbitrary per-module wrapper emission
- optimization tuning
- foreign host ABI redesign

## Task 1: Read M4 Inputs

**Files:**
- Read: `hirct/hirct/docs/plan/2026-04-12-v1-export-roadmap.md`
- Read: `hirct/hirct/docs/plan/2026-04-12-m3-hierarchical-cmodel-plan.md`
- Read: `hirct/hirct/docs/v1-scope.md`

**Step 1: Locate M4 section**

Run: `rg -n "## M4\\.|top-level wrapper|final integration top" hirct/hirct/docs/plan/2026-04-12-v1-export-roadmap.md`

Expected: M4 target is visible.

**Step 2: Read M3 completion boundary**

Run: `rg -n "M3 done|hierarchy-preserving|wrapper" hirct/hirct/docs/plan/2026-04-12-m3-hierarchical-cmodel-plan.md`

Expected: backend assumptions are visible.

**Step 3: Read current wrapper limits**

Run: `rg -n "SystemC wrapper export|multi-clock wrapper|wide wrapper I/O" hirct/hirct/docs/v1-scope.md`

Expected: current v1 wrapper baseline is visible.

## Task 2: Define `final integration top`

**Files:**
- Modify: `hirct/hirct/docs/plan/2026-04-12-m4-top-wrapper-integration-plan.md`

**Step 1: Add a `Definition` section**

Expected: `current export top` and `final integration top` are not conflated.

**Step 2: Add a `Selection Rule` subsection**

Expected: how the user identifies the final wrapper target is explicit.

**Step 3: Add a `Reject Rule` subsection**

Expected: invalid wrapper target shapes are explicit.

## Task 3: Define Top-Only Wrapper Policy

**Files:**
- Modify: this file

**Step 1: Add a `Why Top-Only` section**

Expected: wrapper scope is justified in terms of reuse and readability.

**Step 2: Add a `Child Modules Stay As Backend` subsection**

Expected: child modules are explicitly kept as C model artifacts.

## Task 4: Define Backend Reuse Contract

**Files:**
- Read: `hirct/hirct/lib/Target/SystemCWrapperEmitter.cpp`
- Modify: this file

**Step 1: Locate current wrapper-to-model binding shape**

Run: `rg -n "eval_method|clock_method|model|bind" hirct/hirct/lib/Target/SystemCWrapperEmitter.cpp`

Expected: current wrapper/backend interaction points are visible.

**Step 2: Add a `Wrapper Reuses Generated Backend` section**

Expected: wrapper does not duplicate simulation semantics.

**Step 3: Add a `No Logic Duplication` subsection**

Expected: backend versus wrapper responsibility split is explicit.

## Task 5: Define Top-Level Clock Policy

**Files:**
- Modify: this file

**Step 1: Add a `Single-Clock Top Policy` subsection**

Expected: current supported policy is explicit.

**Step 2: Add a `Multi-Clock Top Decision` subsection**

Expected: whether M4 keeps explicit reject behavior or promotes support is stated with a decision checkpoint tied to `M3` completion.

**Step 3: Add a `Clock Naming / Binding` subsection**

Expected: generated wrapper clock hookup is readable and testable.

## Task 6: Define Top-Level Reset Policy

**Files:**
- Modify: this file

**Step 1: Add a `Reset Exposure` section**

Expected: top-level reset input policy is explicit.

**Step 2: Add a `Backend Reset Propagation` subsection**

Expected: wrapper-to-backend reset flow is explicit.

**Step 3: Add an `Unsupported Reset Shapes` subsection**

Expected: edge cases remain bounded if needed.

## Task 7: Define Top-Level I/O Policy

**Files:**
- Modify: this file

**Step 1: Add a `Supported Top I/O` section**

Expected: currently supported widths and port styles are explicit.

**Step 2: Add a `Deferred Wide-I/O Wrapper Policy` subsection**

Expected: if wide top I/O is still deferred, the document states that M4 keeps reject behavior unless an explicit scope decision reopens it after `M3`.

## Task 8: Add M4 Policy Gate

**Files:**
- Modify: this file

**Step 1: Add a `Default Policy At M4 Start` section**

Expected: single-clock and <=64-bit top wrapper remain the default unless reopened.

**Step 2: Add a `Post-M3 Decision Gate` subsection**

Expected: the document states that the roadmap owner (`manager` session / project maintainer) explicitly decides whether multi-clock or wide-I/O support enters M4.

**Step 3: Add a `Stay-Reject If No Decision` subsection**

Expected: ambiguity is removed; lack of decision means explicit reject remains.

## Task 9: Design End-To-End Integration Fixtures

**Files:**
- Modify: this file

**Step 1: Define one single-clock top integration fixture**

Expected: a positive end-to-end case is named.

**Step 2: Define one backend-reuse fixture**

Expected: wrapper on top, child modules as backend artifacts is named.

**Step 3: Define one negative fixture**

Expected: one unsupported top shape is named.

## Task 10: Define Verification Commands

**Files:**
- Modify: this file

**Step 1: Add wrapper generation test command**

Expected: exact lit command is listed.

**Step 2: Add compile-check command**

Expected: generated wrapper and backend compile together.

**Step 3: Add end-to-end smoke command**

Expected: final integration can be exercised mechanically.

## Task 11: Define Baseline Promotion Gate

**Files:**
- Modify: this file
- Modify if needed: `hirct/hirct/docs/plan/2026-04-12-v1-export-roadmap.md`

**Step 1: Add an `M4 done` statement**

Expected: the final architecture is expressed as a concrete acceptance state.

**Step 2: Add a `Baseline Promotion Gate` section**

Expected: what must be true before promoting this line is explicit.

**Step 3: Add a `Still Deferred After M4` section**

Expected: optional next-scope items remain bounded.

## Completion Criteria

- `final integration top` is precisely defined
- wrapper responsibility versus backend responsibility is precise
- top-level clock/reset/I/O policy is explicit
- multi-clock / wide-I/O decision timing is explicit
- end-to-end fixtures and verification commands exist
- baseline promotion gate is written down
