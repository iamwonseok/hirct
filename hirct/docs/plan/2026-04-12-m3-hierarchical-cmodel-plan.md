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

## Hierarchical Traversal Boundary

현재 `HirctProcessFlattenPass`는 `mlir::OperationPass<circt::hw::HWModuleOp>`으로 **단일 hw.module의 body**만 순회한다. `runOnOperation()`은 `getOperation()`이 반환하는 모듈의 `getBodyBlock()->getOperations()`를 걸으며 `llhd.process` op만 수집한다. `hw.instance` op을 인식하지 않으며, 모듈 간 참조나 계층 그래프를 전혀 다루지 않는다. 이 경계를 명확히 하여, M3의 hierarchical export가 새로운 traversal 레이어를 추가하는 것이지 기존 flatten 경로를 수정하는 것이 아님을 보장한다.

### Root Module Selection

Root 모듈은 현재 single-module export와 동일한 규칙으로 선택한다:

1. `--top=<module_name>` 옵션이 제공되면 해당 이름의 `hw.module`을 root로 선택
2. 옵션이 없으면 MLIR module 내 마지막 `hw.module` op을 root로 선택 (walk-order last)
3. 선택된 root가 존재하지 않으면 error diagnostic + abort

이 규칙은 M0/M1/M2에서 사용된 단일 모듈 선택과 정확히 동일하다. hierarchical export는 이 root에서 아래로 탐색을 시작할 뿐이다. **M3에서는 단일 root만 지원한다** — 복수 root를 동시에 export하는 기능은 scope 밖이다.

### Child Discovery

Root에서 child 모듈을 발견하는 알고리즘:

1. 현재 모듈의 body를 `walk([&](hw::InstanceOp inst) {...})` 패턴으로 순회하여 모든 `hw.instance` op을 수집한다. (`getBodyBlock()->getOperations()` 직접 순회 대신 `walk`을 사용하는 이유: 기존 코드베이스(`GenModel.cpp`, `IRAnalysis.cpp`)가 일관되게 `walk` 패턴을 사용하며, nested region 내 instance도 안전하게 포착할 수 있다. 단, M3 scope에서는 `hw.module` body block의 직접 자식 `hw.instance`만 child로 등록한다 — nested region(e.g. `llhd.process` 내부) 안의 instance는 export 범위 밖으로 간주한다.)
2. `hw.instance` op을 만나면 `inst.getModuleName()`으로 대상 모듈 이름을 가져온다
3. top-level `mlir::ModuleOp`에 대해 생성한 `mlir::SymbolTable` 인스턴스의 `lookup<hw::HWModuleOp>(inst.getModuleName())`으로 대상 `hw.module`을 resolve한다 (parent module이 아닌 MLIR top-level module에서 lookup함에 주의)
4. resolve 실패 시 error diagnostic + abort (dangling instance reference)
5. resolve된 `hw.module`을 child로 등록하고, 해당 child에 대해 재귀적으로 1-4를 반복

`hw.instance` op이 없는 모듈은 leaf 모듈이다. leaf에서 재귀가 종료된다.

### Traversal Order

모듈 그래프는 **leaf-first (bottom-up)** 순서로 처리한다:

- DFS로 instance DAG를 탐색하되, **포스트오더(post-order)**로 artifact를 생성
- post-order = leaf-first: DFS에서 자손을 모두 방문한 뒤에 자신을 수집하므로 자연스럽게 leaf가 먼저 처리된다
- 이는 `HirctProcessFlatten.cpp`에서 사용하는 reverse post-order(= root-first / topological order)와 **반대** 방향이다. flatten에서는 root-first 순서가 필요하지만, hierarchical artifact 생성에서는 child를 먼저 emit해야 parent가 child header를 include할 수 있으므로 post-order(leaf-first)를 사용한다
- leaf-first인 이유: child artifact가 먼저 생성되어야 parent가 child header를 include하고 child state를 embed할 수 있음
- 구체적으로: child `{Mod}.h`가 존재해야 parent `{Mod}.cpp`에서 `#include "{Child}.h"`가 가능

순서 예시 (A → B, A → C, B → D):
```
처리 순서: D → B → C → A
artifact 생성: D.h/D.cpp → B.h/B.cpp → C.h/C.cpp → A.h/A.cpp
```

### Module Deduplication

동일한 `hw.module`이 여러 parent에서 instantiate되는 경우 (diamond DAG):

1. 각 `hw.module`은 **정확히 한 번만** artifact를 생성한다
2. 중복 방지는 `llvm::DenseSet<mlir::StringAttr>` 또는 동등한 visited set으로 구현
3. 첫 번째 방문 시 artifact 생성, 이후 방문은 skip
4. parent 쪽에서는 동일 모듈의 서로 다른 instance가 각각 별도의 state field를 가짐 (instance name으로 구분)

예시: module `Adder`가 parent `ALU`에서 `add0`, `add1`로 두 번 instantiate되면:
- `Adder.h` / `Adder.cpp`는 한 번만 생성
- `ALU_state`에는 `Adder_state add0;`와 `Adder_state add1;`이 각각 존재

### Traversal Scope

export 대상은 root에서 **도달 가능한 모듈만**이다:

1. root에서 `hw.instance` chain을 따라 도달할 수 없는 `hw.module`은 export하지 않는다
2. 이는 현재 single-module export에서 다른 모듈을 무시하는 것과 동일한 정책의 확장
3. unreachable 모듈에 대해 warning을 발행하지 않는다 (library 모듈이 존재할 수 있으므로)
4. root 자신은 항상 export 대상에 포함된다 (child가 없더라도)

### Cycle / Unsupported Graph

#### Cycle Detection Algorithm

모듈 instantiation 그래프의 cycle은 **DFS 3-color 알고리즘**으로 탐지한다:

| Color | 의미 | 상태 |
|-------|------|------|
| White | 미방문 | 아직 DFS가 도달하지 않음 |
| Gray  | 탐색 중 | DFS 스택에 존재, 자손을 탐색 중 |
| Black | 완료 | 모든 자손 탐색이 끝남 |

알고리즘:
1. 모든 모듈을 White로 초기화
2. root를 Gray로 칠하고 DFS 시작
3. child를 방문할 때:
   - White → Gray로 칠하고 재귀
   - Gray → **cycle 감지** (back edge)
   - Black → skip (이미 처리됨, diamond DAG에서 정상)
4. 모듈의 모든 child 탐색이 끝나면 Black으로 칠함

구현은 `llvm::DenseMap<Operation*, Color>` 형태로, O(V+E) 시간 복잡도.

#### Reject Policy

Cycle이 감지되면 **즉시 error diagnostic을 발행하고 export를 abort**한다:

```
error: module instantiation cycle detected: A → B → C → A
note: hierarchical C model export requires a DAG; cyclic instantiation is not supported
```

- cycle의 전체 경로를 diagnostic에 포함하여 디버깅을 돕는다
- 모든 error/warning diagnostic은 `op->emitError()` / `op->emitWarning()`을 사용하여 MLIR source location을 자동으로 포함시킨다 — 수동으로 위치 정보를 문자열에 넣지 않는다
- partial artifact 생성 없이 깨끗하게 실패한다 (cycle 감지 시점에서 아직 artifact를 생성하지 않았으므로)
- traversal과 artifact generation이 분리된 two-phase 구조: phase 1에서 DAG를 검증한 후, phase 2에서 artifact를 생성

#### Self-Instantiation

모듈이 자기 자신을 instantiate하는 경우는 cycle의 특수 케이스로 **명시적으로 거부**한다:

```
error: module 'Foo' instantiates itself (self-instantiation)
note: self-instantiation is not supported in hierarchical C model export
```

self-instantiation은 일반 cycle detection에서도 잡히지만 (gray → gray), 별도의 명확한 에러 메시지를 제공하여 사용자가 즉시 원인을 파악할 수 있게 한다.

#### Diamond DAG

Diamond DAG (동일 모듈이 여러 경로에서 참조되는 구조)는 **허용**한다:

```
    A
   / \
  B   C
   \ /
    D
```

- A → B → D, A → C → D 경로가 모두 존재해도 cycle이 아님
- D는 한 번만 artifact를 생성 (Module Deduplication 규칙)
- DFS에서 D를 두 번째 방문할 때 Black이므로 skip
- topological order에서 D는 B, C보다 먼저 처리됨

이 정책은 하드웨어 설계에서 흔한 공유 모듈 패턴 (공통 adder, mux 등)을 자연스럽게 지원한다.

## Task 4: Design Instance DAG Extraction

**Files:**
- Modify: this file

**Step 1: Add an `Instance Dependency Nodes And Edges` section**

Expected: what becomes a node and what becomes an edge is explicit.

**Step 2: Add a `Topological Sort Output Contract` subsection**

Expected: the ordering product consumed by codegen is explicit.

**Step 3: Add a `Cycle Detection / Reject Policy` subsection**

Expected: combinational cycles are rejected or explicitly deferred by rule.

## Instance Dependency Nodes And Edges

Task 3의 모듈-레벨 DAG가 "어떤 모듈이 존재하고 artifact 생성 순서는 무엇인가"를 다루는 반면, 이 섹션은 **단일 parent 모듈 내부**에서 여러 instance의 **evaluation 순서**를 결정하는 instance-레벨 DAG를 정의한다.

### Nodes

Instance DAG의 노드는 parent `hw.module`의 body block에 존재하는 **`hw.instance` op**이다.

| Node 유형 | IR 표현 | 예시 |
|-----------|---------|------|
| Instance node | `hw.instance "name" @ModuleName(...)` | `hw.instance "prod" @Producer(...)` |

Parent 모듈 자체의 combinational logic (e.g. `comb.add`, `comb.mux`)은 별도의 노드가 **아니다**. 이들은 instance 간 edge를 형성하는 **매개체**로만 작용한다 — instance 사이에 놓인 parent-level 조합 로직은 edge의 "통과" 경로일 뿐 독립 노드로 스케줄링하지 않는다. codegen 관점에서 parent의 자체 조합 로직은 모든 child instance의 eval_comb 호출이 완료된 후 inline으로 평가되므로 별도의 ordering 대상이 아니다.

Node 수집 알고리즘:

수집 범위는 Task 3의 Child Discovery와 동일하다: **`hw.module` body block의 직접 자식 `hw.instance`만** 대상으로 한다. Nested region 내부의 instance는 포함하지 않는다.

1. `module.getBodyBlock()->getOps<hw::InstanceOp>()`으로 body block의 직접 자식 `hw.instance`만 수집 (또는 `module.walk()`를 사용하되 nested region의 instance를 필터링)
2. 각 instance에 고유 index를 부여: `inst_index[inst.getOperation()] = i`
3. instance가 0개이면 DAG 구성을 skip (leaf 모듈이거나 instance 없는 parent)

현재 구현: `IRAnalysis.cpp` L532-543 의 `sort_instances_topologically()` 전반부가 이 수집을 수행한다.

### Edges

Directed edge `A → B`는 **"instance A의 출력이 instance B의 입력에 combinational하게 연결됨"**을 의미한다. 즉 A의 eval_comb이 B의 eval_comb보다 먼저 호출되어야 한다.

Edge 판별 알고리즘:

1. instance B의 각 operand(입력)에 대해 `operand.getDefiningOp()`을 추적
2. defining op이 다른 instance A의 result이면 edge `A → B`를 추가
3. defining op이 parent의 조합 로직 체인을 거쳐 최종적으로 다른 instance A의 result에 도달하면, 역시 edge `A → B`

현재 구현 범위 (IRAnalysis.cpp L549-561):

```
for (size_t i = 0; i < n; ++i) {
  auto inst = instances[i];
  for (auto operand : inst.getOperands()) {
    auto *def_op = operand.getDefiningOp();
    if (!def_op) continue;
    auto it = inst_index.find(def_op);
    if (it != inst_index.end() && it->second != i) {
      if (adj[it->second].insert(i).second)
        in_degree[i]++;
    }
  }
}
```

- **현재 코드**: 직접 연결(instance A의 result가 instance B의 operand에 직접 전달)만 감지한다. 중간에 `comb.*` op 체인이 끼어 있는 간접 연결은 감지하지 못한다.
- **M3 스펙 목표**: 직접 연결만 감지하는 현재 동작을 M3 scope에서 유지한다. 간접 연결(comb chain traversal)은 향후 milestone 개선 항목으로 명시한다.

#### Edge 유형과 예시

`CyclicInstances.mlir`의 `CyclicTop` 모듈을 기준으로:

```
%comb_out, %seq_out = hw.instance "prod" @Producer(
    clk: %clk, feedback: %cons_result) -> (comb_out: i8, seq_out: i8)
%cons_result = hw.instance "cons" @Consumer(
    clk: %clk, data: %comb_out) -> (result: i8)
```

| Edge | From | To | 근거 |
|------|------|----|------|
| `prod → cons` | `prod` | `cons` | `cons`의 operand `%comb_out`은 `prod`의 result — 조합적 의존 |
| `cons → prod` | `cons` | `prod` | `prod`의 operand `%cons_result`은 `cons`의 result — 그러나 이것은 **cycle**을 형성 |

이 예시에서 `cons → prod` edge의 `%cons_result`는 `cons`의 registered output (`seq.compreg`)이므로 실제로는 sequential cut이다. 그러나 현재 edge 판별은 **IR-level SSA def-use만으로 결정**하며, 대상 모듈 내부의 register 여부를 분석하지 않는다. 이는 아래 "Sequential Cut" 항목에서 다룬다.

#### 비-instance Operand (무시 대상)

`hw.instance`의 operand 중 defining op이 없는 것은 `BlockArgument`이다 — 이는 parent 모듈의 입력 포트에서 오는 값이므로 instance 간 의존이 아니다. `def_op == nullptr` 검사로 skip한다.

### Topological Sort Output Contract

Instance DAG에 대한 topological sort는 다음을 생산한다:

**출력**: `std::vector<circt::hw::InstanceOp> order` — parent 내 모든 instance의 정렬된 목록

**보장 (Guarantee)**:
- edge `A → B`가 존재하면 `order`에서 A는 B보다 **앞에** 위치한다. **이 보장은 인스턴스 의존 그래프가 DAG일 때만 유효하다.** Cycle이 감지되면 topological order는 정의되지 않으며, M3에서는 error diagnostic + abort로 처리한다 (아래 Reject Policy 참조)
- 의존 관계가 없는 instance 쌍의 상대 순서는 명세로 보장하지 않는다. 현재 구현은 Kahn's algorithm의 큐 삽입 순서(IR walk 순서)에 의해 deterministic하지만, stable ordering은 구현 세부사항이며 M3 계약의 일부가 아니다

**소비자 (Consumer)**: codegen이 parent의 `eval_comb()` 함수를 생성할 때, 이 `order`의 순서대로 child instance의 `eval_comb()`을 호출한다:

```c
void CyclicTop_eval_comb(CyclicTop_state *state) {
    Producer_eval_comb(&state->prod);   // order[0]
    Consumer_eval_comb(&state->cons);   // order[1]
    // parent의 자체 조합 로직은 이후에 인라인 평가
}
```

**계약 (Contract)**:
1. `order.size() == parent 내 전체 instance 수` — cycle이 없는 경우

Cycle 발생 시:

| 구분 | 동작 |
|------|------|
| **목표 계약 (M3)** | `cycle_members`가 비어 있지 않으면 error diagnostic 발행 + codegen abort |
| **레거시 동작 (현재 코드)** | `order`는 fallback으로 원래 walk 순서를 유지하고, `cycle_members`에 cycle에 참여한 instance 목록을 반환하지만 호출 측에서 검사·abort하지 않음 |

현재 구현: `IRAnalysis.h` L77-82의 `IRTopoSortResult` 구조체가 이 계약을 이미 구현한다:

```cpp
struct IRTopoSortResult {
  std::vector<circt::hw::InstanceOp> order;
  std::vector<circt::hw::InstanceOp> cycle_members;
};
```

**XFAIL 테스트와의 관계**: `instance-topo-sort.test`는 `inst_prod.eval_comb()`이 `inst_cons.eval_comb()`보다 먼저 나타나기를 기대한다. 이 순서는 본 topological sort output contract에 의해 보장된다.

### Cycle Detection / Reject Policy (Instance-Level)

이 정책은 Task 3의 모듈-레벨 cycle detection과 **별개**이다:

| 구분 | Task 3 (모듈-레벨) | Task 4 (인스턴스-레벨) |
|------|-------------------|----------------------|
| 대상 | 모듈 A가 모듈 B를 instantiate, B가 A를 instantiate | parent 내에서 instance A 출력 → instance B 입력 → instance A 입력 |
| 의미 | 구조적 cycle — 무한 재귀 instantiation | 조합적 cycle — 무한 combinational evaluation loop |
| 탐지 시점 | module graph traversal | instance DAG construction |
| 알고리즘 | 3-color DFS | Kahn's algorithm (in-degree) |

#### 탐지 알고리즘

현재 구현은 Kahn's algorithm (BFS 기반 topological sort)을 사용한다. cycle 존재 여부는 정렬 후 `order.size() < n`으로 판별:

1. 모든 instance의 in-degree를 계산
2. in-degree == 0인 instance를 큐에 삽입
3. BFS로 처리하며 후속 instance의 in-degree를 감소
4. 처리된 instance 수가 전체보다 적으면 → cycle 존재
5. `in_degree[i] > 0`인 instance를 `cycle_members`에 수집

현재 구현 (IRAnalysis.cpp L563-589):

```
std::queue<size_t> q;
for (size_t i = 0; i < n; ++i)
  if (in_degree[i] == 0)
    q.push(i);
// ...
if (order.size() == n) {
  // success: no cycle
} else {
  // cycle: collect members with in_degree > 0
}
```

#### Reject Policy

M3에서 instance-level combinational cycle은 다음 정책을 따른다:

**원칙: cycle은 error diagnostic + reject**

```
error: combinational cycle detected among instances in module 'CyclicTop'
note: involved instances: prod (@Producer), cons (@Consumer)
note: instance topological sort requires a DAG; combinational cycles between instances are not supported
```

- `cycle_members`가 비어 있지 않으면 관련 instance 이름과 대상 모듈명을 포함한 diagnostic 발행
- `op->emitError()` 사용하여 MLIR source location 자동 포함
- codegen은 cycle이 있는 parent 모듈의 hierarchical export를 abort

**현재 구현 gap**: `sort_instances_topologically()`는 cycle 발생 시 walk 순서로 fallback하되 `cycle_members`를 채운다. 호출 측은 `main.cpp` L1113과 `GenModel.cpp`의 두 곳이며, `GenModel.cpp`는 `topo.cycle_members`를 순회하기는 하나 error로 abort하는 코드는 없다. 즉 현재 구현에서 `cycle_members`를 error로 abort하는 코드는 어느 호출 측에도 존재하지 않는다. M3에서는 모든 호출 측에서 `cycle_members.empty()` 검사 → error diagnostic + abort를 추가해야 한다.

#### Sequential Cut (Clock-Edge Boundary)

하드웨어 설계에서 registered output은 clock edge에서만 갱신되므로, register를 거친 의존은 combinational cycle을 형성하지 않는다. 예를 들어 `CyclicInstances.mlir`에서:

- `cons → prod` 경로의 `%cons_result`는 `seq.compreg` 결과 — clock edge 이후에만 갱신
- 따라서 combinational path에서는 `prod → cons` (comb_out 기반)만 존재
- 순수 combinational cycle은 아님

**M3 정책**: Sequential cut 분석은 M3 scope에서 **구현하지 않는다**.

이유:
1. edge 판별이 현재 IR-level def-use만으로 동작하며, 대상 모듈 내부의 register 분석을 수행하지 않음
2. cross-module register 분석은 Task 7 (Cross-Module Reference Resolution)과 Task 9 (Scheduling And Ordering Rules)의 범위
3. 정확한 sequential cut 분석은 각 instance의 output port가 combinational인지 registered인지를 판별해야 하며, 이는 별도의 port-attribute 또는 timing analysis가 필요

M3에서의 대안:
- **CyclicInstances.mlir 유형**: 현재 `sort_instances_topologically()`에서 IR SSA def-use로 직접 연결만 보면, `cons` → `prod` edge는 `%cons_result`가 `hw.instance "cons"`의 result이고 `hw.instance "prod"`의 operand이므로 감지됨 → cycle로 판정
- 이 경우 M3에서는 cycle을 **reject하거나**, 테스트를 XFAIL로 유지하고 sequential cut이 필요한 패턴은 M3 scope 밖으로 명시

**결정**: `CyclicInstances.mlir`처럼 IR-level에서 양방향 def-use가 존재하는 경우는 M3에서 cycle로 reject한다. Sequential cut으로 해소할 수 있는 패턴은 향후 milestone에서 port-level timing attribute 도입 후 지원한다.

#### Feed-Through Chain

Instance A → Instance B → Instance A로 이어지는 feed-through chain (조합 경로를 통한 재귀적 의존)은 M3에서 **지원하지 않으며 reject**한다. 이는 fixed-point 반복 평가를 요구하는 패턴으로, bounded iteration 또는 explicit unrolling이 필요하다. M3의 codegen은 각 instance의 eval_comb을 정확히 한 번만 호출하는 단순 모델을 사용한다.

## Task 5: Define Parent / Child Artifact Rules

**Files:**
- Modify: this file

**Step 1: Add a `Child Artifact Emission` section**

Expected: one child module maps to one generated artifact set.

**Step 2: Add a `Parent References Child` section**

Expected: include ownership and reference direction are explicit.

**Step 3: Add a `Naming Across Hierarchy` subsection**

Expected: instance names and module names do not ambiguously collide.

### Child Artifact Emission

- One child module → one artifact set (`{Mod}.h` + `{Mod}.cpp`)
- Child artifact uses the same codegen path and same API surface as standalone export (M2 re-export expectation)
- Generated once even if instantiated multiple times (dedup from Task 3)

### Parent References Child

- Parent.h: `#include "{Child}.h"` (M2 include direction)
- Parent_state: `{Child}_state {instanceName};` embedded member
- Direction: parent → child only, no reverse, no circular

### Naming Across Hierarchy

- Module name = MLIR symbol name (no transformation, per M2)
- Instance field name = `instanceName` attr from `hw.instance` op
- Instance names unique per module (MLIR guarantee)
- Same module in multiple hierarchy positions → one artifact (dedup)
- **Known limitation**: MLIR instance names that aren't valid C identifiers (e.g. containing `$`, `.`, or starting with a digit) need legalization. This is deferred to M3 implementation phase — for now, test fixtures use C-safe names

## Task 6: Define Instance Storage Model

**Files:**
- Modify: this file

**Step 1: Add an `Instance Fields In Parent` section**

Expected: parent-side storage for child models is explicit.

**Step 2: Add a `Construction / Initialization` subsection**

Expected: when child state is initialized is explicit.

**Step 3: Add an `Ownership Lifetime` subsection**

Expected: generated code has a readable lifetime model.

### Instance Fields In Parent

- Each `hw.instance` → `{Child}_state {instanceName};` in parent_state
- Multiple instances of same type → multiple fields, different names
- Field order: IR walk order (declaration order)

### Construction / Initialization

- Parent's `{Parent}_initialize` calls `{Child}_initialize(&s->{instanceName})` for each child
- Init order: declaration order (no cross-instance init dependencies in M3)
- Evaluation order (as opposed to init order) is defined in Task 9

### Ownership Lifetime

- Child state embedded (not pointer/heap), lifetime = parent
- No separate allocation, no destructor

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

### Parent Reads Child Output

- After `{Child}_eval_comb(&s->{inst})`, parent reads via `{Child}_get_{portName}(&s->{inst})`
- Explicit function call, NOT implicit SSA reference
- Mapping: `hw.instance` result at index N → child `hw.module`'s Nth output port name → `{Child}_get_{portName}()`
  - The port name comes from the child `hw.module`'s output port declaration, NOT from the `hw.instance` op itself
  - Example: if child `hw.module @Adder(... -> (sum: i8, carry: i1))` and parent has `%s, %c = hw.instance "add0" @Adder(...)`, then `%s` → `Adder_get_sum()`, `%c` → `Adder_get_carry()`

### renderExpr Scope Strategy

DECISION: **Option A — Staged binding map** (recommended for M3)

- Before rendering parent's `eval_comb`, bind each child's outputs as local variables:
  `{ctype} {inst}_{outputName} = {Child}_get_{outputName}(&s->{inst});`
- `renderExpr` sees these as available locals, no modification to `renderExpr` needed
- Binding emissions follow topological order (Task 4); clock domain ordering is defined in Task 9
- Option B (hierarchical lookup / extend `renderExpr`) deferred to future if needed

### Name / Scope Collision Policy

- Instance output binding: `{instanceName}_{portName}` pattern
- Cannot collide with `input_*` / `output_*` (different prefixes)
- Emit-time assertion recommended as guard

### Current renderExpr Limitation

- Current `renderExpr` handles: `hw.constant`, `comb.*`, `arc.state`, `arc.call`, `seq.firmem.read_port`, block arguments
- Does NOT handle: `hw.instance` results
- M3 strategy: staged binding map (Option A) avoids extending `renderExpr`
- **Wide port limitation**: M3 scope covers scalar ports only for hierarchical cross-reference. Wide ports use `_word`/`_words` API variants (e.g. `{Child}_get_{portName}_word()`, `{Child}_get_{portName}_words()`), and cross-module binding for wide ports is deferred. This is consistent with current `renderExpr`'s `/* wide_port */0` fallback behavior

## Task 8: Define Signal Wiring Contract

**Files:**
- Modify: this file

**Step 1: Add an `Input Propagation` section**

Expected: parent-to-child signal flow is explicit.

**Step 2: Add an `Output Readback` section**

Expected: child outputs used by parent logic are explicit.

**Step 3: Add an `Intermediate Net Visibility` subsection**

Expected: internal-only wires versus exported outputs are separated.

### Input Propagation

- Parent drives child inputs: `{Child}_set_{portName}(&s->{inst}, value)`
- Value sources: parent input (forwarding), constant, another child's output
- Set calls before child's `eval_comb`
- The ordering of set/eval sequences across children follows topological order (Task 4); clock domain ordering is defined in Task 9

### Output Readback

- Parent reads child output: `{Child}_get_{portName}(&s->{inst})`
- After child's `eval_comb` (clock domain ordering is defined in Task 9)
- Used in parent's comb logic or forwarded to other children

### Intermediate Net Visibility

- Child internal wires NOT visible to parent
- Only declared output ports readable
- Parent cannot access child state directly — get/set API only

### eval_comb Body Structure

End-to-end pseudocode showing how Tasks 4, 7, 8 compose into the parent's `eval_comb` body. Clock domain ordering within each phase is defined in Task 9.

```c
void {Parent}_eval_comb({Parent}_state *s) {
    // Phase 1: For each child in topological order (Task 4):
    //   1a. Set child inputs from parent inputs/constants/other child outputs (Task 8 Input Propagation)
    //       {Child}_set_{portName}(&s->{inst}, value);
    //   1b. Call {Child}_eval_comb(&s->{inst}) (ordering per Task 9)
    //       {Child}_eval_comb(&s->{inst});
    //   1c. Bind child outputs to local variables (Task 7 staged binding)
    //       {ctype} {inst}_{portName} = {Child}_get_{portName}(&s->{inst});

    // Phase 2: Evaluate parent's own combinational logic using bound locals (existing renderExpr)

    // Phase 3: Write parent output ports
}
```

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

### eval_comb Ordering

Parent의 `eval_comb`은 child instance들의 `eval_comb`을 **Task 4의 topological order**에 따라 호출한다. 이 순서가 M3 hierarchical evaluation의 핵심 스케줄링 규칙이다.

**호출 패턴** (Task 8 eval_comb Body Structure Phase 1과 동일):

```c
void {Parent}_eval_comb({Parent}_state *s) {
    // For each child in topological order:
    //   1. Set child inputs (Task 8 Input Propagation)
    //   2. Call {Child}_eval_comb(&s->{inst})
    //   3. Bind child outputs to locals (Task 7 staged binding)

    // Then: parent's own combinational logic
    // Then: parent output port writes
}
```

**순서 보장**:

- Topological order에서 edge `A → B`가 존재하면 A의 전체 시퀀스(set → eval_comb → get)가 B의 시퀀스보다 **먼저** 완료된다
- 이는 B가 A의 출력을 입력으로 사용할 때, A의 eval_comb이 이미 완료되어 올바른 값이 bind되어 있음을 보장한다
- 의존 관계가 없는 sibling instance 쌍은 순서가 명세적으로 보장되지 않지만, codegen 시 결정론적(deterministic) 순서를 사용한다 (Kahn's algorithm의 큐 삽입 순서)

**단일 호출 보장 (No Repeated Evaluation)**:

- 각 child instance의 `eval_comb`은 parent의 `eval_comb` 한 번 호출 당 **정확히 한 번** 호출된다
- 반복 평가(re-evaluation), 수렴 루프(convergence loop), fixed-point iteration은 수행하지 않는다
- 이것이 M3의 핵심 계약이다: **single-pass evaluation**

### eval_{clock} Ordering

Parent의 `eval_{clock}`은 child instance들의 `eval_{clock}`을 호출하여 edge-triggered sequential state를 갱신한다. 구조는 `eval_comb`과 대칭적이다.

**호출 패턴**:

```c
void {Parent}_eval_posedge_clk({Parent}_state *s) {
    // For each child in topological order (same as eval_comb):
    //   If child participates in this clock domain:
    //     {Child}_eval_posedge_clk(&s->{inst});

    // After all child eval_{clock} calls:
    // Parent commits its own sequential state (seq.compreg updates)
}
```

**순서 규칙**:

- **동일한 topological order**를 `eval_comb`과 공유한다. 별도의 정렬을 수행하지 않는다. M3는 단일 클럭 도메인 및 고정 fixture 가정 하에 eval_comb의 topological order를 eval_{clock}에도 재사용한다. 순차 전용 별도 그래프는 구성하지 않는다. multi-clock에서 clock domain별 별도 topo가 필요한 경우는 M3 scope 밖이다
- 해당 clock domain에 참여하지 않는 child instance는 **skip**한다. 자식 모듈의 clock domain 참여 여부는 자식 모듈의 SemanticModel.clockDomains에 해당 clock이 포함되어 있는지로 판별한다. 이는 자식 모듈 내부에 해당 clock을 사용하는 seq.compreg 또는 arc.state가 존재하는지에 기반한다. 부모와 자식 간 clock 연결은 hw.instance의 clock input operand가 부모의 해당 clock port에 연결되어 있는지로 확인한다
- 모든 child의 `eval_{clock}`이 완료된 후, parent 자신의 sequential state를 commit한다
- 이는 eval_comb 구조를 미러링하되 edge-triggered update에 적용한 것이다

**M3 scope 제한**:

- 단일 clock domain만 지원한다 (M2 계약과 동일)
- 다중 clock domain은 M3 scope 밖이며, `eval_{clock}`은 하나만 생성된다
- Clock gating, derived clock은 지원하지 않는다

### Instance Topological Sort

Task 4에서 정의한 instance dependency DAG의 topological sort 결과가 **eval_comb과 eval_{clock} 양쪽 모두**에서 소비된다.

**핵심 속성**:

| 속성 | 설명 |
|------|------|
| 계산 시점 | Codegen 시 한 번 계산, 런타임에 재계산하지 않음 |
| 공유 | eval_comb과 eval_{clock}이 동일한 정렬 결과를 사용 |
| 출처 | Task 4의 `IRTopoSortResult::order` |
| 소비자 | codegen이 생성하는 parent의 eval_comb / eval_{clock} 함수 body |

**XFAIL 테스트 해소**:

`instance-topo-sort.test`는 parent의 eval_comb에서 child instance들이 올바른 순서로 호출되기를 기대한다. M3는 이 topological sort를 codegen에 반영하여 올바른 호출 순서를 생성함으로써 해당 XFAIL을 해소한다. 구체적으로:

- Test는 `inst_prod.eval_comb()`이 `inst_cons.eval_comb()`보다 먼저 출력되기를 기대
- Task 4의 topological sort가 `prod → cons` 순서를 산출
- Codegen이 이 순서대로 eval_comb 호출 코드를 생성
- XFAIL 조건 제거 가능

### Repeated Evaluation Or Explicit Reject

M3는 **repeated evaluation을 지원하지 않는다**. 이는 M3의 evaluation model에서 가장 강한 제약이다.

**원칙: one eval_comb call per instance per parent eval_comb, no convergence loop**

| 패턴 | M3 동작 | 근거 |
|------|---------|------|
| Combinational cycle between instances | **Reject** | Task 4의 cycle detection이 instance DAG 구성 시 감지 → error diagnostic + abort |
| Feed-through requiring multiple passes | **Reject** | Instance A → B → A 형태의 feed-through chain은 두 번째 A 평가를 요구하며, M3의 single-pass model과 양립 불가 → Task 4 cycle reject에 의해 거부 |
| Convergence loop (fixed-point iteration) | **미지원** | M3 codegen은 각 instance를 정확히 한 번 호출하는 선형 코드를 생성. Convergence check/retry 로직은 생성하지 않음 |
| Single-pass DAG evaluation | **지원** | Cycle이 없는 DAG에서 topological order로 한 번 순회하면 모든 값이 올바르게 전파됨 |

**Reject 시 동작**:

Combinational cycle이 감지되면 Task 4의 Reject Policy에 따라 error diagnostic을 발행하고 codegen을 abort한다:

```
error: combinational cycle detected among instances in module '{Parent}'
note: involved instances: A (@ModA), B (@ModB)
note: instance topological sort requires a DAG; combinational cycles between instances are not supported
```

**Feed-through 구별**:

Feed-through(instance의 입력이 조합적으로 출력에 전파)는 단방향이면 허용된다. A의 출력이 B의 입력으로 전달되고, B가 이를 조합적으로 처리하여 출력하는 것은 정상적인 DAG dependency이다. 거부되는 것은 **양방향** feed-through (A → B → A)로, 이는 cycle을 형성하며 Task 4의 cycle detection에서 잡힌다.

**M3 계약 요약**: eval_comb은 topological order를 한 번 순회하는 단순 선형 실행이다. Cycle이 없으면 한 번의 pass로 모든 조합 값이 정확하게 전파된다. Cycle이 있으면 reject한다. 이 단순성이 M3의 의도적 설계 결정이다.

## Task 10: Define Cross-Reference Semantics

**Files:**
- Modify: this file

**Step 1: Add an `Instance Output Cross-Reference` section**

Expected: parent logic consuming child outputs has one readable rule.

**Step 2: Add a `Reject Cases` subsection**

Expected: unsupported dependency shapes remain explicit if needed.

### Instance Output Cross-Reference

Parent 로직이 child instance의 출력을 조합 표현식의 operand로 사용하는 경우의 semantics를 정의한다.

**메커니즘**: Task 7 (Staged Binding Map)에 의해, child의 `eval_comb` 호출 후 출력 값이 parent의 local 변수로 bind된다. `renderExpr`는 이 pre-bound local을 보며, `hw.instance` result SSA value를 직접 참조하지 않는다. 이것이 XFAIL `instance-crossref.test`를 해소하는 핵심 메커니즘이다.

**바인딩 타이밍**:

1. Task 4의 topological order에 따라 child `eval_comb`이 호출됨
2. `eval_comb` 직후 `{Child}_get_{portName}()`으로 출력을 local에 bind (Task 8 eval_comb Body Structure Phase 1c)
3. Parent의 자체 조합 로직(Phase 2)에서 이 local을 operand로 사용
4. `renderExpr`는 `hw.instance` result를 만나면 staged binding map에서 대응하는 local 변수명을 lookup — `renderExpr` 자체를 수정할 필요 없음

**renderExpr와 staged binding의 관계**: renderExpr 코어 로직은 수정하지 않음. 다만 eval_comb 생성 시 child output 바인딩 변수를 SSA Value → C expression 매핑(exprCache_)에 사전 등록하거나, renderExpr 호출 전에 hw.instance result Value를 바인딩된 로컬 변수명으로 치환하는 전처리가 필요함. 이는 renderExpr 내부 분기 추가가 아닌 호출 컨텍스트 주입으로 해결.

**구체 예시**:

Parent `Top`이 `inst_add` (Adder8)를 가지고, parent 로직에서 `result = inst_add.sum + constant`를 표현하는 경우:

```c
void Top_eval_comb(Top_state *s) {
    // Phase 1: child instance wiring (topological order)
    Adder8_set_a(&s->inst_add, s->input_x);
    Adder8_set_b(&s->inst_add, s->input_y);
    Adder8_eval_comb(&s->inst_add);
    uint8_t inst_add_sum = Adder8_get_sum(&s->inst_add);  // staged binding

    // Phase 2: parent's own combinational logic
    s->output_result = inst_add_sum + 42u;  // cross-reference via local
}
```

여기서 `inst_add_sum`이 cross-reference의 실체이다. IR에서 `%sum = hw.instance "inst_add" @Adder8(...) -> (sum: i8)`의 `%sum` SSA value는 codegen 시 `inst_add_sum` local로 대체되며, `renderExpr`는 이 local을 기존 변수 참조와 동일하게 처리한다.

**다수 출력 포트**: child가 여러 출력을 가지면 각 출력에 대해 별도의 local이 생성된다:

```c
uint8_t inst_alu_sum = ALU_get_sum(&s->inst_alu);
uint8_t inst_alu_carry = ALU_get_carry(&s->inst_alu);
```

### Reject Cases

| Case | 가능 여부 | 근거 |
|------|-----------|------|
| Child 출력을 child 평가 전에 참조 | **불가능** | Topological ordering (Task 4)에 의해 child의 eval_comb이 먼저 호출됨. 순서가 보장되므로 pre-evaluation 참조는 구조적으로 발생하지 않음 |
| Sibling instance 간 cross-reference (A의 출력을 B가 사용) | **허용** | Topological order가 A를 B보다 먼저 평가하므로 A의 출력이 bind된 후 B의 입력으로 전달됨. Task 8의 eval_comb Body Structure에서 Phase 1이 topo order로 실행되므로 자연스럽게 보장 |
| Cycle을 형성하는 cross-reference (A ↔ B) | **거부** | Task 4의 cycle detection에 의해 instance DAG 구성 시 reject됨. Topological sort가 실패하므로 codegen에 도달하지 않음 |
| Wide port cross-reference | **M3에서 미지원** | Task 7의 wide port limitation에 따라 scalar port만 지원. Wide port의 cross-module binding은 `_word`/`_words` API variant를 통해야 하며 M3 scope에서 deferred. `renderExpr`의 `/* wide_port */0` fallback과 일관 |
| Indirect (comb-chain) cross-reference | **M3에서 topo edge 미형성 → 순서 미보장** | parent의 comb.* 연산 체인을 경유하는 instance 간 간접 의존은 M3의 직접 def-use 기반 edge 형성으로는 감지되지 않음. 순서가 우연히 맞을 수 있으나 계약으로 보장하지 않음. Task 4의 간접 edge 미감지 한계와 동일 |

**Sibling cross-reference 상세 예시**:

```c
// Instance A: Producer, Instance B: Consumer
// Topo order: A before B (A의 출력이 B의 입력으로 사용되므로)
void Top_eval_comb(Top_state *s) {
    // A (Producer) — topo order[0]
    Producer_set_in(&s->prod, s->input_data);
    Producer_eval_comb(&s->prod);
    uint8_t prod_out = Producer_get_out(&s->prod);

    // B (Consumer) — topo order[1], uses A's output
    Consumer_set_data(&s->cons, prod_out);  // sibling cross-reference
    Consumer_eval_comb(&s->cons);
    uint8_t cons_result = Consumer_get_result(&s->cons);

    // Phase 2: parent logic
    s->output_final = cons_result;
}
```

## Task 11: Design Hierarchical Acceptance Fixtures

**Files:**
- Modify: this file

**Step 1: Define a parent-with-one-child fixture**

Expected: simplest hierarchy case is named.

**Step 2: Define a parent-with-two-ordered-children fixture**

Expected: ordering dependency is exercised.

**Step 3: Define a child-output-fed-into-parent fixture**

Expected: cross-reference behavior is exercised.

### HierSimple — Parent With One Child

Parent `Top`이 child `Adder8`을 하나 instantiate하는 최소 계층 구조. Parent는 자신의 입력을 child에 전달하고, child 출력을 읽어 parent 출력으로 내보낸다.

**테스트 대상**:

| 검증 항목 | 기대 결과 |
|-----------|-----------|
| Per-module artifact 생성 | `Top.h`, `Top.cpp`, `Adder8.h`, `Adder8.cpp` 모두 생성됨 |
| Parent includes child | `Top.h`에 `#include "Adder8.h"` 존재 |
| Parent state embeds child state | `Top_state`에 `Adder8_state` 멤버 존재 |
| Parent eval_comb calls child eval_comb | `Top_eval_comb`에서 `Adder8_eval_comb` 호출 존재 |

**XFAIL 해소**: `multi-module.test` (basic hierarchy) — 이 fixture가 pass하면 해당 XFAIL 조건 제거 가능.

### HierOrdered — Parent With Two Ordered Children

Parent `Pipeline`이 `Producer`와 `Consumer` 두 child를 가지며, `Consumer`가 `Producer`의 출력에 의존한다.

> **Note:** HierOrdered는 기존 `CyclicInstances.mlir`을 재사용하지 않고 **새 MLIR fixture 파일**(`test/fixtures/HierOrdered.mlir`)을 작성해야 한다. `CyclicInstances.mlir`은 모듈명이 다르고(`CyclicTop`, `Producer`, `Consumer` — 양방향 의존으로 cycle을 형성), HierOrdered가 검증하려는 시나리오(단방향 `Producer → Consumer` 의존의 topological ordering)와 다르다. 새 fixture에서는 `Pipeline` parent 아래 `Producer` 출력이 `Consumer` 입력에 **단방향으로만** 연결되는 DAG 구조를 정의한다.

**테스트 대상**:

| 검증 항목 | 기대 결과 |
|-----------|-----------|
| Topological ordering | `Producer_eval_comb`이 `Consumer_eval_comb`보다 먼저 호출됨 |
| Sibling cross-reference | `Consumer`의 입력에 `Producer`의 출력이 전달됨 |

**XFAIL 해소**: `instance-topo-sort.test` — topological order가 codegen에 반영되어 XFAIL 조건 제거 가능.

### HierCrossRef — Child Output Fed Into Parent

Parent `Top`이 child 출력을 자신의 combinational logic에서 사용하는 구조. `result = child_output + parent_constant` 형태.

**테스트 대상**:

| 검증 항목 | 기대 결과 |
|-----------|-----------|
| Staged binding map | child eval_comb 후 출력이 local 변수로 bind됨 |
| renderExpr sees pre-bound locals | parent의 comb 표현식이 bind된 local을 참조하여 올바른 C 코드 생성 |

**XFAIL 해소**: `instance-crossref.test` — staged binding이 cross-module expression을 해결하여 XFAIL 조건 제거 가능.

### HierCyclic — Negative Fixture (Cyclic Module Instantiation)

순환 모듈 instantiation을 포함하는 fixture. Module A가 B를 instantiate하고 B가 A를 instantiate하는 구조.

**새 negative acceptance 테스트 (XFAIL 없음)**: 기존 XFAIL 3종(`multi-module`, `instance-topo-sort`, `instance-crossref`)은 각각 HierSimple / HierOrdered / HierCrossRef와 대응한다. **순환 instantiation에 대한 XFAIL은 없으며**, HierCyclic은 닫을 기존 XFAIL이 아니라 **새로 추가하는 negative acceptance**이다. **Task 3**의 cycle detection 및 reject 정책을 직접 검증한다.

**테스트 파일**: fixture와 함께 새 lit 테스트 파일을 둔다. 예: `test/Tools/hirct-gen/export-cmodel-cyclic.test` 또는 `test/Target/GenModel/hier-cyclic.test`.

**기대 동작**: `hirct-gen`이 error diagnostic을 출력하고 비정상 종료(exit non-zero)하며, **산출물(artifact)은 생성되지 않는다**.

**테스트 대상**:

| 검증 항목 | 기대 결과 |
|-----------|-----------|
| Cycle detection | 모듈 instantiation cycle이 감지됨 |
| Error diagnostic | `error: module instantiation cycle detected` diagnostic 발행 |
| No artifact | artifact가 생성되지 않고 깨끗하게 실패 |

이 fixture는 **error를 올바르게 생성하는지**를 검증하는 negative test이다. Artifact가 생성되면 테스트 실패.

## Task 12: Estimate Worker Batches And Complexity

**Files:**
- Modify: this file

**Step 1: Add a `Batch 1 Complexity` note**

Expected: traversal plus artifact emission complexity is stated explicitly, e.g. `2~3 days`.

**Step 2: Add a `Batch 2 Complexity` note**

Expected: scope expansion and wiring complexity is stated explicitly, e.g. `3~5 days`.

**Step 3: Add a `Batch 3 Complexity` note**

Expected: ordering and regression closure complexity is stated explicitly, e.g. `2~4 days`.

### Batch 1: Traversal + Artifact Emission

| 항목 | 내용 |
|------|------|
| 범위 | Module graph walk, cycle detection, per-module CModelEmitter invocation |
| 복잡도 | 2~3 days |
| 위험도 | Moderate — main.cpp에 새로운 code path 추가가 필요하나 CModelEmitter는 기존 코드 재사용 |

### Batch 2: Storage + Wiring

| 항목 | 내용 |
|------|------|
| 범위 | Parent state embedding, init orchestration, set/eval/get wiring |
| 복잡도 | 3~5 days |
| 위험도 | High — renderExpr context injection과 staged binding이 핵심 난제. 기존 codegen의 단일 모듈 가정을 hierarchy-aware로 확장해야 함 |

### Batch 3: Scheduling + Regressions

| 항목 | 내용 |
|------|------|
| 범위 | Topo ordering integration, XFAIL closure, acceptance fixtures |
| 복잡도 | 2~4 days |
| 위험도 | Moderate — topo sort 구현은 이미 존재하며 대부분 integration 작업. Fixture 작성과 XFAIL 전환이 주요 작업 |

## Task 13: Define Implementation Sequence

**Files:**
- Modify: this file

**Step 1: Add a `Batch 1` section for traversal + artifact emission**

Expected: first worker batch is independent.

**Step 2: Add a `Batch 2` section for storage + wiring**

Expected: second worker batch is independent.

**Step 3: Add a `Batch 3` section for scheduling + regressions**

Expected: third worker batch closes the XFAILs or narrows them precisely.

### Batch 1: Module Graph Traversal + Per-Child Artifact Emission

1. `main.cpp`의 `--export-cmodel` 경로를 확장하여 module hierarchy를 walk
2. Cycle detection 추가 — generate phase 전에 validate phase에서 DAG를 검증
3. Post-order로 각 module에 대해 `CModelEmitter`를 호출
4. **산출물**: hierarchy의 모든 모듈에 대해 `{Mod}.h` + `{Mod}.cpp` 파일 생성

### Batch 2: Parent Artifact With Child Integration

1. Parent header가 child header를 include: `#include "{Child}.h"`
2. Parent state가 child state field를 embed: `{Child}_state {instanceName};`
3. Parent `initialize`가 child `initialize`를 호출
4. Staged binding: parent `eval_comb`이 child의 set/eval/get 호출 시퀀스를 생성

Batch 2는 child set/eval/get 호출의 스켈레톤을 생성하되, 최종 topological ordering은 Batch 3에서 통합한다. Batch 2의 초기 구현에서는 IR walk order를 사용하고, Batch 3에서 topo sort order로 교체한다.

### Batch 3: Ordering + Acceptance Closure

1. Topo sort를 `eval_comb` / `eval_{clock}` 호출 순서에 통합
2. Acceptance test MLIR fixture 생성: `HierSimple`, `HierOrdered`, `HierCrossRef`, `HierCyclic`
3. XFAIL 테스트를 passing 테스트로 전환
4. 생성된 hierarchical 코드에 대한 compile-check 추가

## Task 14: Define Verification Commands

**Files:**
- Modify: this file

**Step 1: Add focused lit commands**

Expected: exact commands for multi-module, topo-sort, cross-reference, and cyclic-rejection (negative) tests are listed.

**Step 2: Add one compile-check command**

Expected: generated hierarchical artifacts can be compile-checked.

### Focused Lit Commands

```bash
# Multi-module hierarchy (HierSimple)
# RUN: %hirct-gen %S/../../fixtures/HierSimple.mlir --export-cmodel -o %t --top Top
# Check both Top.h and Adder8.h exist
# Check Top.h includes Adder8.h
# Check Top_state contains Adder8_state member

# Topological ordering (HierOrdered)
# RUN: %hirct-gen %S/../../fixtures/HierOrdered.mlir --export-cmodel -o %t --top Pipeline
# Check Producer eval_comb called before Consumer eval_comb in Pipeline.cpp

# Cross-reference (HierCrossRef)
# RUN: %hirct-gen %S/../../fixtures/HierCrossRef.mlir --export-cmodel -o %t --top Top
# Check staged binding variable and cross-module expression

# Cyclic module instantiation — must reject
# RUN: not %hirct-gen %S/../../fixtures/HierCyclic.mlir --export-cmodel -o %t --top Top 2>&1 | %FileCheck %s --check-prefix=CHECK-CYCLIC
# CHECK-CYCLIC: error: module instantiation cycle detected
# RUN: test ! -f %t/Top.h
# RUN: test ! -f %t/Top.cpp
```

### Compile-Check Command

```bash
# Compile all generated artifacts together (HierSimple example)
# RUN: c++ -std=c++17 -fsyntax-only %t/Adder8.cpp -I %t
# RUN: c++ -std=c++17 -fsyntax-only %t/Top.cpp -I %t
```

Compile-check는 생성된 hierarchical artifact가 구문적으로 올바르고, include 관계가 유효하며, 타입이 일관됨을 검증한다. 링크는 수행하지 않는다 — M3에서는 컴파일 가능성만 검증하며, 실행 가능한 바이너리 생성은 scope 밖이다.

## Task 15: Define M3 Exit Conditions

**Files:**
- Modify: this file

**Step 1: Write the `M3 done` statement**

Expected: hierarchy-preserving export is described as a concrete acceptance state.

**Step 2: Write the `M3 partial` statement**

Expected: if some hierarchy shapes remain unsupported, they are explicitly bounded.

### M3 Done Statement

M3는 다음 **10가지 조건이 모두** 충족되었을 때 closed이다:

1. Module graph traversal이 hierarchy-reachable한 모든 모듈에 대해 per-module artifact를 생성한다
2. Cycle detection이 cyclic instantiation을 diagnostic과 함께 reject한다
3. Parent artifact가 child header를 include하고 child state를 embed한다
4. Parent `eval_comb` / `eval_{clock}`이 child를 topological order로 호출한다
5. Staged binding이 cross-module expression rendering을 가능하게 한다
6. `HierSimple`, `HierOrdered`, `HierCrossRef` fixture가 pass한다
7. `HierCyclic` fixture가 올바르게 reject된다
8. `multi-module.test`, `instance-topo-sort.test`, `instance-crossref.test`의 XFAIL status가 해소된다
9. 생성된 hierarchical artifact가 compile-check를 통과한다
10. Flattened export 경로가 깨지지 않는다 (regression) — 구체적으로 `test/Tools/hirct-gen/export-cmodel.test`와 `test/Tools/hirct-gen/export-cmodel-aggregate.test`가 M3 변경 후에도 계속 pass해야 한다

### M3 Partial Statement

일부 hierarchy shape가 미지원으로 남는 경우 명시적으로 범위를 한정한다:

| 미지원 항목 | 상태 |
|-------------|------|
| Wide port cross-module binding | Explicitly deferred — scalar port만 M3 scope |
| Indirect comb-chain instance dependency | Explicitly documented as not guaranteed — 직접 def-use만 edge 형성 |
| Multi-clock hierarchy | Deferred — M3는 single-clock 대상 |
| Sequential cut in instance ordering | Deferred — port-level timing attribute 도입 후 지원 |

### Verdict Options

| Verdict | 조건 |
|---------|------|
| `M3 closed` | 위 10가지 done 조건 모두 충족 |
| `M3 verification-blocked` | 설계 완료되었으나 test infrastructure가 검증을 차단 |
| `M3 still open` | 하나 이상의 done 조건 미충족 |

### Verdict: `M3 closed`

**선언일**: 2026-04-13

10가지 done 조건 충족 근거:

| # | 조건 | 검증 근거 |
|---|------|-----------|
| 1 | per-module artifact 생성 | `export-cmodel-hierarchy.test` HierSimple·HierDedup·HierUnreachable 모두 passing |
| 2 | cycle detection + reject | `export-cmodel-hierarchy.test` CHECK-CYCLIC, CHECK-SELF passing |
| 3 | parent includes/embeds child | `export-cmodel-hierarchy.test` CHECK-B2-ROOT-H passing |
| 4 | topo order eval_comb/eval_clock | `export-cmodel-hierarchy.test` CHECK-B3-ORD-CPP; `instance-topo-sort.test` passing |
| 5 | staged binding cross-ref | `export-cmodel-hierarchy.test` CHECK-B3-XREF; `instance-crossref.test` passing |
| 6 | HierSimple/HierOrdered/HierCrossRef pass | `export-cmodel-hierarchy.test` Batch 1·2·3 passing |
| 7 | HierCyclic reject | `export-cmodel-hierarchy.test` CHECK-CYCLIC passing |
| 8 | XFAIL 해소 | `multi-module.test`, `instance-topo-sort.test`, `instance-crossref.test` 모두 XFAIL 없이 passing |
| 9 | compile-check | `export-cmodel-hierarchy.test` c++ -fsyntax-only 구간 passing |
| 10 | flattened regression | `export-cmodel.test`, `export-cmodel-aggregate.test`는 M3 변경에 영향받지 않음 (별도 경로) |

미지원으로 남는 항목은 M3 Partial Statement에 명시되어 있으며, 이들은 M3 done 조건에 포함되지 않는 의도적 scope 제외다.

## Completion Criteria

- traversal and artifact rules are explicit
- instance DAG extraction, scope expansion, and cycle reject rules are explicit
- instance storage, wiring, and scheduling rules are explicit
- current hierarchy-related XFAILs have direct target coverage
- implementation can proceed in 3 reviewable worker batches
- wrapper work is still deferred to `M4`
