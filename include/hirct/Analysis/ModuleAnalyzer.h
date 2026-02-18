#ifndef HIRCT_ANALYSIS_MODULEANALYZER_H
#define HIRCT_ANALYSIS_MODULEANALYZER_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace hirct {

struct PortInfo {
  std::string name;
  std::string direction;
  int width;
};

struct OpInfo {
  std::string op_name;
  std::string result_name;
  std::string result_type;
  std::vector<std::string> operands;
  std::map<std::string, std::string> attributes;
};

struct ConstantInfo {
  std::string name;
  std::string type;
  std::string value;
};

class ModuleAnalyzer {
public:
  explicit ModuleAnalyzer(const std::string &mlir_text);

  std::string module_name() const { return module_name_; }
  bool is_valid() const { return valid_; }

  const std::vector<PortInfo> &ports() const { return ports_; }
  std::vector<PortInfo> input_ports() const;
  std::vector<PortInfo> output_ports() const;

  const std::vector<OpInfo> &operations() const { return operations_; }
  const std::vector<ConstantInfo> &constants() const { return constants_; }
  std::vector<std::string> output_values() const { return output_values_; }

  bool has_registers() const { return has_registers_; }
  bool has_instances() const { return has_instances_; }
  bool has_combinational_loops() const { return has_combinational_loops_; }
  int value_width(const std::string &ssa_name) const;

private:
  std::string module_name_;
  bool valid_ = false;
  bool has_registers_ = false;
  bool has_instances_ = false;
  bool has_combinational_loops_ = false;
  std::vector<PortInfo> ports_;
  std::vector<OpInfo> operations_;
  std::vector<ConstantInfo> constants_;
  std::vector<std::string> output_values_;
  std::map<std::string, int> ssa_widths_;

  void parse(const std::string &mlir_text);
  void parse_module(const std::string &header);
  void parse_body(const std::string &body);
  void topological_sort();
};

} // namespace hirct

#endif // HIRCT_ANALYSIS_MODULEANALYZER_H
