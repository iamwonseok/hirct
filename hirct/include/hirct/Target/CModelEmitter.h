//===- CModelEmitter.h - C model code emitter -------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef HIRCT_TARGET_CMODELEMITTER_H
#define HIRCT_TARGET_CMODELEMITTER_H

#include "hirct/SemanticModel/Model.h"

#include "circt/Dialect/HW/HWOps.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace hirct {

struct CModelOptions {
  std::string outputDir;
  std::string headerSuffix = ".h";
  std::string implSuffix = ".cpp";
};

struct ChildInstanceInfo {
  std::string instanceName;
  std::string childModuleName;
  llvm::SmallVector<std::pair<std::string, unsigned>> inputPorts;
  llvm::SmallVector<std::pair<std::string, unsigned>> outputPorts;
  llvm::SmallVector<std::string> clockDomains;
};

struct CModelArtifact {
  std::string moduleName;
  std::string outputRoot;
  std::string headerPath;
  std::string implPath;
  std::string headerContent;
  std::string implContent;
};

class CModelEmitter {
public:
  explicit CModelEmitter(const semantic::ModuleModel &model,
                         const CModelOptions &options,
                         circt::hw::HWModuleOp hwModule = nullptr);

  void setChildInstances(llvm::SmallVector<ChildInstanceInfo> children) {
    childInstances_ = std::move(children);
  }

  CModelArtifact emit();

  const semantic::ModuleModel &getModel() const { return model_; }

private:
  void emitHeader(llvm::raw_string_ostream &os);
  void emitImpl(llvm::raw_string_ostream &os);
  void emitSetters(llvm::raw_string_ostream &os);
  void emitGetters(llvm::raw_string_ostream &os);
  void emitEvalComb(llvm::raw_string_ostream &os);
  void emitEvalClock(llvm::raw_string_ostream &os, llvm::StringRef clockDomain);
  std::string renderExpr(mlir::Value val);
  std::string renderArcExpr(mlir::Value val,
                            const llvm::DenseMap<mlir::Value, std::string> &argMap);
  std::string renderArcExprCached(
      mlir::Value val,
      const llvm::DenseMap<mlir::Value, std::string> &argMap,
      llvm::DenseMap<mlir::Value, std::string> &cache);
  std::string inlineArcCall(mlir::Operation *callOp,
                            unsigned resultIdx,
                            const llvm::DenseMap<mlir::Value, std::string> *outerArgMap,
                            unsigned depth);
  std::string legalCType(unsigned width);
  static std::string legalSignedCType(unsigned width);
  static std::string renderIcmpExpr(int64_t predicate,
                                    const std::string &lhs,
                                    const std::string &rhs,
                                    unsigned operandWidth);
  std::string wideStorageDecl(const semantic::PortInfo &port,
                              llvm::StringRef prefix);
  void emitWideInputApi(llvm::raw_string_ostream &os,
                        const semantic::PortInfo &port);
  void emitWideOutputApi(llvm::raw_string_ostream &os,
                         const semantic::PortInfo &port);
  void emitAggregateInitLiterals(llvm::raw_string_ostream &os,
                                 const semantic::AggregateStateVar &aggVar,
                                 llvm::StringRef indent);

  static constexpr unsigned kMaxInlineDepth = 16;

  void emitChildIncludes(llvm::raw_string_ostream &os);
  void emitChildStateFields(llvm::raw_string_ostream &os);
  void emitChildInitCalls(llvm::raw_string_ostream &os);
  void emitChildEvalCombWiring(llvm::raw_string_ostream &os);
  void emitChildEvalClockCalls(llvm::raw_string_ostream &os,
                               llvm::StringRef clockDomain);
  void preSeedInstanceBindings();
  void resetExprCacheForFunction();

  const semantic::ModuleModel &model_;
  CModelOptions options_;
  circt::hw::HWModuleOp hwModule_;
  llvm::SmallVector<ChildInstanceInfo> childInstances_;
  llvm::DenseMap<mlir::Value, std::string> exprCache_;
};

bool writeArtifact(const CModelArtifact &artifact);

} // namespace hirct

#endif // HIRCT_TARGET_CMODELEMITTER_H
