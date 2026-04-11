//===- Builder.cpp - Build semantic models from Arc MLIR --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file
/// This file implements builders from Arc MLIR to semantic model entities.
///
//===----------------------------------------------------------------------===//

#include "hirct/Analysis/IRAnalysis.h"
#include "hirct/SemanticModel/Builder.h"
#include "hirct/SemanticModel/Validation.h"

#include "circt/Dialect/Arc/ArcOps.h"
#include "circt/Dialect/Arc/ArcTypes.h"
#include "circt/Dialect/Comb/CombDialect.h"
#include "circt/Dialect/Comb/CombOps.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/HW/HWTypes.h"
#include "circt/Dialect/Seq/SeqOps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"

using namespace mlir;
using namespace circt;

namespace hirct::semantic {

namespace {

static hw::HWModuleOp findHWModule(ModuleOp module, llvm::StringRef moduleName) {
  for (auto hwModule : module.getOps<hw::HWModuleOp>())
    if (hwModule.getName() == moduleName)
      return hwModule;
  return {};
}

static unsigned getTypeWidth(Type type) {
  if (auto intType = dyn_cast<IntegerType>(type))
    return intType.getWidth();
  if (auto arrayType = dyn_cast<hw::ArrayType>(type))
    return arrayType.getNumElements() * getTypeWidth(arrayType.getElementType());
  return 0;
}

static void addUniqueClock(llvm::SmallVectorImpl<std::string> &clocks,
                           llvm::StringRef clockName) {
  if (clockName.empty())
    return;
  if (llvm::find(clocks, clockName.str()) == clocks.end())
    clocks.push_back(clockName.str());
}

static hirct::ClockTraceResult traceClockFull(Value value,
                                              hw::HWModuleOp hwModule) {
  auto parentModule = hwModule->getParentOfType<ModuleOp>();
  if (!parentModule)
    return {};
  return hirct::trace_clock_result(value, hwModule, parentModule);
}

static std::string traceClockName(Value value,
                                  const llvm::SmallVector<PortInfo> &inputPorts,
                                  hw::HWModuleOp hwModule) {
  (void)inputPorts;
  auto trace = traceClockFull(value, hwModule);
  if (trace.isResolved())
    return trace.clock_port_name;
  return {};
}

static ClockBoundaryKind classifyBoundaryOp(llvm::StringRef opName) {
  if (opName == "comb.or")
    return ClockBoundaryKind::CombOr;
  if (opName == "comb.and")
    return ClockBoundaryKind::CombAnd;
  if (opName == "comb.xor")
    return ClockBoundaryKind::CombXor;
  if (opName == "seq.clock_gate")
    return ClockBoundaryKind::ClockGate;
  if (opName == "comb.mux")
    return ClockBoundaryKind::MuxTrueUnresolved;
  return ClockBoundaryKind::CombOther;
}

static std::string getStateName(arc::StateOp state, unsigned fallbackIndex) {
  if (auto names = state->getAttrOfType<ArrayAttr>("names")) {
    if (!names.empty())
      if (auto nameAttr = dyn_cast<StringAttr>(names[0]))
        return normalizeIdentifier(nameAttr.getValue());
  }
  return std::string("state_") + llvm::utostr(fallbackIndex);
}

static unsigned getResultWidth(ValueRange values) {
  unsigned totalWidth = 0;
  for (auto value : values)
    totalWidth += getTypeWidth(value.getType());
  return totalWidth;
}

static bool valueReachesOldState(Value v, Value oldState,
                                 llvm::DenseSet<Value> &visited) {
  if (!visited.insert(v).second)
    return false;
  if (v == oldState)
    return true;
  Operation *def = v.getDefiningOp();
  if (!def)
    return false;
  for (Value operand : def->getOperands()) {
    if (valueReachesOldState(operand, oldState, visited))
      return true;
  }
  return false;
}

static bool hasIcmpMuxPattern(Value v, Value oldState) {
  Operation *def = v.getDefiningOp();
  if (!def)
    return false;
  if (def->getName().getStringRef() == "comb.mux") {
    Value cond = def->getOperand(0);
    Operation *condDef = cond.getDefiningOp();
    if (condDef && condDef->getName().getStringRef() == "comb.icmp")
      return true;
  }
  for (Value operand : def->getOperands()) {
    Operation *opDef = operand.getDefiningOp();
    if (opDef && opDef->getName().getStringRef() == "comb.mux") {
      Value cond = opDef->getOperand(0);
      Operation *condDef = cond.getDefiningOp();
      if (condDef && condDef->getName().getStringRef() == "comb.icmp")
        return true;
    }
  }
  return false;
}

static UpdateStyle classifyAggregateUpdateStyle(arc::DefineOp arcDef,
                                                unsigned numElements,
                                                bool &determined) {
  determined = false;

  if (!arcDef || arcDef.getBody().empty())
    return UpdateStyle::FullReplace;

  Block &body = arcDef.getBody().front();
  auto outputOp = dyn_cast<arc::OutputOp>(body.getTerminator());
  if (!outputOp || outputOp.getOutputs().empty())
    return UpdateStyle::FullReplace;

  Value outputVal = outputOp.getOutputs().front();

  if (body.getNumArguments() == 0) {
    determined = true;
    return UpdateStyle::FullReplace;
  }

  Value oldState = body.getArgument(0);
  if (!isa<hw::ArrayType>(oldState.getType())) {
    determined = true;
    return UpdateStyle::FullReplace;
  }

  Operation *outputDef = outputVal.getDefiningOp();
  if (!outputDef) {
    if (outputVal == oldState) {
      determined = true;
      return UpdateStyle::FullReplace;
    }
    determined = true;
    return UpdateStyle::FullReplace;
  }

  bool outputIsArrayCreate =
      outputDef->getName().getStringRef() == "hw.array_create";

  if (!outputIsArrayCreate) {
    llvm::DenseSet<Value> visited;
    if (!valueReachesOldState(outputVal, oldState, visited)) {
      determined = true;
      return UpdateStyle::FullReplace;
    }
    return UpdateStyle::FullReplace;
  }

  unsigned arrayGetCount = 0;
  bool allElementsFromOld = true;
  bool hasSelectPattern = false;
  bool hasExternalInput = false;

  for (Value operand : outputDef->getOperands()) {
    llvm::DenseSet<Value> visited;
    bool reachesOld = valueReachesOldState(operand, oldState, visited);

    if (!reachesOld) {
      hasExternalInput = true;
      continue;
    }

    if (hasIcmpMuxPattern(operand, oldState))
      hasSelectPattern = true;
  }

  for (Operation &op : body) {
    if (op.getName().getStringRef() == "hw.array_get") {
      if (op.getNumOperands() > 0 && op.getOperand(0) == oldState)
        ++arrayGetCount;
    }
  }

  if (hasExternalInput && hasSelectPattern)
    return UpdateStyle::FullReplace;

  if (hasExternalInput && arrayGetCount > 0 && arrayGetCount < numElements)
    return UpdateStyle::FullReplace;

  if (hasSelectPattern && arrayGetCount > 0) {
    determined = true;
    return UpdateStyle::IndexedUpdate;
  }

  if (arrayGetCount == numElements && !hasSelectPattern && !hasExternalInput) {
    determined = true;
    return UpdateStyle::ElementwiseUpdate;
  }

  if (arrayGetCount == 0) {
    llvm::DenseSet<Value> visited;
    if (!valueReachesOldState(outputVal, oldState, visited)) {
      determined = true;
      return UpdateStyle::FullReplace;
    }
  }

  return UpdateStyle::FullReplace;
}

static OpaqueExprRef buildOpaqueRef(Value val) {
  OpaqueExprRef ref;
  if (auto blockArg = dyn_cast<BlockArgument>(val)) {
    ref.opName = "block_arg";
    ref.blockArgNumber = blockArg.getArgNumber();
    return ref;
  }
  if (Operation *def = val.getDefiningOp()) {
    ref.opName = def->getName().getStringRef().str();
    for (unsigned i = 0; i < def->getNumResults(); ++i) {
      if (def->getResult(i) == val) {
        ref.resultIndex = i;
        break;
      }
    }
    Block *block = def->getBlock();
    if (block) {
      unsigned pos = 0;
      for (Operation &op : *block) {
        if (&op == def) {
          ref.positionInBlock = pos;
          break;
        }
        ++pos;
      }
    }
  }
  return ref;
}

} // namespace

FailureOr<ModuleModel> buildModuleModel(ModuleOp module,
                                        llvm::StringRef moduleName) {
  auto hwModule = findHWModule(module, moduleName);
  if (!hwModule) {
    module.emitError() << "failed to find hw.module @" << moduleName;
    return failure();
  }
  return buildModuleModel(hwModule);
}

FailureOr<ModuleModel> buildModuleModel(hw::HWModuleOp hwModule) {
  ModuleModel model;
  model.moduleName = normalizeIdentifier(hwModule.getName());

  for (auto &port : hwModule.getPortList()) {
    PortInfo info;
    info.name = normalizeIdentifier(port.getName());
    info.isInput = port.isInput();
    info.width = getTypeWidth(port.type);
    if (info.isInput)
      model.inputPorts.push_back(info);
    else
      model.outputPorts.push_back(info);
  }

  llvm::DenseSet<Operation *> statefulOps;
  llvm::DenseMap<Operation *, std::string> statefulEntityNames;

  unsigned nextStateIndex = 0;
  unsigned nextAggIndex = 0;
  unsigned walkStateOpIndex = 0;
  hwModule.walk([&](arc::StateOp state) {
    unsigned currentWalkIndex = walkStateOpIndex++;
    bool isAggregate = false;
    unsigned numElements = 0;
    unsigned elementWidth = 0;
    if (!state.getOutputs().empty()) {
      Type resultType = state.getOutputs().front().getType();
      if (auto arrayType = dyn_cast<hw::ArrayType>(resultType)) {
        isAggregate = true;
        numElements = arrayType.getNumElements();
        elementWidth = getTypeWidth(arrayType.getElementType());
      }
    }

    auto clockTrace = traceClockFull(state.getClock(), hwModule);
    std::string clockDomain =
        clockTrace.isResolved() ? clockTrace.clock_port_name : std::string();
    if (!clockTrace.isResolved() && clockTrace.hit_boundary) {
      std::string stateName = getStateName(state, isAggregate ? nextAggIndex : nextStateIndex);
      model.boundaryReasons.push_back(
          {classifyBoundaryOp(clockTrace.boundary_op_name),
           stateName, clockTrace.boundary_op_name});
    }
    statefulOps.insert(state.getOperation());

    if (isAggregate) {
      AggregateStateVar aggVar;
      aggVar.stableName = getStateName(state, nextAggIndex++);
      aggVar.arcName = state.getArcAttr().getRootReference().getValue().str();
      aggVar.clockDomain = clockDomain;
      aggVar.width = getResultWidth(state.getOutputs());
      aggVar.numElements = numElements;
      aggVar.elementWidth = elementWidth;

      arc::DefineOp arcDef = nullptr;
      if (auto parentModule =
              state->getParentOfType<mlir::ModuleOp>()) {
        auto arcSymRef = state.getArcAttr().getRootReference();
        if (auto op = parentModule.lookupSymbol(arcSymRef))
          arcDef = dyn_cast<arc::DefineOp>(op);
      }

      bool styleDetermined = false;
      aggVar.updateStyle =
          classifyAggregateUpdateStyle(arcDef, numElements, styleDetermined);
      aggVar.updateStyleDetermined = styleDetermined;

      if (auto initAttr = state->getAttrOfType<IntegerAttr>("initial_value")) {
        aggVar.hasConstantInit = true;
        llvm::SmallString<32> buf;
        initAttr.getValue().toStringUnsigned(buf);
        aggVar.initValue = std::string(buf);
      }

      if (auto enableVal = state.getEnable()) {
        aggVar.hasEnable = true;
        aggVar.enableRef = buildOpaqueRef(enableVal);
      }
      if (auto resetVal = state.getReset()) {
        aggVar.hasReset = true;
        aggVar.resetRef = buildOpaqueRef(resetVal);
      }
      addUniqueClock(model.clockDomains, clockDomain);
      statefulEntityNames[state.getOperation()] = aggVar.stableName;
      model.aggregateStateVars.push_back(std::move(aggVar));
      return;
    }

    StateVar stateVar;
    stateVar.stableName = getStateName(state, nextStateIndex++);
    stateVar.arcName = state.getArcAttr().getRootReference().getValue().str();
    stateVar.clockDomain = clockDomain;
    stateVar.width = getResultWidth(state.getOutputs());
    stateVar.stateOpIndex = currentWalkIndex;
    if (auto enableVal = state.getEnable()) {
      stateVar.hasEnable = true;
      stateVar.enableRef = buildOpaqueRef(enableVal);
    }
    if (auto resetVal = state.getReset()) {
      stateVar.hasReset = true;
      stateVar.resetRef = buildOpaqueRef(resetVal);
    }

    if (auto initAttr = state->getAttrOfType<IntegerAttr>("initial_value")) {
      stateVar.hasConstantInit = true;
      llvm::SmallString<32> buf;
      initAttr.getValue().toStringUnsigned(buf);
      stateVar.initValue = std::string(buf);
    } else if (!state.getInitials().empty()) {
      stateVar.hasNonConstantInit = true;
    }

    addUniqueClock(model.clockDomains, clockDomain);
    statefulEntityNames[state.getOperation()] = stateVar.stableName;
    model.stateVars.push_back(std::move(stateVar));
  });

  DenseMap<Value, unsigned> memoryIndices;
  unsigned nextMemoryIndex = 0;
  hwModule.walk([&](arc::MemoryOp memory) {
    auto memoryType = dyn_cast<arc::MemoryType>(memory.getMemory().getType());
    if (!memoryType)
      return;

    MemoryVar memoryVar;
    memoryVar.stableName =
        normalizeIdentifier("memory_" + llvm::utostr(nextMemoryIndex));
    memoryVar.depth = memoryType.getNumWords();
    memoryVar.elementWidth = memoryType.getWordType().getWidth();
    memoryVar.addressWidth = getTypeWidth(memoryType.getAddressType());
    memoryIndices[memory.getMemory()] = nextMemoryIndex;
    statefulOps.insert(memory.getOperation());
    statefulEntityNames[memory.getOperation()] = memoryVar.stableName;
    nextMemoryIndex++;
    model.memoryVars.push_back(std::move(memoryVar));
  });

  hwModule.walk([&](arc::MemoryReadPortOp readPort) {
    auto it = memoryIndices.find(readPort.getMemory());
    if (it == memoryIndices.end())
      return;
    auto &memoryVar = model.memoryVars[it->second];
    memoryVar.hasReadPort = true;
    memoryVar.readPortCount++;
    statefulOps.insert(readPort.getOperation());
    statefulEntityNames[readPort.getOperation()] = memoryVar.stableName;
  });

  hwModule.walk([&](arc::MemoryWritePortOp writePort) {
    auto it = memoryIndices.find(writePort.getMemory());
    if (it == memoryIndices.end())
      return;
    auto &memoryVar = model.memoryVars[it->second];
    memoryVar.hasWritePort = true;
    memoryVar.writePortCount++;
    memoryVar.clockDomain =
        traceClockName(writePort.getClock(), model.inputPorts, hwModule);
    addUniqueClock(model.clockDomains, memoryVar.clockDomain);
    statefulOps.insert(writePort.getOperation());
  });

  // seq.firmem (Seq dialect) — same contract as arc.memory but pre-lowering
  hwModule.walk([&](seq::FirMemOp firMem) {
    auto firMemType = firMem.getType();
    MemoryVar memoryVar;
    memoryVar.stableName =
        normalizeIdentifier("memory_" + llvm::utostr(nextMemoryIndex));
    memoryVar.depth = firMemType.getDepth();
    memoryVar.elementWidth = firMemType.getWidth();
    memoryVar.addressWidth =
        llvm::Log2_64_Ceil(std::max<uint64_t>(firMemType.getDepth(), 1));
    memoryIndices[firMem.getResult()] = nextMemoryIndex;
    statefulOps.insert(firMem.getOperation());
    statefulEntityNames[firMem.getOperation()] = memoryVar.stableName;
    nextMemoryIndex++;
    model.memoryVars.push_back(std::move(memoryVar));
  });

  hwModule.walk([&](seq::FirMemReadOp readPort) {
    auto it = memoryIndices.find(readPort.getMemory());
    if (it == memoryIndices.end())
      return;
    auto &memoryVar = model.memoryVars[it->second];
    memoryVar.hasReadPort = true;
    memoryVar.readPortCount++;
    if (readPort.getClk()) {
      memoryVar.clockDomain =
          traceClockName(readPort.getClk(), model.inputPorts, hwModule);
      addUniqueClock(model.clockDomains, memoryVar.clockDomain);
    }
    statefulOps.insert(readPort.getOperation());
    statefulEntityNames[readPort.getOperation()] = memoryVar.stableName;
  });

  hwModule.walk([&](seq::FirMemWriteOp writePort) {
    auto it = memoryIndices.find(writePort.getMemory());
    if (it == memoryIndices.end())
      return;
    auto &memoryVar = model.memoryVars[it->second];
    memoryVar.hasWritePort = true;
    memoryVar.writePortCount++;
    memoryVar.clockDomain =
        traceClockName(writePort.getClk(), model.inputPorts, hwModule);
    addUniqueClock(model.clockDomains, memoryVar.clockDomain);
    statefulOps.insert(writePort.getOperation());
  });

  auto touchesStateful = [&](Value v) -> bool {
    llvm::SmallVector<Value, 16> worklist;
    llvm::DenseSet<Value> visited;
    worklist.push_back(v);
    while (!worklist.empty()) {
      Value cur = worklist.pop_back_val();
      if (!visited.insert(cur).second)
        continue;
      Operation *def = cur.getDefiningOp();
      if (!def)
        continue;
      if (statefulOps.contains(def))
        return true;
      for (Value operand : def->getOperands())
        worklist.push_back(operand);
    }
    return false;
  };

  auto isDirectStateful = [&](Value v) -> bool {
    Operation *def = v.getDefiningOp();
    return def && statefulOps.contains(def);
  };

  if (auto outputOp =
          dyn_cast<hw::OutputOp>(hwModule.getBodyBlock()->getTerminator())) {
    for (auto [index, operand] : llvm::enumerate(outputOp.getOperands())) {
      OutputBinding binding;
      binding.name = index < model.outputPorts.size()
                         ? model.outputPorts[index].name
                         : normalizeIdentifier("out_" + llvm::utostr(index));

      if (isDirectStateful(operand)) {
        Operation *def = operand.getDefiningOp();
        if (isa<arc::StateOp>(def)) {
          binding.sourceKind = "state";
          binding.visibility = OutputVisibility::Edge;
        } else if (isa<arc::MemoryReadPortOp>(def) ||
                   isa<seq::FirMemReadOp>(def)) {
          binding.sourceKind = "memory_read";
          binding.visibility = OutputVisibility::Edge;
        } else {
          binding.sourceKind = "stateful";
          binding.visibility = OutputVisibility::Edge;
        }
        auto nameIt = statefulEntityNames.find(def);
        if (nameIt != statefulEntityNames.end())
          binding.sourceEntity = nameIt->second;
      } else if (touchesStateful(operand)) {
        binding.sourceKind = operand.getDefiningOp()
                                 ? operand.getDefiningOp()
                                       ->getName()
                                       .getStringRef()
                                       .str()
                                 : "mixed";
        binding.visibility = OutputVisibility::PostEdgeComb;
      } else {
        binding.sourceKind = operand.getDefiningOp()
                                 ? operand.getDefiningOp()
                                       ->getName()
                                       .getStringRef()
                                       .str()
                                 : "input";
        binding.visibility = OutputVisibility::Comb;
      }
      model.outputs.push_back(std::move(binding));
    }
  }

  return model;
}

} // namespace hirct::semantic
