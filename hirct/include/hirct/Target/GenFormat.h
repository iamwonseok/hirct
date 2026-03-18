#ifndef HIRCT_TARGET_GENFORMAT_H
#define HIRCT_TARGET_GENFORMAT_H

#include "circt/Dialect/HW/HWOps.h"
#include "hirct/Analysis/IRAnalysis.h"
#include <map>
#include <string>
#include <vector>

namespace hirct {

class GenFormat {
public:
  explicit GenFormat(circt::hw::HWModuleOp hw_module);
  bool emit(const std::string &output_dir);

private:
  circt::hw::HWModuleOp hw_module_;
  std::string post_process(const std::string &raw_verilog);
  std::string generate_fallback(
      const std::string &name,
      const std::map<std::string, std::vector<PortView>> &groups,
      const std::vector<PortView> &ports,
      const std::vector<PortView> &output_ports, bool has_registers);
  std::string insert_section_comments(const std::string &verilog);
  std::string insert_port_group_comments(
      const std::map<std::string, std::vector<PortView>> &groups,
      const std::string &verilog);
};

} // namespace hirct

#endif // HIRCT_TARGET_GENFORMAT_H
