//===- Validation.cpp - Semantic model validation ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file
/// This file implements validation for semantic model entities.
///
//===----------------------------------------------------------------------===//

#include "hirct/SemanticModel/Validation.h"

#include "llvm/ADT/StringSet.h"
#include "llvm/Support/ErrorHandling.h"

#include <cctype>

namespace hirct::semantic {

static constexpr unsigned kMaxSupportedWidth = 64;

std::string normalizeIdentifier(llvm::StringRef raw) {
  std::string result;
  result.reserve(raw.size());
  for (char c : raw) {
    if (std::isalnum(static_cast<unsigned char>(c)))
      result += c;
    else
      result += '_';
  }
  if (!result.empty() && std::isdigit(static_cast<unsigned char>(result[0])))
    result.insert(result.begin(), '_');
  if (result.empty())
    result = "_empty";
  return result;
}

bool isValidCIdentifier(llvm::StringRef name) {
  if (name.empty())
    return false;
  if (!std::isalpha(static_cast<unsigned char>(name[0])) && name[0] != '_')
    return false;
  for (char c : name) {
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
      return false;
  }
  return true;
}

llvm::StringRef stringifyOutputVisibility(OutputVisibility visibility) {
  switch (visibility) {
  case OutputVisibility::Comb:
    return "comb";
  case OutputVisibility::Edge:
    return "edge";
  case OutputVisibility::PostEdgeComb:
    return "post_edge_comb";
  }
  llvm_unreachable("covered switch");
}

llvm::StringRef stringifyRejectCode(RejectCode code) {
  switch (code) {
  case RejectCode::ClockDomainAmbiguous:
    return "CLOCK_DOMAIN_AMBIGUOUS";
  case RejectCode::MemoryShapeAmbiguous:
    return "MEMORY_SHAPE_AMBIGUOUS";
  case RejectCode::OutputVisibilityAmbiguous:
    return "OUTPUT_VISIBILITY_AMBIGUOUS";
  case RejectCode::InitResetConflict:
    return "INIT_RESET_CONFLICT";
  case RejectCode::NonConstantInit:
    return "NON_CONSTANT_INIT";
  case RejectCode::AggregateUpdateAmbiguous:
    return "AGGREGATE_UPDATE_AMBIGUOUS";
  case RejectCode::MemoryPortUnsupported:
    return "MEMORY_PORT_UNSUPPORTED";
  case RejectCode::OutputDecompAmbiguous:
    return "OUTPUT_DECOMP_AMBIGUOUS";
  case RejectCode::InvalidIdentifier:
    return "INVALID_IDENTIFIER";
  case RejectCode::WidthUnsupported:
    return "WIDTH_UNSUPPORTED";
  }
  llvm_unreachable("covered switch");
}

llvm::StringRef stringifyUpdateStyle(UpdateStyle style) {
  switch (style) {
  case UpdateStyle::FullReplace:
    return "full_replace";
  case UpdateStyle::IndexedUpdate:
    return "indexed_update";
  case UpdateStyle::ElementwiseUpdate:
    return "elementwise_update";
  }
  llvm_unreachable("covered switch");
}

llvm::StringRef stringifyInitPolicy(InitPolicy policy) {
  switch (policy) {
  case InitPolicy::Zero:
    return "zero";
  case InitPolicy::Uninitialized:
    return "uninitialized";
  case InitPolicy::ImplementationDefined:
    return "implementation_defined";
  }
  llvm_unreachable("covered switch");
}

static void validateWidth(unsigned width, llvm::StringRef entity,
                          llvm::StringRef entityName,
                          llvm::SmallVectorImpl<ValidationError> &errors) {
  if (width > kMaxSupportedWidth)
    errors.push_back({RejectCode::WidthUnsupported,
                      std::string(entity) + " `" + entityName.str() +
                          "` has unsupported width " + std::to_string(width) +
                          " (max " + std::to_string(kMaxSupportedWidth) + ")"});
}

static void validateIdentifier(llvm::StringRef name, llvm::StringRef context,
                                llvm::SmallVectorImpl<ValidationError> &errors) {
  if (!isValidCIdentifier(name))
    errors.push_back({RejectCode::InvalidIdentifier,
                      std::string(context) + " `" + name.str() +
                          "` is not a valid C identifier"});
}

llvm::SmallVector<ValidationError>
validateModuleModel(const ModuleModel &model) {
  llvm::SmallVector<ValidationError> errors;

  validateIdentifier(model.moduleName, "module name", errors);

  llvm::StringSet<> usedNames;

  for (const auto &port : model.inputPorts) {
    validateIdentifier(port.name, "input port", errors);
    validateWidth(port.width, "input port", port.name, errors);
    if (!usedNames.insert(port.name).second)
      errors.push_back({RejectCode::InvalidIdentifier,
                        "input port `" + port.name + "` has duplicate name"});
  }
  for (const auto &port : model.outputPorts) {
    validateIdentifier(port.name, "output port", errors);
    validateWidth(port.width, "output port", port.name, errors);
    if (!usedNames.insert(port.name).second)
      errors.push_back({RejectCode::InvalidIdentifier,
                        "output port `" + port.name + "` has duplicate name"});
  }

  for (const auto &stateVar : model.stateVars) {
    validateIdentifier(stateVar.stableName, "state", errors);
    validateWidth(stateVar.width, "state", stateVar.stableName, errors);
    if (stateVar.clockDomain.empty()) {
      errors.push_back(
          {RejectCode::ClockDomainAmbiguous,
           "state `" + stateVar.stableName + "` has no clock domain"});
    }
    if (stateVar.hasConstantInit && stateVar.hasReset) {
      errors.push_back(
          {RejectCode::InitResetConflict,
           "state `" + stateVar.stableName +
               "` has both constant init and reset"});
    }
    if (stateVar.hasNonConstantInit) {
      errors.push_back(
          {RejectCode::NonConstantInit,
           "state `" + stateVar.stableName +
               "` has non-constant init expression"});
    }
  }

  for (const auto &aggVar : model.aggregateStateVars) {
    validateIdentifier(aggVar.stableName, "aggregate state", errors);
    validateWidth(aggVar.elementWidth, "aggregate state element",
                  aggVar.stableName, errors);
    if (aggVar.clockDomain.empty()) {
      errors.push_back(
          {RejectCode::ClockDomainAmbiguous,
           "aggregate state `" + aggVar.stableName + "` has no clock domain"});
    }
    if (aggVar.numElements == 0 || aggVar.elementWidth == 0) {
      errors.push_back(
          {RejectCode::AggregateUpdateAmbiguous,
           "aggregate state `" + aggVar.stableName +
               "` has ambiguous shape (cannot determine update style)"});
    }
    if (!aggVar.updateStyleDetermined) {
      errors.push_back(
          {RejectCode::AggregateUpdateAmbiguous,
           "aggregate state `" + aggVar.stableName +
               "` has undetermined update style"});
    }
  }

  for (const auto &memoryVar : model.memoryVars) {
    validateIdentifier(memoryVar.stableName, "memory", errors);
    validateWidth(memoryVar.elementWidth, "memory element",
                  memoryVar.stableName, errors);
    if (memoryVar.depth == 0 || memoryVar.elementWidth == 0) {
      errors.push_back({RejectCode::MemoryShapeAmbiguous,
                        "memory `" + memoryVar.stableName +
                            "` has invalid shape"});
    }
    if (memoryVar.clockDomain.empty()) {
      errors.push_back(
          {RejectCode::ClockDomainAmbiguous,
           "memory `" + memoryVar.stableName + "` has no clock domain"});
    }
    if (memoryVar.addressWidth == 0) {
      errors.push_back({RejectCode::MemoryShapeAmbiguous,
                        "memory `" + memoryVar.stableName +
                            "` has zero address width"});
    }
    if (memoryVar.readPortCount > 1 || memoryVar.writePortCount > 1) {
      errors.push_back(
          {RejectCode::MemoryPortUnsupported,
           "memory `" + memoryVar.stableName +
               "` has multi-port (read=" +
               std::to_string(memoryVar.readPortCount) +
               " write=" + std::to_string(memoryVar.writePortCount) + ")"});
    }
  }

  for (const auto &output : model.outputs) {
    if (output.visibility == OutputVisibility::Edge &&
        output.sourceEntity.empty()) {
      errors.push_back(
          {RejectCode::OutputDecompAmbiguous,
           "output `" + output.name +
               "` has edge visibility but no source entity"});
    }
  }

  for (const auto &clock : model.clockDomains) {
    if (!isValidCIdentifier(clock))
      errors.push_back({RejectCode::InvalidIdentifier,
                        "clock domain `" + clock +
                            "` is not a valid C identifier"});
  }

  auto makeCollisionChecker = [&errors](llvm::StringRef nsLabel) {
    auto names = std::make_shared<llvm::StringSet<>>();
    return [names, nsLabel,
            &errors](llvm::StringRef name, llvm::StringRef context) {
      std::string norm = normalizeIdentifier(name);
      if (!names->insert(norm).second)
        errors.push_back({RejectCode::InvalidIdentifier,
                          std::string(context) + " `" + name.str() +
                              "` collides after normalization (normalized: `" +
                              norm + "`) in " + nsLabel.str() + " namespace"});
    };
  };

  auto checkPortCollision = makeCollisionChecker("port");
  for (const auto &port : model.inputPorts)
    checkPortCollision(port.name, "input port");
  for (const auto &port : model.outputPorts)
    checkPortCollision(port.name, "output port");

  auto checkStructCollision = makeCollisionChecker("struct field");
  for (const auto &sv : model.stateVars)
    checkStructCollision(sv.stableName, "state");
  for (const auto &agg : model.aggregateStateVars)
    checkStructCollision(agg.stableName, "aggregate state");
  for (const auto &mem : model.memoryVars)
    checkStructCollision(mem.stableName, "memory");

  auto checkClockCollision = makeCollisionChecker("clock");
  for (const auto &clock : model.clockDomains)
    checkClockCollision(clock, "clock domain");

  return errors;
}

} // namespace hirct::semantic
