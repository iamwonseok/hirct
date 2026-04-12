# M3 Hierarchical C Model Export Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** flatten 없이도 parent/child 모듈을 개별 artifact로 export하고, 계층 구조를 유지한 채 조합 가능한 C model export 경로를 정의하고 구현한다.

**Architecture:** `M3`는 `M2`에서 고정한 artifact contract를 실제 hierarchy codegen으로 연결하는 단계다. 핵심은 module graph traversal, parent-child artifact emission, instance wiring, eval/step orchestration 순서를 readable한 contract로 구현하는 것이다. flattened 경로는 유지하되, hierarchical 경로는 별도 acceptance 기준으로 추가한다. 이 milestone의 핵심 난제는 instance DAG 추출, cross-module reference 해석, eval ordering, cycle reject 정책이다.

**Tech Stack:** MLIR module traversal, generated C/C++ artifacts, `hirct-gen`, lit tests, compile-check fixtures.

---

## Working Rules

- `M3`는 hierarchy-preserving C model export에 집중한다.
- top-level wrapper는 아직 만들지 않는다.
- flattened export를 깨지 않도록 hierarchical 경로는 별도 regression으로 검증한다.
- instance ordering, cross-reference, orchestration은 문서와 테스트를 함께 잠근다.
- 단순 file layout 정리가 아니라 알고리즘 설계가 필요한 milestone임을 문서에 명시한다.

## Scope

**In scope**
- module graph traversal
- parent/child artifact emission
- instance field generation
- child init/eval/step orchestration
- hierarchical acceptance fixtures

**Out of scope**
- SystemC top wrapper
- host ABI stabilization
- optimization tuning

## Task 1: Read M3 Inputs

**Files:**
- Read: `hirct/hirct/docs/plan/2026-04-12-v1-export-roadmap.md`
- Read: `hirct/hirct/docs/plan/2026-04-12-m2-composition-contract-plan.md`
- Read: `hirct/hirct/docs/v1-scope.md`

**Step 1: Locate M3 section**

Run: `rg -n "## M3\\.|Hierarchical C Model Export" hirct/hirct/docs/plan/2026-04-12-v1-export-roadmap.md`

Expected: M3 target is visible.

**Step 2: Read M2 handoff constraints**

Run: `rg -n "Inputs To M3|Do Not Implement In M2|artifact" hirct/hirct/docs/plan/2026-04-12-m2-composition-contract-plan.md`

Expected: contract inputs for M3 are visible.

**Step 3: Read current unsupported baseline**

Run: `rg -n "hierarchical multi-module export|instance topological sort|instance output cross-reference" hirct/hirct/docs/v1-scope.md`

Expected: the current unsupported rows that M3 aims to close are visible.

## Task 2: Inventory Current Failure Evidence

**Files:**
- Read: `hirct/hirct/test/Target/GenModel/multi-module.test`
- Read: `hirct/hirct/test/Target/GenModel/instance-topo-sort.test`
- Read: `hirct/hirct/test/Target/GenModel/instance-crossref.test`

**Step 1: Read multi-module failure contract**

Run: `rg -n "XFAIL|TODO|known-limitations|v1-scope" hirct/hirct/test/Target/GenModel/multi-module.test`

Expected: current unsupported condition is visible.

**Step 2: Read topo-sort failure contract**

Run: `rg -n "XFAIL|TODO|known-limitations|v1-scope" hirct/hirct/test/Target/GenModel/instance-topo-sort.test`

Expected: current unsupported condition is visible.

**Step 3: Read cross-reference failure contract**

Run: `rg -n "XFAIL|TODO|known-limitations|v1-scope" hirct/hirct/test/Target/GenModel/instance-crossref.test`

Expected: current unsupported condition is visible.

## Task 3: Define Traversal Boundary

**Files:**
- Read: `hirct/hirct/lib/Transforms/HirctProcessFlatten.cpp`
- Modify: `hirct/hirct/docs/plan/2026-04-12-m3-hierarchical-cmodel-plan.md`

**Step 1: Locate current flatten-related traversal assumptions**

Run: `rg -n "flatten|instance|module" hirct/hirct/lib/Transforms/HirctProcessFlatten.cpp`

Expected: current flatten assumptions are visible.

**Step 2: Add a `Hierarchical Traversal Boundary` section**

Expected: root module selection and child discovery rules are written down.

**Step 3: Add a `Cycle / Unsupported Graph` subsection**

Expected: reject cases are explicit.

## Task 4: Design Instance DAG Extraction

**Files:**
- Modify: this file

**Step 1: Add an `Instance Dependency Nodes And Edges` section**

Expected: what becomes a node and what becomes an edge is explicit.

**Step 2: Add a `Topological Sort Output Contract` subsection**

Expected: the ordering product consumed by codegen is explicit.

**Step 3: Add a `Cycle Detection / Reject Policy` subsection**

Expected: combinational cycles are rejected or explicitly deferred by rule.

## Task 5: Define Parent / Child Artifact Rules

**Files:**
- Modify: this file

**Step 1: Add a `Child Artifact Emission` section**

Expected: one child module maps to one generated artifact set.

**Step 2: Add a `Parent References Child` section**

Expected: include ownership and reference direction are explicit.

**Step 3: Add a `Naming Across Hierarchy` subsection**

Expected: instance names and module names do not ambiguously collide.

## Task 6: Define Instance Storage Model

**Files:**
- Modify: this file

**Step 1: Add an `Instance Fields In Parent` section**

Expected: parent-side storage for child models is explicit.

**Step 2: Add a `Construction / Initialization` subsection**

Expected: when child state is initialized is explicit.

**Step 3: Add an `Ownership Lifetime` subsection**

Expected: generated code has a readable lifetime model.

## Task 7: Design Cross-Module Reference Resolution

**Files:**
- Modify: this file

**Step 1: Add a `Parent Reads Child Output` section**

Expected: cross-module expression inputs are described as explicit reads, not implicit SSA magic.

**Step 2: Add a `renderExpr Scope Strategy` subsection**

Expected: whether expression rendering grows a hierarchical lookup map or a staged binding map is spelled out.

**Step 3: Add a `Name / Scope Collision Policy` subsection**

Expected: scope extension does not silently collide names.

**Step 4: Add a `Current renderExpr Limitation` subsection**

Expected: the plan explicitly states that current `renderExpr` is a single-module SSA-to-C renderer and that hierarchy requires binding-map growth, scope chain handling, and instance prefix propagation rather than a local tweak.

## Task 8: Define Signal Wiring Contract

**Files:**
- Modify: this file

**Step 1: Add an `Input Propagation` section**

Expected: parent-to-child signal flow is explicit.

**Step 2: Add an `Output Readback` section**

Expected: child outputs used by parent logic are explicit.

**Step 3: Add an `Intermediate Net Visibility` subsection**

Expected: internal-only wires versus exported outputs are separated.

## Task 9: Define Scheduling And Ordering Rules

**Files:**
- Modify: this file

**Step 1: Add an `eval_comb Ordering` section**

Expected: combinational ordering across instances is explicit.

**Step 2: Add an `eval_<clock>` Ordering section**

Expected: edge-triggered step ordering is explicit.

**Step 3: Add an `Instance Topological Sort` subsection**

Expected: the current XFAIL gap has a target contract.

**Step 4: Add a `Repeated Evaluation Or Explicit Reject` subsection**

Expected: feed-through chains that require fixed-point style reevaluation are either bounded or rejected explicitly.

## Task 10: Define Cross-Reference Semantics

**Files:**
- Modify: this file

**Step 1: Add an `Instance Output Cross-Reference` section**

Expected: parent logic consuming child outputs has one readable rule.

**Step 2: Add a `Reject Cases` subsection**

Expected: unsupported dependency shapes remain explicit if needed.

## Task 11: Design Hierarchical Acceptance Fixtures

**Files:**
- Modify: this file

**Step 1: Define a parent-with-one-child fixture**

Expected: simplest hierarchy case is named.

**Step 2: Define a parent-with-two-ordered-children fixture**

Expected: ordering dependency is exercised.

**Step 3: Define a child-output-fed-into-parent fixture**

Expected: cross-reference behavior is exercised.

## Task 12: Estimate Worker Batches And Complexity

**Files:**
- Modify: this file

**Step 1: Add a `Batch 1 Complexity` note**

Expected: traversal plus artifact emission complexity is stated explicitly, e.g. `2~3 days`.

**Step 2: Add a `Batch 2 Complexity` note**

Expected: scope expansion and wiring complexity is stated explicitly, e.g. `3~5 days`.

**Step 3: Add a `Batch 3 Complexity` note**

Expected: ordering and regression closure complexity is stated explicitly, e.g. `2~4 days`.

## Task 13: Define Implementation Sequence

**Files:**
- Modify: this file

**Step 1: Add a `Batch 1` section for traversal + artifact emission**

Expected: first worker batch is independent.

**Step 2: Add a `Batch 2` section for storage + wiring**

Expected: second worker batch is independent.

**Step 3: Add a `Batch 3` section for scheduling + regressions**

Expected: third worker batch closes the XFAILs or narrows them precisely.

## Task 14: Define Verification Commands

**Files:**
- Modify: this file

**Step 1: Add focused lit commands**

Expected: exact commands for multi-module, topo-sort, and cross-reference tests are listed.

**Step 2: Add one compile-check command**

Expected: generated hierarchical artifacts can be compile-checked.

## Task 15: Define M3 Exit Conditions

**Files:**
- Modify: this file

**Step 1: Write the `M3 done` statement**

Expected: hierarchy-preserving export is described as a concrete acceptance state.

**Step 2: Write the `M3 partial` statement**

Expected: if some hierarchy shapes remain unsupported, they are explicitly bounded.

## Completion Criteria

- traversal and artifact rules are explicit
- instance DAG extraction, scope expansion, and cycle reject rules are explicit
- instance storage, wiring, and scheduling rules are explicit
- current hierarchy-related XFAILs have direct target coverage
- implementation can proceed in 2~3 reviewable worker batches
- wrapper work is still deferred to `M4`
