#include "hirct/Analysis/IRAnalysis.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/HW/HWTypes.h"
#include "circt/Dialect/Seq/SeqOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/SmallPtrSet.h"
#include <algorithm>
#include <map>
#include <queue>
#include <set>

namespace hirct {

unsigned get_type_width(mlir::Type type) {
  if (auto int_type = mlir::dyn_cast<mlir::IntegerType>(type))
    return int_type.getWidth();
  if (auto array_type = mlir::dyn_cast<circt::hw::ArrayType>(type))
    return array_type.getNumElements() *
           get_type_width(array_type.getElementType());
  if (auto struct_type = mlir::dyn_cast<circt::hw::StructType>(type)) {
    unsigned total = 0;
    for (auto &field : struct_type.getElements())
      total += get_type_width(field.type);
    return total;
  }
  return 0;
}

std::string cpp_type_for_width(unsigned width) {
  if (width == 0)
    return "uint8_t";
  if (width == 1)
    return "bool";
  if (width <= 8)
    return "uint8_t";
  if (width <= 16)
    return "uint16_t";
  if (width <= 32)
    return "uint32_t";
  if (width <= 64)
    return "uint64_t";
  return "uint64_t";
}

std::vector<PortView> get_ports(circt::hw::HWModuleOp module) {
  std::vector<PortView> result;
  auto port_list = module.getPortList();
  for (auto &port : port_list) {
    PortView pv;
    pv.name = port.getName().str();
    pv.is_input = port.isInput();
    pv.type = port.type;
    pv.width = get_type_width(port.type);
    result.push_back(std::move(pv));
  }
  return result;
}

std::vector<PortView> get_input_ports(circt::hw::HWModuleOp module) {
  std::vector<PortView> result;
  auto port_list = module.getPortList();
  for (auto &port : port_list) {
    if (!port.isInput())
      continue;
    PortView pv;
    pv.name = port.getName().str();
    pv.is_input = true;
    pv.type = port.type;
    pv.width = get_type_width(port.type);
    result.push_back(std::move(pv));
  }
  return result;
}

std::vector<PortView> get_output_ports(circt::hw::HWModuleOp module) {
  std::vector<PortView> result;
  auto port_list = module.getPortList();
  for (auto &port : port_list) {
    if (!port.isOutput())
      continue;
    PortView pv;
    pv.name = port.getName().str();
    pv.is_input = false;
    pv.type = port.type;
    pv.width = get_type_width(port.type);
    result.push_back(std::move(pv));
  }
  return result;
}

bool is_clock_port(llvm::StringRef name) {
  return name.equals_insensitive("clk") ||
         name.equals_insensitive("clock") ||
         name.equals_insensitive("pclk") ||
         name.equals_insensitive("hclk") ||
         name.equals_insensitive("aclk") ||
         name.equals_insensitive("fclk") ||
         name.equals_insensitive("rclk") ||
         name.equals_insensitive("wclk") ||
         name.ends_with_insensitive("_clk") ||
         name.ends_with_insensitive("_clock") ||
         name.starts_with_insensitive("clk_") ||
         name.starts_with_insensitive("clock_");
}

bool is_reset_port(llvm::StringRef name) {
  return name.equals_insensitive("rst") ||
         name.equals_insensitive("reset") ||
         name.equals_insensitive("rst_n") ||
         name.equals_insensitive("reset_n") ||
         name.equals_insensitive("rstn") ||
         name.equals_insensitive("resetn") ||
         name.equals_insensitive("presetn") ||
         name.equals_insensitive("hresetn") ||
         name.equals_insensitive("aresetn") ||
         name.ends_with_insensitive("_rst") ||
         name.ends_with_insensitive("_reset") ||
         name.ends_with_insensitive("_rst_n") ||
         name.ends_with_insensitive("_reset_n");
}

bool is_active_low_reset(llvm::StringRef name) {
  return name.ends_with_insensitive("_n") ||
         name.ends_with_insensitive("n");
}

// B-3: Register collection
std::vector<RegisterView> collect_registers(circt::hw::HWModuleOp module) {
  std::vector<RegisterView> result;
  std::map<std::string, unsigned> name_counts;

  auto unique_name = [&](const std::string &base) -> std::string {
    auto &cnt = name_counts[base];
    if (cnt == 0) {
      ++cnt;
      return base;
    }
    ++cnt;
    return base + "_" + std::to_string(cnt - 1);
  };

  module.walk([&](circt::seq::FirRegOp reg) {
    RegisterView rv;
    rv.op = reg.getOperation();
    rv.name = unique_name(reg.getName().str());
    rv.width = get_type_width(reg.getType());
    rv.clock = reg.getClk();
    rv.reset = reg.getReset();
    rv.reset_value = reg.getResetValue();
    rv.is_async = reg.getIsAsync();
    result.push_back(std::move(rv));
  });

  module.walk([&](circt::seq::CompRegOp reg) {
    RegisterView rv;
    rv.op = reg.getOperation();
    rv.name = unique_name(reg.getName().value_or("").str());
    rv.width = get_type_width(reg.getType());
    rv.clock = reg.getClk();
    rv.reset = reg.getReset();
    rv.reset_value = reg.getResetValue();
    rv.is_async = false;
    result.push_back(std::move(rv));
  });

  return result;
}

// B-4: Clock domain analysis — trace clock to port
namespace {

unsigned trace_clock_to_port(mlir::Value clk) {
  mlir::Value current = clk;
  while (current) {
    if (auto arg = mlir::dyn_cast<mlir::BlockArgument>(current))
      return arg.getArgNumber();
    auto *def_op = current.getDefiningOp();
    if (!def_op)
      break;
    if (auto to_clk = mlir::dyn_cast<circt::seq::ToClockOp>(def_op)) {
      current = to_clk.getInput();
      continue;
    }
    if (def_op->getNumOperands() > 0) {
      current = def_op->getOperand(0);
      continue;
    }
    break;
  }
  return ~0u;
}

// 특정 모듈의 port_idx번째 입력이 clock으로 구동하는 레지스터 수를 재귀 탐색으로 계산.
// visited로 방문한 HWModuleOp를 추적하여 순환 참조를 방지한다.
static unsigned count_clock_registers_through_instances(
    circt::hw::HWModuleOp module, unsigned port_idx,
    mlir::SymbolTable &sym_table, int depth,
    llvm::SmallPtrSet<mlir::Operation *, 16> &visited) {
  if (depth > 8)
    return 0;
  if (!visited.insert(module.getOperation()).second)
    return 0;

  auto *body = module.getBodyBlock();
  if (!body || port_idx >= body->getNumArguments())
    return 0;

  mlir::Value port_val = body->getArgument(port_idx);
  unsigned total = 0;

  for (auto *user : port_val.getUsers()) {
    // 직접 seq.to_clock → clock 타입으로 변환 후 레지스터에 연결되는 경우
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
      // hw.module.extern(블랙박스)은 lookup 실패 → 조기 종료
      auto sub_mod = sym_table.lookup<circt::hw::HWModuleOp>(
          inst.getModuleName());
      if (!sub_mod)
        break;
      total += count_clock_registers_through_instances(sub_mod, op_idx,
                                                       sym_table, depth + 1,
                                                       visited);
      break;
    }
  }
  return total;
}

} // namespace

ClockDomainMapView build_clock_domain_map(circt::hw::HWModuleOp module,
                                          mlir::ModuleOp mlir_module) {
  ClockDomainMapView result;
  auto input_ports = get_input_ports(module);
  auto registers = collect_registers(module);

  // Step A: 현재 모듈 내 레지스터 기반 분석 (기존 방식 유지)
  std::map<unsigned, ClockDomainView> domain_map;
  for (auto &reg : registers) {
    unsigned port_idx = trace_clock_to_port(reg.clock);
    auto &domain = domain_map[port_idx];
    if (domain.clock_port_name.empty() && port_idx < input_ports.size()) {
      domain.clock_port_name = input_ports[port_idx].name;
      domain.clock_port_index = port_idx;
    }
    domain.reg_count++;
    domain.registers.push_back(reg);
  }

  // Step B: 레지스터 없는 pass-through 모듈 처리
  // uart_top처럼 hw.instance만 있고 직접 레지스터가 없는 경우,
  // 각 i1 입력 포트를 서브모듈 체인 재귀 탐색하여 구동 레지스터 수 계산
  if (domain_map.empty()) {
    mlir::SymbolTable sym_table(mlir_module);
    for (unsigned port_idx = 0; port_idx < input_ports.size(); ++port_idx) {
      const auto &port = input_ports[port_idx];
      if (port.width != 1)
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
    std::set<unsigned> matched_domains;
    for (auto operand : inst.getOperands()) {
      unsigned port_idx = trace_clock_to_port(operand);
      if (port_idx != ~0u && domain_map.count(port_idx) &&
          matched_domains.insert(port_idx).second) {
        domain_map[port_idx].instances.push_back(inst);
      }
    }
  });

  // reg_count 내림차순 정렬 → primary clock(레지스터 수 많은 것)이 domains[0]
  std::vector<std::pair<unsigned, ClockDomainView>> sorted;
  sorted.reserve(domain_map.size());
  for (auto &[idx, domain] : domain_map)
    sorted.push_back({domain.reg_count, std::move(domain)});
  std::sort(sorted.begin(), sorted.end(),
            [](const auto &a, const auto &b) { return a.first > b.first; });

  for (auto &[cnt, domain] : sorted)
    result.domains.push_back(std::move(domain));

  result.is_multi_clock = result.domains.size() > 1;
  return result;
}

// B-5: Topological sort of instances
IRTopoSortResult sort_instances_topologically(circt::hw::HWModuleOp module) {
  IRTopoSortResult result;

  llvm::SmallVector<circt::hw::InstanceOp> instances;
  module.walk([&](circt::hw::InstanceOp inst) { instances.push_back(inst); });

  if (instances.empty())
    return result;

  std::map<mlir::Operation *, size_t> inst_index;
  for (size_t i = 0; i < instances.size(); ++i)
    inst_index[instances[i].getOperation()] = i;

  size_t n = instances.size();
  std::vector<std::set<size_t>> adj(n);
  std::vector<size_t> in_degree(n, 0);

  for (size_t i = 0; i < n; ++i) {
    auto inst = instances[i];
    for (auto operand : inst.getOperands()) {
      auto *def_op = operand.getDefiningOp();
      if (!def_op)
        continue;
      auto it = inst_index.find(def_op);
      if (it != inst_index.end() && it->second != i) {
        if (adj[it->second].insert(i).second)
          in_degree[i]++;
      }
    }
  }

  std::queue<size_t> q;
  for (size_t i = 0; i < n; ++i)
    if (in_degree[i] == 0)
      q.push(i);

  std::vector<size_t> order;
  while (!q.empty()) {
    size_t cur = q.front();
    q.pop();
    order.push_back(cur);
    for (size_t next : adj[cur]) {
      if (--in_degree[next] == 0)
        q.push(next);
    }
  }

  if (order.size() == n) {
    for (size_t idx : order)
      result.order.push_back(instances[idx]);
  } else {
    for (size_t i = 0; i < n; ++i) {
      if (in_degree[i] > 0)
        result.cycle_members.push_back(instances[i]);
    }
    for (size_t i = 0; i < n; ++i)
      result.order.push_back(instances[i]);
  }

  return result;
}

// B-6: Memory analysis
std::vector<MemoryView> collect_memories(circt::hw::HWModuleOp module) {
  std::vector<MemoryView> result;
  module.walk([&](circt::seq::FirMemOp mem) {
    MemoryView mv;
    mv.op = mem.getOperation();
    mv.name = mem.getName().value_or("").str();
    auto mem_type = mem.getType();
    mv.depth = mem_type.getDepth();
    mv.element_width = mem_type.getWidth();
    result.push_back(std::move(mv));
  });
  return result;
}

std::vector<MemoryReadPortView>
collect_memory_read_ports(circt::hw::HWModuleOp module) {
  std::vector<MemoryReadPortView> result;
  module.walk([&](circt::seq::FirMemReadOp port) {
    MemoryReadPortView rpv;
    rpv.op = port.getOperation();
    rpv.addr = port.getAddress();
    rpv.enable = port.getEnable();
    rpv.clock = port.getClk();
    result.push_back(std::move(rpv));
  });
  return result;
}

std::vector<MemoryWritePortView>
collect_memory_write_ports(circt::hw::HWModuleOp module) {
  std::vector<MemoryWritePortView> result;
  module.walk([&](circt::seq::FirMemWriteOp port) {
    MemoryWritePortView wpv;
    wpv.op = port.getOperation();
    wpv.addr = port.getAddress();
    wpv.data = port.getData();
    wpv.enable = port.getEnable();
    wpv.clock = port.getClk();
    wpv.mask = port.getMask();
    result.push_back(std::move(wpv));
  });
  return result;
}

} // namespace hirct
