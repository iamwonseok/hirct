#ifndef HIRCT_TARGET_GENVERIFY_H
#define HIRCT_TARGET_GENVERIFY_H

#include "circt/Dialect/HW/HWOps.h"
#include <string>

namespace hirct {

class GenVerify {
public:
  explicit GenVerify(circt::hw::HWModuleOp hw_module);
  bool emit(const std::string &output_dir);
  const std::string &last_error_reason() const { return last_error_reason_; }

private:
  circt::hw::HWModuleOp hw_module_;
  std::string last_error_reason_;

  std::string cpp_type_for_width(int width) const;
  std::string random_expr_for_width(int width) const;
};

} // namespace hirct

#endif // HIRCT_TARGET_GENVERIFY_H
