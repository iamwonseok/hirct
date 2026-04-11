//===- Model.h - Semantic model entities ------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file
/// This file defines the semantic model entities built from Arc MLIR.
///
//===----------------------------------------------------------------------===//

#ifndef HIRCT_SEMANTICMODEL_MODEL_H
#define HIRCT_SEMANTICMODEL_MODEL_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

#include <climits>
#include <optional>
#include <string>

namespace hirct::semantic {

enum class OutputVisibility { Comb, Edge, PostEdgeComb };

enum class UpdateStyle { FullReplace, IndexedUpdate, ElementwiseUpdate };

enum class InitPolicy { Zero, Uninitialized, ImplementationDefined };

enum class RejectCode {
  ClockDomainAmbiguous,
  MemoryShapeAmbiguous,
  OutputVisibilityAmbiguous,
  InitResetConflict,
  NonConstantInit,
  AggregateUpdateAmbiguous,
  MemoryPortUnsupported,
  OutputDecompAmbiguous,
  InvalidIdentifier,
  WidthUnsupported,
};

enum class ClockBoundaryKind {
  CombOr,
  CombAnd,
  CombXor,
  CombOther,
  ClockGate,
  MuxTrueUnresolved,
};

struct ClockBoundaryReason {
  ClockBoundaryKind kind;
  std::string affectedState;
  std::string opDescription;
};

llvm::StringRef stringifyClockBoundaryKind(ClockBoundaryKind kind);

struct PortInfo {
  std::string name;
  bool isInput = false;
  unsigned width = 0;

  bool isWide() const { return width > 64; }
  unsigned wordCount() const { return isWide() ? (width + 63) / 64 : 0; }
};

struct OpaqueExprRef {
  std::string opName;
  unsigned resultIndex = 0;
  unsigned blockArgNumber = UINT_MAX;
  unsigned positionInBlock = UINT_MAX;

  bool isBlockArg() const { return blockArgNumber != UINT_MAX; }
};

struct StateVar {
  std::string stableName;
  std::string arcName;
  std::string clockDomain;
  unsigned width = 0;
  unsigned stateOpIndex = UINT_MAX;
  bool hasEnable = false;
  bool hasReset = false;
  bool hasConstantInit = false;
  bool hasNonConstantInit = false;
  std::string initValue;
  std::optional<OpaqueExprRef> enableRef;
  std::optional<OpaqueExprRef> resetRef;
};

struct AggregateStateVar {
  std::string stableName;
  std::string arcName;
  std::string clockDomain;
  unsigned width = 0;
  unsigned numElements = 0;
  unsigned elementWidth = 0;
  UpdateStyle updateStyle = UpdateStyle::FullReplace;
  bool updateStyleDetermined = false;
  bool hasEnable = false;
  bool hasReset = false;
  bool hasConstantInit = false;
  std::string initValue;
  std::optional<OpaqueExprRef> enableRef;
  std::optional<OpaqueExprRef> resetRef;
};

struct MemoryVar {
  std::string stableName;
  std::string clockDomain;
  unsigned depth = 0;
  unsigned addressWidth = 0;
  unsigned elementWidth = 0;
  bool hasReadPort = false;
  bool hasWritePort = false;
  unsigned readPortCount = 0;
  unsigned writePortCount = 0;
  InitPolicy initPolicy = InitPolicy::Uninitialized;
};

struct OutputBinding {
  std::string name;
  std::string sourceKind;
  std::string sourceEntity;
  OutputVisibility visibility = OutputVisibility::Comb;
};

struct ValidationError {
  RejectCode code;
  std::string message;
};

struct ModuleModel {
  std::string moduleName;
  llvm::SmallVector<PortInfo> inputPorts;
  llvm::SmallVector<PortInfo> outputPorts;
  llvm::SmallVector<std::string> clockDomains;
  llvm::SmallVector<StateVar> stateVars;
  llvm::SmallVector<AggregateStateVar> aggregateStateVars;
  llvm::SmallVector<MemoryVar> memoryVars;
  llvm::SmallVector<OutputBinding> outputs;
  llvm::SmallVector<ClockBoundaryReason> boundaryReasons;
};

llvm::StringRef stringifyOutputVisibility(OutputVisibility visibility);
llvm::StringRef stringifyRejectCode(RejectCode code);

} // namespace hirct::semantic

#endif // HIRCT_SEMANTICMODEL_MODEL_H
