#ifndef HIRCT_TARGET_GENWRAPPER_H
#define HIRCT_TARGET_GENWRAPPER_H

#include "circt/Dialect/HW/HWOps.h"
#include <string>

namespace hirct {

class GenWrapper {
public:
  explicit GenWrapper(circt::hw::HWModuleOp hw_module);
  bool emit(const std::string &output_dir);

private:
  circt::hw::HWModuleOp hw_module_;
};

} // namespace hirct

#endif // HIRCT_TARGET_GENWRAPPER_H
