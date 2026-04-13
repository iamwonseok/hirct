#ifndef HIRCT_SUPPORT_VERILOGLOADER_H
#define HIRCT_SUPPORT_VERILOGLOADER_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"
#include <string>
#include <vector>

namespace hirct {

struct VerilogLoadOptions {
  std::vector<std::string> input_files;
  std::vector<std::string> lib_dirs;
  std::vector<std::string> include_dirs;
  std::vector<std::string> lib_files;
  std::vector<std::string> defines;
  std::vector<std::string> lib_exts;
  std::string top_module;
  std::string timescale;
  bool ignore_unknown_modules = false;
  bool canonicalize = false;
  bool enable_timing = false;
};

struct VerilogLoadResult {
  mlir::OwningOpRef<mlir::ModuleOp> module;
  std::string error_message;
  bool success = false;
};

VerilogLoadResult load_verilog(mlir::MLIRContext &ctx,
                               const VerilogLoadOptions &opts);

} // namespace hirct

#endif // HIRCT_SUPPORT_VERILOGLOADER_H
