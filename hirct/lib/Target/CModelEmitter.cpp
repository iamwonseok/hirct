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
#include "circt/Dialect/Comb/CombDialect.h"
#include "circt/Dialect/HW/HWOps.h"
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
  llvm_unreachable("width >64 must be rejected by validation before emission");
}

CModelEmitter::CModelEmitter(const semantic::ModuleModel &model,
                              const CModelOptions &options,
                              circt::hw::HWModuleOp hwModule)
    : model_(model), options_(options), hwModule_(hwModule) {}

CModelArtifact CModelEmitter::emit() {
  CModelArtifact artifact;
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

void CModelEmitter::emitHeader(llvm::raw_string_ostream &os) {
  const auto &mod = model_.moduleName;

  os << "#ifndef " << mod << "_MODEL_H\n";
  os << "#define " << mod << "_MODEL_H\n\n";
  os << "#include <cstdint>\n";
  os << "#include <cstddef>\n\n";

  os << "struct " << mod << "_state {\n";

  for (const auto &port : model_.inputPorts)
    os << "  " << legalCType(port.width) << " input_" << port.name << ";\n";

  for (const auto &port : model_.outputPorts)
    os << "  " << legalCType(port.width) << " output_" << port.name << ";\n";

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

  os << "};\n\n";

  os << "void " << mod << "_initialize(" << mod << "_state *s);\n";

  for (const auto &port : model_.inputPorts)
    os << "void " << mod << "_set_" << port.name << "(" << mod << "_state *s, "
       << legalCType(port.width) << " v);\n";

  for (const auto &port : model_.outputPorts)
    os << legalCType(port.width) << " " << mod << "_get_" << port.name
       << "(const " << mod << "_state *s);\n";

  os << "void " << mod << "_eval_comb(" << mod << "_state *s);\n";

  for (const auto &clock : model_.clockDomains)
    os << "void " << mod << "_eval_" << clock << "(" << mod
       << "_state *s);\n";

  os << "\n#endif // " << mod << "_MODEL_H\n";
}

void CModelEmitter::emitSetters(llvm::raw_string_ostream &os) {
  const auto &mod = model_.moduleName;
  for (const auto &port : model_.inputPorts)
    os << "void " << mod << "_set_" << port.name << "(" << mod << "_state *s, "
       << legalCType(port.width) << " v) {\n"
       << "  s->input_" << port.name << " = v;\n}\n\n";
}

void CModelEmitter::emitGetters(llvm::raw_string_ostream &os) {
  const auto &mod = model_.moduleName;
  for (const auto &port : model_.outputPorts)
    os << legalCType(port.width) << " " << mod << "_get_" << port.name
       << "(const " << mod << "_state *s) {\n"
       << "  return s->output_" << port.name << ";\n}\n\n";
}

std::string CModelEmitter::renderExpr(mlir::Value val) {
  if (auto blockArg = mlir::dyn_cast<mlir::BlockArgument>(val)) {
    unsigned argNum = blockArg.getArgNumber();
    if (argNum < model_.inputPorts.size())
      return "s->input_" + model_.inputPorts[argNum].name;
    return "/* unknown_arg_" + std::to_string(argNum) + " */0";
  }

  mlir::Operation *def = val.getDefiningOp();
  if (!def)
    return "/* null_def */0";

  llvm::StringRef opName = def->getName().getStringRef();

  if (opName == "hw.constant") {
    if (auto attr = def->getAttrOfType<mlir::IntegerAttr>("value")) {
      llvm::SmallString<32> buf;
      if (attr.getValue().isNegative()) {
        attr.getValue().toStringSigned(buf);
      } else {
        attr.getValue().toStringUnsigned(buf);
        buf += "u";
      }
      return std::string("(") + legalCType(attr.getValue().getBitWidth()) +
             ")" + std::string(buf);
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
      std::string cOp;
      switch (predAttr.getInt()) {
      case 0: cOp = "=="; break;
      case 1: cOp = "!="; break;
      case 2: cOp = "<"; break;
      case 3: cOp = "<="; break;
      case 4: cOp = ">"; break;
      case 5: cOp = ">="; break;
      case 6: cOp = "<"; break;
      case 7: cOp = "<="; break;
      case 8: cOp = ">"; break;
      case 9: cOp = ">="; break;
      case 10: cOp = "=="; break;
      case 11: cOp = "!="; break;
      case 12: cOp = "=="; break;
      case 13: cOp = "!="; break;
      default: return "/* unsupported_icmp_pred */0";
      }
      return "(" + renderExpr(def->getOperand(0)) + " " + cOp + " " +
             renderExpr(def->getOperand(1)) + ")";
    }
    return "/* bad_icmp */0";
  }

  if (opName == "comb.concat") {
    unsigned totalWidth = 0;
    if (auto ty = mlir::dyn_cast<mlir::IntegerType>(val.getType()))
      totalWidth = ty.getWidth();
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
    uint64_t mask = resultWidth >= 64 ? ~0ULL : ((1ULL << resultWidth) - 1);
    return "((" + legalCType(resultWidth) + ")((" + renderExpr(def->getOperand(0)) +
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

  return "/* unsupported:" + opName.str() + " */0";
}

std::string CModelEmitter::renderArcExpr(
    mlir::Value val,
    const llvm::DenseMap<mlir::Value, std::string> &argMap) {
  auto it = argMap.find(val);
  if (it != argMap.end())
    return it->second;

  mlir::Operation *def = val.getDefiningOp();
  if (!def)
    return "/* null_def */0";

  llvm::StringRef opName = def->getName().getStringRef();

  if (opName == "hw.constant") {
    if (auto attr = def->getAttrOfType<mlir::IntegerAttr>("value")) {
      llvm::SmallString<32> buf;
      if (attr.getValue().isNegative()) {
        attr.getValue().toStringSigned(buf);
      } else {
        attr.getValue().toStringUnsigned(buf);
        buf += "u";
      }
      return std::string("(") + legalCType(attr.getValue().getBitWidth()) +
             ")" + std::string(buf);
    }
    return "/* bad_const */0";
  }

  auto renderBinOp = [&](llvm::StringRef cOp) -> std::string {
    return "(" + renderArcExpr(def->getOperand(0), argMap) + " " +
           cOp.str() + " " + renderArcExpr(def->getOperand(1), argMap) + ")";
  };

  auto renderArcVariadic = [&](llvm::StringRef cOp) -> std::string {
    std::string result = renderArcExpr(def->getOperand(0), argMap);
    for (unsigned i = 1; i < def->getNumOperands(); ++i)
      result = "(" + result + " " + cOp.str() + " " + renderArcExpr(def->getOperand(i), argMap) + ")";
    return result;
  };

  if (opName == "comb.xor" && def->getNumOperands() >= 2)
    return renderArcVariadic("^");
  if (opName == "comb.and" && def->getNumOperands() >= 2)
    return renderArcVariadic("&");
  if (opName == "comb.or" && def->getNumOperands() >= 2)
    return renderArcVariadic("|");
  if (opName == "comb.add" && def->getNumOperands() >= 2)
    return renderArcVariadic("+");
  if (opName == "comb.sub" && def->getNumOperands() == 2)
    return renderBinOp("-");
  if (opName == "comb.mul" && def->getNumOperands() >= 2)
    return renderArcVariadic("*");
  if (opName == "comb.mux" && def->getNumOperands() == 3) {
    return "(" + renderArcExpr(def->getOperand(0), argMap) + " ? " +
           renderArcExpr(def->getOperand(1), argMap) + " : " +
           renderArcExpr(def->getOperand(2), argMap) + ")";
  }

  if (opName == "comb.icmp") {
    auto predAttr = def->getAttrOfType<mlir::IntegerAttr>("predicate");
    if (predAttr && def->getNumOperands() == 2) {
      std::string cOp;
      switch (predAttr.getInt()) {
      case 0: cOp = "=="; break;
      case 1: cOp = "!="; break;
      case 2: cOp = "<"; break;
      case 3: cOp = "<="; break;
      case 4: cOp = ">"; break;
      case 5: cOp = ">="; break;
      case 6: cOp = "<"; break;
      case 7: cOp = "<="; break;
      case 8: cOp = ">"; break;
      case 9: cOp = ">="; break;
      case 10: cOp = "=="; break;
      case 11: cOp = "!="; break;
      case 12: cOp = "=="; break;
      case 13: cOp = "!="; break;
      default: return "/* unsupported_icmp_pred */0";
      }
      return "(" + renderArcExpr(def->getOperand(0), argMap) + " " + cOp + " " +
             renderArcExpr(def->getOperand(1), argMap) + ")";
    }
    return "/* bad_icmp */0";
  }

  if (opName == "comb.concat") {
    unsigned totalWidth = 0;
    if (auto ty = mlir::dyn_cast<mlir::IntegerType>(val.getType()))
      totalWidth = ty.getWidth();
    std::string result = "0";
    unsigned shift = 0;
    for (int i = def->getNumOperands() - 1; i >= 0; --i) {
      unsigned opWidth = 0;
      if (auto ty = mlir::dyn_cast<mlir::IntegerType>(def->getOperand(i).getType()))
        opWidth = ty.getWidth();
      std::string part = "((" + legalCType(totalWidth) + ")" +
                         renderArcExpr(def->getOperand(i), argMap) + " << " +
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
    uint64_t mask = resultWidth >= 64 ? ~0ULL : ((1ULL << resultWidth) - 1);
    return "((" + legalCType(resultWidth) + ")((" + renderArcExpr(def->getOperand(0), argMap) +
           " >> " + std::to_string(lowBit) + ") & " + std::to_string(mask) + "u))";
  }

  if (opName == "arc.call") {
    unsigned resultIdx = 0;
    if (auto opResult = mlir::dyn_cast<mlir::OpResult>(val))
      resultIdx = opResult.getResultNumber();
    return inlineArcCall(def, resultIdx, &argMap, 0);
  }

  return "/* unsupported:" + opName.str() + " */0";
}

void CModelEmitter::emitEvalComb(llvm::raw_string_ostream &os) {
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
    os << "  (void)s;\n";
    os << "}\n\n";
    return;
  }

  bool emittedAnything = false;
  for (auto [idx, operand] : llvm::enumerate(outputOp.getOperands())) {
    if (idx >= model_.outputs.size())
      break;

    const auto &binding = model_.outputs[idx];
    if (binding.visibility != semantic::OutputVisibility::Comb)
      continue;

    if (idx >= model_.outputPorts.size())
      continue;

    std::string expr = renderExpr(operand);
    os << "  s->output_" << model_.outputPorts[idx].name << " = ("
       << legalCType(model_.outputPorts[idx].width) << ")(" << expr << ");\n";
    emittedAnything = true;
  }

  if (!emittedAnything)
    os << "  (void)s;\n";

  os << "}\n\n";
}

void CModelEmitter::emitImpl(llvm::raw_string_ostream &os) {
  const auto &mod = model_.moduleName;

  os << "#include \"" << mod << options_.headerSuffix << "\"\n\n";

  // initialize
  os << "void " << mod << "_initialize(" << mod << "_state *s) {\n";
  for (const auto &port : model_.inputPorts)
    os << "  s->input_" << port.name << " = 0;\n";
  for (const auto &port : model_.outputPorts)
    os << "  s->output_" << port.name << " = 0;\n";
  for (const auto &sv : model_.stateVars) {
    if (sv.hasConstantInit)
      os << "  s->" << sv.stableName << " = " << sv.initValue << ";\n";
    else
      os << "  s->" << sv.stableName << " = 0;\n";
  }
  for (const auto &aggVar : model_.aggregateStateVars) {
    if (aggVar.numElements > 0)
      os << "  for (size_t i = 0; i < " << aggVar.numElements << "; ++i) s->"
         << aggVar.stableName << "[i] = 0;\n";
  }
  for (const auto &mem : model_.memoryVars)
    os << "  for (size_t i = 0; i < " << mem.depth << "; ++i) s->"
       << mem.stableName << "[i] = 0;\n";
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
}

void CModelEmitter::emitEvalClock(llvm::raw_string_ostream &os,
                                  llvm::StringRef clockDomain) {
  const auto &mod = model_.moduleName;
  os << "void " << mod << "_eval_" << clockDomain << "(" << mod
     << "_state *s) {\n";

  if (!hwModule_) {
    os << "  (void)s;\n}\n\n";
    return;
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

  if (domainStates.empty() && domainAggs.empty()) {
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
        argMap[bodyArg] = "/* unresolved_operand */0";
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

  // Phase 2b: aggregate TODO boundary
  for (const auto *av : domainAggs) {
    os << "  /* TODO: aggregate '" << av->stableName << "' update_style="
       << (av->updateStyle == semantic::UpdateStyle::FullReplace ? "full_replace" :
           av->updateStyle == semantic::UpdateStyle::IndexedUpdate ? "indexed_update" :
           "elementwise_update")
       << " -- sequential emit not yet implemented */\n";
  }

  // Phase 3: update edge and post_edge_comb output shadows
  if (auto outputOp = mlir::dyn_cast<circt::hw::OutputOp>(
          hwModule_.getBodyBlock()->getTerminator())) {
    for (auto [idx, operand] : llvm::enumerate(outputOp.getOperands())) {
      if (idx >= model_.outputs.size() || idx >= model_.outputPorts.size())
        break;

      const auto &binding = model_.outputs[idx];
      if (binding.visibility == semantic::OutputVisibility::Edge) {
        if (!binding.sourceEntity.empty()) {
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
          } else {
            os << "  s->output_" << model_.outputPorts[idx].name
               << " = s->" << binding.sourceEntity << ";\n";
          }
        }
      } else if (binding.visibility == semantic::OutputVisibility::PostEdgeComb) {
        // Post-edge comb: re-evaluate combinational expression with new state
        // Reuse renderExpr which reads from input shadows and (now updated) state
        std::string expr = renderExpr(operand);
        os << "  s->output_" << model_.outputPorts[idx].name << " = ("
           << legalCType(model_.outputPorts[idx].width) << ")(" << expr << ");\n";
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

  // Recursively render the target output expression within the callee body
  std::function<std::string(mlir::Value)> renderInBody;
  renderInBody = [&](mlir::Value val) -> std::string {
    auto it = innerArgMap.find(val);
    if (it != innerArgMap.end())
      return it->second;

    mlir::Operation *def = val.getDefiningOp();
    if (!def)
      return "/* null_def */0";

    llvm::StringRef opName = def->getName().getStringRef();

    if (opName == "hw.constant") {
      if (auto attr = def->getAttrOfType<mlir::IntegerAttr>("value")) {
        llvm::SmallString<32> buf;
        if (attr.getValue().isNegative())
          attr.getValue().toStringSigned(buf);
        else {
          attr.getValue().toStringUnsigned(buf);
          buf += "u";
        }
        return std::string("(") + legalCType(attr.getValue().getBitWidth()) +
               ")" + std::string(buf);
      }
      return "/* bad_const */0";
    }

    auto renderVarInBody = [&](llvm::StringRef cOp) -> std::string {
      std::string result = renderInBody(def->getOperand(0));
      for (unsigned i = 1; i < def->getNumOperands(); ++i)
        result = "(" + result + " " + cOp.str() + " " +
                 renderInBody(def->getOperand(i)) + ")";
      return result;
    };

    if (opName == "comb.xor" && def->getNumOperands() >= 2)
      return renderVarInBody("^");
    if (opName == "comb.and" && def->getNumOperands() >= 2)
      return renderVarInBody("&");
    if (opName == "comb.or" && def->getNumOperands() >= 2)
      return renderVarInBody("|");
    if (opName == "comb.add" && def->getNumOperands() >= 2)
      return renderVarInBody("+");
    if (opName == "comb.sub" && def->getNumOperands() == 2) {
      return "(" + renderInBody(def->getOperand(0)) + " - " +
             renderInBody(def->getOperand(1)) + ")";
    }
    if (opName == "comb.mul" && def->getNumOperands() >= 2)
      return renderVarInBody("*");
    if (opName == "comb.mux" && def->getNumOperands() == 3) {
      return "(" + renderInBody(def->getOperand(0)) + " ? " +
             renderInBody(def->getOperand(1)) + " : " +
             renderInBody(def->getOperand(2)) + ")";
    }

    if (opName == "comb.icmp") {
      auto predAttr = def->getAttrOfType<mlir::IntegerAttr>("predicate");
      if (predAttr && def->getNumOperands() == 2) {
        std::string cOp;
        switch (predAttr.getInt()) {
        case 0: cOp = "=="; break; case 1: cOp = "!="; break;
        case 2: cOp = "<"; break;  case 3: cOp = "<="; break;
        case 4: cOp = ">"; break;  case 5: cOp = ">="; break;
        case 6: cOp = "<"; break;  case 7: cOp = "<="; break;
        case 8: cOp = ">"; break;  case 9: cOp = ">="; break;
        case 10: cOp = "=="; break; case 11: cOp = "!="; break;
        case 12: cOp = "=="; break; case 13: cOp = "!="; break;
        default: return "/* unsupported_icmp_pred */0";
        }
        return "(" + renderInBody(def->getOperand(0)) + " " + cOp + " " +
               renderInBody(def->getOperand(1)) + ")";
      }
      return "/* bad_icmp */0";
    }

    if (opName == "comb.concat") {
      unsigned totalWidth = 0;
      if (auto ty = mlir::dyn_cast<mlir::IntegerType>(val.getType()))
        totalWidth = ty.getWidth();
      std::string result = "0";
      unsigned shift = 0;
      for (int i = def->getNumOperands() - 1; i >= 0; --i) {
        unsigned opWidth = 0;
        if (auto ty = mlir::dyn_cast<mlir::IntegerType>(
                def->getOperand(i).getType()))
          opWidth = ty.getWidth();
        std::string part = "((" + legalCType(totalWidth) + ")" +
                           renderInBody(def->getOperand(i)) + " << " +
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
      uint64_t mask =
          resultWidth >= 64 ? ~0ULL : ((1ULL << resultWidth) - 1);
      return "((" + legalCType(resultWidth) + ")((" +
             renderInBody(def->getOperand(0)) + " >> " +
             std::to_string(lowBit) + ") & " + std::to_string(mask) + "u))";
    }

    if (opName == "arc.call") {
      unsigned ri = 0;
      if (auto opResult = mlir::dyn_cast<mlir::OpResult>(val))
        ri = opResult.getResultNumber();
      return inlineArcCall(def, ri, nullptr, depth + 1);
    }

    return "/* unsupported:" + opName.str() + " */0";
  };

  return renderInBody(outputOp.getOutputs()[resultIdx]);
}

} // namespace hirct
