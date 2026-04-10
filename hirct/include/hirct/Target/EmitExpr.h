#ifndef HIRCT_TARGET_EMITEXPR_H
#define HIRCT_TARGET_EMITEXPR_H

#include "mlir/IR/Operation.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/DenseMap.h"
#include <fstream>
#include <string>

namespace hirct {

std::string emit_op_expr(
    mlir::Operation &op,
    llvm::DenseMap<mlir::Value, std::string> &val,
    std::ofstream &ofs, int &tmp_cnt);

std::string width_mask_expr(int width);

std::string inline_arc_call(
    mlir::Operation &callOp, unsigned resultIdx,
    llvm::DenseMap<mlir::Value, std::string> &outerVal,
    std::ofstream &ofs, int &tmp_cnt, unsigned depth,
    unsigned wordIdx = 0, unsigned wordBits = 0);

std::string render_in_callee_body(
    mlir::Value val,
    llvm::DenseMap<mlir::Value, std::string> &argMap,
    llvm::DenseMap<mlir::Value, std::string> &outerVal,
    std::ofstream &ofs, int &tmp_cnt, unsigned depth,
    unsigned wordIdx, unsigned wordBits);

} // namespace hirct

#endif
