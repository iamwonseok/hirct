#ifndef HIRCT_TARGET_GENCOCOTB_H
#define HIRCT_TARGET_GENCOCOTB_H

#include "circt/Dialect/HW/HWOps.h"
#include <fstream>
#include <string>
#include <vector>

namespace hirct {

class GenCocotb {
public:
  explicit GenCocotb(circt::hw::HWModuleOp hw_module);
  bool emit(const std::string &output_dir);

private:
  circt::hw::HWModuleOp hw_module_;

  struct SimplePort {
    std::string name;
    int width;
  };
  void emit_test_reset(std::ofstream &ofs, const std::string &clock_name,
                       const std::string &reset_name, bool has_clock,
                       int reset_active, int reset_inactive,
                       const std::vector<SimplePort> &out_ports);
  void emit_test_random_stimulus(std::ofstream &ofs,
                                 const std::string &clock_name,
                                 const std::string &reset_name, bool has_clock,
                                 int reset_active, int reset_inactive,
                                 const std::vector<SimplePort> &in_ports);
};

} // namespace hirct

#endif // HIRCT_TARGET_GENCOCOTB_H
