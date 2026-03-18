#ifndef HIRCT_TARGET_GENDOC_H
#define HIRCT_TARGET_GENDOC_H

#include "circt/Dialect/HW/HWOps.h"
#include "hirct/Analysis/IRAnalysis.h"
#include <fstream>
#include <string>

namespace hirct {

class GenDoc {
public:
  explicit GenDoc(circt::hw::HWModuleOp hw_module);
  bool emit(const std::string &output_dir);

private:
  circt::hw::HWModuleOp hw_module_;

  static std::string port_description(const std::string &name);

  void emit_hardware_spec(std::ofstream &ofs,
                          const std::vector<PortView> &ports,
                          const std::vector<RegisterView> &registers,
                          int comb_count, int instance_count);
  void emit_port_map(std::ofstream &ofs, const std::vector<PortView> &ports);
  void emit_internal_signals(std::ofstream &ofs,
                             const std::vector<RegisterView> &registers,
                             int comb_count, int instance_count);
  void emit_register_summary(std::ofstream &ofs,
                             const std::vector<RegisterView> &registers);
  void emit_programmers_guide(std::ofstream &ofs, bool has_clock,
                              bool has_reset, const std::string &name,
                              const std::vector<PortView> &ports,
                              const std::vector<PortView> &input_ports,
                              const std::vector<PortView> &output_ports,
                              bool has_registers);
  void emit_quick_start(std::ofstream &ofs,
                        const std::vector<PortView> &input_ports,
                        const std::vector<PortView> &output_ports);
  void emit_reset_sequence(std::ofstream &ofs,
                           const std::vector<PortView> &ports);
  void emit_cpp_model_usage(std::ofstream &ofs, bool has_clock,
                            const std::string &name,
                            const std::vector<PortView> &input_ports,
                            const std::vector<PortView> &output_ports,
                            bool has_registers);
  void emit_module_hierarchy(
      std::ofstream &ofs, const std::string &name,
      const std::vector<std::pair<std::string, std::string>> &instances);
};

} // namespace hirct

#endif // HIRCT_TARGET_GENDOC_H
