//===- SystemCWrapperEmitter.h - SystemC wrapper emitter --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef HIRCT_TARGET_SYSTEMCWRAPPEREMITTER_H
#define HIRCT_TARGET_SYSTEMCWRAPPEREMITTER_H

#include "hirct/Target/CModelEmitter.h"

#include <string>

namespace hirct {

struct SystemCWrapperArtifact {
  std::string moduleName;
  std::string wrapperHeaderPath;
  std::string wrapperImplPath;
  std::string wrapperHeaderContent;
  std::string wrapperImplContent;
};

class SystemCWrapperEmitter {
public:
  explicit SystemCWrapperEmitter(const semantic::ModuleModel &model,
                                 const CModelOptions &options);

  SystemCWrapperArtifact emit();

private:
  void emitWrapperHeader(llvm::raw_string_ostream &os);
  void emitWrapperImpl(llvm::raw_string_ostream &os);

  const semantic::ModuleModel &model_;
  CModelOptions options_;
};

bool writeArtifact(const SystemCWrapperArtifact &artifact);

} // namespace hirct

#endif // HIRCT_TARGET_SYSTEMCWRAPPEREMITTER_H
