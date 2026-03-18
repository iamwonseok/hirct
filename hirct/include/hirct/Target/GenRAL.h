#ifndef HIRCT_TARGET_GENRAL_H
#define HIRCT_TARGET_GENRAL_H

#include "circt/Dialect/HW/HWOps.h"
#include "hirct/Analysis/IRAnalysis.h"
#include <string>
#include <vector>

namespace hirct {

class GenRAL {
public:
  explicit GenRAL(circt::hw::HWModuleOp hw_module);
  bool emit(const std::string &output_dir);
  bool should_skip() const;
  const std::vector<RegisterView> &registers() const { return registers_; }

private:
  circt::hw::HWModuleOp hw_module_;
  std::vector<RegisterView> registers_;

  std::string to_upper(const std::string &s) const;
  std::string to_lower(const std::string &s) const;

  bool emit_ral_sv(const std::string &dir);
  bool emit_hal_h(const std::string &dir);
  bool emit_driver_c(const std::string &dir);
};

} // namespace hirct

#endif // HIRCT_TARGET_GENRAL_H
