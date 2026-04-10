//===- main.cpp - Semantic model dumper -------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file
/// This file implements the hirct-semantic tool.
///
//===----------------------------------------------------------------------===//

#include "hirct/SemanticModel/Builder.h"
#include "hirct/SemanticModel/Validation.h"

#include "circt/Dialect/Arc/ArcDialect.h"
#include "circt/Dialect/Comb/CombDialect.h"
#include "circt/Dialect/HW/HWDialect.h"
#include "circt/Dialect/Seq/SeqDialect.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

static llvm::cl::OptionCategory mainCategory("hirct-semantic options");

static llvm::cl::opt<std::string>
    inputFilename(llvm::cl::Positional, llvm::cl::desc("<input file>"),
                  llvm::cl::Required, llvm::cl::cat(mainCategory));

static llvm::cl::opt<std::string>
    moduleName("module", llvm::cl::desc("Target hw.module name"),
               llvm::cl::Required, llvm::cl::cat(mainCategory));

int main(int argc, char **argv) {
  llvm::InitLLVM y(argc, argv);
  llvm::cl::HideUnrelatedOptions(mainCategory);
  llvm::cl::ParseCommandLineOptions(argc, argv);

  MLIRContext context;
  context.allowUnregisteredDialects();
  context.loadDialect<circt::arc::ArcDialect, circt::comb::CombDialect,
                      circt::hw::HWDialect, circt::seq::SeqDialect,
                      mlir::func::FuncDialect>();

  llvm::SourceMgr sourceMgr;
  auto file = llvm::MemoryBuffer::getFileOrSTDIN(inputFilename);
  if (!file) {
    llvm::errs() << "failed to read input: " << inputFilename << "\n";
    return 1;
  }
  sourceMgr.AddNewSourceBuffer(std::move(*file), llvm::SMLoc());

  OwningOpRef<ModuleOp> module = parseSourceFile<ModuleOp>(sourceMgr, &context);
  if (!module)
    return 1;

  auto model = hirct::semantic::buildModuleModel(*module, moduleName);
  if (failed(model))
    return 1;

  auto errors = hirct::semantic::validateModuleModel(*model);
  if (!errors.empty()) {
    for (const auto &error : errors)
      llvm::errs() << hirct::semantic::stringifyRejectCode(error.code) << ": "
                   << error.message << "\n";
    return 1;
  }

  llvm::outs() << "module: " << model->moduleName << "\n";

  for (const auto &port : model->inputPorts)
    llvm::outs() << "input: " << port.name << " width=" << port.width << "\n";

  for (const auto &port : model->outputPorts)
    llvm::outs() << "output_port: " << port.name << " width=" << port.width << "\n";

  llvm::outs() << "clocks:";
  for (const auto &clock : model->clockDomains)
    llvm::outs() << " " << clock;
  llvm::outs() << "\n";

  for (const auto &stateVar : model->stateVars) {
    llvm::outs() << "state: " << stateVar.stableName
                 << " width=" << stateVar.width
                 << " clock=" << stateVar.clockDomain
                 << " enable=" << (stateVar.hasEnable ? "yes" : "no")
                 << " reset=" << (stateVar.hasReset ? "yes" : "no");
    if (stateVar.hasConstantInit)
      llvm::outs() << " init=" << stateVar.initValue;
    llvm::outs() << "\n";
  }

  for (const auto &aggVar : model->aggregateStateVars) {
    llvm::outs() << "aggregate_state: " << aggVar.stableName
                 << " width=" << aggVar.width
                 << " elements=" << aggVar.numElements
                 << " element_width=" << aggVar.elementWidth
                 << " clock=" << aggVar.clockDomain
                 << " update="
                 << hirct::semantic::stringifyUpdateStyle(aggVar.updateStyle)
                 << "\n";
  }

  for (const auto &output : model->outputs) {
    llvm::outs() << "output: " << output.name << " visibility="
                 << hirct::semantic::stringifyOutputVisibility(
                        output.visibility)
                 << " source=" << output.sourceKind;
    if (!output.sourceEntity.empty())
      llvm::outs() << " entity=" << output.sourceEntity;
    llvm::outs() << "\n";
  }

  for (const auto &memoryVar : model->memoryVars) {
    llvm::outs() << "memory: " << memoryVar.stableName
                 << " depth=" << memoryVar.depth
                 << " addr_width=" << memoryVar.addressWidth
                 << " elem_width=" << memoryVar.elementWidth
                 << " clock=" << memoryVar.clockDomain
                 << " read=" << (memoryVar.hasReadPort ? "yes" : "no")
                 << " write=" << (memoryVar.hasWritePort ? "yes" : "no")
                 << " init="
                 << hirct::semantic::stringifyInitPolicy(memoryVar.initPolicy)
                 << "\n";
  }

  return 0;
}
