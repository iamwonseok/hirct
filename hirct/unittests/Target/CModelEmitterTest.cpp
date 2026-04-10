//===- CModelEmitterTest.cpp - CModel emitter unit tests --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "hirct/Target/CModelEmitter.h"
#include "hirct/Target/GenModel.h"
#include "hirct/SemanticModel/Builder.h"
#include "hirct/SemanticModel/Validation.h"

#include "circt/Dialect/Arc/ArcDialect.h"
#include "circt/Dialect/Comb/CombDialect.h"
#include "circt/Dialect/HW/HWDialect.h"
#include "circt/Dialect/Seq/SeqDialect.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/StringRef.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace {

class CModelEmitterFixture : public ::testing::Test {
public:
  void SetUp() override {
    ctx_.allowUnregisteredDialects();
    ctx_.loadDialect<circt::arc::ArcDialect, circt::comb::CombDialect,
                     circt::hw::HWDialect, circt::seq::SeqDialect,
                     mlir::func::FuncDialect>();
  }

  mlir::OwningOpRef<mlir::ModuleOp> parseInline(llvm::StringRef mlir) {
    return mlir::parseSourceString<mlir::ModuleOp>(mlir, &ctx_);
  }

  hirct::semantic::ModuleModel buildCounterModel() {
    auto module = parseInline(R"mlir(
      module {
        func.func private @exit(i32)
        arc.define @fc_counter_arc(%arg0: i8, %arg1: i1, %arg2: i1) -> i8 {
          %c1_i8 = hw.constant 1 : i8
          %c0_i8 = hw.constant 0 : i8
          %0 = comb.add %arg0, %c1_i8 : i8
          %1 = comb.mux %arg1, %arg0, %0 : i8
          %2 = comb.mux %arg2, %c0_i8, %1 : i8
          arc.output %2 : i8
        }
        arc.define @clk_arc(%arg0: i1) -> !seq.clock {
          %0 = seq.to_clock %arg0
          arc.output %0 : !seq.clock
        }
        hw.module @Counter(in %clk : i1, in %rst : i1, in %en : i1,
                           out count : i8) {
          %clock = arc.call @clk_arc(%clk) : (i1) -> !seq.clock
          %0 = arc.state @fc_counter_arc(%0, %en, %rst) clock %clock latency 1 {names = ["count_reg"]} : (i8, i1, i1) -> i8
          hw.output %0 : i8
        }
      }
    )mlir");
    auto result =
        hirct::semantic::buildModuleModel(*module, "Counter");
    assert(succeeded(result));
    return *result;
  }

  mlir::MLIRContext ctx_;
};

// ---------------------------------------------------------------------------
// Task 3A: Exporter surface tests
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, EmitterAcceptsModuleModel) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  EXPECT_EQ(emitter.getModel().moduleName, "Counter");
}

TEST_F(CModelEmitterFixture, EmitProducesArtifacts) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_EQ(artifact.headerPath, "/tmp/test/Counter.h");
  EXPECT_EQ(artifact.implPath, "/tmp/test/Counter.cpp");
  EXPECT_FALSE(artifact.headerContent.empty());
  EXPECT_FALSE(artifact.implContent.empty());
}

TEST_F(CModelEmitterFixture, HeaderContainsStateStruct) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.headerContent.find("Counter_state"),
            std::string::npos)
      << "header must contain state struct";
  EXPECT_NE(artifact.headerContent.find("count_reg"), std::string::npos)
      << "state field uses stable name from semantic model";
}

TEST_F(CModelEmitterFixture, HeaderContainsInitializeFunction) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.headerContent.find("Counter_initialize"),
            std::string::npos)
      << "header must declare initialize function";
}

TEST_F(CModelEmitterFixture, HeaderContainsEvalComb) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.headerContent.find("Counter_eval_comb"),
            std::string::npos)
      << "header must declare eval_comb function";
}

TEST_F(CModelEmitterFixture, HeaderContainsEvalClock) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.headerContent.find("Counter_eval_clk"),
            std::string::npos)
      << "header must declare eval_<clock> function";
}

TEST_F(CModelEmitterFixture, HeaderContainsSetterGetter) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.headerContent.find("Counter_set_clk"),
            std::string::npos)
      << "header must declare input setters";
  EXPECT_NE(artifact.headerContent.find("Counter_get_count"),
            std::string::npos)
      << "header must declare output getters";
}

TEST_F(CModelEmitterFixture, ImplContainsInitializeBody) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.implContent.find("Counter_initialize"),
            std::string::npos)
      << "impl must contain initialize function body";
  EXPECT_NE(artifact.implContent.find("count_reg"), std::string::npos)
      << "initialize body references state by stable name";
}

TEST_F(CModelEmitterFixture, EmitterUsesSemanticModelDecisionsOnly) {
  // The emitter MUST use semantic model contract as source of truth.
  // State struct field names come from model.stateVars[*].stableName,
  // not from arbitrary IR introspection.
  auto model = buildCounterModel();
  ASSERT_EQ(model.stateVars.size(), 1u);
  EXPECT_EQ(model.stateVars[0].stableName, "count_reg");

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  // Verify the emitter uses exactly the stable name from the model
  EXPECT_NE(artifact.headerContent.find("count_reg"), std::string::npos);

  // Verify the emitter does NOT invent names not in the model
  EXPECT_EQ(artifact.headerContent.find("fc_counter_arc"), std::string::npos)
      << "emitter must not expose arc symbol names as public API";
}

TEST_F(CModelEmitterFixture, NamingConventionUsesStableName) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  // Public API symbols use module-level stable name prefix
  EXPECT_NE(artifact.headerContent.find("Counter_"), std::string::npos)
      << "public symbols use module stable name as prefix";

  // State storage field uses state-level stable name
  EXPECT_NE(artifact.headerContent.find("count_reg"), std::string::npos)
      << "state fields use state stable name";
}

// ---------------------------------------------------------------------------
// F1: Width type legalization
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, WidthTypeLegalizationI1) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "BitMod";
  hirct::semantic::PortInfo in;
  in.name = "flag";
  in.isInput = true;
  in.width = 1;
  model.inputPorts.push_back(in);
  hirct::semantic::PortInfo out;
  out.name = "result";
  out.isInput = false;
  out.width = 1;
  model.outputPorts.push_back(out);
  hirct::semantic::StateVar sv;
  sv.stableName = "bit_reg";
  sv.clockDomain = "clk";
  sv.width = 1;
  model.stateVars.push_back(sv);
  model.clockDomains.push_back("clk");

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.headerContent.find("uint8_t"), std::string::npos)
      << "i1 must map to uint8_t";
  EXPECT_EQ(artifact.headerContent.find("uint1_t"), std::string::npos)
      << "uint1_t is illegal C type";
}

TEST_F(CModelEmitterFixture, WidthTypeLegalizationI10) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "TenBit";
  hirct::semantic::StateVar sv;
  sv.stableName = "reg10";
  sv.clockDomain = "clk";
  sv.width = 10;
  model.stateVars.push_back(sv);
  model.clockDomains.push_back("clk");

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.headerContent.find("uint16_t"), std::string::npos)
      << "i10 must map to uint16_t";
  EXPECT_EQ(artifact.headerContent.find("uint10_t"), std::string::npos)
      << "uint10_t is illegal C type";
}

// ---------------------------------------------------------------------------
// F1: All declared functions have impl stubs
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, ImplContainsAllDeclaredFunctions) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.implContent.find("Counter_initialize"),
            std::string::npos);
  EXPECT_NE(artifact.implContent.find("Counter_set_clk"), std::string::npos)
      << "setter stub must exist";
  EXPECT_NE(artifact.implContent.find("Counter_set_rst"), std::string::npos)
      << "setter stub must exist";
  EXPECT_NE(artifact.implContent.find("Counter_set_en"), std::string::npos)
      << "setter stub must exist";
  EXPECT_NE(artifact.implContent.find("Counter_get_count"), std::string::npos)
      << "getter stub must exist";
  EXPECT_NE(artifact.implContent.find("Counter_eval_comb"), std::string::npos)
      << "eval_comb stub must exist";
  EXPECT_NE(artifact.implContent.find("Counter_eval_clk"), std::string::npos)
      << "eval_clock stub must exist";
}

// ---------------------------------------------------------------------------
// F1: Generated artifact compiles
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GeneratedArtifactCompiles) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_compile";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  std::string headerPath = "/tmp/hirct_test_compile/Counter.h";
  std::string implPath = "/tmp/hirct_test_compile/Counter.cpp";

  std::system("mkdir -p /tmp/hirct_test_compile");

  {
    std::ofstream hf(headerPath);
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf(implPath);
    cf << artifact.implContent;
  }

  std::string compileCmd = "c++ -std=c++17 -fsyntax-only -Werror "
                           "-I/tmp/hirct_test_compile " +
                           implPath + " 2>&1";
  int result = std::system(compileCmd.c_str());
  EXPECT_EQ(result, 0) << "generated C++ artifact must compile without errors";

  std::system("rm -rf /tmp/hirct_test_compile");
}

// ---------------------------------------------------------------------------
// F1: Naming normalization in emitter output
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, EmitterUsesPreNormalizedNames) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "my_module_v2";
  hirct::semantic::PortInfo in;
  in.name = "data_in";
  in.isInput = true;
  in.width = 8;
  model.inputPorts.push_back(in);
  hirct::semantic::PortInfo out;
  out.name = "data_out";
  out.isInput = false;
  out.width = 8;
  model.outputPorts.push_back(out);
  hirct::semantic::StateVar sv;
  sv.stableName = "reg_0";
  sv.clockDomain = "clk";
  sv.width = 8;
  model.stateVars.push_back(sv);
  model.clockDomains.push_back("clk");

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.headerContent.find("my_module_v2_state"),
            std::string::npos)
      << "emitter must use pre-normalized module name directly";
  EXPECT_NE(artifact.headerContent.find("my_module_v2_set_data_in"),
            std::string::npos)
      << "emitter must use pre-normalized port name directly";
  EXPECT_NE(artifact.headerContent.find("my_module_v2_get_data_out"),
            std::string::npos)
      << "emitter must use pre-normalized port name directly";
  EXPECT_NE(artifact.headerContent.find("reg_0"), std::string::npos)
      << "emitter must use pre-normalized state name directly";
}

TEST_F(CModelEmitterFixture, PreNormalizedArtifactCompiles) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "my_module_v2";
  hirct::semantic::PortInfo in;
  in.name = "data_in";
  in.isInput = true;
  in.width = 8;
  model.inputPorts.push_back(in);
  hirct::semantic::PortInfo out;
  out.name = "data_out";
  out.isInput = false;
  out.width = 16;
  model.outputPorts.push_back(out);
  hirct::semantic::StateVar sv;
  sv.stableName = "reg_0";
  sv.clockDomain = "clk";
  sv.width = 8;
  model.stateVars.push_back(sv);
  model.clockDomains.push_back("clk");

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_naming";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_test_naming");
  {
    std::ofstream hf("/tmp/hirct_test_naming/my_module_v2.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_naming/my_module_v2.cpp");
    cf << artifact.implContent;
  }

  int result = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_naming "
      "/tmp/hirct_test_naming/my_module_v2.cpp 2>&1");
  EXPECT_EQ(result, 0)
      << "pre-normalized artifact must compile without errors";

  std::system("rm -rf /tmp/hirct_test_naming");
}

// ---------------------------------------------------------------------------
// Batch3: Builder-normalized names → emitter → compile gate
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, BuilderNormalizedE2ECompile) {
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

  EXPECT_EQ(model->moduleName, "my_mod_v2");
  EXPECT_EQ(model->stateVars[0].stableName, "reg_0");

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_e2e_norm";
  hirct::CModelEmitter emitter(*model, opts);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_test_e2e_norm");
  {
    std::ofstream hf("/tmp/hirct_test_e2e_norm/my_mod_v2.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_e2e_norm/my_mod_v2.cpp");
    cf << artifact.implContent;
  }

  int result = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_e2e_norm "
      "/tmp/hirct_test_e2e_norm/my_mod_v2.cpp 2>&1");
  EXPECT_EQ(result, 0) << "builder-normalized names must produce compilable C++";

  std::system("rm -rf /tmp/hirct_test_e2e_norm");
}

// ---------------------------------------------------------------------------
// Batch4 F3: Aggregate artifact golden
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, AggregateArrayInStateStruct) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "AggGold";
  hirct::semantic::AggregateStateVar av;
  av.stableName = "arr_reg";
  av.clockDomain = "clk";
  av.width = 32;
  av.numElements = 4;
  av.elementWidth = 8;
  av.updateStyleDetermined = true;
  av.updateStyle = hirct::semantic::UpdateStyle::FullReplace;
  model.aggregateStateVars.push_back(av);
  model.clockDomains.push_back("clk");

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.headerContent.find("arr_reg[4]"), std::string::npos)
      << "header must contain array member with element count";
  EXPECT_NE(artifact.headerContent.find("uint8_t"), std::string::npos)
      << "element type must be legal C type";
  EXPECT_NE(artifact.implContent.find("arr_reg[i] = 0"), std::string::npos)
      << "initialize must zero-fill array elements";
}

TEST_F(CModelEmitterFixture, AggregateArtifactCompiles) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "AggComp";
  hirct::semantic::PortInfo in;
  in.name = "d";
  in.isInput = true;
  in.width = 8;
  model.inputPorts.push_back(in);
  hirct::semantic::PortInfo out;
  out.name = "q";
  out.isInput = false;
  out.width = 32;
  model.outputPorts.push_back(out);
  hirct::semantic::AggregateStateVar av;
  av.stableName = "arr_reg";
  av.clockDomain = "clk";
  av.width = 32;
  av.numElements = 4;
  av.elementWidth = 8;
  av.updateStyleDetermined = true;
  av.updateStyle = hirct::semantic::UpdateStyle::FullReplace;
  model.aggregateStateVars.push_back(av);
  model.clockDomains.push_back("clk");

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_agg_compile";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_test_agg_compile");
  {
    std::ofstream hf("/tmp/hirct_test_agg_compile/AggComp.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_agg_compile/AggComp.cpp");
    cf << artifact.implContent;
  }

  int result = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_agg_compile "
      "/tmp/hirct_test_agg_compile/AggComp.cpp 2>&1");
  EXPECT_EQ(result, 0) << "aggregate artifact must compile";
  std::system("rm -rf /tmp/hirct_test_agg_compile");
}

// ---------------------------------------------------------------------------
// Batch4 F4: Multi-clock emitter eval functions
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, MultiClockEvalFunctions) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "DualClk";
  hirct::semantic::PortInfo in;
  in.name = "d";
  in.isInput = true;
  in.width = 8;
  model.inputPorts.push_back(in);
  hirct::semantic::PortInfo out;
  out.name = "qa";
  out.isInput = false;
  out.width = 8;
  model.outputPorts.push_back(out);
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

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.headerContent.find("DualClk_eval_clk_a"),
            std::string::npos)
      << "must declare eval_clk_a";
  EXPECT_NE(artifact.headerContent.find("DualClk_eval_clk_b"),
            std::string::npos)
      << "must declare eval_clk_b";
  EXPECT_NE(artifact.implContent.find("DualClk_eval_clk_a"),
            std::string::npos)
      << "must define eval_clk_a stub";
  EXPECT_NE(artifact.implContent.find("DualClk_eval_clk_b"),
            std::string::npos)
      << "must define eval_clk_b stub";
}

TEST_F(CModelEmitterFixture, AggregateMultiClockCombined) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "AggClk";
  hirct::semantic::PortInfo in;
  in.name = "d";
  in.isInput = true;
  in.width = 8;
  model.inputPorts.push_back(in);
  hirct::semantic::PortInfo out;
  out.name = "q";
  out.isInput = false;
  out.width = 8;
  model.outputPorts.push_back(out);
  hirct::semantic::AggregateStateVar av;
  av.stableName = "arr_a";
  av.clockDomain = "clk_a";
  av.width = 32;
  av.numElements = 4;
  av.elementWidth = 8;
  av.updateStyleDetermined = true;
  av.updateStyle = hirct::semantic::UpdateStyle::IndexedUpdate;
  model.aggregateStateVars.push_back(av);
  hirct::semantic::StateVar sv;
  sv.stableName = "reg_b";
  sv.clockDomain = "clk_b";
  sv.width = 8;
  model.stateVars.push_back(sv);
  model.clockDomains.push_back("clk_a");
  model.clockDomains.push_back("clk_b");

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_aggclk";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.headerContent.find("arr_a[4]"), std::string::npos)
      << "header must contain aggregate array member";
  EXPECT_NE(artifact.headerContent.find("reg_b"), std::string::npos)
      << "header must contain scalar state member";
  EXPECT_NE(artifact.headerContent.find("AggClk_eval_clk_a"),
            std::string::npos)
      << "header must declare eval_clk_a";
  EXPECT_NE(artifact.headerContent.find("AggClk_eval_clk_b"),
            std::string::npos)
      << "header must declare eval_clk_b";

  std::system("mkdir -p /tmp/hirct_test_aggclk");
  {
    std::ofstream hf("/tmp/hirct_test_aggclk/AggClk.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_aggclk/AggClk.cpp");
    cf << artifact.implContent;
  }
  int result = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_aggclk "
      "/tmp/hirct_test_aggclk/AggClk.cpp 2>&1");
  EXPECT_EQ(result, 0) << "combined aggregate+multiclk artifact must compile";
  std::system("rm -rf /tmp/hirct_test_aggclk");
}

TEST_F(CModelEmitterFixture, MultiClockArtifactCompiles) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "DualClk";
  hirct::semantic::PortInfo in;
  in.name = "d";
  in.isInput = true;
  in.width = 8;
  model.inputPorts.push_back(in);
  hirct::semantic::PortInfo out;
  out.name = "qa";
  out.isInput = false;
  out.width = 8;
  model.outputPorts.push_back(out);
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

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_multiclk";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_test_multiclk");
  {
    std::ofstream hf("/tmp/hirct_test_multiclk/DualClk.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_multiclk/DualClk.cpp");
    cf << artifact.implContent;
  }

  int result = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_multiclk "
      "/tmp/hirct_test_multiclk/DualClk.cpp 2>&1");
  EXPECT_EQ(result, 0) << "multi-clock artifact must compile";
  std::system("rm -rf /tmp/hirct_test_multiclk");
}

// ---------------------------------------------------------------------------
// Batch5 Task 3B-3D: Artifact golden -- full text shape
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GoldenHeaderStructLayout) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  // Input shadow fields in state struct
  EXPECT_NE(artifact.headerContent.find("uint8_t input_clk;"),
            std::string::npos)
      << "header must have input shadow field for clk";
  EXPECT_NE(artifact.headerContent.find("uint8_t input_rst;"),
            std::string::npos)
      << "header must have input shadow field for rst";
  EXPECT_NE(artifact.headerContent.find("uint8_t input_en;"),
            std::string::npos)
      << "header must have input shadow field for en";

  // Output shadow fields
  EXPECT_NE(artifact.headerContent.find("uint8_t output_count;"),
            std::string::npos)
      << "header must have output shadow field for count";

  // State field
  EXPECT_NE(artifact.headerContent.find("uint8_t count_reg;"),
            std::string::npos)
      << "header must have state field";
}

TEST_F(CModelEmitterFixture, GoldenSetterWritesToInputShadow) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.implContent.find("s->input_clk = v;"),
            std::string::npos)
      << "setter must write to input shadow";
  EXPECT_NE(artifact.implContent.find("s->input_en = v;"),
            std::string::npos)
      << "setter must write to input shadow";
}

TEST_F(CModelEmitterFixture, GoldenGetterReadsOutputShadow) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.implContent.find("return s->output_count;"),
            std::string::npos)
      << "getter must read from output shadow";
}

// ---------------------------------------------------------------------------
// Batch5 Task 4A: eval_comb() mechanical emission
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, EvalCombXorBody) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @CombXor(in %a : i8, in %b : i8, out out : i8) {
        %0 = comb.xor %a, %b : i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "CombXor");
  ASSERT_TRUE(succeeded(model));

  auto hwModule = (*module).lookupSymbol<circt::hw::HWModuleOp>("CombXor");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  // eval_comb must NOT be a stub
  EXPECT_EQ(artifact.implContent.find("(void)s;"),
            std::string::npos)
      << "eval_comb must not be a stub when IR reference is available";

  // Must contain an xor expression
  EXPECT_NE(artifact.implContent.find("^"),
            std::string::npos)
      << "eval_comb must contain xor operator for comb.xor";

  // Must write to output shadow
  EXPECT_NE(artifact.implContent.find("s->output_out"),
            std::string::npos)
      << "eval_comb must write to output shadow field";
}

TEST_F(CModelEmitterFixture, EvalCombXorCompiles) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @CombXor(in %a : i8, in %b : i8, out out : i8) {
        %0 = comb.xor %a, %b : i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "CombXor");
  ASSERT_TRUE(succeeded(model));

  auto hwModule = (*module).lookupSymbol<circt::hw::HWModuleOp>("CombXor");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_comb_xor";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_test_comb_xor");
  {
    std::ofstream hf("/tmp/hirct_test_comb_xor/CombXor.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_comb_xor/CombXor.cpp");
    cf << artifact.implContent;
  }

  int result = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_comb_xor "
      "/tmp/hirct_test_comb_xor/CombXor.cpp 2>&1");
  EXPECT_EQ(result, 0) << "eval_comb with xor must produce compilable C++";
  std::system("rm -rf /tmp/hirct_test_comb_xor");
}

TEST_F(CModelEmitterFixture, EvalCombAddMux) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @CombAddMux(in %a : i8, in %sel : i1, out out : i8) {
        %c5 = hw.constant 5 : i8
        %sum = comb.add %a, %c5 : i8
        %result = comb.mux %sel, %a, %sum : i8
        hw.output %result : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "CombAddMux");
  ASSERT_TRUE(succeeded(model));

  auto hwModule =
      (*module).lookupSymbol<circt::hw::HWModuleOp>("CombAddMux");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_comb_addmux";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  // Must have actual expressions, not stubs
  EXPECT_NE(artifact.implContent.find("s->output_out"),
            std::string::npos)
      << "eval_comb must write to output shadow";

  std::system("mkdir -p /tmp/hirct_test_comb_addmux");
  {
    std::ofstream hf("/tmp/hirct_test_comb_addmux/CombAddMux.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_comb_addmux/CombAddMux.cpp");
    cf << artifact.implContent;
  }

  int result = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_comb_addmux "
      "/tmp/hirct_test_comb_addmux/CombAddMux.cpp 2>&1");
  EXPECT_EQ(result, 0) << "eval_comb with add+mux must compile";
  std::system("rm -rf /tmp/hirct_test_comb_addmux");
}

// ---------------------------------------------------------------------------
// Batch5: Existing artifact compile-check still passes with new structure
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Batch5 F3: eval_<clock> aggregate updateStyle boundary
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, EvalClockIsStubWhenNoIR) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "StubClk";
  hirct::semantic::StateVar sv;
  sv.stableName = "reg0";
  sv.clockDomain = "clk";
  sv.width = 8;
  model.stateVars.push_back(sv);
  model.clockDomains.push_back("clk");

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  // eval_<clock> must still be a stub since we have no IR reference yet
  EXPECT_NE(artifact.implContent.find("StubClk_eval_clk"),
            std::string::npos);
  // It should contain (void)s; since it's a stub
  auto evalClkPos = artifact.implContent.find("StubClk_eval_clk");
  ASSERT_NE(evalClkPos, std::string::npos);
  auto stubPos = artifact.implContent.find("(void)s;", evalClkPos);
  EXPECT_NE(stubPos, std::string::npos)
      << "eval_<clock> must be stub without IR reference (Task 4B boundary)";
}

TEST_F(CModelEmitterFixture, AggregateUpdateStyleVisibleInModel) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "AggStyle";
  hirct::semantic::AggregateStateVar av;
  av.stableName = "arr0";
  av.clockDomain = "clk";
  av.width = 32;
  av.numElements = 4;
  av.elementWidth = 8;
  av.updateStyleDetermined = true;
  av.updateStyle = hirct::semantic::UpdateStyle::IndexedUpdate;
  model.aggregateStateVars.push_back(av);
  model.clockDomains.push_back("clk");

  // The model carries updateStyle that eval_<clock> will need in Task 4B.
  // For now, the emitter generates a stub, but the style is accessible.
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  EXPECT_EQ(emitter.getModel().aggregateStateVars[0].updateStyle,
            hirct::semantic::UpdateStyle::IndexedUpdate)
      << "emitter must have access to updateStyle for future eval_<clock> emit";

  auto artifact = emitter.emit();
  EXPECT_NE(artifact.headerContent.find("arr0[4]"), std::string::npos)
      << "aggregate storage must be present regardless of updateStyle";
}

TEST_F(CModelEmitterFixture, UnsupportedOpMarker) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @UnsupMod(in %a : i8, out out : i1) {
        %0 = comb.parity %a : i8
        hw.output %0 : i1
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "UnsupMod");
  ASSERT_TRUE(succeeded(model));

  auto hwModule =
      (*module).lookupSymbol<circt::hw::HWModuleOp>("UnsupMod");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.implContent.find("/* unsupported:"),
            std::string::npos)
      << "unsupported ops must not be silently skipped; marker is required";
}

// ---------------------------------------------------------------------------
// Batch6 Task 4B: eval_<clock>() sequential emission
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, EvalClockCounterNextState) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @fc_counter_arc(%arg0: i8, %arg1: i1, %arg2: i1) -> i8 {
        %c1_i8 = hw.constant 1 : i8
        %c0_i8 = hw.constant 0 : i8
        %0 = comb.add %arg0, %c1_i8 : i8
        %1 = comb.mux %arg1, %arg0, %0 : i8
        %2 = comb.mux %arg2, %c0_i8, %1 : i8
        arc.output %2 : i8
      }
      arc.define @clk_arc(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @Counter(in %clk : i1, in %rst : i1, in %en : i1,
                         out count : i8) {
        %clock = arc.call @clk_arc(%clk) : (i1) -> !seq.clock
        %0 = arc.state @fc_counter_arc(%0, %en, %rst) clock %clock latency 1 {names = ["count_reg"]} : (i8, i1, i1) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "Counter");
  ASSERT_TRUE(succeeded(model));

  auto hwModule =
      (*module).lookupSymbol<circt::hw::HWModuleOp>("Counter");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  auto evalClkPos = artifact.implContent.find("Counter_eval_clk(");
  ASSERT_NE(evalClkPos, std::string::npos);

  // eval_clk must NOT be a stub
  auto afterEvalClk = artifact.implContent.substr(evalClkPos);
  auto bodyEnd = afterEvalClk.find("\n}\n");
  auto bodyStr = afterEvalClk.substr(0, bodyEnd);
  EXPECT_EQ(bodyStr.find("(void)s;"), std::string::npos)
      << "eval_clk must not be a stub when IR reference is available";

  // Must reference count_reg (the state variable)
  EXPECT_NE(bodyStr.find("count_reg"), std::string::npos)
      << "eval_clk body must reference the state variable";
}

TEST_F(CModelEmitterFixture, EvalClockCounterCompiles) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @fc_counter_arc(%arg0: i8, %arg1: i1, %arg2: i1) -> i8 {
        %c1_i8 = hw.constant 1 : i8
        %c0_i8 = hw.constant 0 : i8
        %0 = comb.add %arg0, %c1_i8 : i8
        %1 = comb.mux %arg1, %arg0, %0 : i8
        %2 = comb.mux %arg2, %c0_i8, %1 : i8
        arc.output %2 : i8
      }
      arc.define @clk_arc(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @Counter(in %clk : i1, in %rst : i1, in %en : i1,
                         out count : i8) {
        %clock = arc.call @clk_arc(%clk) : (i1) -> !seq.clock
        %0 = arc.state @fc_counter_arc(%0, %en, %rst) clock %clock latency 1 {names = ["count_reg"]} : (i8, i1, i1) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "Counter");
  ASSERT_TRUE(succeeded(model));

  auto hwModule =
      (*module).lookupSymbol<circt::hw::HWModuleOp>("Counter");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_eval_clk";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_test_eval_clk");
  {
    std::ofstream hf("/tmp/hirct_test_eval_clk/Counter.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_eval_clk/Counter.cpp");
    cf << artifact.implContent;
  }

  int result = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_eval_clk "
      "/tmp/hirct_test_eval_clk/Counter.cpp 2>&1");
  EXPECT_EQ(result, 0) << "eval_clk counter artifact must compile";
  std::system("rm -rf /tmp/hirct_test_eval_clk");
}

TEST_F(CModelEmitterFixture, EvalClockEdgeOutputUpdate) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @fc_counter_arc(%arg0: i8, %arg1: i1, %arg2: i1) -> i8 {
        %c1_i8 = hw.constant 1 : i8
        %c0_i8 = hw.constant 0 : i8
        %0 = comb.add %arg0, %c1_i8 : i8
        %1 = comb.mux %arg1, %arg0, %0 : i8
        %2 = comb.mux %arg2, %c0_i8, %1 : i8
        arc.output %2 : i8
      }
      arc.define @clk_arc(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @Counter(in %clk : i1, in %rst : i1, in %en : i1,
                         out count : i8) {
        %clock = arc.call @clk_arc(%clk) : (i1) -> !seq.clock
        %0 = arc.state @fc_counter_arc(%0, %en, %rst) clock %clock latency 1 {names = ["count_reg"]} : (i8, i1, i1) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "Counter");
  ASSERT_TRUE(succeeded(model));

  // output "count" is directly from state -> edge visibility
  ASSERT_GE(model->outputs.size(), 1u);
  EXPECT_EQ(model->outputs[0].visibility,
            hirct::semantic::OutputVisibility::Edge);

  auto hwModule =
      (*module).lookupSymbol<circt::hw::HWModuleOp>("Counter");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  // eval_clk must update edge output shadow
  EXPECT_NE(artifact.implContent.find("s->output_count"),
            std::string::npos)
      << "eval_clk must update edge visibility output shadow";
}

TEST_F(CModelEmitterFixture, EvalClockRuntimeBehavior) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @fc_counter_arc(%arg0: i8, %arg1: i1, %arg2: i1) -> i8 {
        %c1_i8 = hw.constant 1 : i8
        %c0_i8 = hw.constant 0 : i8
        %0 = comb.add %arg0, %c1_i8 : i8
        %1 = comb.mux %arg1, %arg0, %0 : i8
        %2 = comb.mux %arg2, %c0_i8, %1 : i8
        arc.output %2 : i8
      }
      arc.define @clk_arc(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @Counter(in %clk : i1, in %rst : i1, in %en : i1,
                         out count : i8) {
        %clock = arc.call @clk_arc(%clk) : (i1) -> !seq.clock
        %0 = arc.state @fc_counter_arc(%0, %en, %rst) clock %clock latency 1 {names = ["count_reg"]} : (i8, i1, i1) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "Counter");
  ASSERT_TRUE(succeeded(model));
  auto hwModule =
      (*module).lookupSymbol<circt::hw::HWModuleOp>("Counter");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_runtime";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  // Arc IR semantics for this counter:
  //   %0 = comb.add %arg0, 1   (old + 1)
  //   %1 = comb.mux %en, %arg0, %0  (en=1 -> hold, en=0 -> incr)
  //   %2 = comb.mux %rst, 0, %1     (rst=1 -> 0, rst=0 -> %1)
  std::string driverSrc = R"(
#include "Counter.h"
#include <cassert>
int main() {
  Counter_state s;
  Counter_initialize(&s);
  assert(s.count_reg == 0);
  assert(Counter_get_count(&s) == 0);

  // en=0 (enable active-low in this Arc), rst=0 -> increment
  Counter_set_en(&s, 0);
  Counter_set_rst(&s, 0);
  Counter_eval_clk(&s);
  assert(s.count_reg == 1);
  assert(Counter_get_count(&s) == 1);

  // another tick -> 2
  Counter_eval_clk(&s);
  assert(s.count_reg == 2);

  // en=1 -> hold (active-low enable)
  Counter_set_en(&s, 1);
  Counter_eval_clk(&s);
  assert(s.count_reg == 2);

  // rst=1 -> reset to 0 (takes priority over everything)
  Counter_set_rst(&s, 1);
  Counter_eval_clk(&s);
  assert(s.count_reg == 0);

  return 0;
}
)";

  std::system("mkdir -p /tmp/hirct_test_runtime");
  {
    std::ofstream hf("/tmp/hirct_test_runtime/Counter.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_runtime/Counter.cpp");
    cf << artifact.implContent;
  }
  {
    std::ofstream df("/tmp/hirct_test_runtime/driver.cpp");
    df << driverSrc;
  }

  int compileResult = std::system(
      "c++ -std=c++17 -O0 -Werror "
      "-I/tmp/hirct_test_runtime "
      "/tmp/hirct_test_runtime/Counter.cpp "
      "/tmp/hirct_test_runtime/driver.cpp "
      "-o /tmp/hirct_test_runtime/driver 2>&1");
  ASSERT_EQ(compileResult, 0) << "runtime test must compile";

  int runResult = std::system("/tmp/hirct_test_runtime/driver");
  EXPECT_EQ(runResult, 0)
      << "counter runtime: init=0, en=1 ticks, en=0 hold, rst=1 reset";

  std::system("rm -rf /tmp/hirct_test_runtime");
}

TEST_F(CModelEmitterFixture, EvalClockAggregateFullReplaceTODO) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "AggClkTest";
  hirct::semantic::AggregateStateVar av;
  av.stableName = "arr0";
  av.clockDomain = "clk";
  av.width = 32;
  av.numElements = 4;
  av.elementWidth = 8;
  av.updateStyleDetermined = true;
  av.updateStyle = hirct::semantic::UpdateStyle::FullReplace;
  model.aggregateStateVars.push_back(av);
  model.clockDomains.push_back("clk");

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  auto evalClkPos = artifact.implContent.find("AggClkTest_eval_clk(");
  ASSERT_NE(evalClkPos, std::string::npos);
  auto afterPos = artifact.implContent.substr(evalClkPos);

  // Aggregate eval_clock must have a TODO marker, not silent skip
  bool hasTodo = afterPos.find("/* TODO:") != std::string::npos;
  bool hasVoidStub = afterPos.find("(void)s;") != std::string::npos;
  EXPECT_TRUE(hasTodo || hasVoidStub)
      << "aggregate eval_clock must have explicit TODO boundary or stub";
}

// ---------------------------------------------------------------------------
// Batch7 F1: shared arc.define -- two state instances with different inputs
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, SharedArcDefineRuntime) {
  // Two states use the same @add1_arc but with different inputs:
  //   reg_a = add1(reg_a) -> counts from 0
  //   reg_b = add1(input_d) -> latches input_d + 1
  auto module = parseInline(R"mlir(
    module {
      arc.define @add1_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      hw.module @SharedArc(in %clk : !seq.clock, in %d : i8,
                           out qa : i8, out qb : i8) {
        %a = arc.state @add1_arc(%a) clock %clk latency 1 {names = ["reg_a"]} : (i8) -> i8
        %b = arc.state @add1_arc(%d) clock %clk latency 1 {names = ["reg_b"]} : (i8) -> i8
        hw.output %a, %b : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "SharedArc");
  ASSERT_TRUE(succeeded(model));
  ASSERT_EQ(model->stateVars.size(), 2u);
  EXPECT_EQ(model->stateVars[0].stableName, "reg_a");
  EXPECT_EQ(model->stateVars[1].stableName, "reg_b");
  EXPECT_EQ(model->stateVars[0].arcName, "add1_arc");
  EXPECT_EQ(model->stateVars[1].arcName, "add1_arc");

  auto hwModule =
      (*module).lookupSymbol<circt::hw::HWModuleOp>("SharedArc");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_shared_arc";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  std::string driverSrc = R"(
#include "SharedArc.h"
#include <cassert>
int main() {
  SharedArc_state s;
  SharedArc_initialize(&s);
  assert(s.reg_a == 0);
  assert(s.reg_b == 0);

  // reg_a = add1(reg_a) = 0+1 = 1
  // reg_b = add1(input_d) = input_d+1 = 10+1 = 11
  SharedArc_set_d(&s, 10);
  SharedArc_eval_clk(&s);
  assert(s.reg_a == 1);
  assert(s.reg_b == 11);

  // tick again: reg_a = 1+1=2, reg_b = 10+1=11 (d unchanged)
  SharedArc_eval_clk(&s);
  assert(s.reg_a == 2);
  assert(s.reg_b == 11);

  // change d -> reg_b should update
  SharedArc_set_d(&s, 20);
  SharedArc_eval_clk(&s);
  assert(s.reg_a == 3);
  assert(s.reg_b == 21);

  return 0;
}
)";

  std::system("mkdir -p /tmp/hirct_test_shared_arc");
  {
    std::ofstream hf("/tmp/hirct_test_shared_arc/SharedArc.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_shared_arc/SharedArc.cpp");
    cf << artifact.implContent;
  }
  {
    std::ofstream df("/tmp/hirct_test_shared_arc/driver.cpp");
    df << driverSrc;
  }

  int compileResult = std::system(
      "c++ -std=c++17 -O0 -Werror "
      "-I/tmp/hirct_test_shared_arc "
      "/tmp/hirct_test_shared_arc/SharedArc.cpp "
      "/tmp/hirct_test_shared_arc/driver.cpp "
      "-o /tmp/hirct_test_shared_arc/driver 2>&1");
  ASSERT_EQ(compileResult, 0)
      << "shared arc fixture must compile";

  int runResult = std::system("/tmp/hirct_test_shared_arc/driver");
  EXPECT_EQ(runResult, 0)
      << "two states sharing arc.define must update independently";

  std::system("rm -rf /tmp/hirct_test_shared_arc");
}

// ---------------------------------------------------------------------------
// Batch7 F2: post_edge_comb runtime verification
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, PostEdgeCombRuntime) {
  // state reg_a (edge), output qa = reg_a (edge),
  // output qb = reg_a + input_d (post_edge_comb)
  auto module = parseInline(R"mlir(
    module {
      arc.define @add1_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      hw.module @PostEdge(in %clk : !seq.clock, in %d : i8,
                          out qa : i8, out qb : i8) {
        %a = arc.state @add1_arc(%a) clock %clk latency 1 {names = ["reg_a"]} : (i8) -> i8
        %sum = comb.add %a, %d : i8
        hw.output %a, %sum : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "PostEdge");
  ASSERT_TRUE(succeeded(model));

  ASSERT_GE(model->outputs.size(), 2u);
  EXPECT_EQ(model->outputs[0].visibility,
            hirct::semantic::OutputVisibility::Edge);
  EXPECT_EQ(model->outputs[1].visibility,
            hirct::semantic::OutputVisibility::PostEdgeComb);

  auto hwModule =
      (*module).lookupSymbol<circt::hw::HWModuleOp>("PostEdge");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_post_edge";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  std::string driverSrc = R"(
#include "PostEdge.h"
#include <cassert>
int main() {
  PostEdge_state s;
  PostEdge_initialize(&s);

  PostEdge_set_d(&s, 5);
  PostEdge_eval_clk(&s);
  // reg_a = 0+1 = 1 (edge output qa = 1)
  // qb = reg_a + d = 1 + 5 = 6 (post_edge_comb)
  assert(PostEdge_get_qa(&s) == 1);
  assert(PostEdge_get_qb(&s) == 6);

  PostEdge_eval_clk(&s);
  // reg_a = 1+1 = 2, qa=2, qb = 2+5 = 7
  assert(PostEdge_get_qa(&s) == 2);
  assert(PostEdge_get_qb(&s) == 7);

  PostEdge_set_d(&s, 100);
  PostEdge_eval_clk(&s);
  // reg_a = 3, qa=3, qb = 3+100 = 103
  assert(PostEdge_get_qa(&s) == 3);
  assert(PostEdge_get_qb(&s) == 103);

  return 0;
}
)";

  std::system("mkdir -p /tmp/hirct_test_post_edge");
  {
    std::ofstream hf("/tmp/hirct_test_post_edge/PostEdge.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_post_edge/PostEdge.cpp");
    cf << artifact.implContent;
  }
  {
    std::ofstream df("/tmp/hirct_test_post_edge/driver.cpp");
    df << driverSrc;
  }

  int compileResult = std::system(
      "c++ -std=c++17 -O0 -Werror "
      "-I/tmp/hirct_test_post_edge "
      "/tmp/hirct_test_post_edge/PostEdge.cpp "
      "/tmp/hirct_test_post_edge/driver.cpp "
      "-o /tmp/hirct_test_post_edge/driver 2>&1");
  ASSERT_EQ(compileResult, 0)
      << "post_edge_comb fixture must compile";

  int runResult = std::system("/tmp/hirct_test_post_edge/driver");
  EXPECT_EQ(runResult, 0)
      << "post_edge_comb output must reflect new state + input after eval_clk";

  std::system("rm -rf /tmp/hirct_test_post_edge");
}

// ---------------------------------------------------------------------------
// Batch7 F3: memory-derived edge output boundary
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, MemoryEdgeOutputBoundary) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "MemEdge";
  hirct::semantic::PortInfo in;
  in.name = "addr";
  in.isInput = true;
  in.width = 4;
  model.inputPorts.push_back(in);
  hirct::semantic::PortInfo out;
  out.name = "rd";
  out.isInput = false;
  out.width = 8;
  model.outputPorts.push_back(out);
  hirct::semantic::MemoryVar mv;
  mv.stableName = "memory_0";
  mv.clockDomain = "clk";
  mv.depth = 16;
  mv.addressWidth = 4;
  mv.elementWidth = 8;
  mv.hasReadPort = true;
  mv.readPortCount = 1;
  model.memoryVars.push_back(mv);
  model.clockDomains.push_back("clk");

  hirct::semantic::OutputBinding binding;
  binding.name = "rd";
  binding.sourceKind = "memory_read";
  binding.sourceEntity = "memory_0";
  binding.visibility = hirct::semantic::OutputVisibility::Edge;
  model.outputs.push_back(binding);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_mem_edge";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  // The edge output for memory_0 should NOT naively do:
  //   s->output_rd = s->memory_0;  // memory_0 is an array!
  // It must either have a TODO marker or handle it properly.
  auto evalClkPos = artifact.implContent.find("MemEdge_eval_clk(");
  ASSERT_NE(evalClkPos, std::string::npos);
  auto afterPos = artifact.implContent.substr(evalClkPos);

  // Must NOT contain a bare "s->output_rd = s->memory_0;" without indexing
  bool hasBareAssign = afterPos.find("s->output_rd = s->memory_0;") !=
                       std::string::npos;
  EXPECT_FALSE(hasBareAssign)
      << "memory-derived edge output must not naively assign array to scalar";

  // Must compile
  std::system("mkdir -p /tmp/hirct_test_mem_edge");
  {
    std::ofstream hf("/tmp/hirct_test_mem_edge/MemEdge.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_mem_edge/MemEdge.cpp");
    cf << artifact.implContent;
  }

  int result = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_mem_edge "
      "/tmp/hirct_test_mem_edge/MemEdge.cpp 2>&1");
  EXPECT_EQ(result, 0)
      << "memory edge output artifact must compile (even if TODO boundary)";

  std::system("rm -rf /tmp/hirct_test_mem_edge");
}

// ---------------------------------------------------------------------------
// Expression coverage: comb.icmp, comb.concat, variadic comb ops
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, EvalCombIcmpUle) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @IcmpMod(in %a : i4, in %b : i4, out le : i1) {
        %0 = comb.icmp ule %a, %b : i4
        hw.output %0 : i1
      }
    }
  )mlir");
  auto result = hirct::semantic::buildModuleModel(*module, "IcmpMod");
  ASSERT_TRUE(succeeded(result));
  auto model = *result;
  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("IcmpMod");
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_icmp";
  hirct::CModelEmitter emitter(model, opts, hwModule);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.implContent.find("<="), std::string::npos)
      << "icmp ule should produce '<=' operator";
  EXPECT_EQ(artifact.implContent.find("unsupported:comb.icmp"),
            std::string::npos)
      << "comb.icmp should be supported";

  std::system("mkdir -p /tmp/hirct_test_icmp");
  { std::ofstream f("/tmp/hirct_test_icmp/IcmpMod.h");
    f << artifact.headerContent; }
  { std::ofstream f("/tmp/hirct_test_icmp/IcmpMod.cpp");
    f << artifact.implContent; }
  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_icmp /tmp/hirct_test_icmp/IcmpMod.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "icmp artifact must compile";
  std::system("rm -rf /tmp/hirct_test_icmp");
}

TEST_F(CModelEmitterFixture, EvalCombConcatExpr) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @ConcatMod(in %lo : i4, in %hi : i4, out cat : i8) {
        %0 = comb.concat %hi, %lo : i4, i4
        hw.output %0 : i8
      }
    }
  )mlir");
  auto result = hirct::semantic::buildModuleModel(*module, "ConcatMod");
  ASSERT_TRUE(succeeded(result));
  auto model = *result;
  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("ConcatMod");
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_concat";
  hirct::CModelEmitter emitter(model, opts, hwModule);
  auto artifact = emitter.emit();

  EXPECT_EQ(artifact.implContent.find("unsupported:comb.concat"),
            std::string::npos)
      << "comb.concat should be supported";

  std::system("mkdir -p /tmp/hirct_test_concat");
  { std::ofstream f("/tmp/hirct_test_concat/ConcatMod.h");
    f << artifact.headerContent; }
  { std::ofstream f("/tmp/hirct_test_concat/ConcatMod.cpp");
    f << artifact.implContent; }
  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_concat /tmp/hirct_test_concat/ConcatMod.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "concat artifact must compile";
  std::system("rm -rf /tmp/hirct_test_concat");
}

TEST_F(CModelEmitterFixture, VariadicCombAnd3) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @And3Mod(in %a : i1, in %b : i1, in %c : i1, out y : i1) {
        %0 = comb.and %a, %b, %c : i1
        hw.output %0 : i1
      }
    }
  )mlir");
  auto result = hirct::semantic::buildModuleModel(*module, "And3Mod");
  ASSERT_TRUE(succeeded(result));
  auto model = *result;
  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("And3Mod");
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_and3";
  hirct::CModelEmitter emitter(model, opts, hwModule);
  auto artifact = emitter.emit();

  EXPECT_EQ(artifact.implContent.find("unsupported:comb.and"),
            std::string::npos)
      << "variadic comb.and should be supported";

  std::system("mkdir -p /tmp/hirct_test_and3");
  { std::ofstream f("/tmp/hirct_test_and3/And3Mod.h");
    f << artifact.headerContent; }
  { std::ofstream f("/tmp/hirct_test_and3/And3Mod.cpp");
    f << artifact.implContent; }
  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_and3 /tmp/hirct_test_and3/And3Mod.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "variadic and artifact must compile";
  std::system("rm -rf /tmp/hirct_test_and3");
}

// ---------------------------------------------------------------------------
// Real RTL regression: ncs_hp_first_sig_gen Arc MLIR compile-check
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, RealRtlHpFirstSigGenCompiles) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @ncs_hp_first_sig_gen_arc(%arg0: i4, %arg1: i4) -> i1 {
        %0 = comb.icmp ule %arg0, %arg1 : i4
        arc.output %0 : i1
      }
      arc.define @ncs_hp_first_sig_gen_arc_0(%arg0: i4, %arg1: i4, %arg2: i3,
          %arg3: i4, %arg4: i4, %arg5: i1, %arg6: i4, %arg7: i4, %arg8: i4,
          %arg9: i4, %arg10: i4, %arg11: i4, %arg12: i4, %arg13: i4,
          %arg14: i4, %arg15: i4, %arg16: i4, %arg17: i4) -> i4 {
        %0 = comb.icmp ceq %arg0, %arg1 : i4
        %1 = comb.concat %0, %arg2 : i1, i3
        %2 = comb.icmp ceq %arg0, %arg4 : i4
        %3 = comb.xor %2, %arg5 : i1
        %4 = comb.icmp ceq %arg0, %arg6 : i4
        %5 = comb.xor %4, %arg5 : i1
        %6 = comb.and %5, %3 : i1
        %7 = comb.and %6, %0 : i1
        %8 = comb.mux %7, %arg1, %1 : i4
        %9 = comb.mux %2, %arg6, %8 : i4
        arc.output %9 : i4
      }
      arc.define @ncs_hp_first_sig_gen_arc_1(%arg0: i1, %arg1: i1, %arg2: i1) -> (i1, !seq.clock) {
        %0 = comb.xor %arg0, %arg1 : i1
        %1 = seq.to_clock %arg2
        arc.output %0, %1 : i1, !seq.clock
      }
      hw.module @ncs_hp_first_sig_gen(in %RESET_N_I : i1, in %CLK_I : i1,
                                      in %THRES_I : i4, out HP_FIRST_O : i1) {
        %c0_i3 = hw.constant 0 : i3
        %true = hw.constant true
        %c0_i4 = hw.constant 0 : i4
        %c1_i4 = hw.constant 1 : i4
        %c_neg7_i4 = hw.constant -7 : i4
        %c_neg5_i4 = hw.constant -5 : i4
        %0 = arc.state @ncs_hp_first_sig_gen_arc(%1, %THRES_I) clock %2#1 latency 1 {names = ["HP_FIRST_O"]} : (i4, i4) -> i1
        %1 = arc.state @ncs_hp_first_sig_gen_arc_0(%1, %c_neg7_i4, %c0_i3, %c_neg5_i4, %c0_i4, %true, %c1_i4, %c0_i4, %c0_i4, %c0_i4, %c0_i4, %c0_i4, %c0_i4, %c0_i4, %c0_i4, %c0_i4, %c0_i4, %c0_i4) clock %2#1 latency 1 {names = ["r_gray_cnt"]} : (i4, i4, i3, i4, i4, i1, i4, i4, i4, i4, i4, i4, i4, i4, i4, i4, i4, i4) -> i4
        %2:2 = arc.call @ncs_hp_first_sig_gen_arc_1(%RESET_N_I, %true, %CLK_I) : (i1, i1, i1) -> (i1, !seq.clock)
        hw.output %0 : i1
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto result = hirct::semantic::buildModuleModel(*module, "ncs_hp_first_sig_gen");
  ASSERT_TRUE(succeeded(result));
  auto model = *result;

  EXPECT_EQ(model.stateVars.size(), 2u);
  EXPECT_EQ(model.inputPorts.size(), 3u);
  EXPECT_EQ(model.outputPorts.size(), 1u);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("ncs_hp_first_sig_gen");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_hp_first";
  hirct::CModelEmitter emitter(model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_test_hp_first");
  { std::ofstream f("/tmp/hirct_test_hp_first/ncs_hp_first_sig_gen.h");
    f << artifact.headerContent; }
  { std::ofstream f("/tmp/hirct_test_hp_first/ncs_hp_first_sig_gen.cpp");
    f << artifact.implContent; }
  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_hp_first "
      "/tmp/hirct_test_hp_first/ncs_hp_first_sig_gen.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "real RTL artifact must compile";
  std::system("rm -rf /tmp/hirct_test_hp_first");
}

// ---------------------------------------------------------------------------
// Real RTL regression: full ncs_hp_first_sig_gen Arc MLIR file-based
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, RealRtlFullHpFirstSigGen) {
  auto fixtureRoot = std::filesystem::path(__FILE__)
                         .parent_path()
                         .parent_path()
                         .parent_path() /
                     "tests" / "fixtures";
  auto mlirPath = fixtureRoot / "ncs_hp_first_sig_gen_arc.mlir";
  if (!std::filesystem::exists(mlirPath)) {
    GTEST_SKIP() << "fixture not found: " << mlirPath.string();
  }

  std::ifstream ifs(mlirPath);
  std::string mlirContent((std::istreambuf_iterator<char>(ifs)),
                          std::istreambuf_iterator<char>());
  auto module = parseInline(mlirContent);
  ASSERT_TRUE(module) << "failed to parse full real RTL Arc MLIR";

  auto result =
      hirct::semantic::buildModuleModel(*module, "ncs_hp_first_sig_gen");
  ASSERT_TRUE(succeeded(result));
  auto model = *result;

  EXPECT_EQ(model.stateVars.size(), 2u);
  EXPECT_EQ(model.inputPorts.size(), 3u);
  EXPECT_EQ(model.outputPorts.size(), 1u);
  EXPECT_EQ(model.outputPorts[0].name, "HP_FIRST_O");

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("ncs_hp_first_sig_gen");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_rtl_hp_first_full";
  hirct::CModelEmitter emitter(model, opts, hwModule);
  auto artifact = emitter.emit();

  EXPECT_EQ(artifact.implContent.find("unsupported:"), std::string::npos)
      << "no unsupported ops should remain in real RTL CModel output";

  std::system("mkdir -p /tmp/hirct_rtl_hp_first_full");
  {
    std::ofstream f("/tmp/hirct_rtl_hp_first_full/ncs_hp_first_sig_gen.h");
    f << artifact.headerContent;
  }
  {
    std::ofstream f("/tmp/hirct_rtl_hp_first_full/ncs_hp_first_sig_gen.cpp");
    f << artifact.implContent;
  }
  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_rtl_hp_first_full "
      "/tmp/hirct_rtl_hp_first_full/ncs_hp_first_sig_gen.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "real RTL full Arc MLIR CModel must compile";
  std::system("rm -rf /tmp/hirct_rtl_hp_first_full");
}

TEST_F(CModelEmitterFixture, EvalCombExtractExpr) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @ExtractMod(in %d : i8, out lo : i4) {
        %0 = comb.extract %d from 0 : (i8) -> i4
        hw.output %0 : i4
      }
    }
  )mlir");
  auto result = hirct::semantic::buildModuleModel(*module, "ExtractMod");
  ASSERT_TRUE(succeeded(result));
  auto model = *result;
  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("ExtractMod");
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_extract";
  hirct::CModelEmitter emitter(model, opts, hwModule);
  auto artifact = emitter.emit();

  EXPECT_EQ(artifact.implContent.find("unsupported:comb.extract"),
            std::string::npos)
      << "comb.extract should be supported";

  std::system("mkdir -p /tmp/hirct_test_extract");
  { std::ofstream f("/tmp/hirct_test_extract/ExtractMod.h");
    f << artifact.headerContent; }
  { std::ofstream f("/tmp/hirct_test_extract/ExtractMod.cpp");
    f << artifact.implContent; }
  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_extract /tmp/hirct_test_extract/ExtractMod.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "extract artifact must compile";
  std::system("rm -rf /tmp/hirct_test_extract");
}

TEST_F(CModelEmitterFixture, RealRtlHammingDecCompiles) {
  auto fixtureRoot = std::filesystem::path(__FILE__)
                         .parent_path()
                         .parent_path()
                         .parent_path() /
                     "tests" / "fixtures";
  auto mlirPath = fixtureRoot / "secded_hamming_dec_d64_p8_arc.mlir";
  if (!std::filesystem::exists(mlirPath)) {
    GTEST_SKIP() << "fixture not found: " << mlirPath.string();
  }

  std::ifstream ifs(mlirPath);
  std::string mlirContent((std::istreambuf_iterator<char>(ifs)),
                          std::istreambuf_iterator<char>());
  auto module = parseInline(mlirContent);
  ASSERT_TRUE(module) << "failed to parse hamming decoder Arc MLIR";

  auto result =
      hirct::semantic::buildModuleModel(*module, "secded_hamming_dec_d64_p8");
  ASSERT_TRUE(succeeded(result));
  auto model = *result;

  EXPECT_EQ(model.stateVars.size(), 0u);
  EXPECT_EQ(model.inputPorts.size(), 2u);
  EXPECT_EQ(model.outputPorts.size(), 3u);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("secded_hamming_dec_d64_p8");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_rtl_hamming";
  hirct::CModelEmitter emitter(model, opts, hwModule);
  auto artifact = emitter.emit();

  EXPECT_EQ(artifact.implContent.find("unsupported:arc.call"), std::string::npos)
      << "arc.call should be fully inlined now";
  std::system("mkdir -p /tmp/hirct_rtl_hamming");
  {
    std::ofstream f("/tmp/hirct_rtl_hamming/secded_hamming_dec_d64_p8.h");
    f << artifact.headerContent;
  }
  {
    std::ofstream f("/tmp/hirct_rtl_hamming/secded_hamming_dec_d64_p8.cpp");
    f << artifact.implContent;
  }
  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_rtl_hamming "
      "/tmp/hirct_rtl_hamming/secded_hamming_dec_d64_p8.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "hamming decoder CModel must compile";
  std::system("rm -rf /tmp/hirct_rtl_hamming");
}

TEST_F(CModelEmitterFixture, CounterWithShadowsCompiles) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_counter_shadow";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_test_counter_shadow");
  {
    std::ofstream hf("/tmp/hirct_test_counter_shadow/Counter.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_counter_shadow/Counter.cpp");
    cf << artifact.implContent;
  }

  int result = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_test_counter_shadow "
      "/tmp/hirct_test_counter_shadow/Counter.cpp 2>&1");
  EXPECT_EQ(result, 0) << "counter with shadow fields must compile";
  std::system("rm -rf /tmp/hirct_test_counter_shadow");
}

// ---------------------------------------------------------------------------
// Real RTL regression: file-based end-to-end pipeline tests
// ---------------------------------------------------------------------------

static void runRealRtlRegression(
    CModelEmitterFixture &fixture,
    const std::string &fixtureName,
    const std::string &moduleName,
    unsigned expectedStates,
    unsigned expectedInputs,
    unsigned expectedOutputs) {
  auto fixtureRoot = std::filesystem::path(__FILE__)
                         .parent_path()
                         .parent_path()
                         .parent_path() /
                     "tests" / "fixtures";
  auto mlirPath = fixtureRoot / fixtureName;
  if (!std::filesystem::exists(mlirPath)) {
    GTEST_SKIP() << "fixture not found: " << mlirPath.string();
  }
  std::ifstream ifs(mlirPath);
  std::string mlirContent((std::istreambuf_iterator<char>(ifs)),
                          std::istreambuf_iterator<char>());
  auto module = fixture.parseInline(mlirContent);
  ASSERT_TRUE(module) << "parse failed: " << fixtureName;

  auto result = hirct::semantic::buildModuleModel(*module, moduleName);
  ASSERT_TRUE(succeeded(result)) << "buildModuleModel failed: " << moduleName;
  auto model = *result;

  EXPECT_EQ(model.stateVars.size(), expectedStates);
  EXPECT_EQ(model.inputPorts.size(), expectedInputs);
  EXPECT_EQ(model.outputPorts.size(), expectedOutputs);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>(moduleName);
  ASSERT_TRUE(hwModule) << "hwModule not found: " << moduleName;

  hirct::CModelOptions opts;
  std::string outDir = "/tmp/hirct_rtl_" + moduleName;
  opts.outputDir = outDir;
  hirct::CModelEmitter emitter(model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system(("mkdir -p " + outDir).c_str());
  {
    std::ofstream f(outDir + "/" + moduleName + ".h");
    f << artifact.headerContent;
  }
  {
    std::ofstream f(outDir + "/" + moduleName + ".cpp");
    f << artifact.implContent;
  }
  int rc = std::system(
      ("c++ -std=c++17 -fsyntax-only -Werror -I" + outDir + " " +
       outDir + "/" + moduleName + ".cpp 2>&1")
          .c_str());
  EXPECT_EQ(rc, 0) << moduleName << " CModel must compile";
  std::system(("rm -rf " + outDir).c_str());
}

TEST_F(CModelEmitterFixture, RealRtl_ncs_core_ppu_mpm_if) {
  runRealRtlRegression(*this,
                       "ncs_core_ppu_mpm_if_arc.mlir",
                       "ncs_core_ppu_mpm_if",
                       1,   // 1 state (r_current_state)
                       7,   // 7 inputs
                       5);  // 5 outputs
}

TEST_F(CModelEmitterFixture, RealRtl_ncs_core_dmem_ppu_cpu_arb) {
  runRealRtlRegression(*this,
                       "ncs_core_dmem_ppu_cpu_arb_arc.mlir",
                       "ncs_core_dmem_ppu_cpu_arb",
                       1,   // 1 state (WR_CPU_DATA_VALID_O)
                       12,  // 12 inputs
                       9);  // 9 outputs
}

TEST_F(CModelEmitterFixture, RealRtl_ncs_core_timer) {
  runRealRtlRegression(*this,
                       "ncs_core_timer_arc.mlir",
                       "ncs_core_timer",
                       2,   // 2 states (r_timer, r_current_state)
                       6,   // 6 inputs
                       1);  // 1 output
}

TEST_F(CModelEmitterFixture, RealRtl_toggle_gen) {
  runRealRtlRegression(*this,
                       "toggle_gen_arc.mlir",
                       "toggle_gen",
                       2,   // 2 states (toggle, cnt)
                       4,   // 4 inputs (rst_n, clk, enable, period_0b)
                       1);  // 1 output (toggle)
}

TEST_F(CModelEmitterFixture, RealRtl_secded_hamming_dec_d32_p7) {
  runRealRtlRegression(*this,
                       "secded_hamming_dec_d32_p7_arc.mlir",
                       "secded_hamming_dec_d32_p7",
                       0,   // pure combinational
                       2,   // d:i32, p:i7
                       3);  // q:i32, bit_error:i1, uecc_error:i1
}

TEST_F(CModelEmitterFixture, RealRtl_secded_hamming_enc_d32_p7) {
  runRealRtlRegression(*this,
                       "secded_hamming_enc_d32_p7_arc.mlir",
                       "secded_hamming_enc_d32_p7",
                       0,   // pure combinational
                       1,   // d:i32
                       1);  // p:i7
}

// ---------------------------------------------------------------------------
// arc.call inline tests
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, ArcCallInline_EvalComb_EncD32) {
  auto fixtureRoot = std::filesystem::path(__FILE__)
                         .parent_path().parent_path().parent_path() /
                     "tests" / "fixtures";
  auto mlirPath = fixtureRoot / "secded_hamming_enc_d32_p7_arc.mlir";
  ASSERT_TRUE(std::filesystem::exists(mlirPath)) << mlirPath.string();
  std::ifstream ifs(mlirPath);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
  auto module = parseInline(content);
  ASSERT_TRUE(module);

  auto result = hirct::semantic::buildModuleModel(*module,
                                                  "secded_hamming_enc_d32_p7");
  ASSERT_TRUE(succeeded(result));
  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("secded_hamming_enc_d32_p7");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_arccall_enc";
  hirct::CModelEmitter emitter(*result, opts, hwModule);
  auto artifact = emitter.emit();

  EXPECT_EQ(artifact.implContent.find("unsupported:arc.call"), std::string::npos)
      << "arc.call must be inlined in eval_comb for secded_hamming_enc_d32_p7";
  EXPECT_NE(artifact.implContent.find("eval_comb"), std::string::npos);
}

TEST_F(CModelEmitterFixture, ArcCallInline_PostEdgeComb_PpuMpmIf) {
  auto fixtureRoot = std::filesystem::path(__FILE__)
                         .parent_path().parent_path().parent_path() /
                     "tests" / "fixtures";
  auto mlirPath = fixtureRoot / "ncs_core_ppu_mpm_if_arc.mlir";
  ASSERT_TRUE(std::filesystem::exists(mlirPath)) << mlirPath.string();
  std::ifstream ifs(mlirPath);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
  auto module = parseInline(content);
  ASSERT_TRUE(module);

  auto result = hirct::semantic::buildModuleModel(*module,
                                                  "ncs_core_ppu_mpm_if");
  ASSERT_TRUE(succeeded(result));
  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("ncs_core_ppu_mpm_if");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_arccall_ppu";
  hirct::CModelEmitter emitter(*result, opts, hwModule);
  auto artifact = emitter.emit();

  EXPECT_EQ(artifact.implContent.find("unsupported:arc.call"), std::string::npos)
      << "arc.call must be inlined in post_edge_comb for ncs_core_ppu_mpm_if";
}

TEST_F(CModelEmitterFixture, ArcCallInline_NestedGuard) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @inner(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      arc.define @outer(%arg0: i8) -> i8 {
        %0 = arc.call @inner(%arg0) : (i8) -> i8
        %c2 = hw.constant 2 : i8
        %1 = comb.add %0, %c2 : i8
        arc.output %1 : i8
      }
      hw.module @NestedCall(in %a : i8, out out : i8) {
        %0 = arc.call @outer(%a) : (i8) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto result = hirct::semantic::buildModuleModel(*module, "NestedCall");
  ASSERT_TRUE(succeeded(result));
  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("NestedCall");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_arccall_nested";
  hirct::CModelEmitter emitter(*result, opts, hwModule);
  auto artifact = emitter.emit();

  EXPECT_EQ(artifact.implContent.find("unsupported:arc.call"), std::string::npos)
      << "nested arc.call must be fully inlined";
  EXPECT_NE(artifact.implContent.find("eval_comb"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Phase 1 wide port support: emitter tests
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, WidePort_StorageLayout) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "WideIO";
  model.inputPorts.push_back({"sel", true, 4});
  model.inputPorts.push_back({"din", true, 512});
  model.outputPorts.push_back({"dout", false, 256});

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.headerContent.find("input_din"), std::string::npos)
      << "wide input must appear in struct";
  EXPECT_NE(artifact.headerContent.find("output_dout"), std::string::npos)
      << "wide output must appear in struct";
  EXPECT_NE(artifact.headerContent.find("[8]"), std::string::npos)
      << "i512 should map to 8 words (ceil(512/64))";
  EXPECT_NE(artifact.headerContent.find("[4]"), std::string::npos)
      << "i256 should map to 4 words (ceil(256/64))";
}

TEST_F(CModelEmitterFixture, WidePort_ApiSurface) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "WideIO";
  model.inputPorts.push_back({"sel", true, 4});
  model.inputPorts.push_back({"din", true, 512});
  model.outputPorts.push_back({"dout", false, 256});

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.headerContent.find("set_din_word"), std::string::npos)
      << "wide input must have word-level setter";
  EXPECT_NE(artifact.headerContent.find("set_din_words"), std::string::npos)
      << "wide input must have bulk setter";
  EXPECT_NE(artifact.headerContent.find("get_dout_word"), std::string::npos)
      << "wide output must have word-level getter";
  EXPECT_NE(artifact.headerContent.find("get_dout_words"), std::string::npos)
      << "wide output must have bulk getter";

  EXPECT_NE(artifact.headerContent.find("set_sel"), std::string::npos)
      << "scalar input still uses scalar API";
  EXPECT_EQ(artifact.headerContent.find("set_sel_word"), std::string::npos)
      << "scalar input must NOT have word-level API";
}

TEST_F(CModelEmitterFixture, WidePort_CompileCheck) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "WideCompile";
  model.inputPorts.push_back({"sel", true, 4});
  model.inputPorts.push_back({"din", true, 512});
  model.outputPorts.push_back({"narrow_out", false, 32});
  model.outputPorts.push_back({"wide_out", false, 256});

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  auto tmpDir = std::filesystem::temp_directory_path() / "hirct_widecompile_test";
  std::filesystem::create_directories(tmpDir);
  auto hdrPath = tmpDir / "WideCompile.h";
  auto cppPath = tmpDir / "WideCompile.cpp";
  {
    std::ofstream hdr(hdrPath);
    hdr << artifact.headerContent;
    std::ofstream cpp(cppPath);
    cpp << artifact.implContent;
  }

  std::string cmd = "c++ -std=c++17 -fsyntax-only -Werror " +
                    cppPath.string() + " -include " + hdrPath.string() +
                    " 2>&1";
  int ret = std::system(cmd.c_str());
  EXPECT_EQ(ret, 0) << "wide port artifact must compile cleanly";

  std::filesystem::remove_all(tmpDir);
}

TEST_F(CModelEmitterFixture, WidePort_EvalCombBoundary) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @WideExtract(in %din : i512, out dout : i32) {
        %0 = comb.extract %din from 0 : (i512) -> i32
        hw.output %0 : i32
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = (*module).lookupSymbol<circt::hw::HWModuleOp>("WideExtract");
  ASSERT_TRUE(hwModule);

  auto result = hirct::semantic::buildModuleModel(*module, "WideExtract");
  ASSERT_TRUE(succeeded(result));

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/test";
  hirct::CModelEmitter emitter(*result, opts, hwModule);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.implContent.find("eval_comb"), std::string::npos);
  EXPECT_EQ(artifact.implContent.find("unsupported:comb.extract"), std::string::npos)
      << "comb.extract on wide port must be supported";
}

// ---------------------------------------------------------------------------
// GenModel arc.state + multi-result arc.call hardening
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GenModel_ArcState_MultiResultCall_EvtLogIf) {
  auto fixtureRoot = std::filesystem::path(__FILE__)
                         .parent_path().parent_path().parent_path() /
                     "tests" / "fixtures";
  auto mlirPath = fixtureRoot / "ncs_core_evt_log_if_arc.mlir";
  ASSERT_TRUE(std::filesystem::exists(mlirPath)) << mlirPath.string();
  std::ifstream ifs(mlirPath);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
  auto module = parseInline(content);
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("ncs_core_evt_log_if");
  ASSERT_TRUE(hwModule);

  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_evtlog");
  ASSERT_TRUE(ok) << "GenModel::emit failed for ncs_core_evt_log_if";

  std::ifstream cpp_ifs("/tmp/hirct_genmodel_evtlog/cmodel/ncs_core_evt_log_if.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  // r_current_state must be declared as a register and used in step()
  std::ifstream h_ifs("/tmp/hirct_genmodel_evtlog/cmodel/ncs_core_evt_log_if.h");
  std::string h_content((std::istreambuf_iterator<char>(h_ifs)),
                        std::istreambuf_iterator<char>());
  EXPECT_NE(h_content.find("reg_"), std::string::npos)
      << "arc.state register must appear in header";
  EXPECT_NE(h_content.find("next_"), std::string::npos)
      << "arc.state next-value must appear in header";

  // Outputs derived from multi-result arc.call must not be constant 0
  EXPECT_EQ(cpp_content.find("RD_MPM_RD_REQ_READY_O = ((0) &"), std::string::npos)
      << "RD_MPM_RD_REQ_READY_O must not be hardcoded 0";
  EXPECT_EQ(cpp_content.find("WR_LOG_REQ_VALID_O = ((0) &"), std::string::npos)
      << "WR_LOG_REQ_VALID_O must not be hardcoded 0";
  EXPECT_EQ(cpp_content.find("WR_LOG_REQ_64B_ADDR_O = static_cast<uint32_t>((0)"), std::string::npos)
      << "WR_LOG_REQ_64B_ADDR_O must not be hardcoded 0";

  // DBG_EVT_LOG_IF_O is driven by arc.state — must reference register
  EXPECT_EQ(cpp_content.find("DBG_EVT_LOG_IF_O = ((0) &"), std::string::npos)
      << "DBG_EVT_LOG_IF_O (from arc.state) must not be hardcoded 0";

  // step() must contain reg_ = next_ update
  EXPECT_NE(cpp_content.find("next_"), std::string::npos)
      << "step() must contain next-state assignment for arc.state register";

  // Compile check
  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_evtlog/cmodel "
      "/tmp/hirct_genmodel_evtlog/cmodel/ncs_core_evt_log_if.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "generated code must compile";
}

// ---------------------------------------------------------------------------
// arc.state reset/enable boundary tests (GenModel path)
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GenModel_ArcState_SyncResetRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @inc_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      arc.define @clk_arc(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @SyncRst(in %clk : i1, in %rst : i1,
                         out count : i8) {
        %clock = arc.call @clk_arc(%clk) : (i1) -> !seq.clock
        %0 = arc.state @inc_arc(%0) clock %clock reset %rst latency 1 {names = ["cnt"]} : (i8) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("SyncRst");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_syncrst");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_syncrst");
  ASSERT_TRUE(ok) << "GenModel::emit failed for SyncRst";

  std::ifstream cpp_ifs("/tmp/hirct_genmodel_syncrst/cmodel/SyncRst.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_syncrst/cmodel "
      "/tmp/hirct_genmodel_syncrst/cmodel/SyncRst.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "SyncRst must compile";

  // Build a driver to verify reset behavior at runtime
  std::string driverSrc = R"(
#include "SyncRst.h"
#include <cassert>
int main() {
  SyncRst s;
  s.do_reset();
  assert(s.count == 0);

  // tick without reset: should increment
  s.rst = 0;
  s.step();
  assert(s.count == 1);

  s.step();
  assert(s.count == 2);

  // assert reset: state must go to 0
  s.rst = 1;
  s.step();
  assert(s.count == 0);

  // release reset and tick: should start incrementing from 0 again
  s.rst = 0;
  s.step();
  assert(s.count == 1);

  return 0;
}
)";

  {
    std::ofstream df("/tmp/hirct_genmodel_syncrst/cmodel/driver.cpp");
    df << driverSrc;
  }

  int compileRc = std::system(
      "c++ -std=c++17 -O0 -Werror "
      "-I/tmp/hirct_genmodel_syncrst/cmodel "
      "/tmp/hirct_genmodel_syncrst/cmodel/SyncRst.cpp "
      "/tmp/hirct_genmodel_syncrst/cmodel/driver.cpp "
      "-o /tmp/hirct_genmodel_syncrst/driver 2>&1");
  ASSERT_EQ(compileRc, 0) << "SyncRst driver must compile";

  int runRc = std::system("/tmp/hirct_genmodel_syncrst/driver");
  EXPECT_EQ(runRc, 0)
      << "arc.state with sync reset: rst=1 must zero state, rst=0 must allow update";

  std::system("rm -rf /tmp/hirct_genmodel_syncrst");
}

TEST_F(CModelEmitterFixture, GenModel_ArcState_EnableGatingRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @inc_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      arc.define @clk_arc(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @EnGate(in %clk : i1, in %en : i1,
                        out count : i8) {
        %clock = arc.call @clk_arc(%clk) : (i1) -> !seq.clock
        %0 = arc.state @inc_arc(%0) clock %clock enable %en latency 1 {names = ["cnt"]} : (i8) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("EnGate");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_engate");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_engate");
  ASSERT_TRUE(ok) << "GenModel::emit failed for EnGate";

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_engate/cmodel "
      "/tmp/hirct_genmodel_engate/cmodel/EnGate.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "EnGate must compile";

  std::string driverSrc = R"(
#include "EnGate.h"
#include <cassert>
int main() {
  EnGate s;
  s.do_reset();
  assert(s.count == 0);

  // enable=1: should increment
  s.en = 1;
  s.step();
  assert(s.count == 1);

  s.step();
  assert(s.count == 2);

  // enable=0: state must hold
  s.en = 0;
  s.step();
  assert(s.count == 2);

  s.step();
  assert(s.count == 2);

  // re-enable
  s.en = 1;
  s.step();
  assert(s.count == 3);

  return 0;
}
)";

  {
    std::ofstream df("/tmp/hirct_genmodel_engate/cmodel/driver.cpp");
    df << driverSrc;
  }

  int compileRc = std::system(
      "c++ -std=c++17 -O0 -Werror "
      "-I/tmp/hirct_genmodel_engate/cmodel "
      "/tmp/hirct_genmodel_engate/cmodel/EnGate.cpp "
      "/tmp/hirct_genmodel_engate/cmodel/driver.cpp "
      "-o /tmp/hirct_genmodel_engate/driver 2>&1");
  ASSERT_EQ(compileRc, 0) << "EnGate driver must compile";

  int runRc = std::system("/tmp/hirct_genmodel_engate/driver");
  EXPECT_EQ(runRc, 0)
      << "arc.state with enable: en=1 must update, en=0 must hold";

  std::system("rm -rf /tmp/hirct_genmodel_engate");
}

TEST_F(CModelEmitterFixture, GenModel_ArcState_EnableResetCombined) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @inc_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      arc.define @clk_arc(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @EnRst(in %clk : i1, in %en : i1, in %rst : i1,
                       out count : i8) {
        %clock = arc.call @clk_arc(%clk) : (i1) -> !seq.clock
        %0 = arc.state @inc_arc(%0) clock %clock enable %en reset %rst latency 1 {names = ["cnt"]} : (i8) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("EnRst");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_enrst");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_enrst");
  ASSERT_TRUE(ok) << "GenModel::emit failed for EnRst";

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_enrst/cmodel "
      "/tmp/hirct_genmodel_enrst/cmodel/EnRst.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "EnRst must compile";

  std::string driverSrc = R"(
#include "EnRst.h"
#include <cassert>
int main() {
  EnRst s;
  s.do_reset();
  assert(s.count == 0);

  // en=1, rst=0: increment
  s.en = 1; s.rst = 0;
  s.step();
  assert(s.count == 1);

  s.step();
  assert(s.count == 2);

  // en=0, rst=0: hold
  s.en = 0; s.rst = 0;
  s.step();
  assert(s.count == 2);

  // en=1, rst=1: reset takes priority
  s.en = 1; s.rst = 1;
  s.step();
  assert(s.count == 0);

  // en=0, rst=1: reset still takes priority (even with enable=0)
  s.en = 0; s.rst = 1;
  s.step();
  assert(s.count == 0);

  // en=1, rst=0: resume from 0
  s.en = 1; s.rst = 0;
  s.step();
  assert(s.count == 1);

  return 0;
}
)";

  {
    std::ofstream df("/tmp/hirct_genmodel_enrst/cmodel/driver.cpp");
    df << driverSrc;
  }

  int compileRc = std::system(
      "c++ -std=c++17 -O0 -Werror "
      "-I/tmp/hirct_genmodel_enrst/cmodel "
      "/tmp/hirct_genmodel_enrst/cmodel/EnRst.cpp "
      "/tmp/hirct_genmodel_enrst/cmodel/driver.cpp "
      "-o /tmp/hirct_genmodel_enrst/driver 2>&1");
  ASSERT_EQ(compileRc, 0) << "EnRst driver must compile";

  int runRc = std::system("/tmp/hirct_genmodel_enrst/driver");
  EXPECT_EQ(runRc, 0)
      << "arc.state with enable+reset: correct priority and gating";

  std::system("rm -rf /tmp/hirct_genmodel_enrst");
}

TEST_F(CModelEmitterFixture, GenModel_MultiResultArcCall_AllResultsBound) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @multi3(%arg0: i8, %arg1: i8) -> (i8, i8, i8) {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        %1 = comb.xor %arg0, %arg1 : i8
        %2 = comb.and %arg0, %arg1 : i8
        arc.output %0, %1, %2 : i8, i8, i8
      }
      hw.module @MultiOut(in %a : i8, in %b : i8,
                          out x : i8, out y : i8, out z : i8) {
        %0:3 = arc.call @multi3(%a, %b) : (i8, i8) -> (i8, i8, i8)
        hw.output %0#0, %0#1, %0#2 : i8, i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("MultiOut");
  ASSERT_TRUE(hwModule);

  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_multiout");
  ASSERT_TRUE(ok) << "GenModel::emit failed for MultiOut";

  std::ifstream cpp_ifs("/tmp/hirct_genmodel_multiout/cmodel/MultiOut.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  // All three outputs must have non-zero expressions
  EXPECT_EQ(cpp_content.find("x = ((0) &"), std::string::npos)
      << "output x (result#0) must not be hardcoded 0";
  EXPECT_EQ(cpp_content.find("y = ((0) &"), std::string::npos)
      << "output y (result#1) must not be hardcoded 0";
  EXPECT_EQ(cpp_content.find("z = ((0) &"), std::string::npos)
      << "output z (result#2) must not be hardcoded 0";

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_multiout/cmodel "
      "/tmp/hirct_genmodel_multiout/cmodel/MultiOut.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "generated code must compile";
}

// ---------------------------------------------------------------------------
// Async reset-like: nested arc.call providing clock+reset to arc.state
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GenModel_ArcState_NestedCallProvidesClockReset) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @split_clk_rst(%arg0: i1, %arg1: i1, %arg2: i1) -> (i1, !seq.clock) {
        %0 = comb.xor %arg0, %arg1 : i1
        %1 = seq.to_clock %arg2
        arc.output %0, %1 : i1, !seq.clock
      }
      arc.define @counter_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      hw.module @NestedClkRst(in %rst_n : i1, in %clk_i : i1,
                              out cnt : i8) {
        %cr:2 = arc.call @split_clk_rst(%rst_n, %true, %clk_i) : (i1, i1, i1) -> (i1, !seq.clock)
        %true = hw.constant true
        %0 = arc.state @counter_arc(%0) clock %cr#1 reset %cr#0 latency 1 {names = ["cnt_reg"]} : (i8) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("NestedClkRst");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_nestedclkrst");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_nestedclkrst");
  ASSERT_TRUE(ok) << "GenModel::emit failed for NestedClkRst";

  std::ifstream cpp_ifs("/tmp/hirct_genmodel_nestedclkrst/cmodel/NestedClkRst.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("next_"), std::string::npos)
      << "step() must contain next-state logic for arc.state";
  EXPECT_NE(cpp_content.find("reg_"), std::string::npos)
      << "step() must reference arc.state register";

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_nestedclkrst/cmodel "
      "/tmp/hirct_genmodel_nestedclkrst/cmodel/NestedClkRst.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "NestedClkRst must compile";

  std::string driverSrc = R"(
#include "NestedClkRst.h"
#include <cassert>
int main() {
  NestedClkRst s;
  s.do_reset();
  assert(s.cnt == 0);

  // rst_n=1 ^ true = 0, so reset signal is 0 -> no reset
  s.rst_n = 1;
  s.step();
  assert(s.cnt == 1);

  s.step();
  assert(s.cnt == 2);

  // rst_n=0 ^ true = 1, so reset signal is 1 -> reset
  s.rst_n = 0;
  s.step();
  assert(s.cnt == 0);

  // release reset
  s.rst_n = 1;
  s.step();
  assert(s.cnt == 1);

  return 0;
}
)";

  {
    std::ofstream df("/tmp/hirct_genmodel_nestedclkrst/cmodel/driver.cpp");
    df << driverSrc;
  }

  int compileRc = std::system(
      "c++ -std=c++17 -O0 -Werror "
      "-I/tmp/hirct_genmodel_nestedclkrst/cmodel "
      "/tmp/hirct_genmodel_nestedclkrst/cmodel/NestedClkRst.cpp "
      "/tmp/hirct_genmodel_nestedclkrst/cmodel/driver.cpp "
      "-o /tmp/hirct_genmodel_nestedclkrst/driver 2>&1");
  ASSERT_EQ(compileRc, 0) << "NestedClkRst driver must compile";

  int runRc = std::system("/tmp/hirct_genmodel_nestedclkrst/driver");
  EXPECT_EQ(runRc, 0)
      << "nested arc.call providing clock+reset to arc.state must work at runtime";

  std::system("rm -rf /tmp/hirct_genmodel_nestedclkrst");
}

// ---------------------------------------------------------------------------
// Boundary: arc.state with enable+reset from nested arc.call results
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GenModel_ArcState_EnableResetFromArcCall) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @ctrl_arc(%arg0: i1, %arg1: i1, %arg2: i1) -> (i1, i1, !seq.clock) {
        %0 = comb.xor %arg0, %arg1 : i1
        %1 = seq.to_clock %arg2
        arc.output %arg1, %0, %1 : i1, i1, !seq.clock
      }
      arc.define @inc_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      hw.module @EnRstFromCall(in %rst_n : i1, in %en : i1, in %clk : i1,
                               out cnt : i8) {
        %ctrl:3 = arc.call @ctrl_arc(%rst_n, %en, %clk) : (i1, i1, i1) -> (i1, i1, !seq.clock)
        %0 = arc.state @inc_arc(%0) clock %ctrl#2 enable %ctrl#0 reset %ctrl#1 latency 1 {names = ["cnt_reg"]} : (i8) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("EnRstFromCall");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_enrstcall");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_enrstcall");
  ASSERT_TRUE(ok) << "GenModel::emit failed for EnRstFromCall";

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_enrstcall/cmodel "
      "/tmp/hirct_genmodel_enrstcall/cmodel/EnRstFromCall.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "EnRstFromCall must compile";

  std::system("rm -rf /tmp/hirct_genmodel_enrstcall");
}

// ---------------------------------------------------------------------------
// Boundary: nonzero reset value is not yet supported -- arc.state init_value
// Current behavior: reset always goes to 0.
// This test documents the boundary and must be updated when nonzero reset
// is supported.
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GenModel_ArcState_NonZeroResetBoundary) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @pass_arc(%arg0: i8) -> i8 {
        arc.output %arg0 : i8
      }
      arc.define @clk_arc(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @NonZeroRst(in %clk : i1, in %rst : i1, in %d : i8,
                            out q : i8) {
        %clock = arc.call @clk_arc(%clk) : (i1) -> !seq.clock
        %0 = arc.state @pass_arc(%d) clock %clock reset %rst latency 1 {names = ["reg0"]} : (i8) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("NonZeroRst");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_nzrst");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_nzrst");
  ASSERT_TRUE(ok) << "GenModel::emit failed for NonZeroRst";

  std::ifstream cpp_ifs("/tmp/hirct_genmodel_nzrst/cmodel/NonZeroRst.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  // BOUNDARY: currently reset always goes to 0 regardless of arc.state
  // initial_value. When nonzero reset is supported, update this test.
  EXPECT_NE(cpp_content.find("= 0;"), std::string::npos)
      << "current boundary: reset always goes to 0";

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_nzrst/cmodel "
      "/tmp/hirct_genmodel_nzrst/cmodel/NonZeroRst.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "NonZeroRst must compile";

  std::system("rm -rf /tmp/hirct_genmodel_nzrst");
}

// ---------------------------------------------------------------------------
// Real RTL regression: ncs_core_evt_log_if (complex reset+state+multi-result)
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GenModel_RealRtl_EvtLogIf_ResetStateRegression) {
  auto fixtureRoot = std::filesystem::path(__FILE__)
                         .parent_path().parent_path().parent_path() /
                     "tests" / "fixtures";
  auto mlirPath = fixtureRoot / "ncs_core_evt_log_if_arc.mlir";
  ASSERT_TRUE(std::filesystem::exists(mlirPath)) << mlirPath.string();
  std::ifstream ifs(mlirPath);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
  auto module = parseInline(content);
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("ncs_core_evt_log_if");
  ASSERT_TRUE(hwModule);

  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_evtlog_rst");
  ASSERT_TRUE(ok) << "GenModel::emit failed for ncs_core_evt_log_if";

  std::ifstream cpp_ifs("/tmp/hirct_genmodel_evtlog_rst/cmodel/ncs_core_evt_log_if.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  // arc.state @ncs_core_evt_log_if_arc_2 has reset %0#0 (XOR of RESET_N_I ^ true)
  // step() must contain conditional reset logic for the state register
  bool hasResetLogic = cpp_content.find("if (") != std::string::npos &&
                       cpp_content.find("reg_") != std::string::npos;
  EXPECT_TRUE(hasResetLogic)
      << "step() must contain conditional reset logic for arc.state with reset";

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_evtlog_rst/cmodel "
      "/tmp/hirct_genmodel_evtlog_rst/cmodel/ncs_core_evt_log_if.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "ncs_core_evt_log_if must compile with reset logic";

  std::system("rm -rf /tmp/hirct_genmodel_evtlog_rst");
}

// ---------------------------------------------------------------------------
// Real RTL regressions: simple_demux, simple_mux, ncs_core_evt_log_if
// These are the baseline guards -- must never break.
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, RealRtl_simple_demux_Baseline) {
  auto fixtureRoot = std::filesystem::path(__FILE__)
                         .parent_path().parent_path().parent_path() /
                     "tests" / "fixtures";
  auto mlirPath = fixtureRoot / "simple_demux_arc.mlir";
  ASSERT_TRUE(std::filesystem::exists(mlirPath)) << mlirPath.string();
  std::ifstream ifs(mlirPath);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
  auto module = parseInline(content);
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("simple_demux");
  ASSERT_TRUE(hwModule);

  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_baseline_demux");
  ASSERT_TRUE(ok) << "simple_demux GenModel must succeed";

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_baseline_demux/cmodel "
      "/tmp/hirct_baseline_demux/cmodel/simple_demux.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "simple_demux must compile";

  std::system("rm -rf /tmp/hirct_baseline_demux");
}

TEST_F(CModelEmitterFixture, RealRtl_simple_mux_Baseline) {
  auto fixtureRoot = std::filesystem::path(__FILE__)
                         .parent_path().parent_path().parent_path() /
                     "tests" / "fixtures";
  auto mlirPath = fixtureRoot / "simple_mux_arc.mlir";
  ASSERT_TRUE(std::filesystem::exists(mlirPath)) << mlirPath.string();
  std::ifstream ifs(mlirPath);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
  auto module = parseInline(content);
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("simple_mux");
  ASSERT_TRUE(hwModule);

  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_baseline_mux");
  ASSERT_TRUE(ok) << "simple_mux GenModel must succeed";

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_baseline_mux/cmodel "
      "/tmp/hirct_baseline_mux/cmodel/simple_mux.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "simple_mux must compile";

  std::system("rm -rf /tmp/hirct_baseline_mux");
}

TEST_F(CModelEmitterFixture, RealRtl_ncs_core_evt_log_if_Baseline) {
  auto fixtureRoot = std::filesystem::path(__FILE__)
                         .parent_path().parent_path().parent_path() /
                     "tests" / "fixtures";
  auto mlirPath = fixtureRoot / "ncs_core_evt_log_if_arc.mlir";
  ASSERT_TRUE(std::filesystem::exists(mlirPath)) << mlirPath.string();
  std::ifstream ifs(mlirPath);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());
  auto module = parseInline(content);
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("ncs_core_evt_log_if");
  ASSERT_TRUE(hwModule);

  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_baseline_evtlog");
  ASSERT_TRUE(ok) << "ncs_core_evt_log_if GenModel must succeed";

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_baseline_evtlog/cmodel "
      "/tmp/hirct_baseline_evtlog/cmodel/ncs_core_evt_log_if.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "ncs_core_evt_log_if must compile";

  std::system("rm -rf /tmp/hirct_baseline_evtlog");
}

// ---------------------------------------------------------------------------
// multi-result arc.call + arc.state combined pattern
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GenModel_MultiResultCall_PlusState_Runtime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @split_arc(%arg0: i1, %arg1: i1) -> (i1, !seq.clock) {
        %0 = comb.xor %arg0, %arg1 : i1
        %1 = seq.to_clock %arg1
        arc.output %0, %1 : i1, !seq.clock
      }
      arc.define @counter_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      hw.module @CallPlusState(in %rst_n : i1, in %clk_i : i1,
                               out cnt : i8, out xor_out : i1) {
        %xor_val:2 = arc.call @split_arc(%rst_n, %clk_i) : (i1, i1) -> (i1, !seq.clock)
        %cnt = arc.state @counter_arc(%cnt) clock %xor_val#1 latency 1 {names = ["cnt_reg"]} : (i8) -> i8
        hw.output %cnt, %xor_val#0 : i8, i1
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("CallPlusState");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_callstate");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_callstate");
  ASSERT_TRUE(ok) << "GenModel::emit failed for CallPlusState";

  std::ifstream cpp_ifs("/tmp/hirct_genmodel_callstate/cmodel/CallPlusState.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_EQ(cpp_content.find("xor_out = ((0) &"), std::string::npos)
      << "xor_out from multi-result arc.call must not be hardcoded 0";

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_callstate/cmodel "
      "/tmp/hirct_genmodel_callstate/cmodel/CallPlusState.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "CallPlusState must compile";

  std::system("rm -rf /tmp/hirct_genmodel_callstate");
}

// ---------------------------------------------------------------------------
// multi-clock domain step + arc.state + reset/enable hardening
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GenModel_MultiClock_ArcState_Reset_DomainStep) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @inc_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      arc.define @clk_arc(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @DualClkRst(in %clk_a : i1, in %clk_b : i1, in %rst : i1,
                            out cnt_a : i8, out cnt_b : i8) {
        %ca = arc.call @clk_arc(%clk_a) : (i1) -> !seq.clock
        %cb = arc.call @clk_arc(%clk_b) : (i1) -> !seq.clock
        %a = arc.state @inc_arc(%a) clock %ca reset %rst latency 1 {names = ["cnt_a"]} : (i8) -> i8
        %b = arc.state @inc_arc(%b) clock %cb reset %rst latency 1 {names = ["cnt_b"]} : (i8) -> i8
        hw.output %a, %b : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("DualClkRst");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_dualclkrst");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_dualclkrst");
  ASSERT_TRUE(ok) << "GenModel::emit failed for DualClkRst";

  std::ifstream cpp_ifs("/tmp/hirct_genmodel_dualclkrst/cmodel/DualClkRst.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("step_clk_a"), std::string::npos)
      << "multi-clock module must emit step_clk_a";
  EXPECT_NE(cpp_content.find("step_clk_b"), std::string::npos)
      << "multi-clock module must emit step_clk_b";

  auto inFunction = [&](const std::string &src, const std::string &fn,
                        const std::string &needle) -> bool {
    auto pos = src.find(fn);
    if (pos == std::string::npos) return false;
    auto end = src.find("\n}\n", pos);
    if (end == std::string::npos) end = src.size();
    return src.substr(pos, end - pos).find(needle) != std::string::npos;
  };

  EXPECT_TRUE(inFunction(cpp_content, "step_clk_a", "if (rst)") ||
              inFunction(cpp_content, "step_clk_a", "if (arc_rst_"))
      << "step_clk_a must guard cnt_a with reset; generated:\n" << cpp_content;

  EXPECT_TRUE(inFunction(cpp_content, "step_clk_b", "if (rst)") ||
              inFunction(cpp_content, "step_clk_b", "if (arc_rst_"))
      << "step_clk_b must guard cnt_b with reset; generated:\n" << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_dualclkrst/cmodel "
      "/tmp/hirct_genmodel_dualclkrst/cmodel/DualClkRst.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "DualClkRst must compile";

  std::system("rm -rf /tmp/hirct_genmodel_dualclkrst");
}

TEST_F(CModelEmitterFixture, GenModel_MultiClock_ArcState_Enable_DomainStep) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @inc_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      arc.define @clk_arc(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @DualClkEn(in %clk_a : i1, in %clk_b : i1, in %en : i1,
                           out cnt_a : i8, out cnt_b : i8) {
        %ca = arc.call @clk_arc(%clk_a) : (i1) -> !seq.clock
        %cb = arc.call @clk_arc(%clk_b) : (i1) -> !seq.clock
        %a = arc.state @inc_arc(%a) clock %ca enable %en latency 1 {names = ["cnt_a"]} : (i8) -> i8
        %b = arc.state @inc_arc(%b) clock %cb enable %en latency 1 {names = ["cnt_b"]} : (i8) -> i8
        hw.output %a, %b : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("DualClkEn");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_dualclken");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_dualclken");
  ASSERT_TRUE(ok) << "GenModel::emit failed for DualClkEn";

  std::ifstream cpp_ifs("/tmp/hirct_genmodel_dualclken/cmodel/DualClkEn.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("step_clk_a"), std::string::npos)
      << "multi-clock module must emit step_clk_a";
  EXPECT_NE(cpp_content.find("step_clk_b"), std::string::npos)
      << "multi-clock module must emit step_clk_b";

  auto inFnEn = [&](const std::string &src, const std::string &fn,
                     const std::string &needle) -> bool {
    auto pos = src.find(fn);
    if (pos == std::string::npos) return false;
    auto end = src.find("\n}\n", pos);
    if (end == std::string::npos) end = src.size();
    return src.substr(pos, end - pos).find(needle) != std::string::npos;
  };

  EXPECT_TRUE(inFnEn(cpp_content, "step_clk_a", "if (en)") ||
              inFnEn(cpp_content, "step_clk_a", "if (arc_en_"))
      << "step_clk_a must guard cnt_a with enable; generated:\n" << cpp_content;

  EXPECT_TRUE(inFnEn(cpp_content, "step_clk_b", "if (en)") ||
              inFnEn(cpp_content, "step_clk_b", "if (arc_en_"))
      << "step_clk_b must guard cnt_b with enable; generated:\n" << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_dualclken/cmodel "
      "/tmp/hirct_genmodel_dualclken/cmodel/DualClkEn.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "DualClkEn must compile";

  std::system("rm -rf /tmp/hirct_genmodel_dualclken");
}

TEST_F(CModelEmitterFixture, GenModel_MultiClock_ArcState_EnableReset_DomainStep) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @inc_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      arc.define @clk_arc(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @DualClkEnRst(in %clk_a : i1, in %clk_b : i1,
                              in %en : i1, in %rst : i1,
                              out cnt_a : i8, out cnt_b : i8) {
        %ca = arc.call @clk_arc(%clk_a) : (i1) -> !seq.clock
        %cb = arc.call @clk_arc(%clk_b) : (i1) -> !seq.clock
        %a = arc.state @inc_arc(%a) clock %ca enable %en reset %rst latency 1 {names = ["cnt_a"]} : (i8) -> i8
        %b = arc.state @inc_arc(%b) clock %cb enable %en reset %rst latency 1 {names = ["cnt_b"]} : (i8) -> i8
        hw.output %a, %b : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("DualClkEnRst");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_dualclkenrst");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_dualclkenrst");
  ASSERT_TRUE(ok) << "GenModel::emit failed for DualClkEnRst";

  std::ifstream cpp_ifs("/tmp/hirct_genmodel_dualclkenrst/cmodel/DualClkEnRst.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("step_clk_a"), std::string::npos);
  EXPECT_NE(cpp_content.find("step_clk_b"), std::string::npos);

  auto inFnER = [&](const std::string &src, const std::string &fn,
                     const std::string &needle) -> bool {
    auto pos = src.find(fn);
    if (pos == std::string::npos) return false;
    auto end = src.find("\n}\n", pos);
    if (end == std::string::npos) end = src.size();
    return src.substr(pos, end - pos).find(needle) != std::string::npos;
  };

  EXPECT_TRUE(inFnER(cpp_content, "step_clk_a", "if (rst)") ||
              inFnER(cpp_content, "step_clk_a", "if (arc_rst_"))
      << "step_clk_a must handle reset+enable for cnt_a; generated:\n" << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_dualclkenrst/cmodel "
      "/tmp/hirct_genmodel_dualclkenrst/cmodel/DualClkEnRst.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "DualClkEnRst must compile";

  std::system("rm -rf /tmp/hirct_genmodel_dualclkenrst");
}

TEST_F(CModelEmitterFixture, GenModel_MultiClock_ArcState_ResetFromArcCall_DomainStep) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @inc_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      arc.define @ctrl_arc(%arg0: i1, %arg1: i1, %arg2: i1) -> (i1, i1, !seq.clock) {
        %0 = comb.xor %arg0, %arg1 : i1
        %1 = seq.to_clock %arg2
        arc.output %arg0, %0, %1 : i1, i1, !seq.clock
      }
      arc.define @clk_arc(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @DualClkCallRst(in %clk_a : i1, in %clk_b : i1,
                                in %rst_n : i1, in %en : i1,
                                out cnt_a : i8, out cnt_b : i8) {
        %ctrl:3 = arc.call @ctrl_arc(%rst_n, %en, %clk_a) : (i1, i1, i1) -> (i1, i1, !seq.clock)
        %cb = arc.call @clk_arc(%clk_b) : (i1) -> !seq.clock
        %a = arc.state @inc_arc(%a) clock %ctrl#2 enable %ctrl#0 reset %ctrl#1 latency 1 {names = ["cnt_a"]} : (i8) -> i8
        %b = arc.state @inc_arc(%b) clock %cb reset %rst_n latency 1 {names = ["cnt_b"]} : (i8) -> i8
        hw.output %a, %b : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("DualClkCallRst");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_dualclkcallrst");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_dualclkcallrst");
  ASSERT_TRUE(ok) << "GenModel::emit failed for DualClkCallRst";

  std::ifstream cpp_ifs("/tmp/hirct_genmodel_dualclkcallrst/cmodel/DualClkCallRst.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("step_clk_a"), std::string::npos);
  EXPECT_NE(cpp_content.find("step_clk_b"), std::string::npos);

  auto inFnCall = [&](const std::string &src, const std::string &fn,
                       const std::string &needle) -> bool {
    auto pos = src.find(fn);
    if (pos == std::string::npos) return false;
    auto end = src.find("\n}\n", pos);
    if (end == std::string::npos) end = src.size();
    return src.substr(pos, end - pos).find(needle) != std::string::npos;
  };

  EXPECT_TRUE(inFnCall(cpp_content, "step_clk_a", "arc_rst_"))
      << "cnt_a reset from arc.call must appear in domain step; generated:\n" << cpp_content;
  EXPECT_TRUE(inFnCall(cpp_content, "step_clk_a", "arc_en_"))
      << "cnt_a enable from arc.call must appear in domain step; generated:\n" << cpp_content;

  EXPECT_TRUE(inFnCall(cpp_content, "step_clk_b", "if (rst_n)") ||
              inFnCall(cpp_content, "step_clk_b", "if (arc_rst_"))
      << "cnt_b reset from port must appear in domain step; generated:\n" << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_dualclkcallrst/cmodel "
      "/tmp/hirct_genmodel_dualclkcallrst/cmodel/DualClkCallRst.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "DualClkCallRst must compile";

  std::system("rm -rf /tmp/hirct_genmodel_dualclkcallrst");
}

TEST_F(CModelEmitterFixture, GenModel_NestedArcCallChain_MultiClock_DomainStep) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @inner_clk(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      arc.define @outer_clk(%arg0: i1) -> !seq.clock {
        %0 = arc.call @inner_clk(%arg0) : (i1) -> !seq.clock
        arc.output %0 : !seq.clock
      }
      arc.define @inc_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      hw.module @NestedDualClkGen(in %clk_a : i1, in %clk_b : i1,
                                   in %rst : i1,
                                   out cnt_a : i8, out cnt_b : i8) {
        %ca = arc.call @outer_clk(%clk_a) : (i1) -> !seq.clock
        %cb = arc.call @outer_clk(%clk_b) : (i1) -> !seq.clock
        %a = arc.state @inc_arc(%a) clock %ca reset %rst latency 1 {names = ["cnt_a"]} : (i8) -> i8
        %b = arc.state @inc_arc(%b) clock %cb reset %rst latency 1 {names = ["cnt_b"]} : (i8) -> i8
        hw.output %a, %b : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("NestedDualClkGen");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_nesteddualclk");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_nesteddualclk");
  ASSERT_TRUE(ok) << "GenModel::emit failed for NestedDualClkGen";

  std::ifstream cpp_ifs("/tmp/hirct_genmodel_nesteddualclk/cmodel/NestedDualClkGen.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("step_clk_a"), std::string::npos)
      << "nested 2-depth arc.call chain must still produce step_clk_a; got:\n" << cpp_content;
  EXPECT_NE(cpp_content.find("step_clk_b"), std::string::npos)
      << "nested 2-depth arc.call chain must still produce step_clk_b; got:\n" << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_nesteddualclk/cmodel "
      "/tmp/hirct_genmodel_nesteddualclk/cmodel/NestedDualClkGen.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "NestedDualClkGen must compile";

  std::system("rm -rf /tmp/hirct_genmodel_nesteddualclk");
}

TEST_F(CModelEmitterFixture, GenModel_NestedMultiResult_ClockChain_Codegen) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @inner_multi(%arg0: i1, %arg1: i1) -> (i1, !seq.clock) {
        %0 = seq.to_clock %arg1
        arc.output %arg0, %0 : i1, !seq.clock
      }
      arc.define @outer_wrap(%arg0: i1, %arg1: i1) -> (i1, !seq.clock) {
        %r:2 = arc.call @inner_multi(%arg0, %arg1) : (i1, i1) -> (i1, !seq.clock)
        arc.output %r#0, %r#1 : i1, !seq.clock
      }
      arc.define @inc_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      hw.module @NestedMultiResGen(in %clk_a : i1, in %clk_b : i1,
                                    in %rst_n : i1,
                                    out cnt_a : i8, out cnt_b : i8) {
        %ctrla:2 = arc.call @outer_wrap(%rst_n, %clk_a) : (i1, i1) -> (i1, !seq.clock)
        %ctrlb:2 = arc.call @outer_wrap(%rst_n, %clk_b) : (i1, i1) -> (i1, !seq.clock)
        %a = arc.state @inc_arc(%a) clock %ctrla#1 reset %ctrla#0 latency 1 {names = ["cnt_a"]} : (i8) -> i8
        %b = arc.state @inc_arc(%b) clock %ctrlb#1 reset %ctrlb#0 latency 1 {names = ["cnt_b"]} : (i8) -> i8
        hw.output %a, %b : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("NestedMultiResGen");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_nestedmultires");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_nestedmultires");
  ASSERT_TRUE(ok) << "GenModel::emit failed for NestedMultiResGen";

  std::ifstream cpp_ifs("/tmp/hirct_genmodel_nestedmultires/cmodel/NestedMultiResGen.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("step_clk_a"), std::string::npos)
      << "nested multi-result arc.call chain must produce step_clk_a; got:\n" << cpp_content;
  EXPECT_NE(cpp_content.find("step_clk_b"), std::string::npos)
      << "nested multi-result arc.call chain must produce step_clk_b; got:\n" << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_nestedmultires/cmodel "
      "/tmp/hirct_genmodel_nestedmultires/cmodel/NestedMultiResGen.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "NestedMultiResGen must compile";

  std::system("rm -rf /tmp/hirct_genmodel_nestedmultires");
}

TEST_F(CModelEmitterFixture, GenModel_CombMuxClockSelection) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @mux_clk(%sel: i1, %a: i1, %b: i1) -> !seq.clock {
        %m = comb.mux %sel, %a, %b : i1
        %c = seq.to_clock %m
        arc.output %c : !seq.clock
      }
      arc.define @pass_clk(%arg0: i1) -> !seq.clock {
        %c = seq.to_clock %arg0
        arc.output %c : !seq.clock
      }
      arc.define @inc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      hw.module @MuxClkGen(in %sel : i1, in %clk_a : i1, in %clk_b : i1,
                            out q_mux : i8, out q_b : i8) {
        %cm = arc.call @mux_clk(%sel, %clk_a, %clk_b) : (i1, i1, i1) -> !seq.clock
        %cb = arc.call @pass_clk(%clk_b) : (i1) -> !seq.clock
        %0 = arc.state @inc(%0) clock %cm latency 1 {names = ["cnt_mux"]} : (i8) -> i8
        %1 = arc.state @inc(%1) clock %cb latency 1 {names = ["cnt_b"]} : (i8) -> i8
        hw.output %0, %1 : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("MuxClkGen");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_muxclk");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_muxclk");
  ASSERT_TRUE(ok);

  auto readFile = [](const std::string &path) -> std::string {
    std::ifstream f(path);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
  };
  std::string cpp_content = readFile("/tmp/hirct_genmodel_muxclk/cmodel/MuxClkGen.cpp");

  EXPECT_NE(cpp_content.find("step_clk_a"), std::string::npos)
      << "mux-selected clock must resolve to the true-side domain clk_a; got:\n"
      << cpp_content;
  EXPECT_NE(cpp_content.find("step_clk_b"), std::string::npos)
      << "direct clk_b state must keep clk_b domain; got:\n" << cpp_content;
  EXPECT_EQ(cpp_content.find("step_sel"), std::string::npos)
      << "selector must never become an invented clock domain; got:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_muxclk/cmodel "
      "/tmp/hirct_genmodel_muxclk/cmodel/MuxClkGen.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "MuxClkGen must compile";

  std::system("rm -rf /tmp/hirct_genmodel_muxclk");
}

TEST_F(CModelEmitterFixture,
       GenModel_MultiResultNestedMuxChain_UsesTrueSideClockDomain) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @multi_mux_chain(%sel0: i1, %sel1: i1, %a: i1, %b: i1,
                                  %c: i1) -> (i1, !seq.clock) {
        %inner = comb.mux %sel1, %a, %b : i1
        %outer = comb.mux %sel0, %inner, %c : i1
        %clk = seq.to_clock %outer
        arc.output %sel0, %clk : i1, !seq.clock
      }
      arc.define @pass_clk(%arg0: i1) -> !seq.clock {
        %clk = seq.to_clock %arg0
        arc.output %clk : !seq.clock
      }
      arc.define @inc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      hw.module @MultiResMuxChainGen(in %sel0 : i1, in %sel1 : i1,
                                     in %clk_a : i1, in %clk_b : i1,
                                     in %clk_c : i1,
                                     out q_mux : i8, out q_c : i8) {
        %ctrl:2 = arc.call @multi_mux_chain(%sel0, %sel1, %clk_a, %clk_b, %clk_c)
                  : (i1, i1, i1, i1, i1) -> (i1, !seq.clock)
        %cc = arc.call @pass_clk(%clk_c) : (i1) -> !seq.clock
        %0 = arc.state @inc(%0) clock %ctrl#1 latency 1 {names = ["cnt_mux"]} : (i8) -> i8
        %1 = arc.state @inc(%1) clock %cc latency 1 {names = ["cnt_c"]} : (i8) -> i8
        hw.output %0, %1 : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("MultiResMuxChainGen");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_multires_muxchain");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_multires_muxchain");
  ASSERT_TRUE(ok);

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_multires_muxchain/cmodel/MultiResMuxChainGen.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("step_clk_a"), std::string::npos)
      << "multi-result + nested mux chain must use true-side clk_a domain; got:\n"
      << cpp_content;
  EXPECT_NE(cpp_content.find("step_clk_c"), std::string::npos)
      << "direct clk_c state must keep clk_c domain; got:\n" << cpp_content;
  EXPECT_EQ(cpp_content.find("step_sel"), std::string::npos)
      << "selector must not appear as a synthetic step domain; got:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_multires_muxchain/cmodel "
      "/tmp/hirct_genmodel_multires_muxchain/cmodel/MultiResMuxChainGen.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "MultiResMuxChainGen must compile";

  std::system("rm -rf /tmp/hirct_genmodel_multires_muxchain");
}

TEST_F(CModelEmitterFixture,
       GenModel_UnsupportedTrueArmDoesNotInventClockDomain) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @bad_mux_chain(%sel0: i1, %sel1: i1, %clk_a: i1,
                                %clk_b: i1) -> !seq.clock {
        %bad = comb.xor %sel0, %sel1 : i1
        %inner = comb.mux %sel1, %clk_a, %clk_b : i1
        %outer = comb.mux %sel0, %bad, %inner : i1
        %clk = seq.to_clock %outer
        arc.output %clk : !seq.clock
      }
      arc.define @pass_clk(%arg0: i1) -> !seq.clock {
        %clk = seq.to_clock %arg0
        arc.output %clk : !seq.clock
      }
      arc.define @inc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      hw.module @BadMuxChainGen(in %sel0 : i1, in %sel1 : i1,
                                in %clk_a : i1, in %clk_b : i1,
                                out q_bad : i8, out q_b : i8) {
        %cbad = arc.call @bad_mux_chain(%sel0, %sel1, %clk_a, %clk_b)
                : (i1, i1, i1, i1) -> !seq.clock
        %cb = arc.call @pass_clk(%clk_b) : (i1) -> !seq.clock
        %0 = arc.state @inc(%0) clock %cbad latency 1 {names = ["cnt_bad"]} : (i8) -> i8
        %1 = arc.state @inc(%1) clock %cb latency 1 {names = ["cnt_b"]} : (i8) -> i8
        hw.output %0, %1 : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("BadMuxChainGen");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_badmuxchain");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_badmuxchain");
  ASSERT_TRUE(ok);

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_badmuxchain/cmodel/BadMuxChainGen.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_EQ(cpp_content.find("step_sel0"), std::string::npos)
      << "unsupported true-arm expression must not invent selector clock domain; got:\n"
      << cpp_content;
  EXPECT_EQ(cpp_content.find("step_sel1"), std::string::npos)
      << "unsupported true-arm expression must not invent selector clock domain; got:\n"
      << cpp_content;
  EXPECT_EQ(cpp_content.find("step_clk_b"), std::string::npos)
      << "once an unsupported mux-chain arm appears, codegen must fall back to a single step() boundary; got:\n"
      << cpp_content;
  EXPECT_NE(cpp_content.find("void BadMuxChainGen::step()"), std::string::npos)
      << "unsupported mux-chain boundary must still emit generic step(); got:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_badmuxchain/cmodel "
      "/tmp/hirct_genmodel_badmuxchain/cmodel/BadMuxChainGen.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "BadMuxChainGen must compile";

  std::system("rm -rf /tmp/hirct_genmodel_badmuxchain");
}

TEST_F(CModelEmitterFixture, GenModel_UnsupportedCombOrClockBoundary) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @CombOrRegBoundary(in %clk_a : i1, in %clk_b : i1, in %d : i8,
                                   out q_bad : i8, out q_b : i8) {
        %bad_i1 = comb.or %clk_a, %clk_b : i1
        %bad_clk = seq.to_clock %bad_i1
        %clk_b_only = seq.to_clock %clk_b
        %reg_bad = seq.compreg %d, %bad_clk : i8
        %reg_b = seq.compreg %d, %clk_b_only : i8
        hw.output %reg_bad, %reg_b : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("CombOrRegBoundary");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_combor_boundary");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_combor_boundary");
  ASSERT_TRUE(ok);

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_combor_boundary/cmodel/CombOrRegBoundary.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_EQ(cpp_content.find("void CombOrRegBoundary::step_clk_a()"),
            std::string::npos)
      << "unsupported comb.or clock must not fall back to operand-0 domain; got:\n"
      << cpp_content;
  EXPECT_EQ(cpp_content.find("void CombOrRegBoundary::step_clk_b()"),
            std::string::npos)
      << "unsupported comb.or clock must force generic boundary instead of per-domain step; got:\n"
      << cpp_content;
  EXPECT_EQ(cpp_content.find("void CombOrRegBoundary::step_()"),
            std::string::npos)
      << "unsupported comb.or clock must not create empty-name step domain; got:\n"
      << cpp_content;
  EXPECT_NE(cpp_content.find("void CombOrRegBoundary::step()"),
            std::string::npos)
      << "unsupported comb.or clock must still emit generic step(); got:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_combor_boundary/cmodel "
      "/tmp/hirct_genmodel_combor_boundary/cmodel/CombOrRegBoundary.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "CombOrRegBoundary must compile";

  std::system("rm -rf /tmp/hirct_genmodel_combor_boundary");
}

TEST_F(CModelEmitterFixture, GenModel_UnsupportedCombAndClockBoundary) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @CombAndRegBoundary(in %clk_a : i1, in %clk_b : i1, in %d : i8,
                                    out q_bad : i8, out q_b : i8) {
        %bad_i1 = comb.and %clk_a, %clk_b : i1
        %bad_clk = seq.to_clock %bad_i1
        %clk_b_only = seq.to_clock %clk_b
        %reg_bad = seq.compreg %d, %bad_clk : i8
        %reg_b = seq.compreg %d, %clk_b_only : i8
        hw.output %reg_bad, %reg_b : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("CombAndRegBoundary");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_comband_boundary");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_comband_boundary");
  ASSERT_TRUE(ok);

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_comband_boundary/cmodel/CombAndRegBoundary.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_EQ(cpp_content.find("void CombAndRegBoundary::step_clk_a()"),
            std::string::npos)
      << "unsupported comb.and clock must not fall back to operand-0 domain; got:\n"
      << cpp_content;
  EXPECT_EQ(cpp_content.find("void CombAndRegBoundary::step_clk_b()"),
            std::string::npos)
      << "unsupported comb.and clock must force generic boundary instead of per-domain step; got:\n"
      << cpp_content;
  EXPECT_EQ(cpp_content.find("void CombAndRegBoundary::step_()"),
            std::string::npos)
      << "unsupported comb.and clock must not create empty-name step domain; got:\n"
      << cpp_content;
  EXPECT_NE(cpp_content.find("void CombAndRegBoundary::step()"),
            std::string::npos)
      << "unsupported comb.and clock must still emit generic step(); got:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_comband_boundary/cmodel "
      "/tmp/hirct_genmodel_comband_boundary/cmodel/CombAndRegBoundary.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "CombAndRegBoundary must compile";

  std::system("rm -rf /tmp/hirct_genmodel_comband_boundary");
}

TEST_F(CModelEmitterFixture, GenModel_UnsupportedCombXorClockBoundary) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @CombXorRegBoundary(in %clk_a : i1, in %clk_b : i1, in %d : i8,
                                    out q_bad : i8, out q_b : i8) {
        %bad_i1 = comb.xor %clk_a, %clk_b : i1
        %bad_clk = seq.to_clock %bad_i1
        %clk_b_only = seq.to_clock %clk_b
        %reg_bad = seq.compreg %d, %bad_clk : i8
        %reg_b = seq.compreg %d, %clk_b_only : i8
        hw.output %reg_bad, %reg_b : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("CombXorRegBoundary");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_combxor_boundary");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_combxor_boundary");
  ASSERT_TRUE(ok);

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_combxor_boundary/cmodel/CombXorRegBoundary.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_EQ(cpp_content.find("void CombXorRegBoundary::step_clk_a()"),
            std::string::npos)
      << "unsupported comb.xor clock must not fall back to operand-0 domain; got:\n"
      << cpp_content;
  EXPECT_EQ(cpp_content.find("void CombXorRegBoundary::step_clk_b()"),
            std::string::npos)
      << "unsupported comb.xor clock must force generic boundary instead of per-domain step; got:\n"
      << cpp_content;
  EXPECT_EQ(cpp_content.find("void CombXorRegBoundary::step_()"),
            std::string::npos)
      << "unsupported comb.xor clock must not create empty-name step domain; got:\n"
      << cpp_content;
  EXPECT_NE(cpp_content.find("void CombXorRegBoundary::step()"),
            std::string::npos)
      << "unsupported comb.xor clock must still emit generic step(); got:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_combxor_boundary/cmodel "
      "/tmp/hirct_genmodel_combxor_boundary/cmodel/CombXorRegBoundary.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "CombXorRegBoundary must compile";

  std::system("rm -rf /tmp/hirct_genmodel_combxor_boundary");
}

TEST_F(CModelEmitterFixture, GenModel_MultiClock_ArcState_RuntimeBehavior) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @inc_arc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      arc.define @clk_arc(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @DualClkRstRun(in %clk_a : i1, in %clk_b : i1, in %rst : i1,
                               out cnt_a : i8, out cnt_b : i8) {
        %ca = arc.call @clk_arc(%clk_a) : (i1) -> !seq.clock
        %cb = arc.call @clk_arc(%clk_b) : (i1) -> !seq.clock
        %a = arc.state @inc_arc(%a) clock %ca reset %rst latency 1 {names = ["cnt_a"]} : (i8) -> i8
        %b = arc.state @inc_arc(%b) clock %cb reset %rst latency 1 {names = ["cnt_b"]} : (i8) -> i8
        hw.output %a, %b : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("DualClkRstRun");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_dualclkrstrun");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_dualclkrstrun");
  ASSERT_TRUE(ok);

  std::string driver = R"(
#include "DualClkRstRun.h"
#include <cassert>
int main() {
  DualClkRstRun dut;
  dut.do_reset();

  // rst=0, step domain A 3 times
  dut.rst = 0;
  dut.step_clk_a(); dut.step_clk_a(); dut.step_clk_a();
  assert(dut.cnt_a == 3 && "cnt_a should be 3 after 3 clk_a steps");

  // step domain B twice
  dut.step_clk_b(); dut.step_clk_b();
  assert(dut.cnt_b == 2 && "cnt_b should be 2 after 2 clk_b steps");

  // assert rst -> cnt_a back to 0
  dut.rst = 1;
  dut.step_clk_a();
  assert(dut.cnt_a == 0 && "cnt_a must be 0 after reset");

  // cnt_b should also reset when stepping its domain
  dut.step_clk_b();
  assert(dut.cnt_b == 0 && "cnt_b must be 0 after reset on clk_b step");

  // release reset and count again
  dut.rst = 0;
  dut.step_clk_a();
  assert(dut.cnt_a == 1 && "cnt_a should resume counting");
  dut.step_clk_b();
  assert(dut.cnt_b == 1 && "cnt_b should resume counting");

  return 0;
}
)";
  {
    std::ofstream drv("/tmp/hirct_genmodel_dualclkrstrun/driver.cpp");
    drv << driver;
  }

  int rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_genmodel_dualclkrstrun/test "
      "-I/tmp/hirct_genmodel_dualclkrstrun/cmodel "
      "/tmp/hirct_genmodel_dualclkrstrun/cmodel/DualClkRstRun.cpp "
      "/tmp/hirct_genmodel_dualclkrstrun/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "DualClkRstRun must compile with driver";

  if (rc == 0) {
    int run_rc = std::system("/tmp/hirct_genmodel_dualclkrstrun/test");
    EXPECT_EQ(run_rc, 0) << "DualClkRstRun runtime assertions failed";
  }

  std::system("rm -rf /tmp/hirct_genmodel_dualclkrstrun");
}

TEST_F(CModelEmitterFixture, GenModel_BoundaryCommentEmittedForCombOr) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @BoundaryCommentOr(in %clk_a : i1, in %clk_b : i1, in %d : i8,
                                   out q : i8) {
        %bad_i1 = comb.or %clk_a, %clk_b : i1
        %bad_clk = seq.to_clock %bad_i1
        %reg = seq.compreg %d, %bad_clk : i8
        hw.output %reg : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("BoundaryCommentOr");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_boundary_comment");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_boundary_comment");
  ASSERT_TRUE(ok);

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_boundary_comment/cmodel/BoundaryCommentOr.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("BOUNDARY"), std::string::npos)
      << "unsupported comb-clock boundary must emit BOUNDARY comment; got:\n"
      << cpp_content;
  EXPECT_NE(cpp_content.find("generic step()"), std::string::npos)
      << "boundary comment must mention generic step(); got:\n"
      << cpp_content;

  std::system("rm -rf /tmp/hirct_genmodel_boundary_comment");
}

} // namespace
