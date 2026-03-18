#ifndef HIRCT_ANALYSIS_IRANALYSIS_H
#define HIRCT_ANALYSIS_IRANALYSIS_H

#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/Seq/SeqOps.h"
#include "mlir/IR/BuiltinOps.h"
#include <string>
#include <vector>

namespace hirct {

struct PortView {
  std::string name;
  bool is_input;
  unsigned width;
  mlir::Type type;
};

std::vector<PortView> get_ports(circt::hw::HWModuleOp module);
std::vector<PortView> get_input_ports(circt::hw::HWModuleOp module);
std::vector<PortView> get_output_ports(circt::hw::HWModuleOp module);

bool is_clock_port(llvm::StringRef name);
bool is_reset_port(llvm::StringRef name);
bool is_active_low_reset(llvm::StringRef name);

unsigned get_type_width(mlir::Type type);
std::string cpp_type_for_width(unsigned width);

// B-3: Register collection
struct RegisterView {
  mlir::Operation *op;
  std::string name;
  unsigned width;
  mlir::Value clock;
  mlir::Value reset;
  mlir::Value reset_value;
  bool is_async;
};

std::vector<RegisterView> collect_registers(circt::hw::HWModuleOp module);

// B-4: Clock domain analysis
struct ClockDomainView {
  std::string clock_port_name;
  unsigned clock_port_index = 0;
  unsigned reg_count = 0; // 이 clock이 구동하는 레지스터 수 (primary 선택 기준)
  std::vector<RegisterView> registers;
  std::vector<circt::hw::InstanceOp> instances;
};

struct ClockDomainMapView {
  std::vector<ClockDomainView> domains;
  bool is_multi_clock;
};

ClockDomainMapView build_clock_domain_map(circt::hw::HWModuleOp module,
                                          mlir::ModuleOp mlir_module);

// B-5: Topological sort of instances
struct IRTopoSortResult {
  std::vector<circt::hw::InstanceOp> order;
  std::vector<circt::hw::InstanceOp> cycle_members;
};

IRTopoSortResult sort_instances_topologically(circt::hw::HWModuleOp module);

// B-6: Memory analysis
struct MemoryView {
  mlir::Operation *op;
  std::string name;
  unsigned depth;
  unsigned element_width;
};

struct MemoryReadPortView {
  mlir::Operation *op;
  mlir::Value addr;
  mlir::Value enable;
  mlir::Value clock;
};

struct MemoryWritePortView {
  mlir::Operation *op;
  mlir::Value addr;
  mlir::Value data;
  mlir::Value enable;
  mlir::Value clock;
  mlir::Value mask;
};

std::vector<MemoryView> collect_memories(circt::hw::HWModuleOp module);
std::vector<MemoryReadPortView>
collect_memory_read_ports(circt::hw::HWModuleOp module);
std::vector<MemoryWritePortView>
collect_memory_write_ports(circt::hw::HWModuleOp module);

} // namespace hirct

#endif // HIRCT_ANALYSIS_IRANALYSIS_H
