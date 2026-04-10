//===- SemanticModelTest.cpp - Semantic model unit tests --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "hirct/SemanticModel/Builder.h"
#include "hirct/SemanticModel/Validation.h"

#include "circt/Dialect/Arc/ArcDialect.h"
#include "circt/Dialect/Comb/CombDialect.h"
#include "circt/Dialect/HW/HWDialect.h"
#include "circt/Dialect/Seq/SeqDialect.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Parser/Parser.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace {

std::filesystem::path getFixtureRoot() {
  auto path = std::filesystem::path(__FILE__).parent_path();
  return path.parent_path().parent_path().parent_path() / "tests" /
         "fixtures";
}

std::string readFixture(const std::filesystem::path &path) {
  std::ifstream ifs(path);
  return {std::istreambuf_iterator<char>(ifs),
          std::istreambuf_iterator<char>()};
}

class SemanticModelFixture : public ::testing::Test {
protected:
  void SetUp() override {
    ctx_.allowUnregisteredDialects();
    ctx_.loadDialect<circt::arc::ArcDialect, circt::comb::CombDialect,
                     circt::hw::HWDialect, circt::seq::SeqDialect,
                     mlir::func::FuncDialect>();
  }

  mlir::OwningOpRef<mlir::ModuleOp> parseFixture(const char *name) {
    auto path = getFixtureRoot() / name;
    if (!std::filesystem::exists(path)) {
      ADD_FAILURE() << "missing fixture: " << path.string();
      return {};
    }
    return mlir::parseSourceString<mlir::ModuleOp>(readFixture(path), &ctx_);
  }

  mlir::OwningOpRef<mlir::ModuleOp> parseInline(llvm::StringRef mlir) {
    return mlir::parseSourceString<mlir::ModuleOp>(mlir, &ctx_);
  }

  mlir::MLIRContext ctx_;
};

// ---------------------------------------------------------------------------
// Baseline fixture tests
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, BuildsSimpleArcFixture) {
  auto module = parseFixture("simple_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "Top");
  ASSERT_TRUE(succeeded(model));
  EXPECT_EQ(model->moduleName, "Top");
  EXPECT_EQ(model->clockDomains.size(), 1u);
  EXPECT_EQ(model->stateVars.size(), 2u);
}

TEST_F(SemanticModelFixture, BuildsMemoryFixture) {
  auto module = parseFixture("memory_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "simple_mem2");
  ASSERT_TRUE(succeeded(model));
  EXPECT_EQ(model->memoryVars.size(), 1u);
  EXPECT_EQ(model->memoryVars.front().depth, 1024u);
}

TEST_F(SemanticModelFixture, ValidatesCounterFixture) {
  auto module = parseFixture("counter_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "fc_counter");
  ASSERT_TRUE(succeeded(model));
  auto errors = hirct::semantic::validateModuleModel(*model);
  EXPECT_TRUE(errors.empty());
}

// ---------------------------------------------------------------------------
// Task 2C: StateVar contract
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, StateVarEnableResetFields) {
  auto module = parseFixture("enable_reset_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "RegBank");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->stateVars.size(), 2u);

  auto &q0 = model->stateVars[0];
  EXPECT_EQ(q0.stableName, "q0");
  EXPECT_EQ(q0.width, 32u);
  EXPECT_TRUE(q0.hasEnable);
  EXPECT_FALSE(q0.hasReset);

  auto &q1 = model->stateVars[1];
  EXPECT_EQ(q1.stableName, "q1");
  EXPECT_EQ(q1.width, 32u);
  EXPECT_TRUE(q1.hasEnable);
  EXPECT_TRUE(q1.hasReset);
}

TEST_F(SemanticModelFixture, CounterStateEnableReset) {
  auto module = parseFixture("counter_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "fc_counter");
  ASSERT_TRUE(succeeded(model));

  ASSERT_GE(model->stateVars.size(), 1u);
  for (const auto &sv : model->stateVars) {
    EXPECT_TRUE(sv.hasEnable) << "state " << sv.stableName << " should have enable";
    EXPECT_FALSE(sv.clockDomain.empty())
        << "state " << sv.stableName << " should have clock domain";
  }
}

TEST_F(SemanticModelFixture, StateVarConstantInitOnly) {
  // v1: init_expr is constant-only; hasConstantInit reflects this
  auto module = parseFixture("enable_reset_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "RegBank");
  ASSERT_TRUE(succeeded(model));

  for (const auto &sv : model->stateVars) {
    // These states don't carry initial_value attrs in fixture -> no init
    EXPECT_FALSE(sv.hasConstantInit)
        << "state " << sv.stableName << " has no initial_value attr";
  }
}

// ---------------------------------------------------------------------------
// Task 2D: AggregateStateVar contract (stub - no aggregate fixture yet)
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, AggregateStateVarUpdateStyleEnum) {
  // Verify enum stringification is stable
  EXPECT_EQ(hirct::semantic::stringifyUpdateStyle(
                hirct::semantic::UpdateStyle::FullReplace),
            "full_replace");
  EXPECT_EQ(hirct::semantic::stringifyUpdateStyle(
                hirct::semantic::UpdateStyle::IndexedUpdate),
            "indexed_update");
  EXPECT_EQ(hirct::semantic::stringifyUpdateStyle(
                hirct::semantic::UpdateStyle::ElementwiseUpdate),
            "elementwise_update");
}

// ---------------------------------------------------------------------------
// Task 2E: MemoryVar contract
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, MemoryVarFieldSet) {
  auto module = parseFixture("memory_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "simple_mem2");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->memoryVars.size(), 1u);
  auto &mem = model->memoryVars.front();

  EXPECT_EQ(mem.depth, 1024u);
  EXPECT_EQ(mem.elementWidth, 32u);
  EXPECT_EQ(mem.addressWidth, 10u);
  EXPECT_TRUE(mem.hasReadPort);
  EXPECT_TRUE(mem.hasWritePort);
  EXPECT_EQ(mem.clockDomain, "clk");
  EXPECT_EQ(mem.initPolicy, hirct::semantic::InitPolicy::Uninitialized);
}

TEST_F(SemanticModelFixture, MemoryVarRejectZeroShape) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "bad_mem";
  hirct::semantic::MemoryVar mv;
  mv.stableName = "mem0";
  mv.depth = 0;
  mv.elementWidth = 0;
  mv.addressWidth = 0;
  model.memoryVars.push_back(mv);

  auto errors = hirct::semantic::validateModuleModel(model);
  ASSERT_FALSE(errors.empty());
  bool foundShapeReject = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::MemoryShapeAmbiguous)
      foundShapeReject = true;
  }
  EXPECT_TRUE(foundShapeReject);
}

// ---------------------------------------------------------------------------
// Task 2F: OutputBinding visibility
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, OutputVisibilitySimpleArc) {
  auto module = parseFixture("simple_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "Top");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->outputs.size(), 1u);
  // Top: hw.output %3 where %3 = arc.call (comb mul of two state results)
  // -> touches stateful through arc.state results but is a comb op on top
  EXPECT_EQ(model->outputs[0].visibility,
            hirct::semantic::OutputVisibility::PostEdgeComb);
}

TEST_F(SemanticModelFixture, OutputVisibilityCounterFixture) {
  auto module = parseFixture("counter_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "fc_counter");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->outputs.size(), 3u);
  // fc_counter: hw.output %0, %1, %2 — all direct arc.state results
  for (const auto &out : model->outputs) {
    EXPECT_EQ(out.visibility, hirct::semantic::OutputVisibility::Edge)
        << "output " << out.name << " is direct state -> edge";
  }
}

TEST_F(SemanticModelFixture, OutputVisibilityMemoryRead) {
  auto module = parseFixture("memory_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "simple_mem2");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->outputs.size(), 1u);
  // memory read port result -> edge
  EXPECT_EQ(model->outputs[0].visibility,
            hirct::semantic::OutputVisibility::Edge);
}

TEST_F(SemanticModelFixture, OutputVisibilityCombOnly) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @CombOnly(in %a : i8, in %b : i8, out out : i8) {
        %0 = comb.xor %a, %b : i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "CombOnly");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->outputs.size(), 1u);
  EXPECT_EQ(model->outputs[0].visibility,
            hirct::semantic::OutputVisibility::Comb);
}

TEST_F(SemanticModelFixture, OutputVisibilityEnableReset) {
  auto module = parseFixture("enable_reset_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "RegBank");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->outputs.size(), 2u);
  // Both outputs are direct arc.state results -> edge
  for (const auto &out : model->outputs) {
    EXPECT_EQ(out.visibility, hirct::semantic::OutputVisibility::Edge)
        << "output " << out.name << " should be edge";
  }
}

// ---------------------------------------------------------------------------
// F3: Enable/reset opaque reference
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, EnableResetOpaqueRef) {
  auto module = parseFixture("enable_reset_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "RegBank");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->stateVars.size(), 2u);
  auto &q0 = model->stateVars[0];
  EXPECT_TRUE(q0.hasEnable);
  EXPECT_TRUE(q0.enableRef.has_value()) << "enable ref must be populated";
  EXPECT_FALSE(q0.hasReset);
  EXPECT_FALSE(q0.resetRef.has_value());

  auto &q1 = model->stateVars[1];
  EXPECT_TRUE(q1.hasEnable);
  EXPECT_TRUE(q1.enableRef.has_value());
  EXPECT_TRUE(q1.hasReset);
  EXPECT_TRUE(q1.resetRef.has_value());
}

TEST_F(SemanticModelFixture, InitResetConflictReject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "conflict";
  hirct::semantic::StateVar sv;
  sv.stableName = "reg0";
  sv.clockDomain = "clk";
  sv.width = 8;
  sv.hasConstantInit = true;
  sv.initValue = "42";
  sv.hasReset = true;
  model.stateVars.push_back(sv);

  auto errors = hirct::semantic::validateModuleModel(model);
  bool found = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::InitResetConflict)
      found = true;
  }
  EXPECT_TRUE(found) << "must reject init+reset conflict";
}

TEST_F(SemanticModelFixture, NonConstantInitReject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "ncinit";
  hirct::semantic::StateVar sv;
  sv.stableName = "reg0";
  sv.clockDomain = "clk";
  sv.width = 8;
  sv.hasNonConstantInit = true;
  model.stateVars.push_back(sv);

  auto errors = hirct::semantic::validateModuleModel(model);
  bool found = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::NonConstantInit)
      found = true;
  }
  EXPECT_TRUE(found) << "must reject non-constant init";
}

// ---------------------------------------------------------------------------
// F5: Memory multi-port reject
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, MemoryMultiPortReject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "multi_port";
  hirct::semantic::MemoryVar mv;
  mv.stableName = "mem0";
  mv.clockDomain = "clk";
  mv.depth = 1024;
  mv.elementWidth = 32;
  mv.addressWidth = 10;
  mv.hasReadPort = true;
  mv.hasWritePort = true;
  mv.readPortCount = 2;
  mv.writePortCount = 1;
  model.memoryVars.push_back(mv);

  auto errors = hirct::semantic::validateModuleModel(model);
  bool found = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::MemoryPortUnsupported)
      found = true;
  }
  EXPECT_TRUE(found) << "must reject multi-port memory";
}

TEST_F(SemanticModelFixture, MemoryMultiWritePortReject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "multi_write";
  hirct::semantic::MemoryVar mv;
  mv.stableName = "mem0";
  mv.clockDomain = "clk";
  mv.depth = 256;
  mv.elementWidth = 16;
  mv.addressWidth = 8;
  mv.hasReadPort = true;
  mv.hasWritePort = true;
  mv.readPortCount = 1;
  mv.writePortCount = 2;
  model.memoryVars.push_back(mv);

  auto errors = hirct::semantic::validateModuleModel(model);
  bool found = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::MemoryPortUnsupported)
      found = true;
  }
  EXPECT_TRUE(found) << "must reject multi-write-port memory";
}

// ---------------------------------------------------------------------------
// F6: Output source entity + ambiguous decomposition reject
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, OutputSourceEntity) {
  auto module = parseFixture("enable_reset_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "RegBank");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->outputs.size(), 2u);
  EXPECT_EQ(model->outputs[0].sourceEntity, "q0");
  EXPECT_EQ(model->outputs[1].sourceEntity, "q1");
}

TEST_F(SemanticModelFixture, OutputMemorySourceEntity) {
  auto module = parseFixture("memory_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "simple_mem2");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->outputs.size(), 1u);
  EXPECT_EQ(model->outputs[0].sourceEntity, "memory_0");
}

TEST_F(SemanticModelFixture, OutputDecompAmbiguousReject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "ambiguous";
  hirct::semantic::OutputBinding ob;
  ob.name = "out0";
  ob.visibility = hirct::semantic::OutputVisibility::Edge;
  ob.sourceEntity = "";
  model.outputs.push_back(ob);

  auto errors = hirct::semantic::validateModuleModel(model);
  bool found = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::OutputDecompAmbiguous)
      found = true;
  }
  EXPECT_TRUE(found) << "must reject edge output with no source entity";
}

// ---------------------------------------------------------------------------
// F4: AggregateStateVar reject
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, AggregateStateVarBuilderClassifiesArrayState) {
  auto module = parseFixture("aggregate_state_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "AggMod");
  ASSERT_TRUE(succeeded(model));

  EXPECT_EQ(model->stateVars.size(), 0u)
      << "array-typed state should not be in stateVars";
  ASSERT_EQ(model->aggregateStateVars.size(), 1u);

  auto &agg = model->aggregateStateVars[0];
  EXPECT_EQ(agg.stableName, "arr_reg");
  EXPECT_EQ(agg.numElements, 4u);
  EXPECT_EQ(agg.elementWidth, 8u);
  EXPECT_EQ(agg.width, 32u);
  EXPECT_EQ(agg.updateStyle, hirct::semantic::UpdateStyle::FullReplace);
  EXPECT_EQ(agg.clockDomain, "clock");
}

TEST_F(SemanticModelFixture, AggregateUpdateAmbiguousReject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "agg_bad";
  hirct::semantic::AggregateStateVar av;
  av.stableName = "agg0";
  av.clockDomain = "clk";
  av.width = 32;
  av.numElements = 0;
  av.elementWidth = 0;
  model.aggregateStateVars.push_back(av);

  auto errors = hirct::semantic::validateModuleModel(model);
  bool found = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::AggregateUpdateAmbiguous)
      found = true;
  }
  EXPECT_TRUE(found) << "must reject aggregate with ambiguous shape";
}

// ---------------------------------------------------------------------------
// F1: Naming normalization
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, NormalizeIdentifierReplacesInvalidChars) {
  EXPECT_EQ(hirct::semantic::normalizeIdentifier("hello-world.v2"),
            "hello_world_v2");
  EXPECT_EQ(hirct::semantic::normalizeIdentifier("123start"), "_123start");
  EXPECT_EQ(hirct::semantic::normalizeIdentifier(""), "_empty");
  EXPECT_EQ(hirct::semantic::normalizeIdentifier("ok_name"), "ok_name");
  EXPECT_EQ(hirct::semantic::normalizeIdentifier("a.b-c:d"), "a_b_c_d");
}

TEST_F(SemanticModelFixture, IsValidCIdentifier) {
  EXPECT_TRUE(hirct::semantic::isValidCIdentifier("foo"));
  EXPECT_TRUE(hirct::semantic::isValidCIdentifier("_bar"));
  EXPECT_TRUE(hirct::semantic::isValidCIdentifier("x123"));
  EXPECT_FALSE(hirct::semantic::isValidCIdentifier(""));
  EXPECT_FALSE(hirct::semantic::isValidCIdentifier("123abc"));
  EXPECT_FALSE(hirct::semantic::isValidCIdentifier("a-b"));
  EXPECT_FALSE(hirct::semantic::isValidCIdentifier("a.b"));
}

TEST_F(SemanticModelFixture, InvalidIdentifierReject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "my-module.v2";
  hirct::semantic::PortInfo port;
  port.name = "data-in";
  port.isInput = true;
  port.width = 8;
  model.inputPorts.push_back(port);

  auto errors = hirct::semantic::validateModuleModel(model);
  unsigned count = 0;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::InvalidIdentifier)
      ++count;
  }
  EXPECT_GE(count, 2u)
      << "must reject invalid module name and invalid port name";
}

TEST_F(SemanticModelFixture, DuplicatePortNameReject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "DupMod";
  hirct::semantic::PortInfo p1;
  p1.name = "data";
  p1.isInput = true;
  p1.width = 8;
  model.inputPorts.push_back(p1);
  hirct::semantic::PortInfo p2;
  p2.name = "data";
  p2.isInput = false;
  p2.width = 8;
  model.outputPorts.push_back(p2);

  auto errors = hirct::semantic::validateModuleModel(model);
  bool found = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::InvalidIdentifier &&
        e.message.find("duplicate") != std::string::npos)
      found = true;
  }
  EXPECT_TRUE(found) << "must reject duplicate port names";
}

// ---------------------------------------------------------------------------
// F2: >64-bit width reject
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, WidthOver64Reject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "WideMod";
  hirct::semantic::StateVar sv;
  sv.stableName = "wide_reg";
  sv.clockDomain = "clk";
  sv.width = 128;
  model.stateVars.push_back(sv);

  auto errors = hirct::semantic::validateModuleModel(model);
  bool found = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::WidthUnsupported)
      found = true;
  }
  EXPECT_TRUE(found) << "must reject >64-bit width";
}

TEST_F(SemanticModelFixture, WidthOver64PortReject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "WidePort";
  hirct::semantic::PortInfo port;
  port.name = "bigdata";
  port.isInput = true;
  port.width = 65;
  model.inputPorts.push_back(port);

  auto errors = hirct::semantic::validateModuleModel(model);
  bool found = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::WidthUnsupported)
      found = true;
  }
  EXPECT_TRUE(found) << "must reject >64-bit port width";
}

TEST_F(SemanticModelFixture, Width64PassesValidation) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "Exact64";
  hirct::semantic::StateVar sv;
  sv.stableName = "reg64";
  sv.clockDomain = "clk";
  sv.width = 64;
  model.stateVars.push_back(sv);

  auto errors = hirct::semantic::validateModuleModel(model);
  for (const auto &e : errors) {
    EXPECT_NE(e.code, hirct::semantic::RejectCode::WidthUnsupported)
        << "64-bit should pass";
  }
}

// ---------------------------------------------------------------------------
// F3: Aggregate update-style strict reject
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, AggregateFullReplaceIsDefault) {
  auto module = parseFixture("aggregate_state_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "AggMod");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->aggregateStateVars.size(), 1u);
  EXPECT_EQ(model->aggregateStateVars[0].updateStyle,
            hirct::semantic::UpdateStyle::FullReplace);
}

// ---------------------------------------------------------------------------
// F4: OpaqueExprRef identity
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, OpaqueRefHasStrongIdentity) {
  auto module = parseFixture("enable_reset_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "RegBank");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->stateVars.size(), 2u);
  auto &q0 = model->stateVars[0];
  auto &q1 = model->stateVars[1];

  ASSERT_TRUE(q0.enableRef.has_value());
  ASSERT_TRUE(q1.enableRef.has_value());

  auto &ref0 = *q0.enableRef;
  auto &ref1 = *q1.enableRef;

  EXPECT_TRUE(ref0.isBlockArg()) << "enable is a module input (block arg)";
  EXPECT_TRUE(ref1.isBlockArg());
  EXPECT_EQ(ref0.blockArgNumber, ref1.blockArgNumber)
      << "both share the same enable signal";

  ASSERT_TRUE(q1.resetRef.has_value());
  auto &rstRef = *q1.resetRef;
  EXPECT_TRUE(rstRef.isBlockArg());
  EXPECT_NE(rstRef.blockArgNumber, ref1.blockArgNumber)
      << "reset and enable are different signals";
}

TEST_F(SemanticModelFixture, OpaqueRefDistinguishesSameOpType) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @Id8(%arg0: i8) -> i8 {
        arc.output %arg0 : i8
      }
      hw.module @TwoState(in %clock : !seq.clock, in %a : i8, in %b : i8,
                           out q0 : i8, out q1 : i8) {
        %q0 = arc.state @Id8(%a) clock %clock latency 1 {names = ["r0"]} : (i8) -> i8
        %q1 = arc.state @Id8(%b) clock %clock latency 1 {names = ["r1"]} : (i8) -> i8
        hw.output %q0, %q1 : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "TwoState");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->outputs.size(), 2u);
  EXPECT_EQ(model->outputs[0].sourceEntity, "r0");
  EXPECT_EQ(model->outputs[1].sourceEntity, "r1");
  EXPECT_NE(model->outputs[0].sourceEntity, model->outputs[1].sourceEntity)
      << "same op-type must have distinct source entity";
}

TEST_F(SemanticModelFixture, AggregateOpaqueRef) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @AggArcEn(%arg0: !hw.array<4xi8>) -> !hw.array<4xi8> {
        arc.output %arg0 : !hw.array<4xi8>
      }
      hw.module @AggEnMod(in %clock : !seq.clock, in %d : !hw.array<4xi8>,
                           in %en : i1, out q : !hw.array<4xi8>) {
        %q = arc.state @AggArcEn(%d) clock %clock enable %en latency 1 {names = ["agg_reg"]} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        hw.output %q : !hw.array<4xi8>
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "AggEnMod");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->aggregateStateVars.size(), 1u);
  auto &agg = model->aggregateStateVars[0];
  EXPECT_TRUE(agg.hasEnable);
  ASSERT_TRUE(agg.enableRef.has_value());
  EXPECT_TRUE(agg.enableRef->isBlockArg());
}

// ---------------------------------------------------------------------------
// Validation reject code stringification
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, RejectCodeStringification) {
  using RC = hirct::semantic::RejectCode;
  EXPECT_EQ(hirct::semantic::stringifyRejectCode(RC::InitResetConflict),
            "INIT_RESET_CONFLICT");
  EXPECT_EQ(hirct::semantic::stringifyRejectCode(RC::NonConstantInit),
            "NON_CONSTANT_INIT");
  EXPECT_EQ(hirct::semantic::stringifyRejectCode(RC::AggregateUpdateAmbiguous),
            "AGGREGATE_UPDATE_AMBIGUOUS");
  EXPECT_EQ(hirct::semantic::stringifyRejectCode(RC::MemoryPortUnsupported),
            "MEMORY_PORT_UNSUPPORTED");
  EXPECT_EQ(hirct::semantic::stringifyRejectCode(RC::OutputDecompAmbiguous),
            "OUTPUT_DECOMP_AMBIGUOUS");
  EXPECT_EQ(hirct::semantic::stringifyRejectCode(RC::InvalidIdentifier),
            "INVALID_IDENTIFIER");
  EXPECT_EQ(hirct::semantic::stringifyRejectCode(RC::WidthUnsupported),
            "WIDTH_UNSUPPORTED");
}

// ---------------------------------------------------------------------------
// Init policy stringification
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, InitPolicyStringification) {
  using IP = hirct::semantic::InitPolicy;
  EXPECT_EQ(hirct::semantic::stringifyInitPolicy(IP::Zero), "zero");
  EXPECT_EQ(hirct::semantic::stringifyInitPolicy(IP::Uninitialized),
            "uninitialized");
  EXPECT_EQ(hirct::semantic::stringifyInitPolicy(IP::ImplementationDefined),
            "implementation_defined");
}

// ---------------------------------------------------------------------------
// Batch3 F2: Builder produces normalized names (source-of-truth)
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, BuilderNormalizesNames) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @Id8(%arg0: i8) -> i8 {
        arc.output %arg0 : i8
      }
      hw.module @my_mod_v2(in %clk_in : !seq.clock, in %data_in : i8,
                            out data_out : i8) {
        %q = arc.state @Id8(%data_in) clock %clk_in latency 1 {names = ["reg-0"]} : (i8) -> i8
        hw.output %q : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "my_mod_v2");
  ASSERT_TRUE(succeeded(model));

  EXPECT_EQ(model->moduleName, "my_mod_v2")
      << "valid module name stays as-is";
  ASSERT_EQ(model->stateVars.size(), 1u);
  EXPECT_EQ(model->stateVars[0].stableName, "reg_0")
      << "builder must normalize state name with hyphen";
}

TEST_F(SemanticModelFixture, BuilderNormalizesPortNames) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @NormPorts(in %data_in : i8, in %rst_n : i1, out data_out : i8) {
        hw.output %data_in : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "NormPorts");
  ASSERT_TRUE(succeeded(model));

  for (const auto &port : model->inputPorts) {
    EXPECT_TRUE(hirct::semantic::isValidCIdentifier(port.name))
        << "builder must produce valid C identifiers for ports: " << port.name;
  }
  for (const auto &port : model->outputPorts) {
    EXPECT_TRUE(hirct::semantic::isValidCIdentifier(port.name))
        << "builder must produce valid C identifiers for ports: " << port.name;
  }
}

// ---------------------------------------------------------------------------
// Batch3 F3: Normalized collision detection
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, NormalizedCollisionReject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "CollisionMod";
  hirct::semantic::PortInfo p1;
  p1.name = "a_b";
  p1.isInput = true;
  p1.width = 8;
  model.inputPorts.push_back(p1);
  hirct::semantic::PortInfo p2;
  p2.name = "a_b";
  p2.isInput = false;
  p2.width = 8;
  model.outputPorts.push_back(p2);

  auto errors = hirct::semantic::validateModuleModel(model);
  bool foundCollision = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::InvalidIdentifier &&
        e.message.find("collides") != std::string::npos)
      foundCollision = true;
  }
  EXPECT_TRUE(foundCollision) << "must detect normalized name collision";
}

TEST_F(SemanticModelFixture, NormalizedCollisionStateField) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "CrossMod";
  hirct::semantic::StateVar sv1;
  sv1.stableName = "x_y";
  sv1.clockDomain = "clk";
  sv1.width = 8;
  model.stateVars.push_back(sv1);
  hirct::semantic::StateVar sv2;
  sv2.stableName = "x_y";
  sv2.clockDomain = "clk";
  sv2.width = 8;
  model.stateVars.push_back(sv2);
  model.clockDomains.push_back("clk");

  auto errors = hirct::semantic::validateModuleModel(model);
  bool foundCollision = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::InvalidIdentifier &&
        e.message.find("collides") != std::string::npos)
      foundCollision = true;
  }
  EXPECT_TRUE(foundCollision) << "must detect struct field collision";
}

TEST_F(SemanticModelFixture, PortStateNoFalseCollision) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "NoFalse";
  hirct::semantic::PortInfo p1;
  p1.name = "data";
  p1.isInput = true;
  p1.width = 8;
  model.inputPorts.push_back(p1);
  hirct::semantic::StateVar sv;
  sv.stableName = "data";
  sv.clockDomain = "clk";
  sv.width = 8;
  model.stateVars.push_back(sv);
  model.clockDomains.push_back("clk");

  auto errors = hirct::semantic::validateModuleModel(model);
  bool foundCollision = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::InvalidIdentifier &&
        e.message.find("collides") != std::string::npos)
      foundCollision = true;
  }
  EXPECT_FALSE(foundCollision)
      << "port and state are in different namespaces, no collision";
}

// ---------------------------------------------------------------------------
// Batch3 F4: Clock domain naming validation
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, ClockInvalidIdentifierReject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "ClkMod";
  hirct::semantic::StateVar sv;
  sv.stableName = "reg0";
  sv.clockDomain = "clk-fast";
  sv.width = 8;
  model.stateVars.push_back(sv);
  model.clockDomains.push_back("clk-fast");

  auto errors = hirct::semantic::validateModuleModel(model);
  bool found = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::InvalidIdentifier &&
        e.message.find("clock") != std::string::npos)
      found = true;
  }
  EXPECT_TRUE(found) << "must reject invalid clock domain identifier";
}

TEST_F(SemanticModelFixture, ClockCollisionReject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "MultiClk";
  model.clockDomains.push_back("clk_a");
  model.clockDomains.push_back("clk_a");

  auto errors = hirct::semantic::validateModuleModel(model);
  bool found = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::InvalidIdentifier &&
        e.message.find("clock") != std::string::npos &&
        e.message.find("collides") != std::string::npos)
      found = true;
  }
  EXPECT_TRUE(found) << "must reject clock domain collision";
}

// ---------------------------------------------------------------------------
// Batch3 F5: Aggregate update-style undetermined reject
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, AggregateUndeterminedStyleReject) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "AggUnd";
  hirct::semantic::AggregateStateVar av;
  av.stableName = "agg0";
  av.clockDomain = "clk";
  av.width = 32;
  av.numElements = 4;
  av.elementWidth = 8;
  av.updateStyleDetermined = false;
  model.aggregateStateVars.push_back(av);
  model.clockDomains.push_back("clk");

  auto errors = hirct::semantic::validateModuleModel(model);
  bool found = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::AggregateUpdateAmbiguous &&
        e.message.find("undetermined") != std::string::npos)
      found = true;
  }
  EXPECT_TRUE(found) << "must reject aggregate with undetermined update style";
}

TEST_F(SemanticModelFixture, AggregateDeterminedStylePasses) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "AggDet";
  hirct::semantic::AggregateStateVar av;
  av.stableName = "agg0";
  av.clockDomain = "clk";
  av.width = 32;
  av.numElements = 4;
  av.elementWidth = 8;
  av.updateStyleDetermined = true;
  av.updateStyle = hirct::semantic::UpdateStyle::FullReplace;
  model.aggregateStateVars.push_back(av);
  model.clockDomains.push_back("clk");

  auto errors = hirct::semantic::validateModuleModel(model);
  for (const auto &e : errors) {
    EXPECT_NE(e.code, hirct::semantic::RejectCode::AggregateUpdateAmbiguous)
        << "determined aggregate should not trigger ambiguous reject: "
        << e.message;
  }
}

// ---------------------------------------------------------------------------
// Batch4 F1: Aggregate update-style deterministic classification
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, AggregateFullReplaceClassification) {
  auto module = parseFixture("aggregate_state_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "AggMod");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->aggregateStateVars.size(), 1u);
  auto &agg = model->aggregateStateVars[0];
  EXPECT_EQ(agg.stableName, "arr_reg");
  EXPECT_EQ(agg.updateStyle, hirct::semantic::UpdateStyle::FullReplace);
  EXPECT_TRUE(agg.updateStyleDetermined);
}

TEST_F(SemanticModelFixture, AggregateIndexedUpdateClassification) {
  auto module = parseFixture("aggregate_indexed_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "IdxMod");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->aggregateStateVars.size(), 1u);
  auto &agg = model->aggregateStateVars[0];
  EXPECT_EQ(agg.stableName, "idx_reg");
  EXPECT_EQ(agg.updateStyle, hirct::semantic::UpdateStyle::IndexedUpdate)
      << "indexed select+inject pattern must classify as IndexedUpdate";
  EXPECT_TRUE(agg.updateStyleDetermined);
}

TEST_F(SemanticModelFixture, AggregateElementwiseUpdateClassification) {
  auto module = parseFixture("aggregate_elementwise_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "EwMod");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->aggregateStateVars.size(), 1u);
  auto &agg = model->aggregateStateVars[0];
  EXPECT_EQ(agg.stableName, "ew_reg");
  EXPECT_EQ(agg.updateStyle, hirct::semantic::UpdateStyle::ElementwiseUpdate)
      << "uniform element transform must classify as ElementwiseUpdate";
  EXPECT_TRUE(agg.updateStyleDetermined);
}

TEST_F(SemanticModelFixture, AggregateAmbiguousClassificationReject) {
  auto module = parseFixture("aggregate_ambiguous_arc.mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "AmbigMod");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->aggregateStateVars.size(), 1u);
  auto &agg = model->aggregateStateVars[0];
  EXPECT_EQ(agg.stableName, "ambig_reg");
  EXPECT_FALSE(agg.updateStyleDetermined)
      << "mixed pattern must not be determined";

  auto errors = hirct::semantic::validateModuleModel(*model);
  bool foundAmbiguous = false;
  for (const auto &e : errors) {
    if (e.code == hirct::semantic::RejectCode::AggregateUpdateAmbiguous &&
        e.message.find("undetermined") != std::string::npos)
      foundAmbiguous = true;
  }
  EXPECT_TRUE(foundAmbiguous)
      << "ambiguous aggregate must produce AGGREGATE_UPDATE_AMBIGUOUS reject";
}

// ---------------------------------------------------------------------------
// Batch4 F4: Multi-clock semantic fixture
// ---------------------------------------------------------------------------

TEST_F(SemanticModelFixture, MultiClockDomains) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @Id8(%arg0: i8) -> i8 {
        arc.output %arg0 : i8
      }
      hw.module @DualClk(in %clk_a : !seq.clock, in %clk_b : !seq.clock,
                          in %d : i8, out qa : i8, out qb : i8) {
        %qa = arc.state @Id8(%d) clock %clk_a latency 1 {names = ["reg_a"]} : (i8) -> i8
        %qb = arc.state @Id8(%d) clock %clk_b latency 1 {names = ["reg_b"]} : (i8) -> i8
        hw.output %qa, %qb : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "DualClk");
  ASSERT_TRUE(succeeded(model));

  EXPECT_EQ(model->clockDomains.size(), 2u)
      << "two distinct clocks -> two domains";
  EXPECT_EQ(model->stateVars.size(), 2u);
  EXPECT_EQ(model->stateVars[0].clockDomain, "clk_a");
  EXPECT_EQ(model->stateVars[1].clockDomain, "clk_b");
  EXPECT_NE(model->stateVars[0].clockDomain, model->stateVars[1].clockDomain);
}

TEST_F(SemanticModelFixture, MultiClockEvalNaming) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "DualClk";
  hirct::semantic::StateVar sv1;
  sv1.stableName = "reg_a";
  sv1.clockDomain = "clk_a";
  sv1.width = 8;
  model.stateVars.push_back(sv1);
  hirct::semantic::StateVar sv2;
  sv2.stableName = "reg_b";
  sv2.clockDomain = "clk_b";
  sv2.width = 8;
  model.stateVars.push_back(sv2);
  model.clockDomains.push_back("clk_a");
  model.clockDomains.push_back("clk_b");

  auto errors = hirct::semantic::validateModuleModel(model);
  EXPECT_TRUE(errors.empty()) << "dual-clock with valid names should pass";
}

TEST_F(SemanticModelFixture, MultiClockOutputVisibility) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @Id8(%arg0: i8) -> i8 {
        arc.output %arg0 : i8
      }
      hw.module @DualOut(in %clk_a : !seq.clock, in %clk_b : !seq.clock,
                          in %d : i8, out qa : i8, out qb : i8) {
        %qa = arc.state @Id8(%d) clock %clk_a latency 1 {names = ["reg_a"]} : (i8) -> i8
        %qb = arc.state @Id8(%d) clock %clk_b latency 1 {names = ["reg_b"]} : (i8) -> i8
        hw.output %qa, %qb : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "DualOut");
  ASSERT_TRUE(succeeded(model));

  ASSERT_EQ(model->outputs.size(), 2u);
  EXPECT_EQ(model->outputs[0].visibility,
            hirct::semantic::OutputVisibility::Edge);
  EXPECT_EQ(model->outputs[0].sourceEntity, "reg_a");
  EXPECT_EQ(model->outputs[1].visibility,
            hirct::semantic::OutputVisibility::Edge);
  EXPECT_EQ(model->outputs[1].sourceEntity, "reg_b");
}

} // namespace
