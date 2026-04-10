#include "hirct/Target/EmitExpr.h"
#include "hirct/Analysis/IRAnalysis.h"
#include "circt/Dialect/Arc/ArcOps.h"
#include "circt/Dialect/Comb/CombOps.h"
#include "circt/Dialect/HW/HWOps.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/SymbolTable.h"

#include <functional>
#include <iostream>
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
    // ConcatOp: operands are ordered MSB-first. We compute the bit offset of
    // each operand from the LSB, then only include operands whose bits overlap
    // with the lower 64 bits of the result. This avoids UB shifts >= 64.
    unsigned total_w = w_of(concat.getResult());
    struct ConcatSlice { unsigned lo; unsigned ow; mlir::Value val; };
    llvm::SmallVector<ConcatSlice> slices;
    {
      unsigned cursor = total_w;
      for (auto operand : concat.getOperands()) {
        unsigned ow = w_of(operand);
        cursor -= ow;
        slices.push_back({cursor, ow, operand});
      }
    }
    std::string acc = "0ULL";
    for (auto &s : slices) {
      if (s.lo >= 64)
        continue;
      std::string oe = expr(s.val);
      unsigned usable = std::min(s.ow, 64u - s.lo);
      std::string masked = "(static_cast<uint64_t>(" + oe + ") & " +
                           width_mask_expr(usable) + ")";
      if (s.lo == 0)
        acc = "(" + acc + " | " + masked + ")";
      else
        acc = "(" + acc + " | (" + masked + " << " + std::to_string(s.lo) + "))";
    }
    e = "(" + acc + ")";
  } else if (auto ext = mlir::dyn_cast<circt::comb::ExtractOp>(op)) {
    unsigned from = ext.getLowBit();
    unsigned src_w = w_of(ext.getInput());
    if (from >= 64 && src_w > 64) {
      // Extracting from bit >= 64 of a wide value. The scalar val only holds
      // the lower 64 bits, so the result is 0 within the scalar model.
      // If the source is a wide port array, access the correct word.
      std::string src_e = expr(ext.getInput());
      unsigned wordIdx = from / 64;
      unsigned bitInWord = from % 64;
      // Check if src_e is a port name (wide port array); if so use word access.
      // We generate a conditional expression that is always compile-safe.
      e = "((static_cast<uint64_t>(" + src_e + "[" + std::to_string(wordIdx) +
          "]) >> " + std::to_string(bitInWord) + ") & " + width_mask_expr(w) + ")";
    } else if (from >= 64) {
      // Source is <= 64 bits but from >= 64: logically unreachable in valid IR,
      // but guard against UB.
      e = "0ULL";
    } else {
      e = "((static_cast<uint64_t>(" + expr(ext.getInput()) + ") >> " +
          std::to_string(from) + ") & " + width_mask_expr(w) + ")";
    }
  } else if (auto rep = mlir::dyn_cast<circt::comb::ReplicateOp>(op)) {
    unsigned src_w = w_of(rep.getInput());
    int cnt = (src_w > 0) ? w / src_w : 0;
    std::ostringstream oss;
    oss << "(";
    bool first = true;
    for (int i = 0; i < cnt; ++i) {
      unsigned shift = static_cast<unsigned>(i) * src_w;
      if (shift >= 64)
        break;
      if (!first)
        oss << " | ";
      first = false;
      oss << "(static_cast<uint64_t>(" << expr(rep.getInput())
          << ") << " << shift << ")";
    }
    if (first)
      oss << "0ULL";
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
  } else if (op.getName().getStringRef() == "arc.call") {
    unsigned numResults = op.getNumResults();
    if (numResults == 0)
      return "";
    unsigned resultW = w_of(result);
    if (resultW > 64) {
      std::string rn = next_tmp();
      unsigned numWords = (resultW + 63) / 64;
      ofs << "  uint64_t " << rn << "[" << numWords << "];\n";
      for (unsigned ri = 0; ri < numWords; ++ri) {
        unsigned bitsForWord = std::min(64u, resultW - ri * 64);
        std::string re =
            inline_arc_call(op, 0, val, ofs, tmp_cnt, 0,
                            ri, bitsForWord);
        ofs << "  " << rn << "[" << ri << "] = static_cast<uint64_t>("
            << re << ") & " << width_mask_expr(bitsForWord) << ";\n";
      }
      val[result] = rn;
      return "";
    }
    e = inline_arc_call(op, 0, val, ofs, tmp_cnt, 0);
  } else {
    return "\x01";
  }

  return e;
}

static constexpr unsigned kMaxInlineDepth = 8;

static std::string render_callee_expr(
    mlir::Operation *def,
    mlir::Value val,
    llvm::DenseMap<mlir::Value, std::string> &argMap,
    llvm::DenseMap<mlir::Value, std::string> &outerVal,
    std::ofstream &ofs, int &tmp_cnt, unsigned depth,
    unsigned wordIdx, unsigned wordBits) {
  auto renderOp = [&](mlir::Value v) -> std::string {
    return render_in_callee_body(v, argMap, outerVal, ofs, tmp_cnt, depth,
                                 0, 0);
  };
  auto renderVariadic = [&](const char *cOp) -> std::string {
    std::string acc = renderOp(def->getOperand(0));
    for (unsigned i = 1; i < def->getNumOperands(); ++i)
      acc = "(" + acc + " " + cOp + " " + renderOp(def->getOperand(i)) + ")";
    return acc;
  };

  llvm::StringRef opName = def->getName().getStringRef();

  if (opName == "hw.constant") {
    if (auto attr = def->getAttrOfType<mlir::IntegerAttr>("value")) {
      uint64_t uv = attr.getValue().zextOrTrunc(64).getZExtValue();
      unsigned w = attr.getValue().getBitWidth();
      std::string ctype = hirct::cpp_type_for_width(w);
      std::string vs = std::to_string(uv);
      if (uv > static_cast<uint64_t>(std::numeric_limits<long long>::max()))
        vs += "ULL";
      return "static_cast<" + ctype + ">(" + vs + ")";
    }
    return "0";
  }

  if (opName == "comb.add" && def->getNumOperands() >= 2)
    return renderVariadic("+");
  if (opName == "comb.mul" && def->getNumOperands() >= 2)
    return renderVariadic("*");
  if (opName == "comb.and" && def->getNumOperands() >= 2)
    return renderVariadic("&");
  if (opName == "comb.or" && def->getNumOperands() >= 2)
    return renderVariadic("|");
  if (opName == "comb.xor" && def->getNumOperands() >= 2)
    return renderVariadic("^");
  if (opName == "comb.sub" && def->getNumOperands() == 2)
    return "(" + renderOp(def->getOperand(0)) + " - " +
           renderOp(def->getOperand(1)) + ")";
  if (opName == "comb.shl" && def->getNumOperands() == 2)
    return "(static_cast<uint64_t>(" + renderOp(def->getOperand(0)) +
           ") << (static_cast<uint64_t>(" + renderOp(def->getOperand(1)) +
           ") & 63ULL))";
  if (opName == "comb.shru" && def->getNumOperands() == 2)
    return "(static_cast<uint64_t>(" + renderOp(def->getOperand(0)) +
           ") >> (static_cast<uint64_t>(" + renderOp(def->getOperand(1)) +
           ") & 63ULL))";
  if (opName == "comb.shrs" && def->getNumOperands() == 2)
    return "(static_cast<int64_t>(" + renderOp(def->getOperand(0)) +
           ") >> (static_cast<uint64_t>(" + renderOp(def->getOperand(1)) +
           ") & 63ULL))";

  if (opName == "comb.mux" && def->getNumOperands() == 3) {
    if (auto arrTy =
            mlir::dyn_cast<circt::hw::ArrayType>(val.getType())) {
      unsigned arrDepth = arrTy.getNumElements();
      unsigned elemW = hirct::get_type_width(arrTy.getElementType());
      if (elemW == 0) elemW = 1;
      std::string etype = hirct::cpp_type_for_width(elemW);
      std::string tn = "t" + std::to_string(tmp_cnt++);
      ofs << "  " << etype << " " << tn << "[" << arrDepth << "];\n";
      ofs << "  for (int __i = 0; __i < " << arrDepth << "; ++__i) "
          << tn << "[__i] = (" << renderOp(def->getOperand(0)) << ") ? "
          << renderOp(def->getOperand(1)) << "[__i] : "
          << renderOp(def->getOperand(2)) << "[__i];\n";
      argMap[val] = tn;
      return tn;
    }
    return "(" + renderOp(def->getOperand(0)) + " ? " +
           renderOp(def->getOperand(1)) + " : " +
           renderOp(def->getOperand(2)) + ")";
  }

  if (opName == "comb.icmp") {
    auto predAttr = def->getAttrOfType<mlir::IntegerAttr>("predicate");
    if (predAttr && def->getNumOperands() == 2) {
      std::string cOp;
      using P = circt::comb::ICmpPredicate;
      switch (static_cast<P>(predAttr.getInt())) {
      case P::eq: case P::ceq: case P::weq: cOp = "=="; break;
      case P::ne: case P::cne: case P::wne: cOp = "!="; break;
      case P::ult: cOp = "<"; break;
      case P::ule: cOp = "<="; break;
      case P::ugt: cOp = ">"; break;
      case P::uge: cOp = ">="; break;
      case P::slt: cOp = "<"; break;
      case P::sle: cOp = "<="; break;
      case P::sgt: cOp = ">"; break;
      case P::sge: cOp = ">="; break;
      }
      unsigned cw = 0;
      if (auto intTy =
              mlir::dyn_cast<mlir::IntegerType>(def->getOperand(0).getType()))
        cw = intTy.getWidth();
      std::string lhs = renderOp(def->getOperand(0));
      std::string rhs = renderOp(def->getOperand(1));
      if (cw > 0 && cw < 64 &&
          (static_cast<P>(predAttr.getInt()) == P::eq ||
           static_cast<P>(predAttr.getInt()) == P::ceq ||
           static_cast<P>(predAttr.getInt()) == P::weq ||
           static_cast<P>(predAttr.getInt()) == P::ne ||
           static_cast<P>(predAttr.getInt()) == P::cne ||
           static_cast<P>(predAttr.getInt()) == P::wne)) {
        std::string mask = width_mask_expr(cw);
        return "(((" + lhs + ") & " + mask + ") " + cOp + " ((" + rhs +
               ") & " + mask + "))";
      }
      return "((" + lhs + ") " + cOp + " (" + rhs + "))";
    }
    return "0";
  }

  if (opName == "comb.concat") {
    unsigned totalW = 0;
    if (auto ty = mlir::dyn_cast<mlir::IntegerType>(val.getType()))
      totalW = ty.getWidth();

    unsigned wLo = wordIdx * 64;
    unsigned wHi = wLo + (wordBits > 0 ? wordBits : 64);
    if (wordBits == 0 && totalW <= 64) {
      wLo = 0;
      wHi = totalW;
    }

    struct Slice { unsigned lo; unsigned ow; unsigned opIdx; };
    llvm::SmallVector<Slice> slices;
    {
      unsigned cursor = totalW;
      for (unsigned i = 0; i < def->getNumOperands(); ++i) {
        unsigned ow = 0;
        if (auto ty = mlir::dyn_cast<mlir::IntegerType>(
                def->getOperand(i).getType()))
          ow = ty.getWidth();
        cursor -= ow;
        slices.push_back({cursor, ow, i});
      }
    }

    std::string acc = "0ULL";
    for (auto &s : slices) {
      unsigned sHi = s.lo + s.ow;
      if (sHi <= wLo || s.lo >= wHi)
        continue;
      unsigned overlapLo = std::max(s.lo, wLo);
      unsigned overlapHi = std::min(sHi, wHi);
      unsigned usable = overlapHi - overlapLo;
      unsigned srcShift = overlapLo - s.lo;
      unsigned dstShift = overlapLo - wLo;

      std::string part = renderOp(def->getOperand(s.opIdx));
      std::string shifted = part;
      if (srcShift > 0)
        shifted = "(static_cast<uint64_t>(" + shifted + ") >> " +
                  std::to_string(srcShift) + ")";
      std::string masked = "(static_cast<uint64_t>(" + shifted + ") & " +
                           width_mask_expr(usable) + ")";
      if (dstShift == 0)
        acc = "(" + acc + " | " + masked + ")";
      else
        acc = "(" + acc + " | (" + masked + " << " +
              std::to_string(dstShift) + "))";
    }
    return "(" + acc + ")";
  }

  if (opName == "comb.extract") {
    auto lowBitAttr = def->getAttrOfType<mlir::IntegerAttr>("lowBit");
    unsigned from = lowBitAttr ? lowBitAttr.getInt() : 0;
    unsigned resultW = 0;
    if (auto ty = mlir::dyn_cast<mlir::IntegerType>(val.getType()))
      resultW = ty.getWidth();
    unsigned srcW = 0;
    if (auto ty = mlir::dyn_cast<mlir::IntegerType>(
            def->getOperand(0).getType()))
      srcW = ty.getWidth();
    if (srcW > 64) {
      unsigned wIdx = from / 64;
      unsigned bitInWord = from % 64;
      std::string srcE = render_in_callee_body(
          def->getOperand(0), argMap, outerVal, ofs, tmp_cnt, depth,
          wIdx, 64);
      if (bitInWord == 0)
        return "(static_cast<uint64_t>(" + srcE + ") & " +
               width_mask_expr(resultW) + ")";
      return "((static_cast<uint64_t>(" + srcE + ") >> " +
             std::to_string(bitInWord) + ") & " +
             width_mask_expr(resultW) + ")";
    }
    if (from >= 64)
      return "0ULL";
    return "((static_cast<uint64_t>(" + renderOp(def->getOperand(0)) +
           ") >> " + std::to_string(from) + ") & " +
           width_mask_expr(resultW) + ")";
  }

  if (opName == "comb.replicate") {
    unsigned srcW = 0;
    if (auto ty = mlir::dyn_cast<mlir::IntegerType>(
            def->getOperand(0).getType()))
      srcW = ty.getWidth();
    unsigned w = 0;
    if (auto ty = mlir::dyn_cast<mlir::IntegerType>(val.getType()))
      w = ty.getWidth();
    int cnt = (srcW > 0) ? w / srcW : 0;
    std::ostringstream oss;
    oss << "(";
    bool first = true;
    for (int i = 0; i < cnt; ++i) {
      unsigned shift = static_cast<unsigned>(i) * srcW;
      if (shift >= 64) break;
      if (!first) oss << " | ";
      first = false;
      oss << "(static_cast<uint64_t>(" << renderOp(def->getOperand(0))
          << ") << " << shift << ")";
    }
    if (first) oss << "0ULL";
    oss << ")";
    return oss.str();
  }

  if (auto ac = mlir::dyn_cast<circt::hw::ArrayCreateOp>(def)) {
    unsigned elemW = 0;
    if (auto arrTy = mlir::dyn_cast<circt::hw::ArrayType>(val.getType()))
      elemW = hirct::get_type_width(arrTy.getElementType());
    if (elemW == 0) elemW = 1;
    std::string etype = hirct::cpp_type_for_width(elemW);
    std::string rn = "t" + std::to_string(tmp_cnt++);
    auto operands = ac.getOperands();
    ofs << "  const " << etype << " " << rn << "[] = {";
    for (int i = static_cast<int>(operands.size()) - 1; i >= 0; --i) {
      if (i < static_cast<int>(operands.size()) - 1) ofs << ", ";
      ofs << "static_cast<" << etype << ">(" << renderOp(operands[i]) << ")";
    }
    ofs << "};\n";
    argMap[val] = rn;
    return rn;
  }

  if (auto ag = mlir::dyn_cast<circt::hw::ArrayGetOp>(def)) {
    std::string arrE = renderOp(ag.getInput());
    std::string idxE = renderOp(ag.getIndex());
    auto arrTy =
        mlir::dyn_cast<circt::hw::ArrayType>(ag.getInput().getType());
    unsigned sz = arrTy ? arrTy.getNumElements() : 0;
    if (sz > 0)
      return "(static_cast<size_t>(" + idxE + ") < " +
             std::to_string(sz) + " ? " + arrE +
             "[static_cast<size_t>(" + idxE + ")] : 0)";
    return "0";
  }

  if (opName == "hw.bitcast") {
    return "static_cast<uint64_t>(" + renderOp(def->getOperand(0)) + ")";
  }

  if (opName == "comb.parity") {
    return "static_cast<bool>(__builtin_parityll(static_cast<uint64_t>(" +
           renderOp(def->getOperand(0)) + ")))";
  }

  if (opName == "seq.to_clock" || opName == "seq.from_clock") {
    return renderOp(def->getOperand(0));
  }

  if (opName == "arc.call") {
    unsigned ri = 0;
    if (auto opResult = mlir::dyn_cast<mlir::OpResult>(val))
      ri = opResult.getResultNumber();
    return inline_arc_call(*def, ri, outerVal, ofs, tmp_cnt, depth + 1,
                           wordIdx, wordBits);
  }

  std::cerr << "GenModel: unsupported op (in arc.call body) '"
            << opName.str() << "'\n";
  return "0";
}

std::string render_in_callee_body(
    mlir::Value val,
    llvm::DenseMap<mlir::Value, std::string> &argMap,
    llvm::DenseMap<mlir::Value, std::string> &outerVal,
    std::ofstream &ofs, int &tmp_cnt, unsigned depth,
    unsigned wordIdx, unsigned wordBits) {
  if (wordIdx == 0 && wordBits == 0) {
    auto it = argMap.find(val);
    if (it != argMap.end())
      return it->second;
  }

  mlir::Operation *def = val.getDefiningOp();
  if (!def) {
    auto it = argMap.find(val);
    if (it == argMap.end())
      return "0";
    unsigned valW = 0;
    if (auto ty = mlir::dyn_cast<mlir::IntegerType>(val.getType()))
      valW = ty.getWidth();
    if (valW > 64 && (wordIdx > 0 || wordBits > 0))
      return it->second + "[" + std::to_string(wordIdx) + "]";
    return it->second;
  }

  std::string result = render_callee_expr(
      def, val, argMap, outerVal, ofs, tmp_cnt, depth, wordIdx, wordBits);
  if (wordIdx == 0 && wordBits == 0)
    argMap[val] = result;
  return result;
}

std::string inline_arc_call(
    mlir::Operation &callOp, unsigned resultIdx,
    llvm::DenseMap<mlir::Value, std::string> &outerVal,
    std::ofstream &ofs, int &tmp_cnt, unsigned depth,
    unsigned wordIdx, unsigned wordBits) {
  if (depth >= kMaxInlineDepth) {
    std::cerr << "GenModel: arc.call inline depth limit reached\n";
    return "0";
  }

  auto callSymRef =
      callOp.getAttrOfType<mlir::FlatSymbolRefAttr>("arc");
  if (!callSymRef)
    return "0";

  auto parentModule = callOp.getParentOfType<mlir::ModuleOp>();
  if (!parentModule)
    return "0";

  auto *calleeSym = parentModule.lookupSymbol(callSymRef.getValue());
  auto calleeDef =
      mlir::dyn_cast_or_null<circt::arc::DefineOp>(calleeSym);
  if (!calleeDef || calleeDef.getBody().empty())
    return "0";

  mlir::Block &body = calleeDef.getBody().front();
  auto outputOp =
      mlir::dyn_cast<circt::arc::OutputOp>(body.getTerminator());
  if (!outputOp || outputOp.getOutputs().empty())
    return "0";

  if (resultIdx >= outputOp.getOutputs().size())
    resultIdx = 0;

  llvm::DenseMap<mlir::Value, std::string> argMap;
  for (unsigned i = 0;
       i < callOp.getNumOperands() && i < body.getNumArguments(); ++i) {
    mlir::Value callOperand = callOp.getOperand(i);
    auto it = outerVal.find(callOperand);
    argMap[body.getArgument(i)] = (it != outerVal.end()) ? it->second : "0";
  }

  mlir::Value outVal = outputOp.getOutputs()[resultIdx];
  return render_in_callee_body(outVal, argMap, outerVal, ofs, tmp_cnt, depth,
                               wordIdx, wordBits);
}

} // namespace hirct
