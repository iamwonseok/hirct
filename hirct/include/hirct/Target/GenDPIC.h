#ifndef HIRCT_TARGET_GENDPIC_H
#define HIRCT_TARGET_GENDPIC_H

#include "hirct/Analysis/IRAnalysis.h"
#include "circt/Dialect/HW/HWOps.h"
#include "mlir/IR/BuiltinOps.h"
#include <string>
#include <vector>

namespace hirct {

struct ClockResetPair {
  std::string clock_name;
  std::string reset_name;
  bool active_low = false;
};

class GenDPIC {
public:
  explicit GenDPIC(circt::hw::HWModuleOp hw_module, mlir::ModuleOp mlir_module);
  bool emit(const std::string &output_dir);

private:
  circt::hw::HWModuleOp hw_module_;
  mlir::ModuleOp mlir_module_;
  std::string module_name_;
  hirct::ClockDomainMapView cdm_view_;

  std::string dpi_c_type_for_width(int width) const;
  std::string sv_dpi_type_for_width(int width) const;
  std::string func_prefix() const;

  std::vector<ClockResetPair> build_clock_reset_pairs() const;

  bool emit_header(const std::string &dir);
  bool emit_impl(const std::string &dir);
  bool emit_sv(const std::string &dir);
};

} // namespace hirct

#endif // HIRCT_TARGET_GENDPIC_H
