#include "hirct/Target/EmitExpr.h"
#include "hirct/Analysis/IRAnalysis.h"
#include "circt/Dialect/Comb/CombOps.h"
#include "circt/Dialect/HW/HWOps.h"
#include "mlir/IR/BuiltinAttributes.h"

#include <limits>
#include <sstream>

namespace hirct {

std::string width_mask_expr(int width) {
  if (width <= 0) {
    return "0ULL";
  }
  if (width >= 64) {
    return "0xFFFFFFFFFFFFFFFFULL";
  }
  return "((1ULL << " + std::to_string(width) + ") - 1ULL)";
}

std::string emit_op_expr(
    mlir::Operation &op,
    llvm::DenseMap<mlir::Value, std::string> &val,
    std::ofstream &ofs, int &tmp_cnt) {
  auto w_of = [](mlir::Value v) -> unsigned {
    return hirct::get_type_width(v.getType());
  };
  auto expr = [&](mlir::Value v) -> std::string {
    auto it = val.find(v);
    return it != val.end() ? it->second : std::string();
  };
  auto array_hold_expr = [&](mlir::Value v) -> std::string {
    if (auto inject = v.getDefiningOp<circt::hw::ArrayInjectOp>())
      return expr(inject.getInput());
    return std::string();
  };
  auto next_tmp = [&]() -> std::string {
    return "t" + std::to_string(tmp_cnt++);
  };
  auto emit_variadic = [&](mlir::Operation &o, const char *op_char,
                            const char *identity = nullptr) {
    std::ostringstream oss;
    // Collect non-empty operand expressions; skip any whose SSA value
    // was not resolved (empty string from expr()).
    std::vector<std::string> parts;
    parts.reserve(o.getNumOperands());
    for (unsigned i = 0; i < o.getNumOperands(); ++i) {
      std::string s = expr(o.getOperand(i));
      if (!s.empty())
        parts.push_back(std::move(s));
    }
    if (parts.empty()) {
      // 0-operand (or all-empty): return identity element if provided,
      // otherwise 0.
      oss << (identity ? identity : "0ULL");
    } else {
      oss << "(";
      for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0)
          oss << " " << op_char << " ";
        oss << "(" << parts[i] << ")";
      }
      oss << ")";
    }
    return oss.str();
  };

  if (op.getNumResults() == 0)
    return "";

  mlir::Value result = op.getResult(0);
  unsigned w = w_of(result);
  std::string ctype = hirct::cpp_type_for_width(w);

  if (auto c = mlir::dyn_cast<circt::hw::ConstantOp>(op)) {
    llvm::APInt apv = c.getValue();
    // APInt::getZExtValue() asserts when bit-width > 64; always truncate to 64.
    // For signals wider than 64 bits, only the lower 64 bits are representable
    // in the generated uint64_t model — emit a warning.
    if (w > 64) {
      llvm::errs() << "warning: constant of width " << w
                   << " truncated to 64 bits in C++ model\n";
    }
    uint64_t uv = apv.zextOrTrunc(64).getZExtValue();
    std::string vs = std::to_string(uv);
    if (uv > static_cast<uint64_t>(std::numeric_limits<long long>::max()))
      vs += "ULL";
    val[result] = "static_cast<" + ctype + ">(" + vs + ")";
    return "";
  }

  std::string e;

  if (mlir::isa<circt::comb::AddOp>(op))
    e = emit_variadic(op, "+", "0ULL");
  else if (mlir::isa<circt::comb::MulOp>(op))
    e = emit_variadic(op, "*", "1ULL");
  else if (mlir::isa<circt::comb::AndOp>(op))
    e = emit_variadic(op, "&", "0xFFFFFFFFFFFFFFFFULL");
  else if (mlir::isa<circt::comb::OrOp>(op))
    e = emit_variadic(op, "|", "0ULL");
  else if (mlir::isa<circt::comb::XorOp>(op))
    e = emit_variadic(op, "^", "0ULL");
  else if (auto sub = mlir::dyn_cast<circt::comb::SubOp>(op)) {
    e = "((" + expr(sub.getLhs()) + ") - (" + expr(sub.getRhs()) + "))";
  } else if (auto shl = mlir::dyn_cast<circt::comb::ShlOp>(op)) {
    e = "(static_cast<uint64_t>(" + expr(shl.getLhs()) +
        ") << (static_cast<uint64_t>(" + expr(shl.getRhs()) +
        ") & 63ULL))";
  } else if (auto shru = mlir::dyn_cast<circt::comb::ShrUOp>(op)) {
    e = "(static_cast<uint64_t>(" + expr(shru.getLhs()) +
        ") >> (static_cast<uint64_t>(" + expr(shru.getRhs()) +
        ") & 63ULL))";
  } else if (auto shrs = mlir::dyn_cast<circt::comb::ShrSOp>(op)) {
    e = "(static_cast<int64_t>(" + expr(shrs.getLhs()) +
        ") >> (static_cast<uint64_t>(" + expr(shrs.getRhs()) +
        ") & 63ULL))";
  } else if (auto mux = mlir::dyn_cast<circt::comb::MuxOp>(op)) {
    if (auto arr_ty =
            mlir::dyn_cast<circt::hw::ArrayType>(mux.getResult().getType())) {
      unsigned depth = arr_ty.getNumElements();
      unsigned elem_w = hirct::get_type_width(arr_ty.getElementType());
      if (elem_w == 0)
        elem_w = 1;
      std::string etype = hirct::cpp_type_for_width(elem_w);
      std::string tn = "t" + std::to_string(tmp_cnt++);
      std::string cond_e = expr(mux.getCond());
      std::string true_e = expr(mux.getTrueValue());
      std::string false_e = expr(mux.getFalseValue());
      bool true_is_self = mux.getTrueValue() == result;
      bool false_is_self = mux.getFalseValue() == result;
      if (true_is_self != false_is_self) {
        mlir::Value non_self_value =
            true_is_self ? mux.getFalseValue() : mux.getTrueValue();
        std::string non_self_e = expr(non_self_value);
        if (!non_self_e.empty()) {
          std::string self_e = expr(result);
          if (self_e.empty())
            self_e = array_hold_expr(non_self_value);
          if (self_e.empty())
            self_e = non_self_e;
          if (true_is_self)
            true_e = self_e;
          if (false_is_self)
            false_e = self_e;
        }
      }
      ofs << "  " << etype << " " << tn << "[" << depth << "];\n";
      ofs << "  for (int __i = 0; __i < " << depth << "; ++__i) " << tn
          << "[__i] = (" << cond_e << ") ? " << true_e << "[__i] : "
          << false_e << "[__i];\n";
      val[op.getResult(0)] = tn;
      return "";
    }
    e = "((" + expr(mux.getCond()) + ") ? (" +
        expr(mux.getTrueValue()) + ") : (" +
        expr(mux.getFalseValue()) + "))";
  } else if (auto concat = mlir::dyn_cast<circt::comb::ConcatOp>(op)) {
    std::string acc = "0ULL";
    for (auto operand : concat.getOperands()) {
      unsigned ow = w_of(operand);
      std::string oe = expr(operand);
      if (ow >= 64)
        acc = "(static_cast<uint64_t>(" + oe + "))";
      else
        acc = "((" + acc + ") << " + std::to_string(ow) +
              ") | (static_cast<uint64_t>(" + oe + ") & " +
              width_mask_expr(ow) + ")";
    }
    e = "(" + acc + ")";
  } else if (auto ext = mlir::dyn_cast<circt::comb::ExtractOp>(op)) {
    unsigned from = ext.getLowBit();
    e = "((static_cast<uint64_t>(" + expr(ext.getInput()) + ") >> " +
        std::to_string(from) + ") & " + width_mask_expr(w) + ")";
  } else if (auto rep = mlir::dyn_cast<circt::comb::ReplicateOp>(op)) {
    unsigned src_w = w_of(rep.getInput());
    int cnt = (src_w > 0) ? w / src_w : 0;
    std::ostringstream oss;
    oss << "(";
    for (int i = 0; i < cnt; ++i) {
      if (i > 0)
        oss << " | ";
      oss << "(static_cast<uint64_t>(" << expr(rep.getInput())
          << ") << " << (i * src_w) << ")";
    }
    oss << ")";
    e = oss.str();
  } else if (auto par = mlir::dyn_cast<circt::comb::ParityOp>(op)) {
    unsigned src_w = w_of(par.getInput());
    if (src_w > 64)
      e = "static_cast<bool>(__builtin_parityll(static_cast<uint64_t>(" +
          expr(par.getInput()) +
          ")) ^ __builtin_parityll(static_cast<uint64_t>("
          "static_cast<unsigned __int128>(" +
          expr(par.getInput()) + ") >> 64)))";
    else
      e = "static_cast<bool>(__builtin_parityll(static_cast<uint64_t>(" +
          expr(par.getInput()) + ")))";
  } else if (auto icmp = mlir::dyn_cast<circt::comb::ICmpOp>(op)) {
    std::string lhs = expr(icmp.getLhs());
    std::string rhs = expr(icmp.getRhs());
    unsigned cw = w_of(icmp.getLhs());
    auto pred = icmp.getPredicate();
    using P = circt::comb::ICmpPredicate;
    switch (pred) {
    case P::eq:
    case P::ceq:
    case P::weq:
      if (cw > 0 && cw < 64) {
        std::string mask = width_mask_expr(cw);
        e = "(((" + lhs + ") & " + mask + ") == ((" + rhs + ") & " +
            mask + "))";
      } else
        e = "((" + lhs + ") == (" + rhs + "))";
      break;
    case P::ne:
    case P::cne:
    case P::wne:
      if (cw > 0 && cw < 64) {
        std::string mask = width_mask_expr(cw);
        e = "(((" + lhs + ") & " + mask + ") != ((" + rhs + ") & " +
            mask + "))";
      } else
        e = "((" + lhs + ") != (" + rhs + "))";
      break;
    case P::ult:
      e = "(static_cast<uint64_t>(" + lhs +
          ") < static_cast<uint64_t>(" + rhs + "))";
      break;
    case P::ule:
      e = "(static_cast<uint64_t>(" + lhs +
          ") <= static_cast<uint64_t>(" + rhs + "))";
      break;
    case P::ugt:
      e = "(static_cast<uint64_t>(" + lhs +
          ") > static_cast<uint64_t>(" + rhs + "))";
      break;
    case P::uge:
      e = "(static_cast<uint64_t>(" + lhs +
          ") >= static_cast<uint64_t>(" + rhs + "))";
      break;
    case P::slt:
      e = "(static_cast<int64_t>(" + lhs +
          ") < static_cast<int64_t>(" + rhs + "))";
      break;
    case P::sle:
      e = "(static_cast<int64_t>(" + lhs +
          ") <= static_cast<int64_t>(" + rhs + "))";
      break;
    case P::sgt:
      e = "(static_cast<int64_t>(" + lhs +
          ") > static_cast<int64_t>(" + rhs + "))";
      break;
    case P::sge:
      e = "(static_cast<int64_t>(" + lhs +
          ") >= static_cast<int64_t>(" + rhs + "))";
      break;
    }
  } else if (auto divu = mlir::dyn_cast<circt::comb::DivUOp>(op)) {
    e = "((" + expr(divu.getRhs()) +
        ") != 0 ? static_cast<uint64_t>(" + expr(divu.getLhs()) +
        ") / static_cast<uint64_t>(" + expr(divu.getRhs()) +
        ") : 0ULL)";
  } else if (auto divs = mlir::dyn_cast<circt::comb::DivSOp>(op)) {
    e = "(static_cast<uint64_t>((" + expr(divs.getRhs()) +
        ") != 0 ? static_cast<int64_t>(" + expr(divs.getLhs()) +
        ") / static_cast<int64_t>(" + expr(divs.getRhs()) +
        ") : 0))";
  } else if (auto modu = mlir::dyn_cast<circt::comb::ModUOp>(op)) {
    e = "((" + expr(modu.getRhs()) +
        ") != 0 ? static_cast<uint64_t>(" + expr(modu.getLhs()) +
        ") % static_cast<uint64_t>(" + expr(modu.getRhs()) +
        ") : 0ULL)";
  } else if (auto mods = mlir::dyn_cast<circt::comb::ModSOp>(op)) {
    e = "(static_cast<uint64_t>((" + expr(mods.getRhs()) +
        ") != 0 ? static_cast<int64_t>(" + expr(mods.getLhs()) +
        ") % static_cast<int64_t>(" + expr(mods.getRhs()) +
        ") : 0))";
  } else if (mlir::isa<circt::hw::BitcastOp>(op)) {
    if (auto arr_ty =
            mlir::dyn_cast<circt::hw::ArrayType>(op.getResult(0).getType())) {
      unsigned depth = arr_ty.getNumElements();
      unsigned elem_w = hirct::get_type_width(arr_ty.getElementType());
      if (elem_w == 0)
        elem_w = 1;
      std::string etype = hirct::cpp_type_for_width(elem_w);
      std::string tn = "t" + std::to_string(tmp_cnt++);
      ofs << "  " << etype << " " << tn << "[" << depth << "] = {};\n";
      val[op.getResult(0)] = tn;
      return "";
    }
    e = "static_cast<" + ctype + ">(" + expr(op.getOperand(0)) + ")";
  } else if (auto ac = mlir::dyn_cast<circt::hw::ArrayCreateOp>(op)) {
    std::string rn = next_tmp();
    ofs << "  const " << ctype << " " << rn << "[] = {";
    auto operands = ac.getOperands();
    for (int i = static_cast<int>(operands.size()) - 1; i >= 0; --i) {
      if (i < static_cast<int>(operands.size()) - 1)
        ofs << ", ";
      ofs << "static_cast<" << ctype << ">(" << expr(operands[i])
          << ")";
    }
    ofs << "};\n";
    val[result] = rn;
    return "";
  } else if (auto ag = mlir::dyn_cast<circt::hw::ArrayGetOp>(op)) {
    std::string arr_e = expr(ag.getInput());
    std::string idx_e = expr(ag.getIndex());
    auto arr_ty =
        mlir::dyn_cast<circt::hw::ArrayType>(ag.getInput().getType());
    unsigned sz = arr_ty ? arr_ty.getNumElements() : 0;
    bool arr_is_literal_zero =
        !ag.getInput().getDefiningOp() ? false
        : (arr_e == "0" || arr_e == "static_cast<uint8_t>(0)" ||
           arr_e == "static_cast<uint16_t>(0)" ||
           arr_e == "static_cast<uint32_t>(0)" ||
           arr_e == "static_cast<uint64_t>(0)");
    if (sz > 0 && !arr_is_literal_zero)
      e = "(static_cast<size_t>(" + idx_e + ") < " +
          std::to_string(sz) + " ? " + arr_e +
          "[static_cast<size_t>(" + idx_e + ")] : 0)";
    else
      e = "0";
  } else if (auto ai = mlir::dyn_cast<circt::hw::ArrayInjectOp>(op)) {
    auto arr_ty =
        mlir::dyn_cast<circt::hw::ArrayType>(ai.getInput().getType());
    if (!arr_ty)
      return "\x01";
    std::string arr_e = expr(ai.getInput());
    std::string idx_e = expr(ai.getIndex());
    std::string elem_e = expr(ai.getElement());
    unsigned arr_depth = arr_ty.getNumElements();
    unsigned elem_w = hirct::get_type_width(arr_ty.getElementType());
    if (elem_w == 0)
      elem_w = 1;
    std::string etype = hirct::cpp_type_for_width(elem_w);
    std::string tn = "t" + std::to_string(tmp_cnt++);
    ofs << "  " << etype << " " << tn << "[" << arr_depth << "];\n";
    ofs << "  for (int __i = 0; __i < " << arr_depth << "; ++__i) "
        << tn << "[__i] = " << arr_e << "[__i];\n";
    ofs << "  " << tn << "[static_cast<size_t>(" << idx_e << ") % "
        << arr_depth << "] = static_cast<" << etype << ">(" << elem_e
        << ");\n";
    val[op.getResult(0)] = tn;
    return "";
  } else if (auto agg =
                 mlir::dyn_cast<circt::hw::AggregateConstantOp>(op)) {
    std::string rn = next_tmp();
    // For array-typed constants use the element type, not the aggregate width.
    std::string elem_ctype = ctype;
    if (auto arr_ty =
            mlir::dyn_cast<circt::hw::ArrayType>(result.getType())) {
      unsigned elem_w = hirct::get_type_width(arr_ty.getElementType());
      if (elem_w == 0)
        elem_w = 1;
      elem_ctype = hirct::cpp_type_for_width(elem_w);
    }
    ofs << "  const " << elem_ctype << " " << rn << "[] = {";
    auto fields = agg.getFields();
    for (size_t i = 0; i < fields.size(); ++i) {
      if (i > 0)
        ofs << ", ";
      if (auto ia = mlir::dyn_cast<mlir::IntegerAttr>(fields[i]))
        ofs << "static_cast<" << elem_ctype << ">("
            << ia.getValue().zextOrTrunc(64).getZExtValue() << ")";
      else
        ofs << "0";
    }
    ofs << "};\n";
    val[result] = rn;
    return "";
  } else {
    return "\x01";
  }

  return e;
}

} // namespace hirct
