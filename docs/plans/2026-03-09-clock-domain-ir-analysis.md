# Clock Domain IR-Analysis 전면 교체 Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** `is_clock_port()` 이름 휴리스틱 의존을 제거하고, MLIR IR을 inter-module 재귀 탐색하여 레지스터 구동 수 기반으로 clock domain을 정확히 판별한다.

**Architecture:**
`build_clock_domain_map(module, mlir_module)` 에 `mlir::ModuleOp`를 추가하여 `SymbolTable` 기반 서브모듈 탐색을 가능하게 한다. pass-through 모듈(레지스터 없는 uart_top 같은 경우)은 `count_clock_registers_through_instances()` 재귀 함수로 각 i1 입력 포트가 하위 모듈을 통해 몇 개의 레지스터를 구동하는지 계산하고, 그 수가 많은 포트를 primary clock으로 선택한다. `is_clock_port()`는 GenDPIC의 always@ 선택 경로에서 완전히 제거되고, cdm_view_ 기반 경로로 통일된다. 다른 emitter(GenTB, GenVerify 등)는 단순 포트 필터링 용도로만 `is_clock_port()`를 유지하므로 삭제하지 않는다.

**Tech Stack:** MLIR/CIRCT (HWOps, SeqOps), `mlir::SymbolTable`, C++17

---

## 배경 — 실증된 루트 원인

```
UART_PCLK: is_clock_port = FALSE
  "uart_pclk".endswith("_clk")  → False  (_pclk ≠ _clk)
  _pclk suffix 조건이 is_clock_port()에 아예 없음

UART_CLK:  is_clock_port = TRUE
  "uart_clk".endswith("_clk")   → True
```

`uart_top`은 레지스터가 없어 `cdm_view_.domains`가 비어있고 `is_multi_clock=false`이므로,
`emit_sv()`의 단일 clock 경로에서 `is_clock_port()` 기반으로 `clock_name = "UART_CLK"`이 선택됨.
결과: `always @(posedge UART_CLK)` — 잘못된 primary clock.

---

## Task 1: `ClockDomainView`에 `reg_count` 필드 추가

**Files:**
- Modify: `hirct/include/hirct/Analysis/IRAnalysis.h:44-50`

**현재 코드 (44~50행):**
```cpp
struct ClockDomainView {
  std::string clock_port_name;
  unsigned clock_port_index;
  std::vector<RegisterView> registers;
  std::vector<circt::hw::InstanceOp> instances;
};
```

**Step 1: `reg_count` 필드 추가**

```cpp
struct ClockDomainView {
  std::string clock_port_name;
  unsigned clock_port_index = 0;
  unsigned reg_count = 0;          // primary clock 선택 기준 (구동 레지스터 수)
  std::vector<RegisterView> registers;
  std::vector<circt::hw::InstanceOp> instances;
};
```

**Step 2: 빌드 확인 (컴파일 오류 없어야 함)**

Run: `cmake --build /user/wonseok/project-iamwonseok/llvm-cpp-model/build --target hirct-gen -j$(nproc) 2>&1 | tail -5`
Expected: `[100%] Linking CXX executable hirct-gen` 또는 오류 없음

---

## Task 2: `build_clock_domain_map()` 시그니처 변경

**Files:**
- Modify: `hirct/include/hirct/Analysis/IRAnalysis.h:56`
- Modify: `hirct/lib/Analysis/IRAnalysis.cpp:182` (함수 정의)

**Step 1: 헤더 시그니처 변경**

변경 전:
```cpp
ClockDomainMapView build_clock_domain_map(circt::hw::HWModuleOp module);
```

변경 후:
```cpp
ClockDomainMapView build_clock_domain_map(
    circt::hw::HWModuleOp module,
    mlir::ModuleOp mlir_module);
```

`mlir/IR/BuiltinOps.h`는 `IRAnalysis.h`에 이미 include되어 있음 (`#include "mlir/IR/BuiltinOps.h"`).

**Step 2: IRAnalysis.cpp 함수 정의 시그니처 변경 (구현은 Task 3에서)**

변경 전:
```cpp
ClockDomainMapView build_clock_domain_map(circt::hw::HWModuleOp module) {
```

변경 후:
```cpp
ClockDomainMapView build_clock_domain_map(
    circt::hw::HWModuleOp module,
    mlir::ModuleOp mlir_module) {
```

**Step 3: 호출자 일괄 수정 (컴파일 오류 확인용)**

`build_clock_domain_map`의 모든 호출자:
- `hirct/lib/Target/GenDPIC.cpp:36` — 생성자
- `hirct/lib/Target/GenModel.cpp:163` — emit_cpp
- `hirct/lib/Target/GenModel.cpp:326` — emit_step
- `hirct/lib/Target/GenModel.cpp:1152` — child_cdm (symbol_table_ 컨텍스트 내)

각 호출을 다음과 같이 수정:

```cpp
// GenDPIC.cpp:36 — 생성자 (Task 4에서 mlir_module_ 멤버 추가 후 사용)
// 임시로 dummy: 실제 수정은 Task 4에서
```

**GenModel.cpp 3곳 수정:**

```cpp
// 163행
auto cdm = hirct::build_clock_domain_map(hw_module_, mlir_module_);

// 326행
auto cdm = hirct::build_clock_domain_map(hw_module_, mlir_module_);

// 1152행 (symbol_table_->lookup으로 얻은 child_mod)
auto child_cdm = hirct::build_clock_domain_map(child_mod, mlir_module_);
```

**Step 4: 빌드해서 남은 컴파일 오류 목록 확인**

Run: `cmake --build /user/wonseok/project-iamwonseok/llvm-cpp-model/build --target hirct-gen -j$(nproc) 2>&1 | grep "error:" | head -20`

---

## Task 3: `count_clock_registers_through_instances()` 구현

**Files:**
- Modify: `hirct/lib/Analysis/IRAnalysis.cpp` — anonymous namespace에 추가 (현재 `trace_clock_to_port` 뒤)

`trace_clock_to_port` 뒤, `} // namespace` 닫는 줄 바로 앞에 다음 함수를 삽입:

```cpp
// 특정 모듈의 port_idx번째 입력이 clock으로 구동하는 레지스터 수를 재귀 탐색으로 계산.
// 순환 참조 방지를 위해 visited_modules로 방문한 모듈 이름을 추적.
static unsigned count_clock_registers_through_instances(
    circt::hw::HWModuleOp module,
    unsigned port_idx,
    mlir::SymbolTable &sym_table,
    int depth,
    llvm::SmallPtrSet<mlir::Operation *, 16> &visited) {
  if (depth > 8)
    return 0;
  // 이미 방문한 모듈은 건너뜀 (순환 참조 방지)
  if (!visited.insert(module.getOperation()).second)
    return 0;

  auto *body = module.getBodyBlock();
  if (!body || port_idx >= body->getNumArguments())
    return 0;

  mlir::Value port_val = body->getArgument(port_idx);
  unsigned total = 0;

  for (auto *user : llvm::make_early_inc_range(port_val.getUsers())) {
    // 직접 seq.to_clock → 레지스터 수 카운트
    if (auto to_clk = mlir::dyn_cast<circt::seq::ToClockOp>(user)) {
      mlir::Value clk_val = to_clk.getResult();
      for (auto *clk_user : clk_val.getUsers()) {
        if (mlir::isa<circt::seq::CompRegOp, circt::seq::FirRegOp>(clk_user))
          ++total;
      }
      continue;
    }
    // hw.instance의 operand로 전달 → 서브모듈 재귀 탐색
    auto inst = mlir::dyn_cast<circt::hw::InstanceOp>(user);
    if (!inst)
      continue;
    for (unsigned op_idx = 0; op_idx < inst.getNumOperands(); ++op_idx) {
      if (inst.getOperand(op_idx) != port_val)
        continue;
      auto sub_mod = sym_table.lookup<circt::hw::HWModuleOp>(
          inst.getModuleName());
      if (!sub_mod)
        break; // hw.module.extern(블랙박스) — 탐색 불가
      total += count_clock_registers_through_instances(
          sub_mod, op_idx, sym_table, depth + 1, visited);
      break;
    }
  }
  return total;
}
```

**Step 0: IRAnalysis.cpp 상단에 include 추가 (없으면)**

`llvm::SmallPtrSet`와 `llvm::make_early_inc_range`를 위해 IRAnalysis.cpp 상단 include 블록에 추가:
```cpp
#include "llvm/ADT/SmallPtrSet.h"
```
(이미 있으면 생략)

**Step 1: 위 함수를 삽입하고 빌드 확인**

Run: `cmake --build /user/wonseok/project-iamwonseok/llvm-cpp-model/build --target hirct-gen -j$(nproc) 2>&1 | grep "error:" | head -20`
Expected: 오류 없음

---

## Task 4: `build_clock_domain_map()` 구현 교체 (Step B — pass-through 처리)

**Files:**
- Modify: `hirct/lib/Analysis/IRAnalysis.cpp:182~215` (현재 구현 전체)

**현재 구현:**
```cpp
ClockDomainMapView build_clock_domain_map(circt::hw::HWModuleOp module) {
  ClockDomainMapView result;
  auto ports = get_ports(module);
  auto registers = collect_registers(module);

  std::map<unsigned, ClockDomainView> domain_map;
  for (auto &reg : registers) {
    unsigned port_idx = trace_clock_to_port(reg.clock);
    auto &domain = domain_map[port_idx];
    if (domain.clock_port_name.empty() && port_idx < ports.size()) {
      domain.clock_port_name = ports[port_idx].name;
      domain.clock_port_index = port_idx;
    }
    domain.registers.push_back(reg);
  }

  module.walk([&](circt::hw::InstanceOp inst) {
    for (auto operand : inst.getOperands()) {
      unsigned port_idx = trace_clock_to_port(operand);
      if (port_idx != ~0u && domain_map.count(port_idx)) {
        domain_map[port_idx].instances.push_back(inst);
        break;
      }
    }
  });

  for (auto &[idx, domain] : domain_map)
    result.domains.push_back(std::move(domain));

  result.is_multi_clock = result.domains.size() > 1;
  return result;
}
```

**교체할 구현:**
```cpp
ClockDomainMapView build_clock_domain_map(
    circt::hw::HWModuleOp module,
    mlir::ModuleOp mlir_module) {
  ClockDomainMapView result;
  auto ports = get_ports(module);
  auto registers = collect_registers(module);

  // Step A: 현재 모듈 내 레지스터 기반 분석 (기존 방식 유지)
  std::map<unsigned, ClockDomainView> domain_map;
  for (auto &reg : registers) {
    unsigned port_idx = trace_clock_to_port(reg.clock);
    auto &domain = domain_map[port_idx];
    if (domain.clock_port_name.empty() && port_idx < ports.size()) {
      domain.clock_port_name = ports[port_idx].name;
      domain.clock_port_index = port_idx;
    }
    domain.reg_count++;
    domain.registers.push_back(reg);
  }

  // Step B: 레지스터 없는 pass-through 모듈 처리
  // (uart_top처럼 hw.instance만 있고 직접 레지스터가 없는 경우)
  if (domain_map.empty()) {
    mlir::SymbolTable sym_table(mlir_module);
    for (unsigned port_idx = 0; port_idx < ports.size(); ++port_idx) {
      const auto &port = ports[port_idx];
      if (!port.is_input || port.width != 1)
        continue;
      llvm::SmallPtrSet<mlir::Operation *, 16> visited;
      unsigned cnt = count_clock_registers_through_instances(
          module, port_idx, sym_table, /*depth=*/0, visited);
      if (cnt > 0) {
        ClockDomainView domain;
        domain.clock_port_name = port.name;
        domain.clock_port_index = port_idx;
        domain.reg_count = cnt;
        domain_map[port_idx] = std::move(domain);
      }
    }
  }

  // instance 연결 정보 추가 (기존 코드 유지)
  module.walk([&](circt::hw::InstanceOp inst) {
    for (auto operand : inst.getOperands()) {
      unsigned port_idx = trace_clock_to_port(operand);
      if (port_idx != ~0u && domain_map.count(port_idx)) {
        domain_map[port_idx].instances.push_back(inst);
        break;
      }
    }
  });

  // reg_count 내림차순 정렬 → primary clock(레지스터 수 많은 것)이 domains[0]
  std::vector<std::pair<unsigned, ClockDomainView>> sorted;
  for (auto &[idx, domain] : domain_map)
    sorted.push_back({domain.reg_count, std::move(domain)});
  std::sort(sorted.begin(), sorted.end(),
            [](const auto &a, const auto &b) { return a.first > b.first; });

  for (auto &[cnt, domain] : sorted)
    result.domains.push_back(std::move(domain));

  result.is_multi_clock = result.domains.size() > 1;
  return result;
}
```

**Step 1: 위 구현으로 교체 후 빌드**

Run: `cmake --build /user/wonseok/project-iamwonseok/llvm-cpp-model/build --target hirct-gen -j$(nproc) 2>&1 | grep "error:" | head -20`
Expected: 오류 없음

---

## Task 5: `GenDPIC` 생성자에 `mlir::ModuleOp` 추가

**Files:**
- Modify: `hirct/include/hirct/Target/GenDPIC.h:19,24`
- Modify: `hirct/lib/Target/GenDPIC.cpp:33-37`

**Step 1: GenDPIC.h 변경**

변경 전:
```cpp
#include "hirct/Analysis/IRAnalysis.h"
#include "circt/Dialect/HW/HWOps.h"
#include <string>
#include <vector>
...
  explicit GenDPIC(circt::hw::HWModuleOp hw_module);
...
  circt::hw::HWModuleOp hw_module_;
  std::string module_name_;
  hirct::ClockDomainMapView cdm_view_;
```

변경 후:
```cpp
#include "hirct/Analysis/IRAnalysis.h"
#include "circt/Dialect/HW/HWOps.h"
#include "mlir/IR/BuiltinOps.h"
#include <string>
#include <vector>
...
  explicit GenDPIC(circt::hw::HWModuleOp hw_module, mlir::ModuleOp mlir_module);
...
  circt::hw::HWModuleOp hw_module_;
  mlir::ModuleOp mlir_module_;
  std::string module_name_;
  hirct::ClockDomainMapView cdm_view_;
```

**Step 2: GenDPIC.cpp 생성자 변경**

변경 전:
```cpp
GenDPIC::GenDPIC(circt::hw::HWModuleOp hw_module)
    : hw_module_(hw_module),
      module_name_(hw_module.getSymName().str()) {
  cdm_view_ = hirct::build_clock_domain_map(hw_module_);
}
```

변경 후:
```cpp
GenDPIC::GenDPIC(circt::hw::HWModuleOp hw_module, mlir::ModuleOp mlir_module)
    : hw_module_(hw_module),
      mlir_module_(mlir_module),
      module_name_(hw_module.getSymName().str()) {
  cdm_view_ = hirct::build_clock_domain_map(hw_module_, mlir_module_);
}
```

**Step 3: main.cpp 호출부 변경**

파일: `hirct/tools/hirct-gen/main.cpp:1105`

변경 전:
```cpp
hirct::GenDPIC gen_dpic(top_hw);
```

변경 후:
```cpp
hirct::GenDPIC gen_dpic(top_hw, *mlir_module);
```

**Step 4: 빌드 확인**

Run: `cmake --build /user/wonseok/project-iamwonseok/llvm-cpp-model/build --target hirct-gen -j$(nproc) 2>&1 | grep "error:" | head -20`
Expected: 오류 없음

---

## Task 6: `emit_sv()`의 `is_clock_port()` 기반 clock 선택 → `cdm_view_` 기반으로 교체

**Files:**
- Modify: `hirct/lib/Target/GenDPIC.cpp:316~350`

**배경:** `cdm_view_.domains`가 이제 pass-through 모듈도 채워지므로, `emit_sv()`의 단일 clock 선택 로직을 `cdm_view_`로 통일한다.
`is_clock_port()` 자체는 삭제하지 않는다 (GenTB, GenVerify 등 8개 파일에서 포트 필터링 용도로 사용 중).

**변경 전 (316~350행):**
```cpp
  std::string clock_name;
  std::string reset_name;
  bool active_low = false;
  for (const auto &p : ports) {
    bool is_clock = hirct::is_clock_port(p.name);
    if (is_clock) {
      std::string lower = to_lower(p.name);
      bool is_pclk = lower == "pclk" || ends_with(lower, "_pclk") ||
                     starts_with(lower, "pclk_");
      if (clock_name.empty() || is_pclk)
        clock_name = p.name;
    }
    bool is_rst = hirct::is_reset_port(p.name);
    if (reset_name.empty() && is_rst) {
      reset_name = p.name;
      active_low = hirct::is_active_low_reset(p.name);
    }
  }
  if (!clock_name.empty() && !reset_name.empty()) {
    std::string clk_lower = to_lower(clock_name);
    bool clk_is_pclk = clk_lower == "pclk" || ends_with(clk_lower, "_pclk");
    if (clk_is_pclk) {
      for (const auto &p : ports) {
        bool is_rst = hirct::is_reset_port(p.name);
        if (!is_rst)
          continue;
        std::string rst_lower = to_lower(p.name);
        if (rst_lower.find("preset") != std::string::npos ||
            rst_lower.find("prst") != std::string::npos) {
          reset_name = p.name;
          active_low = hirct::is_active_low_reset(p.name);
          break;
        }
      }
    }
  }
```

**변경 후:** `cdm_view_.domains[0]`에서 primary clock을 가져온다.
build_clock_reset_pairs()가 이미 `cdm_view_.domains`를 순회하므로, is_multi 경로는 변경 없음.
단일 clock 경로(`else if (!clock_name.empty())`)의 `clock_name`만 교체한다.

```cpp
  // cdm_view_.domains[0]이 primary clock (reg_count 내림차순 정렬됨)
  std::string clock_name;
  std::string reset_name;
  bool active_low = false;
  if (!cdm_view_.domains.empty()) {
    clock_name = cdm_view_.domains[0].clock_port_name;
  }
  // reset은 clock과 대응하는 이름 휴리스틱으로 선택 (기존 로직 유지)
  // primary clock 이름 기반으로 대응 reset 찾기
  if (!clock_name.empty()) {
    std::string clk_lower = to_lower(clock_name);
    std::string clk_prefix;
    if (clk_lower == "pclk" || ends_with(clk_lower, "_pclk") ||
        starts_with(clk_lower, "pclk_"))
      clk_prefix = "p";
    else if (ends_with(clk_lower, "_clk"))
      clk_prefix = clk_lower.substr(0, clk_lower.size() - 4);

    // prefix 매칭 reset 우선
    for (const auto &p : ports) {
      if (!hirct::is_reset_port(p.name))
        continue;
      std::string rst_lower = to_lower(p.name);
      if (!clk_prefix.empty() &&
          (rst_lower.find(clk_prefix + "reset") != std::string::npos ||
           rst_lower.find(clk_prefix + "rst") != std::string::npos ||
           rst_lower.find(clk_prefix + "preset") != std::string::npos)) {
        reset_name = p.name;
        active_low = hirct::is_active_low_reset(p.name);
        break;
      }
    }
    // fallback: 첫 번째 reset
    if (reset_name.empty()) {
      for (const auto &p : ports) {
        if (hirct::is_reset_port(p.name)) {
          reset_name = p.name;
          active_low = hirct::is_active_low_reset(p.name);
          break;
        }
      }
    }
  }
```

**Step 1: 위 내용으로 교체 후 빌드**

Run: `cmake --build /user/wonseok/project-iamwonseok/llvm-cpp-model/build --target hirct-gen -j$(nproc) 2>&1 | grep "error:" | head -20`
Expected: 오류 없음

---

## Task 7: 검증

**Step 1: uart_top 재생성 및 primary clock 확인**

Run:
```bash
cd /user/wonseok/project-iamwonseok/llvm-cpp-model && \
build/bin/hirct-gen \
  -o /tmp/uart_fix_verify \
  --top uart_top \
  --lib-dir examples/fc6161/pt_plat/config/stubs \
  examples/fc6161/pt_plat/skip-analysis-results/uart/preprocessed.v 2>/dev/null && \
grep "always @" /tmp/uart_fix_verify/dpi/uart_top_dpi.sv
```

Expected:
```
  always @(posedge UART_PCLK) begin
```
(multi-clock이면 두 줄, single clock fallback이면 UART_PCLK 단독)

**Step 2: 단일 clock 모듈 회귀 테스트**

Run:
```bash
cd /user/wonseok/project-iamwonseok/llvm-cpp-model && \
build/bin/hirct-gen \
  -o /tmp/single_clk_regress \
  --only DW_apb_uart_regfile \
  --lib-dir examples/fc6161/pt_plat/config/stubs \
  examples/fc6161/pt_plat/skip-analysis-results/uart/preprocessed.v 2>/dev/null && \
grep "always @" /tmp/single_clk_regress/dpi/DW_apb_uart_regfile_dpi.sv
```

Expected:
```
  always @(posedge pclk) begin
```

**Step 3: DW_apb_uart_tx (serial clock) 단독 테스트**

Run:
```bash
cd /user/wonseok/project-iamwonseok/llvm-cpp-model && \
build/bin/hirct-gen \
  -o /tmp/tx_clk_regress \
  --only DW_apb_uart_tx \
  --lib-dir examples/fc6161/pt_plat/config/stubs \
  examples/fc6161/pt_plat/skip-analysis-results/uart/preprocessed.v 2>/dev/null && \
grep "always @" /tmp/tx_clk_regress/dpi/DW_apb_uart_tx_dpi.sv
```

Expected:
```
  always @(posedge sclk) begin
```

**Step 4: 전체 빌드 + lit 테스트 (회귀 없음 확인)**

Run:
```bash
cd /user/wonseok/project-iamwonseok/llvm-cpp-model && \
cmake --build build --target hirct-gen hirct-verify -j$(nproc) 2>&1 | tail -3
```

Expected: `Linking CXX executable` 성공

**Step 5: 커밋**

```bash
cd /user/wonseok/project-iamwonseok/llvm-cpp-model && \
git add hirct/include/hirct/Analysis/IRAnalysis.h \
        hirct/lib/Analysis/IRAnalysis.cpp \
        hirct/include/hirct/Target/GenDPIC.h \
        hirct/lib/Target/GenDPIC.cpp \
        hirct/tools/hirct-gen/main.cpp \
        hirct/lib/Target/GenModel.cpp && \
git commit -m "fix(dpic): replace is_clock_port heuristic with IR-based clock domain analysis

- build_clock_domain_map() takes mlir::ModuleOp for inter-module traversal
- count_clock_registers_through_instances() recursively counts registers
  driven through hw.instance chains to determine clock significance
- pass-through modules (e.g. uart_top) now correctly identify UART_PCLK
  as primary clock (APB domain, more registers) over UART_CLK (serial)
- GenDPIC::emit_sv() uses cdm_view_.domains[0] instead of is_clock_port()
- ClockDomainView gains reg_count field for primary selection ordering"
```

---

## 주의사항 요약

| 항목 | 처리 방법 |
|------|----------|
| `is_clock_port()` 삭제 | **하지 않음** — GenTB/GenVerify/GenFuncModel/GenCocotb/GenWrapper/GenDoc/GenFormat 8개 파일에서 포트 필터링 용도로 사용 중 |
| `reg_count_map` 변수 | `ClockDomainView.reg_count` 필드로 통합 — 별도 map 불필요 |
| 순환 참조 방지 | `llvm::SmallPtrSet<mlir::Operation*, 16> visited`로 방문한 HWModuleOp 추적 |
| `hw.module.extern` (블랙박스) | `sym_table.lookup<HWModuleOp>()` 실패 시 조기 종료 — 자동 처리 |
| GenModel 3곳 호출 | `mlir_module_` 멤버가 이미 있으므로 단순 인자 추가로 해결 |
