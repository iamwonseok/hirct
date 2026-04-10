//===- Validation.h - Semantic model validation -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file
/// This file declares validation for semantic model entities.
///
//===----------------------------------------------------------------------===//

#ifndef HIRCT_SEMANTICMODEL_VALIDATION_H
#define HIRCT_SEMANTICMODEL_VALIDATION_H

#include "hirct/SemanticModel/Model.h"

#include "llvm/ADT/SmallVector.h"

namespace hirct::semantic {

llvm::SmallVector<ValidationError> validateModuleModel(const ModuleModel &model);
llvm::StringRef stringifyUpdateStyle(UpdateStyle style);
llvm::StringRef stringifyInitPolicy(InitPolicy policy);

std::string normalizeIdentifier(llvm::StringRef raw);

bool isValidCIdentifier(llvm::StringRef name);

} // namespace hirct::semantic

#endif // HIRCT_SEMANTICMODEL_VALIDATION_H
