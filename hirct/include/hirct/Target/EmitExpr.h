#ifndef HIRCT_TARGET_EMITEXPR_H
#define HIRCT_TARGET_EMITEXPR_H

#include "mlir/IR/Operation.h"
#include "llvm/ADT/DenseMap.h"
#include <fstream>
#include <string>

namespace hirct {

std::string emit_op_expr(
    mlir::Operation &op,
    llvm::DenseMap<mlir::Value, std::string> &val,
    std::ofstream &ofs, int &tmp_cnt);

std::string width_mask_expr(int width);

} // namespace hirct

#endif
