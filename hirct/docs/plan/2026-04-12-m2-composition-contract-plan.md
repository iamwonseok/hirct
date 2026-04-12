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
- Read (reference only): `hirct/hirct/lib/Target/CModelEmitter.cpp` — 현재 emit 패턴을 확인하는 용도이며, naming 규칙은 **본 plan 문서에만** 기록한다. 구현 소스(`.cpp`)를 M2에서 수정하지 않는다.
- Modify: this plan document (`hirct/hirct/docs/plan/2026-04-12-m2-composition-contract-plan.md`)

**Step 1: Identify current module/file naming pattern**

Run: `rg -n "typedef struct|_state|headerPath|implPath|moduleName" hirct/hirct/lib/Target/CModelEmitter.cpp`

Expected: current naming conventions are visible.

**Step 2: Define stable file naming rules**

Modify: this plan document.

Expected: file names and emitted type names are documented.

**Step 3: Define collision policy**

Modify: this plan document.

Expected: what happens when user names collide is stated or deferred explicitly.

## Task 5: Define Include Contract

**Files:**
- Modify: this plan document (이 plan 문서)

**Step 1: Add a `Header Inclusion Rules` section**

Expected: whether a module can be included independently is stated.

**Step 2: Add a `Generated-To-Generated Include` subsection**

Expected: future parent-child include direction is defined at the contract level.

**Step 3: Add a `Host Include Expectations` subsection**

Expected: host code assumptions are minimal and explicit.

## Task 6: Define Ownership And Lifetime Contract

**Files:**
- Modify: this plan document (이 plan 문서)

**Step 1: Add a `State Ownership` section**

Expected: who owns module state is explicit.

**Step 2: Add an `Initialization Responsibility` subsection**

Expected: when `initialize` must be called is explicit.

**Step 3: Add an `Input / Output Mutation Rules` subsection**

Expected: host-written inputs and generated outputs follow one readable contract.

## Task 7: Define Reusable Export-Top Contract

**Files:**
- Modify: this plan document (이 plan 문서)

**Step 1: Add a `Current Export Top` subsection**

Expected: exporting an intermediate module is explicitly allowed or disallowed.

**Step 2: Add a `Re-export Expectations` subsection**

Expected: whether the same module may be regenerated in a different top context is documented.

## Task 8: Design Minimal Composition Fixtures

**Files:**
- Modify: this plan document (이 plan 문서)

**Step 1: Define a two-module fixture shape**

Expected: one parent and one child module case is described.

**Step 2: Define a sibling reuse fixture shape**

Expected: one parent using two child instances is described.

**Step 3: Define one negative fixture**

Expected: one unsupported composition case is named.

## Task 9: Define M2 Acceptance Tests

**Files:**
- Modify: this plan document (이 plan 문서)

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
- Modify: this plan document (이 plan 문서)

**Step 1: Add a `Why M2 Cannot Pretend The Runtime Exists` section**

Expected: the current single-target `buildModuleModel -> CModelEmitter` path is called out directly.

**Step 2: Add a `Documentation-Level Closure` subsection**

Expected: the contract can close without full hierarchy codegen.

**Step 3: Add an `Optional M3 De-Risking Spike` subsection**

Expected: any prototype remains clearly out of M2 closure criteria.

## Task 11: Mark Handoff To M3

**Files:**
- Modify: this plan document (이 plan 문서)

**Step 1: Add an `Inputs To M3` section**

Expected: M3 receives traversal, composition semantics, and orchestration work explicitly.

**Step 2: Add a `Do Not Implement In M2` section**

Expected: hierarchy-preserving codegen is clearly deferred.

---

## Per-Module Artifact Set

> **용어: `{Mod}`**
> 이 문서에서 `{Mod}`는 export 대상 `hw.module`의 MLIR symbol name 그대로를 의미한다 (예: `Counter`, `FirmemBasic`). 대소문자 변환이나 escaping은 수행하지 않는다. 현재 `CModelEmitter`는 `model_.moduleName`을 변환 없이 심볼 접두사로 사용한다. MLIR symbol name이 C 식별자로 유효하지 않은 경우의 정리(legalization)는 아직 정의되지 않았으며, 필요 시 M3에서 다룬다.

현재 `CModelEmitter`가 단일 `hw.module`에 대해 생성하는 artifact는 아래 두 파일이다.

| # | File | Path Pattern | Content |
|---|------|-------------|---------|
| 1 | Header | `{outputDir}/{moduleName}.h` | include guard, type definitions, function declarations |
| 2 | Implementation | `{outputDir}/{moduleName}.cpp` | `#include "{moduleName}.h"`, all function bodies |

`CModelOptions`에서 suffix는 변경 가능하나(`headerSuffix`, `implSuffix`), 기본값은 `.h` / `.cpp`이다.
invocation 당 하나의 `hw.module`만 export되며, 두 파일은 항상 쌍으로 생성된다.

### Required Symbols

하나의 모듈 artifact는 다음 심볼을 반드시 포함해야 한다.

| Symbol | Kind | Header | Impl | Variant | Description |
|--------|------|--------|------|---------|-------------|
| `{Mod}_state` | typedef struct | O | - | (항상) | 모듈의 전체 state를 담는 struct. port members (`input_*`, `output_*`), state vars, aggregate arrays, memory vars 포함. |
| `{Mod}_initialize` | function | decl | def | (항상) | state struct를 초기값으로 설정한다. eval 전에 반드시 한 번 호출해야 한다. |
| `{Mod}_set_{portName}` | function | decl | def | scalar (non-wide) input | host가 input port 값을 쓸 때 사용한다. 시그니처는 `{Mod}_state *s`와 해당 port의 scalar C 타입 인자 하나다. |
| `{Mod}_set_{portName}_word` | function | decl | def | wide input | wide input의 한 word를 `idx`로 쓴다: `void …({Mod}_state *s, size_t idx, uint64_t v)`. |
| `{Mod}_set_{portName}_words` | function | decl | def | wide input | wide input 전체를 버퍼에서 복사: `void …({Mod}_state *s, const uint64_t *src, size_t count)`. |
| `{Mod}_get_{portName}` | function | decl | def | scalar 또는 aggregate (non-wide) output | scalar 출력: 값을 반환하는 getter (`…(const {Mod}_state *s)`). aggregate 출력: `void …(const {Mod}_state *s, {elemType} *dst, size_t count)`로 `dst`에 요소를 채운다. wide output은 이 심볼 대신 `_word`/`_words` API를 쓴다. **wide input에 대해서는 이 scalar getter가 생성되지 않으며, `_word`/`_words`만 사용한다.** |
| `{Mod}_get_{portName}_word` | function | decl | def | wide input 또는 wide output | wide port의 한 word를 `idx`로 읽는다: `uint64_t …(const {Mod}_state *s, size_t idx)`. (wide input은 스테이징된 입력 word 조회, wide output은 출력 word 조회.) |
| `{Mod}_get_{portName}_words` | function | decl | def | wide output | wide 출력 전체를 `dst`로 복사: `void …(const {Mod}_state *s, uint64_t *dst, size_t count)`. |
| `{Mod}_get_{portName}_word_count` | function | decl | def | wide input 또는 wide output | 해당 wide port의 word 개수(상한): `size_t …(void)`. |
| `{Mod}_eval_comb` | function | decl | def | (항상) | combinational logic을 평가한다. |
| `{Mod}_eval_{clockName}` | function | decl | def | clock domain당 (없으면 생략) | clock edge 시 sequential logic을 평가한다. clock이 없는 모듈에서는 생략될 수 있다. |

Header에는 `#ifdef __cplusplus extern "C" { #endif` guard가 포함되어, C/C++ 양쪽에서 링크 가능하다.

### Not Promised Yet

- **Parent-child composition codegen은 M2에 포함되지 않는다.** 현재 코드는 단일 모듈의 `SemanticModel`을 `CModelEmitter`에 넘기는 경로만 존재한다. 부모 모듈이 자식 모듈의 `_initialize`나 `_eval_*`을 호출하는 코드를 자동 생성하는 기능은 없다.
- **현재 export는 single-target이다.** `--export-cmodel`은 `--top`으로 지정하거나 walk order 마지막인 하나의 `hw.module`만 export한다. 한 invocation에서 복수 모듈을 동시에 export하는 기능은 존재하지 않는다.
- **Multi-module orchestration wrapper는 M4 범위이다.**

---

## Naming Contract

### File Naming

| Artifact | Pattern | Example (`moduleName = "Counter"`) |
|----------|---------|--------------------------------------|
| Header | `{moduleName}{headerSuffix}` | `Counter.h` |
| Implementation | `{moduleName}{implSuffix}` | `Counter.cpp` |

`moduleName`은 export 대상 `hw.module`의 MLIR symbol name에서 직접 가져온다.

### Type And Function Naming

| Category | Pattern | Example |
|----------|---------|---------|
| State struct | `{Mod}_state` | `Counter_state` |
| Initialize | `{Mod}_initialize` | `Counter_initialize` |
| Set input | `{Mod}_set_{portName}` | `Counter_set_clk` |
| Get output | `{Mod}_get_{portName}` | `Counter_get_count` |
| Eval combinational | `{Mod}_eval_comb` | `Counter_eval_comb` |
| Eval clock | `{Mod}_eval_{clockName}` | `Counter_eval_clock` |
| Include guard | `{Mod}_MODEL_H` | `Counter_MODEL_H` (`moduleName` 대소문자 그대로) |

State struct 내부 멤버 이름은 `SemanticModel`의 stable name을 그대로 사용한다 (예: `counter`, `arr_reg`). Port 멤버는 `input_{portName}`, `output_{portName}` 형식이다.

### Collision Policy

현재 코드에는 모듈 이름 충돌을 감지하거나 해소하는 로직이 없다.

- **동일 이름 모듈**: 한 invocation이 단일 모듈만 export하므로, 동일 invocation 내 이름 충돌은 발생하지 않는다.
- **Case-insensitive filesystem 충돌** (예: `Counter`와 `counter`가 같은 디렉토리에 export되는 경우): 현재 별도 처리가 없다. 이는 multi-module export가 도입되는 **M3에서 해결해야 할 사항**이다.
- **M2 계약**: M2는 "단일 invocation = 단일 모듈 = 충돌 없음"이라는 현재 상태를 문서화한다. 복수 모듈 export 시 collision detection/resolution은 M3 범위로 명시적으로 이관한다.

---

## Include Contract

### Header Inclusion Rules

각 모듈 header는 **독립적으로 include 가능**하다.

- `#ifndef {Mod}_MODEL_H` / `#define {Mod}_MODEL_H` guard로 중복 include를 방지한다 (`moduleName`과 동일한 대소문자, 예: `Counter_MODEL_H`).
- 외부 의존은 표준 라이브러리 헤더 세 개뿐이다: `<stdint.h>`, `<stddef.h>`, `<string.h>`.
- 다른 generated header나 프로젝트 내부 header를 include하지 않는다.
- `#ifdef __cplusplus extern "C" { #endif` guard로 C++ 컴파일러에서도 C linkage를 보장한다.

### Generated-To-Generated Include

현재 상태: **generated header 간 cross-include는 존재하지 않는다.** 단일 모듈만 export되므로 다른 generated header를 참조할 필요가 없다.

M3 contract 방향: parent 모듈의 header가 child 모듈의 header를 `#include`하는 방향이 된다. 즉:
- `Parent.h`가 `#include "Child.h"`를 포함한다.
- `Child.h`는 `Parent.h`를 include하지 않는다.
- Circular include는 허용되지 않는다.

이 방향은 M2에서 계약으로 고정하되, 실제 codegen 구현은 M3에서 수행한다.

### Host Include Expectations

Host 코드가 generated module을 사용하기 위한 최소 요구사항:

1. `#include "{Mod}.h"`
2. `{Mod}.cpp`를 컴파일하여 링크
3. `{Mod}_state` 변수를 할당 (stack 또는 heap)
4. `{Mod}_initialize(&state)` 호출
5. Input 설정(`{Mod}_set_{portName}` 또는 wide 변형) → `{Mod}_eval_comb` 또는 `{Mod}_eval_{clockName}` 호출 → output 읽기(`{Mod}_get_{portName}` 또는 wide/aggregate 변형)

Host는 generated header 외에 별도의 runtime library나 framework header를 include할 필요가 없다.

---

## Ownership And Lifetime Contract

### State Ownership

- `{Mod}_state`는 **host가 할당**한다. Stack 할당이든 heap 할당이든 host의 선택이다.
- Module 함수(`{Mod}_initialize`, `{Mod}_eval_*`, `{Mod}_set_*`, `{Mod}_get_*`)는 state를 할당하거나 해제하지 않는다.
- State의 lifetime은 전적으로 caller(host)가 관리한다.
- Module 함수는 전달받은 `{Mod}_state*` 포인터의 유효성을 검증하지 않는다. 유효한 포인터를 전달하는 것은 host의 책임이다.

### Initialization Responsibility

- `{Mod}_initialize(&state)`는 다른 어떤 module 함수보다 **먼저 정확히 한 번** 호출되어야 한다.
- `initialize` 없이 `eval_comb`, `eval_{clock}`, `set_*`, `get_*`를 호출하면 **undefined behavior**이다.
- `initialize`를 복수 회 호출하면 state가 초기값으로 재설정된다. 이것은 현재 동작이지만, 계약으로 보장하는 것은 "최소 한 번 호출" 뿐이다.

### Input / Output Mutation Rules

호출 순서 계약:

1. Host가 `{Mod}_set_{portName}(&state, value)`로 input을 설정한다.
2. Host가 `{Mod}_eval_comb(&state)` 또는 `{Mod}_eval_{clockName}(&state)`를 호출한다.
3. Host가 `{Mod}_get_{portName}(&state)`로 output을 읽는다.

Mutation 규칙:

- `set_*` 함수는 `state` 내의 해당 input 필드만 변경한다.
- `eval_*` 함수는 전달받은 `state` struct의 내부 상태와 output 필드를 변경할 수 있다. **자신의 state struct 외부 메모리를 변경하지 않는다.**
- `get_*` 함수는 `state`를 변경하지 않는다. 단, wide/aggregate getter(`{Mod}_get_{portName}_word`, `{Mod}_get_{portName}_words`, aggregate `{Mod}_get_{portName}`)는 호출자가 제공한 외부 버퍼(`dst`)에 값을 쓴다. 이는 `state` 변경이 아니라 caller 소유 메모리에 대한 쓰기이다.
- Host가 `state` struct의 필드를 `set_*`/`get_*` 를 거치지 않고 직접 접근하는 것은 현재 struct가 public이므로 가능하지만, 계약으로 보장하지 않는다. Struct 내부 레이아웃은 향후 변경될 수 있다.

---

## Reusable Export-Top Contract

### Current Export Top

- `--export-cmodel`은 단일 `hw.module`을 export한다.
- Export 대상은 `--top <name>`으로 지정하거나, 미지정 시 walk order의 마지막 `hw.module`이 선택된다.
- **어떤 `hw.module`이든 export top이 될 수 있다.** "최상위"가 아닌 중간 모듈도 export 가능하다. 이는 의도된 동작이다.
- 단, **한 invocation 당 하나의 모듈만 export된다.** 복수 모듈을 한 번에 export하려면 별도 invocation이 필요하다.

### Re-export Expectations

- 동일한 MLIR 모듈을 다른 `outputDir`이나 다른 빌드 설정으로 **여러 번 export할 수 있다.**
- Generated artifact는 **deterministic**이다: 동일한 input MLIR과 동일한 옵션이 주어지면, 동일한 output을 생성한다.
- 동일 모듈이 standalone export로 생성된 artifact와, 향후 M3에서 parent context 내 child로 사용될 때의 per-module artifact는 **동일한 contract를 따라야 한다.** 즉:
  - `{Mod}_state` 구조체 레이아웃이 동일하다.
  - `{Mod}_initialize`, `{Mod}_eval_*` 시그니처가 동일하다.
  - Header의 include guard와 extern "C" guard가 동일하다.
- 이 동일성 보장은 M3 hierarchy codegen이 각 child를 "이미 export된 모듈과 동일한 artifact"로 취급할 수 있게 하는 기반이다.

---

## Minimal Composition Fixtures

이 절의 fixture는 **설계용(문서 수준)**이다. 현재 경로는 **단일 `hw.module` export**이며 **composition codegen이 없으므로**, 아래 시나리오는 **end-to-end로 검증할 수 없다**. 각 항목은 **M3 acceptance target**으로만 취급한다: M3에서 구현되어야 “계약대로 조합이 된다”고 말할 수 있는 최소 형태를 고정한다.

### Step 1: Two-Module Fixture Shape (Parent + One Child)

**역할**

- **Child `Counter`**: 클럭 입력과 카운트 출력만 갖는 단순 카운터 모듈.
- **Parent `TopCounter`**: `Counter`를 한 인스턴스로 감싸고, 부모의 포트로 자식 출력(및 필요 시 클럭)을 노출한다.

**MLIR 모듈 시그니처 (본문 생략, 예시)**

```mlir
hw.module @Counter(in %clk : i1, out %count : i32) { ... }
hw.module @TopCounter(in %clk : i1, out %count : i32) { ... }  // 내부에서 @Counter 인스턴스
```

**M3가 존재할 때 기대하는 artifact 형태 (계약 관점)**

- `Counter.h` / `Counter.cpp`: 기존 per-module 계약대로 `{Mod}_state`, `{Mod}_initialize`, `{Mod}_set_*`, `{Mod}_get_*`, `{Mod}_eval_comb`, `{Mod}_eval_{clockName}`.
- `TopCounter.h` / `TopCounter.cpp`: `TopCounter.h`가 `#include "Counter.h"`를 포함하고, `TopCounter_state`가 **임베디드** `Counter_state` 멤버(예: 인스턴스별 stable 이름)를 포함한다.
- Parent의 `{TopCounter}_eval_*` 구현은 해당 임베디드 `Counter_state`에 대해 `{Counter}_initialize` / `{Counter}_set_*` / `{Counter}_eval_comb` / `{Counter}_eval_{clock}`를 **적절한 순서로** 호출해 자식을 구동한다.

**현재 상태**

- 위 조합은 **현재 testable하지 않다** (single-target export, composition codegen 부재). M3에서 parent/child 동시 산출과 wiring이 구현되면 이 fixture가 **acceptance의 최소 통과선**이 된다.

### Step 2: Sibling Reuse Fixture Shape (One Parent, Two Child Instances)

**역할**

- **Parent `DualCounter`**: 동일한 `Counter` 모듈을 **두 인스턴스**(`counter_a`, `counter_b`)로 붙인다.
- **검증 의도**: 동일 `Counter` artifact가 **한 번만** 정의되고 링크 시 **이름/상태 충돌 없이** 재사용되는지, 각 인스턴스가 **독립 state**를 갖는지 확인한다.

**MLIR 모듈 시그니처 (본문 생략, 예시)**

```mlir
hw.module @Counter(in %clk : i1, out %count : i32) { ... }
hw.module @DualCounter(in %clk : i1, out %count_a : i32, out %count_b : i32) { ... }
```

**M3가 존재할 때 기대하는 artifact 형태 (계약 관점)**

- `Counter.*`는 Step 1과 **동일한** per-module contract를 유지한다.
- `DualCounter_state`에는 **두 개의** 임베디드 `Counter_state` 멤버가 있다 (인스턴스별로 구분되는 필드명). 각 인스턴스에 대해 `{Counter}_eval_*` 호출 시 **서로 다른** `Counter_state*`가 전달되어야 한다.

**현재 상태**

- sibling reuse는 **설계 fixture**이며 **현재 testable하지 않다**. M3 acceptance target으로, “동일 child 모듈 다중 인스턴스”가 contract를 깨지 않는지의 최소 검증 단위다.

### Step 3: Negative Fixture (Cyclic Module Dependency)

**시나리오**

- **Module A**가 **Module B**를 인스턴스하고, **Module B**가 다시 **Module A**를 인스턴스하는 **순환 의존** (예: `A` → `B` → `A`).

**MLIR 모듈 시그니처 (본문 생략, 예시)**

```mlir
hw.module @A(...) { ... }  // 내부 @B
hw.module @B(...) { ... }  // 내부 @A
```

**기대 동작 (계약)**

- 이 조합은 **지원하지 않는다**. 실패는 **codegen 시점이 아니라 export(또는 export 직전의 구조 검증) 시점**에서 **거부(reject)**되어야 한다. 호스트가 잘못된 순환 그래프를 받아 C를 생성한 뒤 런타임에서 터지는 형태는 피한다.

**M3 수용 기준**

- M3는 **cycle detection**을 구현하고, 순환이면 **명확한 진단과 함께 export를 중단**한다. 이 negative fixture는 그 **M3 acceptance target**이다.

**현재 상태**

- 순환 검출/거부는 **현재 end-to-end로 검증할 수 없다** (composition export 경로 부재). 본 절은 요구사항을 문서에만 고정한다.

---

## M2 Acceptance Tests

> **Single-target 제약 주의:** 현재 export 경로는 `main.cpp`에서 단일 `hw.module`을 선택해 `buildModuleModel()` → `CModelEmitter.emit()` → `.h`/`.cpp` 한 쌍을 생성하는 구조다. Multi-module composition codegen은 존재하지 않는다. 따라서 아래 acceptance 중 **Documentation**과 **Golden Artifact**만 M2 closure 조건이며, Integration Fixture와 Spike는 M3 이후의 검증 대상이다.

### Documentation Acceptance

M2 closure에 필요한 contract 문서 섹션 체크리스트. **모든 항목이 존재하고 상호 모순 없이 일관되어야** M2를 닫을 수 있다.

- [ ] Per-Module Artifact Set (files, symbols, not-promised)
- [ ] Naming Contract (file naming, type naming, collision policy)
- [ ] Include Contract (header independence, G2G direction, host expectations)
- [ ] Ownership And Lifetime Contract (state ownership, initialization, mutation rules)
- [ ] Reusable Export-Top Contract (current export top, re-export expectations)
- [ ] Minimal Composition Fixtures (two-module, sibling, negative)
- [ ] M2 Closure Rule이 frozen 상태이며 다음 executor가 읽을 수 있다
- [ ] Single-target export 제약이 문서 내 최소 3곳에서 명시적으로 보인다

### Golden Artifact Acceptance

**현재 single-module export 경로**에서 관찰 가능한 항목만 검증한다. 이것이 M2에서 runtime으로 확인할 수 있는 전부다.

- [ ] Single module export가 `{moduleName}.h` + `{moduleName}.cpp`를 생성한다 (`--export-cmodel`로 관찰 가능)
- [ ] Header에 `{Mod}_state`, `{Mod}_initialize`, `{Mod}_eval_comb` 선언이 포함된다
- [ ] Implementation이 `{Mod}.h`를 include한다 (현재 `headerSuffix` 기본값)
- [ ] Include guard가 `{Mod}_MODEL_H` 패턴을 따른다 (원본 대소문자 보존)
- [ ] `extern "C"` guard가 존재한다

**이것이 M2에서 가능한 유일한 golden artifact 검증이다.** Export 경로가 single-target이므로, multi-module artifact 형태 검증은 M3로 이관된다.

### Integration Fixture Acceptance

**M3 구현 배치의 finish line — M2 closure 조건이 아니다.**

- [ ] Two-module fixture가 컴파일·링크된다 (parent가 child API를 호출)
- [ ] Sibling reuse fixture: 동일한 child artifact를 두 인스턴스가 독립 state로 사용
- [ ] Negative fixture: cyclic dependency가 export 시점에서 reject됨

> 이 체크리스트는 M3 구현 배치의 finish line이며, M2 closure 조건이 아니다.

### Spike Acceptance

**Optional — M2 closure 조건이 아니다.**

- [ ] Spike를 시도한 경우: spike 코드는 별도 디렉토리에 위치한다 (예: `experiments/m2-spike/`)
- [ ] Spike는 production 코드를 수정하지 않는다
- [ ] Spike 결과는 문서화되지만, M2 closure 판정에 영향을 주지 않는다
- [ ] Spike는 명시적으로 "non-contract, non-closure, M3 de-risking only"로 레이블된다

---

## Prototype / Spike Boundary

### Why M2 Cannot Pretend The Runtime Exists

현재 export 경로를 정확히 짚는다:

1. `main.cpp`에서 `--top` 또는 walk order 마지막으로 **단일 `hw.module`을 선택**한다.
2. `buildModuleModel(target_hw)` → 해당 모듈의 `SemanticModel`을 생성한다.
3. `CModelEmitter.emit()` → 단일 `.h`/`.cpp` 쌍을 출력한다.

이 경로에는:

- **Child module 순회(iteration)가 없다.** 부모의 instance를 따라 자식 module을 찾아 내려가는 코드가 없다.
- **Composition codegen이 없다.** 부모가 자식의 `_initialize`/`_eval_*`를 호출하는 코드를 자동 생성하지 않는다.
- **Parent→child `#include` 생성이 없다.** Parent header에 child header를 include하는 코드를 emit하지 않는다.

**따라서:** per-module composition contract를 M2에서 runtime으로 **완전히** 검증하는 것은 불가능하다. M2는 **문서 합의(document agreement)**로 닫히며, runtime proof는 M3에서 시작한다.

### Documentation-Level Closure

Contract가 full hierarchy codegen 없이도 닫힐 수 있는 이유:

1. **Per-module contract는 single-module export로 관찰 가능하다.** Naming, symbols, include guard, ownership 규칙은 현재 단일 모듈 export에서 이미 확인된다.
2. **Composition 방향은 설계 계약이다.** "parent가 child를 include하고, parent가 child state를 embed한다"는 방향 결정은 runtime assertion이 아니라 design contract다.
3. **M1 baseline이 기반을 제공한다.** M1에서 frozen된 artifact contract와 CLI contract 위에, M2가 composition layer를 추가하는 구조다.
4. **M2는 M1의 per-module contract 위에 composition layer를 얹는다.** Single-module 계약이 유효하면, multi-module 조합 규칙은 그 위의 추가 계약으로 문서화할 수 있다.

### Optional M3 De-Risking Spike

M3 구현 리스크를 낮추기 위한 탐색적 작업을 M2 기간에 **선택적으로** 수행할 수 있다. 단, 아래 규칙을 반드시 지킨다:

- Spike 코드는 **`experiments/` 디렉토리** 아래에만 위치한다 (예: `experiments/m2-spike/`).
- `lib/`, `include/`, `tools/` 아래 파일을 **수정하지 않는다.**
- Spike 결과는 M3 planning의 입력으로 활용될 수 있으나, **M2 closure를 gate하지 않는다.**
- Spike acceptance 체크리스트는 위 "Spike Acceptance" 절에 정의되어 있다.

---

## Handoff To M3

### Inputs To M3

M3는 M2로부터 다음을 받는다:

| # | M2 산출물 | 설명 |
|---|-----------|------|
| 1 | Frozen per-module artifact contract | naming, symbols, layout, include, ownership 규칙 |
| 2 | Composition direction contract | parent가 child를 include하고, parent가 child state를 embed |
| 3 | 세 가지 composition fixture design | two-module, sibling reuse, negative/cyclic |
| 4 | Integration fixture acceptance checklist | M3 finish line |
| 5 | Current single-target export limitation | M3가 multi-module export로 확장해야 함 |

M3가 구현해야 할 항목:

1. **Module graph traversal** — 모든 `hw.module` instance를 walk한다.
2. **Instance DAG extraction 및 cycle detection** — 순환 의존을 export 전에 감지·거부한다.
3. **Per-child `CModelEmitter` invocation** — 각 child module에 대해 독립적으로 artifact를 생성한다.
4. **Parent artifact generation with child includes and embedded state** — parent header가 child header를 include하고, parent state가 child state를 embed한다.
5. **Eval orchestration** — parent의 `eval_*` 구현이 child의 `init`/`eval`을 topological order로 호출한다.

### Do Not Implement In M2

M2에서 **명시적으로 구현하지 않는** 항목:

- Hierarchy-preserving codegen
- Multi-module export in single invocation
- Parent→child `#include` generation in `CModelEmitter`
- Instance DAG extraction
- Cycle detection
- Eval orchestration across modules
- Top-level wrapper integration (M4 범위)

이 항목들은 M3(또는 M4) 범위이며, M2에서 이들을 구현하거나 시도하는 것은 milestone 경계 위반이다.

---

## M2 Closure Rule (Final)

### Closure Summary Table

| 항목 | Closure 반영 | 설명 |
|------|-------------|------|
| Documentation Acceptance | **포함** | M2 closure의 핵심 기준. 모든 contract 섹션이 존재하고 일관되어야 한다 |
| Golden Artifact Acceptance | **포함** | 현재 단일 모듈 export 경로에서 관찰 가능한 naming/layout 범위까지 확인한다 |
| Integration Fixture Acceptance | **미포함** | composition codegen이 없으므로 M3 입력으로 넘긴다 |
| Spike Acceptance | **미포함** | de-risking 용도이며 M2 closure 조건이 아니다 |

### M2 Done Statement

**아래 조건이 모두 참이면 M2는 closed다:**

1. Documentation acceptance 체크리스트가 완료되었다 (모든 항목 checked).
2. Golden artifact acceptance 체크리스트가 통과한다 (단일 모듈 export 관찰).
3. Per-module artifact contract, naming/include/ownership 규칙이 상호 모순 없이 일관된다.
4. Single-target 제약이 문서 내 **3곳 이상**에서 명시적으로 보인다.
5. M3 handoff 항목(Inputs To M3, Do Not Implement In M2)이 명시되어 있다.
6. M2 기간에 **implementation 코드**(`.cpp`, `.h`, `CMakeLists.txt` 등)가 수정되지 않았다. (Task descriptions에서 `Modify: this file`은 이 plan 문서를 의미한다. 구현 소스 수정은 M2 scope가 아니다.)

### M2 Not-Done Statement

**아래 중 하나라도 해당하면 M2는 아직 열려 있다:**

1. Documentation acceptance 항목 중 누락된 것이 있다.
2. Golden artifact 관찰 결과가 문서화된 contract와 모순된다.
3. M3 범위의 구현 작업이 M2에 혼입되었다.

### Verdict

M2 최종 판정은 아래 셋 중 하나다:

| Verdict | 의미 |
|---------|------|
| **`M2 closed`** | Done Statement 6개 조건 모두 충족. 다음 executor는 M3를 시작할 수 있다. |
| **`M2 verification-blocked`** | Golden artifact 관찰이 현재 환경에서 수행 불가능(예: 빌드 실패). Done Statement의 나머지 조건은 충족. 빌드 복구 후 재검증 필요. |
| **`M2 still open`** | Not-Done Statement 중 하나 이상 해당. 추가 작업 필요. |

> **다음 executor를 위한 지침:** 이 Closure Rule을 위에서 아래로 읽는다 → Done Statement 6개 항목을 하나씩 확인한다 → 모두 참이면 `M2 closed`를 선언한다 → 하나라도 아니면 Not-Done Statement를 확인해 어디가 열려 있는지 파악한다. 2분 이내에 판정 가능해야 한다.

---

## Completion Criteria

- per-module artifact contract is explicit
- naming/include/ownership rules are explicit
- minimal reusable export-top assumptions are explicit
- composition acceptance fixtures are defined
- documentation acceptance and optional spike acceptance are separated
- everything requiring real hierarchy codegen is handed to `M3`
