#ifndef HIRCT_TARGET_GENFUNCMODEL_H
#define HIRCT_TARGET_GENFUNCMODEL_H

#include "hirct/Analysis/FSMAnalysis.h"
#include "circt/Dialect/HW/HWOps.h"
#include "mlir/IR/BuiltinOps.h"
#include <string>

namespace hirct {

class GenFuncModel {
public:
  GenFuncModel(circt::hw::HWModuleOp hw_module, mlir::ModuleOp mlir_module);
  bool emit(const std::string &output_dir);
  const std::string &last_error_reason() const { return last_error_reason_; }

private:
  bool emit_header(const std::string &dir, const std::string &name,
                   const FSMView &fsm);
  bool emit_impl(const std::string &dir, const std::string &name,
                 const FSMView &fsm);
  std::string resolve_condition_expr(mlir::Value cond);
  std::string resolve_value_expr(mlir::Value v, const FSMView &fsm);

  circt::hw::HWModuleOp hw_module_;
  mlir::ModuleOp mlir_module_;
  std::string last_error_reason_;
};

} // namespace hirct

#endif // HIRCT_TARGET_GENFUNCMODEL_H
