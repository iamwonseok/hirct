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

struct CModelArtifact {
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
  std::string inlineArcCall(mlir::Operation *callOp,
                            unsigned resultIdx,
                            const llvm::DenseMap<mlir::Value, std::string> *outerArgMap,
                            unsigned depth);
  std::string legalCType(unsigned width);
  std::string wideStorageDecl(const semantic::PortInfo &port,
                              llvm::StringRef prefix);
  void emitWideInputApi(llvm::raw_string_ostream &os,
                        const semantic::PortInfo &port);
  void emitWideOutputApi(llvm::raw_string_ostream &os,
                         const semantic::PortInfo &port);

  static constexpr unsigned kMaxInlineDepth = 16;

  const semantic::ModuleModel &model_;
  CModelOptions options_;
  circt::hw::HWModuleOp hwModule_;
};

} // namespace hirct

#endif // HIRCT_TARGET_CMODELEMITTER_H
