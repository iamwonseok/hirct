//===- CModelEmitter.cpp - C model code emitter -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "hirct/Target/CModelEmitter.h"
#include "hirct/SemanticModel/Validation.h"

#include "circt/Dialect/Arc/ArcOps.h"
#include "llvm/Support/FileSystem.h"
#include "circt/Dialect/Comb/CombDialect.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/Seq/SeqOps.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/TypeSwitch.h"
#include "llvm/Support/ErrorHandling.h"

namespace hirct {

std::string CModelEmitter::legalCType(unsigned width) {
  if (width == 0)
    return "uint8_t";
  if (width <= 8)
    return "uint8_t";
  if (width <= 16)
    return "uint16_t";
  if (width <= 32)
    return "uint32_t";
  if (width <= 64)
    return "uint64_t";
  llvm_unreachable("legalCType called for width >64; use wide storage instead");
}

std::string CModelEmitter::legalSignedCType(unsigned width) {
  if (width == 0)
    return "int8_t";
  if (width <= 8)
    return "int8_t";
  if (width <= 16)
    return "int16_t";
  if (width <= 32)
    return "int32_t";
  if (width <= 64)
    return "int64_t";
  return "int64_t";
}

std::string CModelEmitter::renderIcmpExpr(int64_t predicate,
                                          const std::string &lhs,
                                          const std::string &rhs,
                                          unsigned operandWidth) {
  bool isSigned = (predicate >= 2 && predicate <= 5);

  std::string cOp;
  switch (predicate) {
  case 0: cOp = "=="; break;
  case 1: cOp = "!="; break;
  case 2: cOp = "<"; break;   // slt
  case 3: cOp = "<="; break;  // sle
  case 4: cOp = ">"; break;   // sgt
  case 5: cOp = ">="; break;  // sge
  case 6: cOp = "<"; break;   // ult
  case 7: cOp = "<="; break;  // ule
  case 8: cOp = ">"; break;   // ugt
  case 9: cOp = ">="; break;  // uge
  case 10: cOp = "=="; break; // ceq
  case 11: cOp = "!="; break; // cne
  case 12: cOp = "=="; break; // weq
  case 13: cOp = "!="; break; // wne
  default: return "/* unsupported_icmp_pred */0";
  }

  if (isSigned) {
    std::string sType = legalSignedCType(operandWidth);
    return "((" + sType + ")(" + lhs + ") " + cOp + " (" + sType + ")(" +
           rhs + "))";
  }
  return "(" + lhs + " " + cOp + " " + rhs + ")";
}

std::string CModelEmitter::wideStorageDecl(const semantic::PortInfo &port,
                                           llvm::StringRef prefix) {
  unsigned words = port.wordCount();
  return ("  uint64_t " + prefix + port.name + "[" + std::to_string(words) +
          "];\n")
      .str();
}

void CModelEmitter::emitWideInputApi(llvm::raw_string_ostream &os,
                                     const semantic::PortInfo &port) {
  const auto &mod = model_.moduleName;
  unsigned words = port.wordCount();
  os << "void " << mod << "_set_" << port.name << "_word(" << mod
     << "_state *s, size_t idx, uint64_t v);\n";
  os << "void " << mod << "_set_" << port.name << "_words(" << mod
     << "_state *s, const uint64_t *src, size_t count);\n";
  os << "uint64_t " << mod << "_get_" << port.name << "_word(const " << mod
     << "_state *s, size_t idx);\n";
  os << "size_t " << mod << "_get_" << port.name
     << "_word_count(void);\n";
  (void)words;
}

void CModelEmitter::emitWideOutputApi(llvm::raw_string_ostream &os,
                                      const semantic::PortInfo &port) {
  const auto &mod = model_.moduleName;
  os << "uint64_t " << mod << "_get_" << port.name << "_word(const " << mod
     << "_state *s, size_t idx);\n";
  os << "void " << mod << "_get_" << port.name << "_words(const " << mod
     << "_state *s, uint64_t *dst, size_t count);\n";
  os << "size_t " << mod << "_get_" << port.name
     << "_word_count(void);\n";
}

CModelEmitter::CModelEmitter(const semantic::ModuleModel &model,
                              const CModelOptions &options,
                              circt::hw::HWModuleOp hwModule)
    : model_(model), options_(options), hwModule_(hwModule) {}

CModelArtifact CModelEmitter::emit() {
  CModelArtifact artifact;
  artifact.moduleName = model_.moduleName;
  artifact.outputRoot = options_.outputDir;
  artifact.headerPath =
      options_.outputDir + "/" + model_.moduleName + options_.headerSuffix;
  artifact.implPath =
      options_.outputDir + "/" + model_.moduleName + options_.implSuffix;

  std::string headerBuf;
  llvm::raw_string_ostream headerOS(headerBuf);
  emitHeader(headerOS);
  artifact.headerContent = std::move(headerBuf);

  std::string implBuf;
  llvm::raw_string_ostream implOS(implBuf);
  emitImpl(implOS);
  artifact.implContent = std::move(implBuf);

  return artifact;
}

void CModelEmitter::emitChildIncludes(llvm::raw_string_ostream &os) {
  llvm::SmallVector<std::string> seen;
  for (const auto &child : childInstances_) {
    bool dup = false;
    for (const auto &s : seen)
      if (s == child.childModuleName) { dup = true; break; }
    if (dup) continue;
    seen.push_back(child.childModuleName);
    os << "#include \"" << child.childModuleName << options_.headerSuffix
       << "\"\n";
  }
}

void CModelEmitter::emitChildStateFields(llvm::raw_string_ostream &os) {
  for (const auto &child : childInstances_)
    os << "  " << child.childModuleName << "_state " << child.instanceName
       << ";\n";
}

void CModelEmitter::emitChildInitCalls(llvm::raw_string_ostream &os) {
  for (const auto &child : childInstances_)
    os << "  " << child.childModuleName << "_initialize(&s->"
       << child.instanceName << ");\n";
}

void CModelEmitter::emitChildEvalCombWiring(llvm::raw_string_ostream &os) {
  if (childInstances_.empty() || !hwModule_)
    return;

  // Build name→InstanceOp map from the body block for operand/result access
  llvm::StringMap<circt::hw::InstanceOp> instMap;
  auto *bodyBlock = hwModule_.getBodyBlock();
  for (auto &op : bodyBlock->getOperations()) {
    auto inst = mlir::dyn_cast<circt::hw::InstanceOp>(op);
    if (!inst)
      continue;
    instMap[inst.getInstanceName()] = inst;
  }

  // Emit child wiring in childInstances_ order (topologically sorted by caller)
  for (const auto &info : childInstances_) {
    auto it = instMap.find(info.instanceName);
    if (it == instMap.end())
      continue;
    circt::hw::InstanceOp inst = it->second;

    os << "  // --- child instance: " << info.instanceName << " ("
       << info.childModuleName << ") ---\n";

    // Set child inputs
    for (unsigned i = 0; i < inst.getNumOperands() && i < info.inputPorts.size();
         ++i) {
      const auto &portPair = info.inputPorts[i];
      if (portPair.second > 64) {
        os << "  /* TODO: wide input " << portPair.first << " */\n";
        continue;
      }
      std::string valExpr = renderExpr(inst.getOperand(i));
      os << "  " << info.childModuleName << "_set_" << portPair.first
         << "(&s->" << info.instanceName << ", (" << legalCType(portPair.second)
         << ")(" << valExpr << "));\n";
    }

    // Call child eval_comb
    os << "  " << info.childModuleName << "_eval_comb(&s->"
       << info.instanceName << ");\n";

    // Get child outputs → staged local bindings
    for (unsigned i = 0; i < inst.getNumResults() && i < info.outputPorts.size();
         ++i) {
      const auto &portPair = info.outputPorts[i];
      if (portPair.second > 64) {
        os << "  /* TODO: wide output " << portPair.first << " */\n";
        continue;
      }
      std::string localName =
          info.instanceName + "_" + portPair.first;
      os << "  " << legalCType(portPair.second) << " " << localName << " = "
         << info.childModuleName << "_get_" << portPair.first << "(&s->"
         << info.instanceName << ");\n";
      exprCache_[inst.getResult(i)] = localName;
    }
  }
}

void CModelEmitter::emitChildEvalClockCalls(llvm::raw_string_ostream &os,
                                            llvm::StringRef clockDomain) {
  for (const auto &child : childInstances_) {
    bool childHasDomain = false;
    for (const auto &cd : child.clockDomains) {
      if (cd == clockDomain) {
        childHasDomain = true;
        break;
      }
    }
    if (!childHasDomain)
      continue;
    os << "  " << child.childModuleName << "_eval_" << clockDomain << "(&s->"
       << child.instanceName << ");\n";
  }
}

void CModelEmitter::preSeedInstanceBindings() {
  // no-op: bindings are seeded during emitChildEvalCombWiring
}

void CModelEmitter::resetExprCacheForFunction() {
  exprCache_.clear();
}

void CModelEmitter::emitHeader(llvm::raw_string_ostream &os) {
  const auto &mod = model_.moduleName;

  os << "#ifndef " << mod << "_MODEL_H\n";
  os << "#define " << mod << "_MODEL_H\n\n";
  os << "#include <stdint.h>\n";
  os << "#include <stddef.h>\n";
  os << "#include <string.h>\n";
  emitChildIncludes(os);
  os << "\n";
  os << "#ifdef __cplusplus\n";
  os << "extern \"C\" {\n";
  os << "#endif\n\n";

  os << "typedef struct " << mod << "_state {\n";

  for (const auto &port : model_.inputPorts) {
    if (port.isWide())
      os << wideStorageDecl(port, "input_");
    else
      os << "  " << legalCType(port.width) << " input_" << port.name << ";\n";
  }

  for (const auto &port : model_.outputPorts) {
    if (port.isWide())
      os << wideStorageDecl(port, "output_");
    else if (port.isAggregate)
      os << "  " << legalCType(port.elementWidth) << " output_" << port.name
         << "[" << port.numElements << "];\n";
    else
      os << "  " << legalCType(port.width) << " output_" << port.name << ";\n";
  }

  for (const auto &sv : model_.stateVars)
    os << "  " << legalCType(sv.width) << " " << sv.stableName << ";\n";

  for (const auto &aggVar : model_.aggregateStateVars) {
    if (aggVar.elementWidth > 0 && aggVar.numElements > 0)
      os << "  " << legalCType(aggVar.elementWidth) << " "
         << aggVar.stableName << "[" << aggVar.numElements << "];\n";
  }

  for (const auto &mem : model_.memoryVars)
    os << "  " << legalCType(mem.elementWidth) << " " << mem.stableName << "["
       << mem.depth << "];\n";

  emitChildStateFields(os);

  os << "} " << mod << "_state;\n\n";

  os << "void " << mod << "_initialize(" << mod << "_state *s);\n";

  for (const auto &port : model_.inputPorts) {
    if (port.isWide())
      emitWideInputApi(os, port);
    else
      os << "void " << mod << "_set_" << port.name << "(" << mod
         << "_state *s, " << legalCType(port.width) << " v);\n";
  }

  for (const auto &port : model_.outputPorts) {
    if (port.isWide())
      emitWideOutputApi(os, port);
    else if (port.isAggregate)
      os << "void " << mod << "_get_" << port.name << "(const " << mod
         << "_state *s, " << legalCType(port.elementWidth) << " *dst, size_t count);\n";
    else
      os << legalCType(port.width) << " " << mod << "_get_" << port.name
         << "(const " << mod << "_state *s);\n";
  }

  os << "void " << mod << "_eval_comb(" << mod << "_state *s);\n";

  for (const auto &clock : model_.clockDomains)
    os << "void " << mod << "_eval_" << clock << "(" << mod
       << "_state *s);\n";

  os << "\n#ifdef __cplusplus\n";
  os << "}\n";
  os << "#endif\n\n";
  os << "#endif // " << mod << "_MODEL_H\n";
}

void CModelEmitter::emitSetters(llvm::raw_string_ostream &os) {
  const auto &mod = model_.moduleName;
  for (const auto &port : model_.inputPorts) {
    if (port.isWide()) {
      unsigned words = port.wordCount();
      os << "void " << mod << "_set_" << port.name << "_word(" << mod
         << "_state *s, size_t idx, uint64_t v) {\n"
         << "  if (idx < " << words << ") s->input_" << port.name
         << "[idx] = v;\n}\n\n";
      os << "void " << mod << "_set_" << port.name << "_words(" << mod
         << "_state *s, const uint64_t *src, size_t count) {\n"
         << "  size_t n = count < " << words << " ? count : " << words
         << ";\n"
         << "  memcpy(s->input_" << port.name << ", src, n * sizeof(uint64_t));\n"
         << "}\n\n";
      os << "uint64_t " << mod << "_get_" << port.name << "_word(const " << mod
         << "_state *s, size_t idx) {\n"
         << "  return idx < " << words << " ? s->input_" << port.name
         << "[idx] : 0;\n}\n\n";
      os << "size_t " << mod << "_get_" << port.name
         << "_word_count(void) {\n"
         << "  return " << words << ";\n}\n\n";
    } else {
      os << "void " << mod << "_set_" << port.name << "(" << mod
         << "_state *s, " << legalCType(port.width) << " v) {\n"
         << "  s->input_" << port.name << " = v;\n}\n\n";
    }
  }
}

void CModelEmitter::emitGetters(llvm::raw_string_ostream &os) {
  const auto &mod = model_.moduleName;
  for (const auto &port : model_.outputPorts) {
    if (port.isWide()) {
      unsigned words = port.wordCount();
      os << "uint64_t " << mod << "_get_" << port.name << "_word(const " << mod
         << "_state *s, size_t idx) {\n"
         << "  return idx < " << words << " ? s->output_" << port.name
         << "[idx] : 0;\n}\n\n";
      os << "void " << mod << "_get_" << port.name << "_words(const " << mod
         << "_state *s, uint64_t *dst, size_t count) {\n"
         << "  size_t n = count < " << words << " ? count : " << words
         << ";\n"
         << "  memcpy(dst, s->output_" << port.name
         << ", n * sizeof(uint64_t));\n"
         << "}\n\n";
      os << "size_t " << mod << "_get_" << port.name
         << "_word_count(void) {\n"
         << "  return " << words << ";\n}\n\n";
    } else if (port.isAggregate) {
      os << "void " << mod << "_get_" << port.name << "(const " << mod
         << "_state *s, " << legalCType(port.elementWidth)
         << " *dst, size_t count) {\n"
         << "  size_t n = count < " << port.numElements << " ? count : "
         << port.numElements << ";\n"
         << "  memcpy(dst, s->output_" << port.name << ", n * sizeof("
         << legalCType(port.elementWidth) << "));\n"
         << "}\n\n";
    } else {
      os << legalCType(port.width) << " " << mod << "_get_" << port.name
         << "(const " << mod << "_state *s) {\n"
         << "  return s->output_" << port.name << ";\n}\n\n";
    }
  }
}

std::string CModelEmitter::renderExpr(mlir::Value val) {
  auto cacheIt = exprCache_.find(val);
  if (cacheIt != exprCache_.end())
    return cacheIt->second;

  auto result = [&]() -> std::string {
  if (auto blockArg = mlir::dyn_cast<mlir::BlockArgument>(val)) {
    unsigned argNum = blockArg.getArgNumber();
    if (argNum < model_.inputPorts.size()) {
      const auto &port = model_.inputPorts[argNum];
      if (port.isWide())
        return "/* wide_port:" + port.name + " */0";
      return "s->input_" + port.name;
    }
    return "/* unknown_arg_" + std::to_string(argNum) + " */0";
  }

  mlir::Operation *def = val.getDefiningOp();
  if (!def)
    return "/* null_def */0";

  llvm::StringRef opName = def->getName().getStringRef();

  if (opName == "hw.constant") {
    if (auto attr = def->getAttrOfType<mlir::IntegerAttr>("value")) {
      unsigned bitWidth = attr.getValue().getBitWidth();
      if (bitWidth > 64) {
        if (attr.getValue().isZero())
          return "/* wide_const_zero */0";
        return "/* unsupported:hw.constant_wide */0";
      }
      llvm::SmallString<32> buf;
      if (attr.getValue().isNegative()) {
        attr.getValue().toStringSigned(buf);
      } else {
        attr.getValue().toStringUnsigned(buf);
        buf += "u";
      }
      return std::string("(") + legalCType(bitWidth) + ")" +
             std::string(buf);
    }
    return "/* bad_const */0";
  }

  auto renderVariadic = [&](llvm::StringRef cOp) -> std::string {
    std::string result = renderExpr(def->getOperand(0));
    for (unsigned i = 1; i < def->getNumOperands(); ++i)
      result = "(" + result + " " + cOp.str() + " " + renderExpr(def->getOperand(i)) + ")";
    return result;
  };

  if (opName == "comb.xor" && def->getNumOperands() >= 2)
    return renderVariadic("^");
  if (opName == "comb.and" && def->getNumOperands() >= 2)
    return renderVariadic("&");
  if (opName == "comb.or" && def->getNumOperands() >= 2)
    return renderVariadic("|");
  if (opName == "comb.add" && def->getNumOperands() >= 2)
    return renderVariadic("+");
  if (opName == "comb.sub" && def->getNumOperands() == 2) {
    return "(" + renderExpr(def->getOperand(0)) + " - " +
           renderExpr(def->getOperand(1)) + ")";
  }
  if (opName == "comb.mul" && def->getNumOperands() >= 2)
    return renderVariadic("*");
  if (opName == "comb.mux" && def->getNumOperands() == 3) {
    return "(" + renderExpr(def->getOperand(0)) + " ? " +
           renderExpr(def->getOperand(1)) + " : " +
           renderExpr(def->getOperand(2)) + ")";
  }

  if (opName == "comb.icmp") {
    auto predAttr = def->getAttrOfType<mlir::IntegerAttr>("predicate");
    if (predAttr && def->getNumOperands() == 2) {
      unsigned opWidth = 0;
      if (auto ty = mlir::dyn_cast<mlir::IntegerType>(
              def->getOperand(0).getType()))
        opWidth = ty.getWidth();
      return renderIcmpExpr(predAttr.getInt(),
                            renderExpr(def->getOperand(0)),
                            renderExpr(def->getOperand(1)), opWidth);
    }
    return "/* bad_icmp */0";
  }

  if (opName == "comb.concat") {
    unsigned totalWidth = 0;
    if (auto ty = mlir::dyn_cast<mlir::IntegerType>(val.getType()))
      totalWidth = ty.getWidth();
    if (totalWidth > 64)
      return "/* unsupported:comb.concat_wide */0";
    std::string result = "0";
    unsigned shift = 0;
    for (int i = def->getNumOperands() - 1; i >= 0; --i) {
      unsigned opWidth = 0;
      if (auto ty = mlir::dyn_cast<mlir::IntegerType>(def->getOperand(i).getType()))
        opWidth = ty.getWidth();
      std::string part = "((" + legalCType(totalWidth) + ")" +
                         renderExpr(def->getOperand(i)) + " << " +
                         std::to_string(shift) + ")";
      result = "(" + result + " | " + part + ")";
      shift += opWidth;
    }
    return result;
  }

  if (opName == "comb.extract") {
    auto lowBitAttr = def->getAttrOfType<mlir::IntegerAttr>("lowBit");
    unsigned lowBit = lowBitAttr ? lowBitAttr.getInt() : 0;
    unsigned resultWidth = 0;
    if (auto ty = mlir::dyn_cast<mlir::IntegerType>(val.getType()))
      resultWidth = ty.getWidth();

    mlir::Value src = def->getOperand(0);
    unsigned srcWidth = 0;
    if (auto ty = mlir::dyn_cast<mlir::IntegerType>(src.getType()))
      srcWidth = ty.getWidth();

    if (srcWidth > 64) {
      if (auto blockArg = mlir::dyn_cast<mlir::BlockArgument>(src)) {
        unsigned argNum = blockArg.getArgNumber();
        if (argNum < model_.inputPorts.size()) {
          const auto &port = model_.inputPorts[argNum];
          unsigned wordIdx = lowBit / 64;
          unsigned bitInWord = lowBit % 64;
          if (resultWidth <= 64 && bitInWord + resultWidth <= 64) {
            uint64_t mask = resultWidth >= 64 ? ~0ULL : ((1ULL << resultWidth) - 1);
            return "((" + legalCType(resultWidth) + ")((s->input_" + port.name +
                   "[" + std::to_string(wordIdx) + "] >> " +
                   std::to_string(bitInWord) + ") & " +
                   std::to_string(mask) + "u))";
          }
          return "/* wide_extract_cross_word:" + port.name + " */0";
        }
      }
      return "/* unsupported:comb.extract_wide */0";
    }

    uint64_t mask = resultWidth >= 64 ? ~0ULL : ((1ULL << resultWidth) - 1);
    return "((" + legalCType(resultWidth) + ")((" + renderExpr(src) +
           " >> " + std::to_string(lowBit) + ") & " + std::to_string(mask) + "u))";
  }

  if (opName == "comb.parity") {
    return "/* unsupported:comb.parity */0";
  }

  if (opName == "arc.call") {
    unsigned resultIdx = 0;
    if (auto opResult = mlir::dyn_cast<mlir::OpResult>(val))
      resultIdx = opResult.getResultNumber();
    return inlineArcCall(def, resultIdx, nullptr, 0);
  }

  if (opName == "arc.state") {
    // Find which stateVar this arc.state corresponds to
    if (hwModule_) {
      unsigned walkIdx = 0;
      unsigned matchIdx = UINT_MAX;
      hwModule_.walk([&](circt::arc::StateOp s) {
        if (s.getOperation() == def)
          matchIdx = walkIdx;
        ++walkIdx;
      });
      for (const auto &msv : model_.stateVars) {
        if (msv.stateOpIndex == matchIdx)
          return "s->" + msv.stableName;
      }
    }
    return "/* unresolved_arc_state */0";
  }

  if (opName == "seq.firmem.read_port") {
    auto readPort = mlir::cast<circt::seq::FirMemReadOp>(def);
    auto firMem = readPort.getMemory().getDefiningOp<circt::seq::FirMemOp>();
    if (!firMem)
      return "/* firmem_read_no_mem */0";

    std::string memName;
    // Find the MemoryVar index for this FirMemOp
    if (hwModule_) {
      unsigned memIdx = 0;
      hwModule_.walk([&](circt::seq::FirMemOp fm) {
        if (fm.getOperation() == firMem.getOperation()) {
          if (memIdx < model_.memoryVars.size())
            memName = model_.memoryVars[memIdx].stableName;
        }
        ++memIdx;
      });
    }
    if (memName.empty())
      return "/* firmem_read_unresolved */0";

    unsigned depth = firMem.getType().getDepth();
    std::string addrExpr = renderExpr(readPort.getAddress());
    if (depth > 0) {
      if (readPort.getEnable()) {
        std::string enExpr = renderExpr(readPort.getEnable());
        return "(" + enExpr + " ? s->" + memName + "[(" + legalCType(32) +
               ")(" + addrExpr + ") % " + std::to_string(depth) +
               "] : 0)";
      }
      return "s->" + memName + "[(" + legalCType(32) + ")(" + addrExpr +
             ") % " + std::to_string(depth) + "]";
    }
    return "0";
  }

  return "/* unsupported:" + opName.str() + " */0";
  }();
  exprCache_[val] = result;
  return result;
}

std::string CModelEmitter::renderArcExpr(
    mlir::Value val,
    const llvm::DenseMap<mlir::Value, std::string> &argMap) {
  llvm::DenseMap<mlir::Value, std::string> cache;
  return renderArcExprCached(val, argMap, cache);
}

std::string CModelEmitter::renderArcExprCached(
    mlir::Value val,
    const llvm::DenseMap<mlir::Value, std::string> &argMap,
    llvm::DenseMap<mlir::Value, std::string> &cache) {
  auto it = argMap.find(val);
  if (it != argMap.end())
    return it->second;

  auto cacheIt = cache.find(val);
  if (cacheIt != cache.end())
    return cacheIt->second;

  auto storeAndReturn = [&](std::string r) -> std::string {
    cache[val] = r;
    return r;
  };

  mlir::Operation *def = val.getDefiningOp();
  if (!def)
    return storeAndReturn("/* null_def */0");

  llvm::StringRef opName = def->getName().getStringRef();

  if (opName == "hw.constant") {
    if (auto attr = def->getAttrOfType<mlir::IntegerAttr>("value")) {
      unsigned bitWidth = attr.getValue().getBitWidth();
      if (bitWidth > 64) {
        if (attr.getValue().isZero())
          return storeAndReturn("/* wide_const_zero */0");
        return storeAndReturn("/* unsupported:hw.constant_wide */0");
      }
      llvm::SmallString<32> buf;
      if (attr.getValue().isNegative()) {
        attr.getValue().toStringSigned(buf);
      } else {
        attr.getValue().toStringUnsigned(buf);
        buf += "u";
      }
      return storeAndReturn(std::string("(") + legalCType(bitWidth) + ")" +
                            std::string(buf));
    }
    return storeAndReturn("/* bad_const */0");
  }

  auto renderBinOp = [&](llvm::StringRef cOp) -> std::string {
    return "(" + renderArcExprCached(def->getOperand(0), argMap, cache) + " " +
           cOp.str() + " " + renderArcExprCached(def->getOperand(1), argMap, cache) + ")";
  };

  auto renderArcVariadic = [&](llvm::StringRef cOp) -> std::string {
    std::string result = renderArcExprCached(def->getOperand(0), argMap, cache);
    for (unsigned i = 1; i < def->getNumOperands(); ++i)
      result = "(" + result + " " + cOp.str() + " " + renderArcExprCached(def->getOperand(i), argMap, cache) + ")";
    return result;
  };

  if (opName == "comb.xor" && def->getNumOperands() >= 2)
    return storeAndReturn(renderArcVariadic("^"));
  if (opName == "comb.and" && def->getNumOperands() >= 2)
    return storeAndReturn(renderArcVariadic("&"));
  if (opName == "comb.or" && def->getNumOperands() >= 2)
    return storeAndReturn(renderArcVariadic("|"));
  if (opName == "comb.add" && def->getNumOperands() >= 2)
    return storeAndReturn(renderArcVariadic("+"));
  if (opName == "comb.sub" && def->getNumOperands() == 2)
    return storeAndReturn(renderBinOp("-"));
  if (opName == "comb.mul" && def->getNumOperands() >= 2)
    return storeAndReturn(renderArcVariadic("*"));
  if (opName == "comb.mux" && def->getNumOperands() == 3) {
    return storeAndReturn("(" + renderArcExprCached(def->getOperand(0), argMap, cache) + " ? " +
           renderArcExprCached(def->getOperand(1), argMap, cache) + " : " +
           renderArcExprCached(def->getOperand(2), argMap, cache) + ")");
  }

  if (opName == "comb.icmp") {
    auto predAttr = def->getAttrOfType<mlir::IntegerAttr>("predicate");
    if (predAttr && def->getNumOperands() == 2) {
      unsigned opWidth = 0;
      if (auto ty = mlir::dyn_cast<mlir::IntegerType>(
              def->getOperand(0).getType()))
        opWidth = ty.getWidth();
      return storeAndReturn(renderIcmpExpr(predAttr.getInt(),
                            renderArcExprCached(def->getOperand(0), argMap, cache),
                            renderArcExprCached(def->getOperand(1), argMap, cache),
                            opWidth));
    }
    return storeAndReturn("/* bad_icmp */0");
  }

  if (opName == "comb.concat") {
    unsigned totalWidth = 0;
    if (auto ty = mlir::dyn_cast<mlir::IntegerType>(val.getType()))
      totalWidth = ty.getWidth();
    if (totalWidth > 64)
      return storeAndReturn("/* unsupported:comb.concat_wide */0");
    std::string result = "0";
    unsigned shift = 0;
    for (int i = def->getNumOperands() - 1; i >= 0; --i) {
      unsigned opWidth = 0;
      if (auto ty = mlir::dyn_cast<mlir::IntegerType>(def->getOperand(i).getType()))
        opWidth = ty.getWidth();
      std::string part = "((" + legalCType(totalWidth) + ")" +
                         renderArcExprCached(def->getOperand(i), argMap, cache) + " << " +
                         std::to_string(shift) + ")";
      result = "(" + result + " | " + part + ")";
      shift += opWidth;
    }
    return storeAndReturn(result);
  }

  if (opName == "comb.extract") {
    auto lowBitAttr = def->getAttrOfType<mlir::IntegerAttr>("lowBit");
    unsigned lowBit = lowBitAttr ? lowBitAttr.getInt() : 0;
    unsigned resultWidth = 0;
    if (auto ty = mlir::dyn_cast<mlir::IntegerType>(val.getType()))
      resultWidth = ty.getWidth();
    if (resultWidth > 64)
      return storeAndReturn("/* unsupported:comb.extract_wide_result */0");
    uint64_t mask = resultWidth >= 64 ? ~0ULL : ((1ULL << resultWidth) - 1);
    return storeAndReturn("((" + legalCType(resultWidth) + ")((" +
           renderArcExprCached(def->getOperand(0), argMap, cache) +
           " >> " + std::to_string(lowBit) + ") & " + std::to_string(mask) + "u))");
  }

  if (opName == "hw.array_get") {
    auto ag = mlir::cast<circt::hw::ArrayGetOp>(def);
    std::string arrE = renderArcExprCached(ag.getInput(), argMap, cache);
    std::string idxE = renderArcExprCached(ag.getIndex(), argMap, cache);
    auto arrTy =
        mlir::dyn_cast<circt::hw::ArrayType>(ag.getInput().getType());
    unsigned sz = arrTy ? arrTy.getNumElements() : 0;
    if (sz > 0)
      return storeAndReturn("(" + arrE + "[(" + legalCType(32) + ")(" + idxE + ") % " +
             std::to_string(sz) + "])");
    return storeAndReturn("0");
  }

  if (opName == "arc.call") {
    unsigned resultIdx = 0;
    if (auto opResult = mlir::dyn_cast<mlir::OpResult>(val))
      resultIdx = opResult.getResultNumber();
    return storeAndReturn(inlineArcCall(def, resultIdx, &argMap, 0));
  }

  return storeAndReturn("/* unsupported:" + opName.str() + " */0");
}

void CModelEmitter::emitEvalComb(llvm::raw_string_ostream &os) {
  resetExprCacheForFunction();

  const auto &mod = model_.moduleName;
  os << "void " << mod << "_eval_comb(" << mod << "_state *s) {\n";

  if (!hwModule_) {
    os << "  (void)s;\n";
    os << "}\n\n";
    return;
  }

  auto *block = hwModule_.getBodyBlock();
  auto outputOp =
      mlir::dyn_cast<circt::hw::OutputOp>(block->getTerminator());

  if (!outputOp) {
    emitChildEvalCombWiring(os);
    if (childInstances_.empty())
      os << "  (void)s;\n";
    os << "}\n\n";
    return;
  }

  // Phase 1: child instance wiring (set → eval_comb → get → staged binding)
  emitChildEvalCombWiring(os);

  // Phase 2: parent's own combinational logic
  bool emittedAnything = !childInstances_.empty();
  for (auto [idx, operand] : llvm::enumerate(outputOp.getOperands())) {
    if (idx >= model_.outputs.size())
      break;

    const auto &binding = model_.outputs[idx];
    if (binding.visibility != semantic::OutputVisibility::Comb)
      continue;

    if (idx >= model_.outputPorts.size())
      continue;

    const auto &oport = model_.outputPorts[idx];
    if (oport.isWide()) {
      os << "  /* TODO: wide comb output `" << oport.name << "` */\n";
      emittedAnything = true;
      continue;
    }
    if (oport.isAggregate) {
      os << "  /* TODO: aggregate comb output `" << oport.name << "` */\n";
      emittedAnything = true;
      continue;
    }
    std::string expr = renderExpr(operand);
    os << "  s->output_" << oport.name << " = ("
       << legalCType(oport.width) << ")(" << expr << ");\n";
    emittedAnything = true;
  }

  if (!emittedAnything)
    os << "  (void)s;\n";

  os << "}\n\n";
}

void CModelEmitter::emitAggregateInitLiterals(
    llvm::raw_string_ostream &os, const semantic::AggregateStateVar &aggVar,
    llvm::StringRef indent) {
  unsigned totalBits = aggVar.numElements * aggVar.elementWidth;
  llvm::APInt packed(std::max(totalBits, 1u), aggVar.initValue, 10);
  if (packed.getBitWidth() < totalBits)
    packed = packed.zext(totalBits);
  else if (packed.getBitWidth() > totalBits)
    packed = packed.trunc(totalBits);

  uint64_t elemMask =
      aggVar.elementWidth >= 64 ? ~0ULL : ((1ULL << aggVar.elementWidth) - 1);
  std::string etype = legalCType(aggVar.elementWidth);
  for (unsigned k = 0; k < aggVar.numElements; ++k) {
    uint64_t elemVal =
        packed.extractBits(aggVar.elementWidth, k * aggVar.elementWidth)
            .getZExtValue();
    os << indent << "s->" << aggVar.stableName << "[" << k << "] = (" << etype
       << ")" << elemVal << ";\n";
  }
}

void CModelEmitter::emitImpl(llvm::raw_string_ostream &os) {
  const auto &mod = model_.moduleName;

  os << "#include \"" << mod << options_.headerSuffix << "\"\n\n";

  os << "#include <string.h>\n\n";

  os << "#ifdef __cplusplus\n";
  os << "extern \"C\" {\n";
  os << "#endif\n\n";

  // initialize
  os << "void " << mod << "_initialize(" << mod << "_state *s) {\n";
  for (const auto &port : model_.inputPorts) {
    if (port.isWide())
      os << "  memset(s->input_" << port.name << ", 0, sizeof(s->input_"
         << port.name << "));\n";
    else
      os << "  s->input_" << port.name << " = 0;\n";
  }
  for (const auto &port : model_.outputPorts) {
    if (port.isWide())
      os << "  memset(s->output_" << port.name << ", 0, sizeof(s->output_"
         << port.name << "));\n";
    else if (port.isAggregate)
      os << "  memset(s->output_" << port.name << ", 0, sizeof(s->output_"
         << port.name << "));\n";
    else
      os << "  s->output_" << port.name << " = 0;\n";
  }
  for (const auto &sv : model_.stateVars) {
    if (sv.hasConstantInit)
      os << "  s->" << sv.stableName << " = " << sv.initValue << ";\n";
    else
      os << "  s->" << sv.stableName << " = 0;\n";
  }
  for (const auto &aggVar : model_.aggregateStateVars) {
    if (aggVar.numElements == 0)
      continue;
    if (aggVar.hasConstantInit && !aggVar.initValue.empty()) {
      emitAggregateInitLiterals(os, aggVar, "  ");
    } else {
      os << "  for (size_t i = 0; i < " << aggVar.numElements << "; ++i) s->"
         << aggVar.stableName << "[i] = 0;\n";
    }
  }
  for (const auto &mem : model_.memoryVars)
    os << "  for (size_t i = 0; i < " << mem.depth << "; ++i) s->"
       << mem.stableName << "[i] = 0;\n";
  emitChildInitCalls(os);
  os << "}\n\n";

  // setters
  emitSetters(os);

  // getters
  emitGetters(os);

  // eval_comb
  emitEvalComb(os);

  // eval_<clock>
  for (const auto &clock : model_.clockDomains)
    emitEvalClock(os, clock);

  os << "#ifdef __cplusplus\n";
  os << "}\n";
  os << "#endif\n";
}

void CModelEmitter::emitEvalClock(llvm::raw_string_ostream &os,
                                  llvm::StringRef clockDomain) {
  resetExprCacheForFunction();

  const auto &mod = model_.moduleName;
  os << "void " << mod << "_eval_" << clockDomain << "(" << mod
     << "_state *s) {\n";

  if (!hwModule_) {
    os << "  (void)s;\n}\n\n";
    return;
  }

  // Child eval_clock calls before parent's own sequential logic
  emitChildEvalClockCalls(os, clockDomain);

  // Build instance result → getter expression map for cross-module references.
  // In eval_clock, exprCache_ is cleared, so we build a local map from
  // hw.instance results to "{Child}_get_{port}(&s->{inst})" expressions.
  llvm::DenseMap<mlir::Value, std::string> instanceResultMap;
  if (!childInstances_.empty()) {
    llvm::StringMap<circt::hw::InstanceOp> instMap;
    auto *bodyBlock = hwModule_.getBodyBlock();
    for (auto &op : bodyBlock->getOperations()) {
      auto inst = mlir::dyn_cast<circt::hw::InstanceOp>(op);
      if (!inst)
        continue;
      instMap[inst.getInstanceName()] = inst;
    }
    for (const auto &info : childInstances_) {
      auto it = instMap.find(info.instanceName);
      if (it == instMap.end())
        continue;
      circt::hw::InstanceOp inst = it->second;
      for (unsigned r = 0; r < inst.getNumResults() && r < info.outputPorts.size(); ++r) {
        const auto &portPair = info.outputPorts[r];
        if (portPair.second > 64)
          continue;
        instanceResultMap[inst.getResult(r)] =
            info.childModuleName + "_get_" + portPair.first +
            "(&s->" + info.instanceName + ")";
      }
    }
  }

  auto parentModule = hwModule_->getParentOfType<mlir::ModuleOp>();

  // Collect scalar state vars in this clock domain
  llvm::SmallVector<const semantic::StateVar *> domainStates;
  for (const auto &sv : model_.stateVars) {
    if (sv.clockDomain == clockDomain)
      domainStates.push_back(&sv);
  }

  // Collect aggregate state vars in this clock domain
  llvm::SmallVector<const semantic::AggregateStateVar *> domainAggs;
  for (const auto &av : model_.aggregateStateVars) {
    if (av.clockDomain == clockDomain)
      domainAggs.push_back(&av);
  }

  bool hasMemoryInDomain = false;
  for (const auto &mv : model_.memoryVars) {
    if (mv.clockDomain == clockDomain)
      hasMemoryInDomain = true;
  }

  bool hasEdgeOutputs = false;
  for (const auto &binding : model_.outputs) {
    if (binding.visibility == semantic::OutputVisibility::Edge ||
        binding.visibility == semantic::OutputVisibility::PostEdgeComb)
      hasEdgeOutputs = true;
  }

  if (domainStates.empty() && domainAggs.empty() && !hasMemoryInDomain &&
      !hasEdgeOutputs) {
    os << "  (void)s;\n}\n\n";
    return;
  }

  // Phase 1: compute next-state temporaries for scalar states
  for (const auto *sv : domainStates) {
    circt::arc::DefineOp arcDef = nullptr;
    if (parentModule) {
      if (auto *op = parentModule.lookupSymbol(sv->arcName))
        arcDef = mlir::dyn_cast<circt::arc::DefineOp>(op);
    }

    if (!arcDef || arcDef.getBody().empty()) {
      os << "  /* no arc body for " << sv->stableName << " */\n";
      continue;
    }

    mlir::Block &body = arcDef.getBody().front();
    auto outputOp = mlir::dyn_cast<circt::arc::OutputOp>(body.getTerminator());
    if (!outputOp || outputOp.getOutputs().empty()) {
      os << "  /* empty arc output for " << sv->stableName << " */\n";
      continue;
    }

    llvm::DenseMap<mlir::Value, std::string> argMap;

    // Find the exact arc.state instance using stateOpIndex
    circt::arc::StateOp stateOp = nullptr;
    unsigned walkIdx = 0;
    hwModule_.walk([&](circt::arc::StateOp s) {
      if (walkIdx == sv->stateOpIndex)
        stateOp = s;
      ++walkIdx;
    });

    if (!stateOp) {
      os << "  /* cannot find arc.state for " << sv->stableName << " */\n";
      continue;
    }

    // Build stateOp -> stateVar lookup by walk index
    llvm::DenseMap<mlir::Operation *, const semantic::StateVar *> opToStateVar;
    walkIdx = 0;
    hwModule_.walk([&](circt::arc::StateOp s) {
      for (const auto &msv : model_.stateVars) {
        if (msv.stateOpIndex == walkIdx) {
          opToStateVar[s.getOperation()] = &msv;
          break;
        }
      }
      ++walkIdx;
    });

    unsigned bodyArgIdx = 0;
    for (unsigned i = 0; i < stateOp.getInputs().size() && bodyArgIdx < body.getNumArguments(); ++i) {
      mlir::Value hwOperand = stateOp.getInputs()[i];
      mlir::Value bodyArg = body.getArgument(bodyArgIdx);

      if (auto blockArg = mlir::dyn_cast<mlir::BlockArgument>(hwOperand)) {
        if (blockArg.getOwner() == hwModule_.getBodyBlock() &&
            blockArg.getArgNumber() < model_.inputPorts.size()) {
          argMap[bodyArg] = "s->input_" + model_.inputPorts[blockArg.getArgNumber()].name;
        } else {
          argMap[bodyArg] = "/* unknown_hw_arg */0";
        }
      } else if (hwOperand.getDefiningOp() &&
                 mlir::isa<circt::arc::StateOp>(hwOperand.getDefiningOp())) {
        auto it = opToStateVar.find(hwOperand.getDefiningOp());
        if (it != opToStateVar.end()) {
          argMap[bodyArg] = "s->" + it->second->stableName;
        } else {
          argMap[bodyArg] = "/* unknown_state_ref */0";
        }
      } else {
        auto instIt = instanceResultMap.find(hwOperand);
        if (instIt != instanceResultMap.end()) {
          argMap[bodyArg] = instIt->second;
        } else {
          argMap[bodyArg] = "/* unresolved_operand */0";
        }
      }
      ++bodyArgIdx;
    }

    mlir::Value outputVal = outputOp.getOutputs().front();
    std::string nextExpr = renderArcExpr(outputVal, argMap);

    os << "  " << legalCType(sv->width) << " next_" << sv->stableName
       << " = (" << legalCType(sv->width) << ")(" << nextExpr << ");\n";
  }

  // Phase 2: atomic commit
  for (const auto *sv : domainStates)
    os << "  s->" << sv->stableName << " = next_" << sv->stableName << ";\n";

  // Phase 2b: aggregate state next-state computation and commit
  for (const auto *av : domainAggs) {
    if (av->numElements == 0 || av->elementWidth == 0)
      continue;

    std::string etype = legalCType(av->elementWidth);
    unsigned depth = av->numElements;

    circt::arc::StateOp aggStateOp = nullptr;
    hwModule_.walk([&](circt::arc::StateOp s) {
      if (aggStateOp)
        return;
      std::string sname;
      if (auto names = s->getAttrOfType<mlir::ArrayAttr>("names")) {
        if (!names.empty())
          if (auto sa = mlir::dyn_cast<mlir::StringAttr>(names[0]))
            sname = semantic::normalizeIdentifier(sa.getValue());
      }
      if (sname == av->stableName)
        aggStateOp = s;
    });

    circt::arc::DefineOp arcDef = nullptr;
    if (parentModule) {
      if (auto *op = parentModule.lookupSymbol(av->arcName))
        arcDef = mlir::dyn_cast<circt::arc::DefineOp>(op);
    }

    if (!arcDef || arcDef.getBody().empty() || !aggStateOp) {
      os << "  /* aggregate '" << av->stableName
         << "': no arc body or state op found */\n";
      continue;
    }

    mlir::Block &body = arcDef.getBody().front();
    auto outputOp =
        mlir::dyn_cast<circt::arc::OutputOp>(body.getTerminator());
    if (!outputOp || outputOp.getOutputs().empty()) {
      os << "  /* aggregate '" << av->stableName
         << "': empty arc output */\n";
      continue;
    }

    llvm::DenseMap<mlir::Value, std::string> argMap;

    llvm::DenseMap<mlir::Operation *, const semantic::StateVar *>
        opToStateVar;
    {
      unsigned walkIdx = 0;
      hwModule_.walk([&](circt::arc::StateOp s) {
        for (const auto &msv : model_.stateVars) {
          if (msv.stateOpIndex == walkIdx) {
            opToStateVar[s.getOperation()] = &msv;
            break;
          }
        }
        ++walkIdx;
      });
    }

    llvm::DenseMap<mlir::Operation *, const semantic::AggregateStateVar *>
        opToAggVar;
    hwModule_.walk([&](circt::arc::StateOp s) {
      std::string sn;
      if (auto names = s->getAttrOfType<mlir::ArrayAttr>("names")) {
        if (!names.empty())
          if (auto sa = mlir::dyn_cast<mlir::StringAttr>(names[0]))
            sn = semantic::normalizeIdentifier(sa.getValue());
      }
      for (const auto &mav : model_.aggregateStateVars) {
        if (mav.stableName == sn) {
          opToAggVar[s.getOperation()] = &mav;
          break;
        }
      }
    });

    unsigned bodyArgIdx = 0;
    for (unsigned i = 0;
         i < aggStateOp.getInputs().size() &&
         bodyArgIdx < body.getNumArguments();
         ++i) {
      mlir::Value hwOperand = aggStateOp.getInputs()[i];
      mlir::Value bodyArg = body.getArgument(bodyArgIdx);

      if (auto blockArg = mlir::dyn_cast<mlir::BlockArgument>(hwOperand)) {
        if (blockArg.getOwner() == hwModule_.getBodyBlock() &&
            blockArg.getArgNumber() < model_.inputPorts.size()) {
          argMap[bodyArg] =
              "s->input_" +
              model_.inputPorts[blockArg.getArgNumber()].name;
        } else {
          argMap[bodyArg] = "/* unknown_hw_arg */0";
        }
      } else if (hwOperand.getDefiningOp()) {
        auto aggIt = opToAggVar.find(hwOperand.getDefiningOp());
        if (aggIt != opToAggVar.end()) {
          argMap[bodyArg] = "s->" + aggIt->second->stableName;
        } else {
          auto svIt = opToStateVar.find(hwOperand.getDefiningOp());
          if (svIt != opToStateVar.end()) {
            argMap[bodyArg] = "s->" + svIt->second->stableName;
          } else {
            auto instIt = instanceResultMap.find(hwOperand);
            if (instIt != instanceResultMap.end()) {
              argMap[bodyArg] = instIt->second;
            } else {
              argMap[bodyArg] = "/* unresolved_operand */0";
            }
          }
        }
      } else {
        argMap[bodyArg] = "/* unresolved_operand */0";
      }
      ++bodyArgIdx;
    }

    mlir::Value outputVal = outputOp.getOutputs().front();
    mlir::Operation *outputDef = outputVal.getDefiningOp();

    bool isArrayCreate =
        outputDef &&
        outputDef->getName().getStringRef() == "hw.array_create";

    if (isArrayCreate) {
      auto operands = outputDef->getOperands();
      os << "  " << etype << " next_" << av->stableName << "[" << depth
         << "];\n";
      for (unsigned k = 0; k < depth && k < operands.size(); ++k) {
        unsigned srcIdx = operands.size() - 1 - k;
        std::string elemExpr = renderArcExpr(operands[srcIdx], argMap);
        os << "  next_" << av->stableName << "[" << k << "] = (" << etype
           << ")(" << elemExpr << ");\n";
      }
    } else {
      os << "  /* aggregate '" << av->stableName
         << "': non-array_create output pattern, skipped */\n";
      continue;
    }

    std::string rstSig, enSig;
    if (av->hasReset && aggStateOp.getReset()) {
      if (auto arg = mlir::dyn_cast<mlir::BlockArgument>(
              aggStateOp.getReset())) {
        if (arg.getOwner() == hwModule_.getBodyBlock() &&
            arg.getArgNumber() < model_.inputPorts.size())
          rstSig = "s->input_" +
                   model_.inputPorts[arg.getArgNumber()].name;
      } else {
        std::string expr = renderExpr(aggStateOp.getReset());
        if (expr.find("/* unsupported") == std::string::npos &&
            expr.find("/* unresolved") == std::string::npos)
          rstSig = expr;
      }
    }
    if (av->hasEnable && aggStateOp.getEnable()) {
      if (auto arg = mlir::dyn_cast<mlir::BlockArgument>(
              aggStateOp.getEnable())) {
        if (arg.getOwner() == hwModule_.getBodyBlock() &&
            arg.getArgNumber() < model_.inputPorts.size())
          enSig = "s->input_" +
                  model_.inputPorts[arg.getArgNumber()].name;
      } else {
        std::string expr = renderExpr(aggStateOp.getEnable());
        if (expr.find("/* unsupported") == std::string::npos &&
            expr.find("/* unresolved") == std::string::npos)
          enSig = expr;
      }
    }

    if (!rstSig.empty()) {
      os << "  if (" << rstSig << ") {\n";
      if (av->hasConstantInit && !av->initValue.empty()) {
        emitAggregateInitLiterals(os, *av, "    ");
      } else {
        os << "    for (unsigned __k = 0; __k < " << depth
           << "; ++__k) s->" << av->stableName << "[__k] = (" << etype
           << ")0;\n";
      }
      if (!enSig.empty()) {
        os << "  } else if (" << enSig << ") {\n";
      } else {
        os << "  } else {\n";
      }
      os << "    for (unsigned __k = 0; __k < " << depth
         << "; ++__k) s->" << av->stableName << "[__k] = next_"
         << av->stableName << "[__k];\n";
      os << "  }\n";
    } else if (!enSig.empty()) {
      os << "  if (" << enSig << ") {\n";
      os << "    for (unsigned __k = 0; __k < " << depth
         << "; ++__k) s->" << av->stableName << "[__k] = next_"
         << av->stableName << "[__k];\n";
      os << "  }\n";
    } else {
      os << "  for (unsigned __k = 0; __k < " << depth << "; ++__k) s->"
         << av->stableName << "[__k] = next_" << av->stableName
         << "[__k];\n";
    }
  }

  // Phase 3a: snapshot memory-derived edge outputs BEFORE memory writes
  // (read_latency=1 semantics: reads see old memory, not same-cycle writes)
  llvm::SmallVector<std::pair<unsigned, std::string>> memEdgeSnapshots;
  if (auto outputOp = mlir::dyn_cast<circt::hw::OutputOp>(
          hwModule_.getBodyBlock()->getTerminator())) {
    for (auto [idx, operand] : llvm::enumerate(outputOp.getOperands())) {
      if (idx >= model_.outputs.size() || idx >= model_.outputPorts.size())
        break;
      const auto &binding = model_.outputs[idx];
      if (binding.visibility != semantic::OutputVisibility::Edge)
        continue;
      if (binding.sourceEntity.empty())
        continue;
      bool isMemory = false;
      for (const auto &mv : model_.memoryVars) {
        if (mv.stableName == binding.sourceEntity) {
          isMemory = true;
          break;
        }
      }
      if (!isMemory)
        continue;
      std::string readExpr = renderExpr(operand);
      if (readExpr.find("/* unsupported") != std::string::npos ||
          readExpr.find("/* firmem_read") != std::string::npos)
        continue;
      const auto &oport = model_.outputPorts[idx];
      std::string snapName = "snap_mem_rd_" + oport.name;
      os << "  " << legalCType(oport.width) << " " << snapName << " = ("
         << legalCType(oport.width) << ")(" << readExpr << ");\n";
      memEdgeSnapshots.push_back({static_cast<unsigned>(idx), snapName});
    }
  }

  // Phase 3b: memory writes (seq.firmem.write_port)
  if (hwModule_) {
    hwModule_.walk([&](circt::seq::FirMemWriteOp wp) {
      auto firMem = wp.getMemory().getDefiningOp<circt::seq::FirMemOp>();
      if (!firMem)
        return;

      std::string memName;
      unsigned memIdx = 0;
      hwModule_.walk([&](circt::seq::FirMemOp fm) {
        if (fm.getOperation() == firMem.getOperation()) {
          if (memIdx < model_.memoryVars.size())
            memName = model_.memoryVars[memIdx].stableName;
        }
        ++memIdx;
      });
      if (memName.empty())
        return;

      unsigned depth = firMem.getType().getDepth();
      unsigned elemW = firMem.getType().getWidth();
      std::string addrExpr = renderExpr(wp.getAddress());
      std::string dataExpr = renderExpr(wp.getData());

      if (wp.getEnable()) {
        std::string enExpr = renderExpr(wp.getEnable());
        os << "  if (" << enExpr << ") ";
      } else {
        os << "  ";
      }
      if (depth > 0) {
        os << "s->" << memName << "[(" << legalCType(32) << ")(" << addrExpr
           << ") % " << depth << "] = (" << legalCType(elemW) << ")("
           << dataExpr << ");\n";
      }
    });
  }

  // Phase 4: update edge and post_edge_comb output shadows
  llvm::DenseSet<unsigned> snappedOutputs;
  for (const auto &snap : memEdgeSnapshots)
    snappedOutputs.insert(snap.first);

  if (auto outputOp = mlir::dyn_cast<circt::hw::OutputOp>(
          hwModule_.getBodyBlock()->getTerminator())) {
    for (auto [idx, operand] : llvm::enumerate(outputOp.getOperands())) {
      if (idx >= model_.outputs.size() || idx >= model_.outputPorts.size())
        break;

      const auto &binding = model_.outputs[idx];
      if (binding.visibility == semantic::OutputVisibility::Edge) {
        if (snappedOutputs.count(idx)) {
          // Use pre-write snapshot for memory-derived edge outputs
          for (const auto &snap : memEdgeSnapshots) {
            if (snap.first == static_cast<unsigned>(idx)) {
              os << "  s->output_" << model_.outputPorts[idx].name
                 << " = " << snap.second << ";\n";
              break;
            }
          }
        } else if (!binding.sourceEntity.empty()) {
          bool isMemory = false;
          for (const auto &mv : model_.memoryVars) {
            if (mv.stableName == binding.sourceEntity) {
              isMemory = true;
              break;
            }
          }
          if (isMemory) {
            os << "  /* TODO: memory-derived edge output '"
               << model_.outputPorts[idx].name
               << "' from '" << binding.sourceEntity
               << "' -- requires address/index resolution */\n";
          } else if (model_.outputPorts[idx].isAggregate) {
            os << "  memcpy(s->output_" << model_.outputPorts[idx].name
               << ", s->" << binding.sourceEntity << ", sizeof(s->output_"
               << model_.outputPorts[idx].name << "));\n";
          } else {
            os << "  s->output_" << model_.outputPorts[idx].name
               << " = s->" << binding.sourceEntity << ";\n";
          }
        }
      } else if (binding.visibility == semantic::OutputVisibility::PostEdgeComb) {
        if (model_.outputPorts[idx].isAggregate) {
          os << "  /* TODO: aggregate post-edge-comb output `"
             << model_.outputPorts[idx].name << "` */\n";
        } else {
          std::string expr = renderExpr(operand);
          os << "  s->output_" << model_.outputPorts[idx].name << " = ("
             << legalCType(model_.outputPorts[idx].width) << ")(" << expr << ");\n";
        }
      }
    }
  }

  os << "}\n\n";
}

std::string CModelEmitter::inlineArcCall(
    mlir::Operation *callOp, unsigned resultIdx,
    const llvm::DenseMap<mlir::Value, std::string> *outerArgMap,
    unsigned depth) {
  if (depth >= kMaxInlineDepth)
    return "/* arc.call_max_depth */0";

  auto callSymRef = callOp->getAttrOfType<mlir::FlatSymbolRefAttr>("arc");
  if (!callSymRef)
    return "/* arc.call_no_sym */0";

  auto parentModule = callOp->getParentOfType<mlir::ModuleOp>();
  if (!parentModule)
    return "/* arc.call_no_parent */0";

  auto *calleeSym = parentModule.lookupSymbol(callSymRef.getValue());
  auto calleeDef = mlir::dyn_cast_or_null<circt::arc::DefineOp>(calleeSym);
  if (!calleeDef || calleeDef.getBody().empty())
    return "/* arc.call_bad_callee */0";

  mlir::Block &body = calleeDef.getBody().front();
  auto outputOp =
      mlir::dyn_cast<circt::arc::OutputOp>(body.getTerminator());
  if (!outputOp || resultIdx >= outputOp.getOutputs().size())
    return "/* arc.call_bad_output */0";

  llvm::DenseMap<mlir::Value, std::string> innerArgMap;
  for (unsigned i = 0; i < callOp->getNumOperands() &&
                       i < body.getNumArguments(); ++i) {
    mlir::Value callOperand = callOp->getOperand(i);
    mlir::Value bodyArg = body.getArgument(i);

    if (outerArgMap) {
      innerArgMap[bodyArg] = renderArcExpr(callOperand, *outerArgMap);
    } else {
      innerArgMap[bodyArg] = renderExpr(callOperand);
    }
  }

  // Emit arc body as a series of C temporaries to avoid exponential
  // expression blowup from shared SSA subgraphs.
  llvm::DenseMap<mlir::Value, std::string> nameMap(innerArgMap);
  std::string preamble;
  unsigned tmpIdx = 0;

  // Topological walk: MLIR blocks list ops in dominance order, so a
  // forward walk over the block's operations is a valid topo order.
  for (auto &op : body.getOperations()) {
    if (mlir::isa<circt::arc::OutputOp>(&op))
      continue;

    for (mlir::Value res : op.getResults()) {
      std::string expr;
      llvm::StringRef opName = op.getName().getStringRef();

      auto renderOp = [&](mlir::Value v) -> std::string {
        auto it = nameMap.find(v);
        if (it != nameMap.end())
          return it->second;
        return "/* unresolved */0";
      };

      auto renderVariadicTmp = [&](llvm::StringRef cOp) -> std::string {
        std::string r = renderOp(op.getOperand(0));
        for (unsigned i = 1; i < op.getNumOperands(); ++i)
          r = "(" + r + " " + cOp.str() + " " + renderOp(op.getOperand(i)) + ")";
        return r;
      };

      if (opName == "hw.constant") {
        if (auto attr = op.getAttrOfType<mlir::IntegerAttr>("value")) {
          unsigned bw = attr.getValue().getBitWidth();
          if (bw > 64) {
            expr = attr.getValue().isZero() ? "0" : "/* wide_const */0";
          } else {
            llvm::SmallString<32> buf;
            if (attr.getValue().isNegative())
              attr.getValue().toStringSigned(buf);
            else {
              attr.getValue().toStringUnsigned(buf);
              buf += "u";
            }
            expr = std::string("(") + legalCType(bw) + ")" + std::string(buf);
          }
        } else {
          expr = "/* bad_const */0";
        }
      } else if ((opName == "comb.xor" || opName == "comb.and" ||
                   opName == "comb.or" || opName == "comb.add" ||
                   opName == "comb.mul") && op.getNumOperands() >= 2) {
        llvm::StringRef cOp = opName == "comb.xor" ? "^" :
                              opName == "comb.and" ? "&" :
                              opName == "comb.or"  ? "|" :
                              opName == "comb.add" ? "+" : "*";
        expr = renderVariadicTmp(cOp);
      } else if (opName == "comb.sub" && op.getNumOperands() == 2) {
        expr = "(" + renderOp(op.getOperand(0)) + " - " +
               renderOp(op.getOperand(1)) + ")";
      } else if (opName == "comb.mux" && op.getNumOperands() == 3) {
        expr = "(" + renderOp(op.getOperand(0)) + " ? " +
               renderOp(op.getOperand(1)) + " : " +
               renderOp(op.getOperand(2)) + ")";
      } else if (opName == "comb.icmp") {
        auto predAttr = op.getAttrOfType<mlir::IntegerAttr>("predicate");
        if (predAttr && op.getNumOperands() == 2) {
          unsigned opWidth = 0;
          if (auto ty = mlir::dyn_cast<mlir::IntegerType>(
                  op.getOperand(0).getType()))
            opWidth = ty.getWidth();
          expr = renderIcmpExpr(predAttr.getInt(),
                                renderOp(op.getOperand(0)),
                                renderOp(op.getOperand(1)), opWidth);
        } else {
          expr = "/* bad_icmp */0";
        }
      } else if (opName == "comb.concat") {
        unsigned totalWidth = 0;
        if (auto ty = mlir::dyn_cast<mlir::IntegerType>(res.getType()))
          totalWidth = ty.getWidth();
        if (totalWidth > 64) {
          expr = "/* unsupported:comb.concat_wide */0";
        } else {
          expr = "0";
          unsigned shift = 0;
          for (int i = op.getNumOperands() - 1; i >= 0; --i) {
            unsigned opWidth = 0;
            if (auto ty = mlir::dyn_cast<mlir::IntegerType>(
                    op.getOperand(i).getType()))
              opWidth = ty.getWidth();
            std::string part = "((" + legalCType(totalWidth) + ")" +
                               renderOp(op.getOperand(i)) + " << " +
                               std::to_string(shift) + ")";
            expr = "(" + expr + " | " + part + ")";
            shift += opWidth;
          }
        }
      } else if (opName == "comb.extract") {
        auto lowBitAttr = op.getAttrOfType<mlir::IntegerAttr>("lowBit");
        unsigned lowBit = lowBitAttr ? lowBitAttr.getInt() : 0;
        unsigned resultWidth = 0;
        if (auto ty = mlir::dyn_cast<mlir::IntegerType>(res.getType()))
          resultWidth = ty.getWidth();
        uint64_t mask =
            resultWidth >= 64 ? ~0ULL : ((1ULL << resultWidth) - 1);
        expr = "((" + legalCType(resultWidth) + ")((" +
               renderOp(op.getOperand(0)) + " >> " +
               std::to_string(lowBit) + ") & " + std::to_string(mask) + "u))";
      } else if (opName == "arc.call") {
        unsigned ri = 0;
        if (auto opResult = mlir::dyn_cast<mlir::OpResult>(res))
          ri = opResult.getResultNumber();
        expr = inlineArcCall(&op, ri, nullptr, depth + 1);
      } else {
        expr = "/* unsupported:" + opName.str() + " */0";
      }

      // Determine C type width for the temporary
      unsigned width = 0;
      if (auto ty = mlir::dyn_cast<mlir::IntegerType>(res.getType()))
        width = ty.getWidth();

      bool multiUse = !res.hasOneUse();
      if (multiUse && width <= 64) {
        std::string varName = "_t" + std::to_string(tmpIdx++);
        preamble += "    " + legalCType(width) + " " + varName +
                    " = (" + legalCType(width) + ")(" + expr + ");\n";
        nameMap[res] = varName;
      } else {
        nameMap[res] = expr;
      }
    }
  }

  // Build final expression from the arc output
  mlir::Value outputVal = outputOp.getOutputs()[resultIdx];
  auto outIt = nameMap.find(outputVal);
  std::string finalExpr = (outIt != nameMap.end()) ? outIt->second
                                                    : "/* unresolved_output */0";

  if (preamble.empty())
    return finalExpr;

  // Use GCC/Clang statement expression to keep this as a single C expression
  return "({ \\\n" + preamble + "    " + finalExpr + "; \\\n  })";
}

bool writeArtifact(const CModelArtifact &artifact) {
  std::error_code ec;
  llvm::raw_fd_ostream headerOS(artifact.headerPath, ec);
  if (ec)
    return false;
  headerOS << artifact.headerContent;
  headerOS.close();
  if (headerOS.has_error())
    return false;

  llvm::raw_fd_ostream implOS(artifact.implPath, ec);
  if (ec) {
    llvm::sys::fs::remove(artifact.headerPath);
    return false;
  }
  implOS << artifact.implContent;
  implOS.close();
  if (implOS.has_error()) {
    llvm::sys::fs::remove(artifact.headerPath);
    llvm::sys::fs::remove(artifact.implPath);
    return false;
  }

  return true;
}

} // namespace hirct
