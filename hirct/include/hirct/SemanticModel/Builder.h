//===- Builder.h - Build semantic models from Arc MLIR ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file
/// This file declares builders from Arc MLIR to semantic model entities.
///
//===----------------------------------------------------------------------===//

#ifndef HIRCT_SEMANTICMODEL_BUILDER_H
#define HIRCT_SEMANTICMODEL_BUILDER_H

#include "hirct/SemanticModel/Model.h"

#include "circt/Dialect/HW/HWOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Support/LogicalResult.h"
#include "llvm/ADT/StringRef.h"

namespace hirct::semantic {

mlir::FailureOr<ModuleModel> buildModuleModel(mlir::ModuleOp module,
                                              llvm::StringRef moduleName);
mlir::FailureOr<ModuleModel> buildModuleModel(circt::hw::HWModuleOp hwModule);

} // namespace hirct::semantic

#endif // HIRCT_SEMANTICMODEL_BUILDER_H
