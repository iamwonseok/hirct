# M2 Flatten-Capable Composition Contract Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** flattened single-top 산출물을 넘어서, 모듈 단위 C model artifact를 나중에 조합 가능한 contract로 정리한다.

**Architecture:** `M2`는 hierarchy-preserving export 자체를 구현하는 단계가 아니다. 대신 “작은 모듈을 먼저 성공시키고, artifact contract가 맞으면 나중에 연결 가능하다”는 중간층을 만든다. 핵심은 파일 레이아웃, 심볼 이름, include 규칙, 생성자/step/eval 연결 규칙을 미리 고정하는 것이다. 다만 현재 export 경로가 선택된 단일 `hw.module`의 `SemanticModel`을 `CModelEmitter`에 넘기는 구조라는 점을 문서에 명시하고, acceptance를 문서 수준과 spike 수준으로 분리한다.

**Tech Stack:** Markdown, generated C/C++ artifact layout, `hirct-gen`, export tests, small integration fixtures.

---

## Working Rules

- `M2`는 contract 설계와 acceptance fixture 정의가 중심이다.
- 하위 모듈 재사용 가능성에 대한 약속을 문서로 먼저 고정한다.
- parent/child orchestration 구현은 `M3`로 넘긴다.
- top-level wrapper 정책은 `M4`로 넘긴다.
- current export path의 single-target 제약을 숨기지 않는다.
- 필요 시 `M3` 리스크를 낮추는 최소 spike/prototype을 허용하되, 본 milestone의 주 산출물은 여전히 contract 문서다.

## M2 Closure Rule

| 항목 | closure 반영 여부 | 설명 |
|------|-------------------|------|
| Documentation Acceptance | 포함 | `M2` closure의 핵심 기준이다 |
| Golden Artifact Acceptance | 부분 포함 | 현재 단일 모듈 export 경로에서 관찰 가능한 naming / layout 범위까지만 확인한다 |
| Integration Fixture Acceptance | 미포함 | 실제 parent/child composition codegen이 없으므로 `M3` 입력으로 넘긴다 |
| Spike Acceptance | 미포함 | de-risking 용도이며 `M2` closure 조건이 아니다 |

현재 코드 기준으로 per-module composition contract 전체를 실행 검증할 수는 없다. `M2`는 문서 계약과 현재 단일 모듈 export 관찰치 사이의 합의까지를 닫고, 실제 조합 검증은 `M3`에서 시작한다.

## Scope

**In scope**
- per-module artifact layout
- generated naming rules
- include and ownership rules
- re-export / reuse assumptions
- minimum composition acceptance fixture design

**Out of scope**
- full hierarchical traversal
- automatic parent-child wiring
- final wrapper orchestration
- host ABI stabilization

## Task 1: Read M2 Boundary

**Files:**
- Read: `hirct/hirct/docs/plan/2026-04-12-v1-export-roadmap.md`
- Read: `hirct/hirct/docs/plan/2026-04-12-m1-flatten-scope-freeze-plan.md`

**Step 1: Locate M2 section**

Run: `rg -n "## M2\\.|composition contract|artifact layout" hirct/hirct/docs/plan/2026-04-12-v1-export-roadmap.md`

Expected: M2 intent is visible.

**Step 2: Read M1 artifact freeze**

Run: `rg -n "artifact contract|CLI|acceptance" hirct/hirct/docs/plan/2026-04-12-m1-flatten-scope-freeze-plan.md`

Expected: M2 starts from the frozen M1 baseline.

## Task 2: Inventory Current Generated Artifact Shape

**Files:**
- Read: `hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`
- Read: `hirct/hirct/lib/Target/CModelEmitter.cpp`
- Read: `hirct/hirct/tools/hirct-gen/main.cpp`

**Step 1: Locate current file naming shape**

Run: `rg -n "\\.h|\\.cpp|CHECK-COMB-H|CHECK-CLK-H" hirct/hirct/test/Tools/hirct-gen/export-cmodel.test`

Expected: current single-module output shape is visible.

**Step 2: Locate current exported API members**

Run: `rg -n "_initialize|_eval_comb|_eval_|buildModuleModel|CModelEmitter" hirct/hirct/lib/Target/CModelEmitter.cpp hirct/hirct/tools/hirct-gen/main.cpp`

Expected: current external API and single-target export path are visible.

## Task 3: Define Per-Module Artifact Layout

**Files:**
- Modify: `hirct/hirct/docs/plan/2026-04-12-m2-composition-contract-plan.md`

**Step 1: Add a `Per-Module Artifact Set` section**

Expected: the plan names exactly which files belong to one exported module.

**Step 2: Add a `Required Symbols` subsection**

Expected: constructor/state/init/eval entry points are listed.

**Step 3: Add a `Not Promised Yet` subsection**

Expected: parent-child composition codegen is explicitly excluded.

## Task 4: Define Naming Contract

**Files:**
- Read: `hirct/hirct/lib/Target/CModelEmitter.cpp`
- Modify: this file

**Step 1: Identify current module/file naming pattern**

Run: `rg -n "typedef struct|_state|headerPath|implPath|moduleName" hirct/hirct/lib/Target/CModelEmitter.cpp`

Expected: current naming conventions are visible.

**Step 2: Define stable file naming rules**

Modify: this file.

Expected: file names and emitted type names are documented.

**Step 3: Define collision policy**

Modify: this file.

Expected: what happens when user names collide is stated or deferred explicitly.

## Task 5: Define Include Contract

**Files:**
- Modify: this file

**Step 1: Add a `Header Inclusion Rules` section**

Expected: whether a module can be included independently is stated.

**Step 2: Add a `Generated-To-Generated Include` subsection**

Expected: future parent-child include direction is defined at the contract level.

**Step 3: Add a `Host Include Expectations` subsection**

Expected: host code assumptions are minimal and explicit.

## Task 6: Define Ownership And Lifetime Contract

**Files:**
- Modify: this file

**Step 1: Add a `State Ownership` section**

Expected: who owns module state is explicit.

**Step 2: Add an `Initialization Responsibility` subsection**

Expected: when `initialize` must be called is explicit.

**Step 3: Add an `Input / Output Mutation Rules` subsection**

Expected: host-written inputs and generated outputs follow one readable contract.

## Task 7: Define Reusable Export-Top Contract

**Files:**
- Modify: this file

**Step 1: Add a `Current Export Top` subsection**

Expected: exporting an intermediate module is explicitly allowed or disallowed.

**Step 2: Add a `Re-export Expectations` subsection**

Expected: whether the same module may be regenerated in a different top context is documented.

## Task 8: Design Minimal Composition Fixtures

**Files:**
- Modify: this file

**Step 1: Define a two-module fixture shape**

Expected: one parent and one child module case is described.

**Step 2: Define a sibling reuse fixture shape**

Expected: one parent using two child instances is described.

**Step 3: Define one negative fixture**

Expected: one unsupported composition case is named.

## Task 9: Define M2 Acceptance Tests

**Files:**
- Modify: this file

**Step 1: Add a `Documentation Acceptance` checklist**

Expected: contract sections required for M2 are listed.

**Step 2: Add a `Golden Artifact Acceptance` checklist**

Expected: generated names and files 중 현재 단일 모듈 export 경로로 관찰 가능한 부분만 testable하다고 분리해 적는다.

**Step 3: Add an `Integration Fixture Acceptance` checklist**

Expected: future implementation has a clear finish line.

**Step 4: Add a `Spike Acceptance` checklist**

Expected: any optional spike is explicitly marked non-contract, non-closure, and only for de-risking M3.

## Task 10: Define Prototype / Spike Boundary

**Files:**
- Modify: this file

**Step 1: Add a `Why M2 Cannot Pretend The Runtime Exists` section**

Expected: the current single-target `buildModuleModel -> CModelEmitter` path is called out directly.

**Step 2: Add a `Documentation-Level Closure` subsection**

Expected: the contract can close without full hierarchy codegen.

**Step 3: Add an `Optional M3 De-Risking Spike` subsection**

Expected: any prototype remains clearly out of M2 closure criteria.

## Task 11: Mark Handoff To M3

**Files:**
- Modify: this file

**Step 1: Add an `Inputs To M3` section**

Expected: M3 receives traversal, composition semantics, and orchestration work explicitly.

**Step 2: Add a `Do Not Implement In M2` section**

Expected: hierarchy-preserving codegen is clearly deferred.

## Completion Criteria

- per-module artifact contract is explicit
- naming/include/ownership rules are explicit
- minimal reusable export-top assumptions are explicit
- composition acceptance fixtures are defined
- documentation acceptance and optional spike acceptance are separated
- everything requiring real hierarchy codegen is handed to `M3`
