#ifndef HIRCT_TARGET_GENMAKEFILE_H
#define HIRCT_TARGET_GENMAKEFILE_H

#include "circt/Dialect/HW/HWOps.h"
#include <string>
#include <vector>

namespace hirct {

class GenMakefile {
public:
  explicit GenMakefile(circt::hw::HWModuleOp hw_module,
                       const std::string &rtl_src = "",
                       const std::vector<std::string> &lib_dirs = {},
                       bool has_func_model = false);
  bool emit(const std::string &output_dir);

private:
  circt::hw::HWModuleOp hw_module_;
  std::string rtl_src_;
  std::vector<std::string> lib_dirs_;
  bool has_func_model_;
};

} // namespace hirct

#endif // HIRCT_TARGET_GENMAKEFILE_H
