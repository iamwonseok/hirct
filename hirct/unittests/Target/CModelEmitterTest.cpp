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

static std::string getHostCAbiPortableCompileFlags() {
#if defined(__linux__)
  // Linux toolchains may link executables as PIE by default. Compile the
  // temporary Host C ABI test sources accordingly so the link step stays
  // portable without changing the emitted C ABI surface under test.
  return " -fPIE";
#else
  return "";
#endif
}

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

TEST_F(CModelEmitterFixture, EvalClockAggregateIndexedUpdateResetEnable) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @IdxArc(%old: !hw.array<4xi8>, %idx: i2, %val: i8) -> !hw.array<4xi8> {
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %c2 = hw.constant 2 : i2
        %c3 = hw.constant 3 : i2
        %e0 = hw.array_get %old[%c0] : !hw.array<4xi8>, i2
        %e1 = hw.array_get %old[%c1] : !hw.array<4xi8>, i2
        %e2 = hw.array_get %old[%c2] : !hw.array<4xi8>, i2
        %e3 = hw.array_get %old[%c3] : !hw.array<4xi8>, i2
        %sel0 = comb.icmp eq %idx, %c0 : i2
        %sel1 = comb.icmp eq %idx, %c1 : i2
        %sel2 = comb.icmp eq %idx, %c2 : i2
        %sel3 = comb.icmp eq %idx, %c3 : i2
        %n0 = comb.mux %sel0, %val, %e0 : i8
        %n1 = comb.mux %sel1, %val, %e1 : i8
        %n2 = comb.mux %sel2, %val, %e2 : i8
        %n3 = comb.mux %sel3, %val, %e3 : i8
        %result = hw.array_create %n3, %n2, %n1, %n0 : i8
        arc.output %result : !hw.array<4xi8>
      }
      hw.module @IdxRstEnCM(in %clk : !seq.clock, in %rst : i1, in %en : i1,
                            in %idx : i2, in %val : i8,
                            out q0 : i8) {
        %s = arc.state @IdxArc(%s, %idx, %val) clock %clk enable %en reset %rst latency 1
              {names = ["arr"]} : (!hw.array<4xi8>, i2, i8) -> !hw.array<4xi8>
        %c0 = hw.constant 0 : i2
        %q0 = hw.array_get %s[%c0] : !hw.array<4xi8>, i2
        hw.output %q0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("IdxRstEnCM");
  ASSERT_TRUE(hwModule);

  auto model = hirct::semantic::buildModuleModel(*module, "IdxRstEnCM");
  ASSERT_TRUE(succeeded(model));
  ASSERT_EQ(model->aggregateStateVars.size(), 1u);

  const auto &agg = model->aggregateStateVars[0];
  EXPECT_TRUE(agg.hasReset);
  EXPECT_TRUE(agg.hasEnable);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cm_idxrsten";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  auto evalClkPos = artifact.implContent.find("IdxRstEnCM_eval_clk(");
  ASSERT_NE(evalClkPos, std::string::npos)
      << "must have eval_clk function";
  auto afterPos = artifact.implContent.substr(evalClkPos);

  EXPECT_EQ(afterPos.find("/* TODO:"), std::string::npos)
      << "aggregate eval_clock must NOT have TODO marker; should have real "
         "implementation; got:\n" << afterPos;
  EXPECT_NE(afterPos.find("s->arr"), std::string::npos)
      << "aggregate eval_clock must reference s->arr for commit; got:\n"
      << afterPos;
}

TEST_F(CModelEmitterFixture, EvalClockAggregateElementwiseResetEnable) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @EwArc(%old: !hw.array<4xi8>) -> !hw.array<4xi8> {
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %c2 = hw.constant 2 : i2
        %c3 = hw.constant 3 : i2
        %c1_8 = hw.constant 1 : i8
        %e0 = hw.array_get %old[%c0] : !hw.array<4xi8>, i2
        %e1 = hw.array_get %old[%c1] : !hw.array<4xi8>, i2
        %e2 = hw.array_get %old[%c2] : !hw.array<4xi8>, i2
        %e3 = hw.array_get %old[%c3] : !hw.array<4xi8>, i2
        %n0 = comb.add %e0, %c1_8 : i8
        %n1 = comb.add %e1, %c1_8 : i8
        %n2 = comb.add %e2, %c1_8 : i8
        %n3 = comb.add %e3, %c1_8 : i8
        %result = hw.array_create %n3, %n2, %n1, %n0 : i8
        arc.output %result : !hw.array<4xi8>
      }
      hw.module @EwRstEnCM(in %clk : !seq.clock, in %rst : i1, in %en : i1,
                           out q0 : i8) {
        %s = arc.state @EwArc(%s) clock %clk enable %en reset %rst latency 1
              {names = ["ew"]} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        %c0 = hw.constant 0 : i2
        %q0 = hw.array_get %s[%c0] : !hw.array<4xi8>, i2
        hw.output %q0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("EwRstEnCM");
  ASSERT_TRUE(hwModule);

  auto model = hirct::semantic::buildModuleModel(*module, "EwRstEnCM");
  ASSERT_TRUE(succeeded(model));
  ASSERT_EQ(model->aggregateStateVars.size(), 1u);

  const auto &agg = model->aggregateStateVars[0];
  EXPECT_TRUE(agg.hasReset);
  EXPECT_TRUE(agg.hasEnable);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cm_ewrsten";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  auto evalClkPos = artifact.implContent.find("EwRstEnCM_eval_clk(");
  ASSERT_NE(evalClkPos, std::string::npos)
      << "must have eval_clk function";
  auto afterPos = artifact.implContent.substr(evalClkPos);

  EXPECT_EQ(afterPos.find("/* TODO:"), std::string::npos)
      << "aggregate eval_clock must NOT have TODO marker; should have real "
         "implementation; got:\n" << afterPos;
  EXPECT_NE(afterPos.find("s->ew"), std::string::npos)
      << "aggregate eval_clock must reference s->ew for commit; got:\n"
      << afterPos;
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

// ---------------------------------------------------------------------------
// Batch: Nonzero reset / aggregate state + reset/enable codegen hardening
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GenModel_NonzeroResetCodegen) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @inc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      hw.module @NonzeroRstGen(in %clk : i1, in %rst : i1, out q : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @inc(%0) clock %c reset %rst latency 1
              {names = ["cnt"], initial_value = 42 : i8} : (i8) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("NonzeroRstGen");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_nonzero_rst");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_nonzero_rst");
  ASSERT_TRUE(ok);

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_nonzero_rst/cmodel/NonzeroRstGen.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("42"), std::string::npos)
      << "nonzero reset value 42 must appear in generated code; got:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_nonzero_rst/cmodel "
      "/tmp/hirct_genmodel_nonzero_rst/cmodel/NonzeroRstGen.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "NonzeroRstGen must compile";

  std::system("rm -rf /tmp/hirct_genmodel_nonzero_rst");
}

TEST_F(CModelEmitterFixture, GenModel_NonzeroResetRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @inc(%arg0: i8) -> i8 {
        %c1 = hw.constant 1 : i8
        %0 = comb.add %arg0, %c1 : i8
        arc.output %0 : i8
      }
      hw.module @NzRstRun(in %clk : i1, in %rst : i1, out q : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @inc(%0) clock %c reset %rst latency 1
              {names = ["cnt"], initial_value = 42 : i8} : (i8) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("NzRstRun");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_nzrstrun");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_nzrstrun");
  ASSERT_TRUE(ok);

  std::string driver = R"(
#include "NzRstRun.h"
#include <cassert>
int main() {
  NzRstRun dut;
  dut.do_reset();
  // After do_reset, initial value must be 42
  assert(dut.q == 42 && "do_reset must set initial value 42");

  // Step with rst=0 -> should count up from 42
  dut.rst = 0;
  dut.step();
  assert(dut.q == 43 && "first step from 42 should give 43");
  dut.step();
  assert(dut.q == 44 && "second step should give 44");

  // Assert rst=1 -> back to 42
  dut.rst = 1;
  dut.step();
  assert(dut.q == 42 && "reset must restore to 42, not 0");

  // Release and count again
  dut.rst = 0;
  dut.step();
  assert(dut.q == 43 && "count resumes from 42 -> 43");

  return 0;
}
)";
  {
    std::ofstream drv("/tmp/hirct_genmodel_nzrstrun/driver.cpp");
    drv << driver;
  }

  int rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_genmodel_nzrstrun/test "
      "-I/tmp/hirct_genmodel_nzrstrun/cmodel "
      "/tmp/hirct_genmodel_nzrstrun/cmodel/NzRstRun.cpp "
      "/tmp/hirct_genmodel_nzrstrun/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "NzRstRun must compile with driver";

  if (rc == 0) {
    int run_rc = std::system("/tmp/hirct_genmodel_nzrstrun/test");
    EXPECT_EQ(run_rc, 0) << "NzRstRun runtime assertions failed";
  }

  std::system("rm -rf /tmp/hirct_genmodel_nzrstrun");
}

TEST_F(CModelEmitterFixture, GenModel_AggregateStateResetCodegen) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @arr_id(%arg0: !hw.array<4xi8>) -> !hw.array<4xi8> {
        arc.output %arg0 : !hw.array<4xi8>
      }
      hw.module @AggRstCodegen(in %clk : i1, in %rst : i1, in %d : i8,
                               out q : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @arr_id(%0) clock %c reset %rst latency 1
              {names = ["arr"]} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        %idx = hw.constant 0 : i2
        %elem = hw.array_get %0[%idx] : !hw.array<4xi8>, i2
        hw.output %elem : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggRstCodegen");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_aggrst");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_aggrst");
  ASSERT_TRUE(ok);

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_aggrst/cmodel/AggRstCodegen.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("reg_"), std::string::npos)
      << "aggregate state must produce reg_ declaration; got:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_aggrst/cmodel "
      "/tmp/hirct_genmodel_aggrst/cmodel/AggRstCodegen.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "AggRstCodegen must compile";

  std::system("rm -rf /tmp/hirct_genmodel_aggrst");
}

TEST_F(CModelEmitterFixture, GenModel_AggregateStateEnableCodegen) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @arr_id(%arg0: !hw.array<4xi8>) -> !hw.array<4xi8> {
        arc.output %arg0 : !hw.array<4xi8>
      }
      hw.module @AggEnCodegen(in %clk : i1, in %en : i1, in %d : i8,
                              out q : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @arr_id(%0) clock %c enable %en latency 1
              {names = ["arr"]} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        %idx = hw.constant 0 : i2
        %elem = hw.array_get %0[%idx] : !hw.array<4xi8>, i2
        hw.output %elem : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggEnCodegen");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_aggen");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_aggen");
  ASSERT_TRUE(ok);

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_aggen/cmodel/AggEnCodegen.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_aggen/cmodel "
      "/tmp/hirct_genmodel_aggen/cmodel/AggEnCodegen.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "AggEnCodegen must compile";

  std::system("rm -rf /tmp/hirct_genmodel_aggen");
}

TEST_F(CModelEmitterFixture, GenModel_AggregateStateResetEnableCombined) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @arr_id(%arg0: !hw.array<4xi8>) -> !hw.array<4xi8> {
        arc.output %arg0 : !hw.array<4xi8>
      }
      hw.module @AggRstEnCombined(in %clk : i1, in %rst : i1, in %en : i1,
                                  in %d : i8, out q : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @arr_id(%0) clock %c enable %en reset %rst latency 1
              {names = ["arr"]} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        %idx = hw.constant 0 : i2
        %elem = hw.array_get %0[%idx] : !hw.array<4xi8>, i2
        hw.output %elem : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggRstEnCombined");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_aggrstencomb");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_aggrstencomb");
  ASSERT_TRUE(ok);

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_aggrstencomb/cmodel/AggRstEnCombined.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("step"), std::string::npos)
      << "aggregate state with reset+enable must have step logic; got:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_aggrstencomb/cmodel "
      "/tmp/hirct_genmodel_aggrstencomb/cmodel/AggRstEnCombined.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "AggRstEnCombined must compile";

  std::system("rm -rf /tmp/hirct_genmodel_aggrstencomb");
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

// ---------------------------------------------------------------------------
// Batch: Aggregate output port direct codegen
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GenModel_AggregateOutputDirectFromState) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @arr_id(%arg0: !hw.array<4xi8>) -> !hw.array<4xi8> {
        arc.output %arg0 : !hw.array<4xi8>
      }
      hw.module @AggOutState(in %clk : i1, in %rst : i1, in %d : i8,
                             out q : !hw.array<4xi8>) {
        %c = seq.to_clock %clk
        %0 = arc.state @arr_id(%0) clock %c reset %rst latency 1
              {names = ["arr"]} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        hw.output %0 : !hw.array<4xi8>
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggOutState");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_aggout_state");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_aggout_state");
  ASSERT_TRUE(ok) << "emit must succeed; reason: " << gen.last_error_reason();

  std::ifstream h_ifs(
      "/tmp/hirct_genmodel_aggout_state/cmodel/AggOutState.h");
  std::string h_content((std::istreambuf_iterator<char>(h_ifs)),
                        std::istreambuf_iterator<char>());
  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_aggout_state/cmodel/AggOutState.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(h_content.find("q[4]"), std::string::npos)
      << "array output port must be declared as C array, not scalar; header:\n"
      << h_content;

  EXPECT_NE(cpp_content.find("q["), std::string::npos)
      << "eval_comb must assign array output elements; cpp:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_aggout_state/cmodel "
      "/tmp/hirct_genmodel_aggout_state/cmodel/AggOutState.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "aggregate output from state must compile";

  std::system("rm -rf /tmp/hirct_genmodel_aggout_state");
}

TEST_F(CModelEmitterFixture, GenModel_AggregateOutputDirectFromComb) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @AggOutComb(in %a : i8, in %b : i8, in %c : i8, in %d : i8,
                            out q : !hw.array<4xi8>) {
        %arr = hw.array_create %a, %b, %c, %d : i8
        hw.output %arr : !hw.array<4xi8>
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggOutComb");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_aggout_comb");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_aggout_comb");
  ASSERT_TRUE(ok) << "emit must succeed; reason: " << gen.last_error_reason();

  std::ifstream h_ifs(
      "/tmp/hirct_genmodel_aggout_comb/cmodel/AggOutComb.h");
  std::string h_content((std::istreambuf_iterator<char>(h_ifs)),
                        std::istreambuf_iterator<char>());
  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_aggout_comb/cmodel/AggOutComb.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(h_content.find("q[4]"), std::string::npos)
      << "array output port must be declared as C array; header:\n"
      << h_content;

  EXPECT_NE(cpp_content.find("q["), std::string::npos)
      << "eval_comb must produce element-wise output assignment; cpp:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_aggout_comb/cmodel "
      "/tmp/hirct_genmodel_aggout_comb/cmodel/AggOutComb.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "aggregate output from comb must compile";

  std::system("rm -rf /tmp/hirct_genmodel_aggout_comb");
}

// ---------------------------------------------------------------------------
// Batch: Array input port handling in eval_comb (GenModel path)
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GenModel_ArrayInputDirectOutput) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @ArrInOut(in %inp : !hw.array<4xi8>, out q : !hw.array<4xi8>) {
        hw.output %inp : !hw.array<4xi8>
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("ArrInOut");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_arrin_direct");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_arrin_direct");
  ASSERT_TRUE(ok) << "emit must succeed";

  std::ifstream h_ifs(
      "/tmp/hirct_genmodel_arrin_direct/cmodel/ArrInOut.h");
  std::string h_content((std::istreambuf_iterator<char>(h_ifs)),
                        std::istreambuf_iterator<char>());
  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_arrin_direct/cmodel/ArrInOut.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(h_content.find("inp[4]"), std::string::npos)
      << "array input port must be declared as C array; header:\n"
      << h_content;
  EXPECT_NE(h_content.find("q[4]"), std::string::npos)
      << "array output port must be declared as C array; header:\n"
      << h_content;

  EXPECT_NE(cpp_content.find("q["), std::string::npos)
      << "eval_comb must assign array output from array input; cpp:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_arrin_direct/cmodel "
      "/tmp/hirct_genmodel_arrin_direct/cmodel/ArrInOut.cpp 2>&1");
  EXPECT_EQ(rc, 0)
      << "array input -> direct output must produce compilable C++";

  std::system("rm -rf /tmp/hirct_genmodel_arrin_direct");
}

TEST_F(CModelEmitterFixture, GenModel_ArrayInputGetScalarOutput) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @ArrGetScalar(in %inp : !hw.array<4xi8>, out elem : i8) {
        %idx = hw.constant 2 : i2
        %0 = hw.array_get %inp[%idx] : !hw.array<4xi8>, i2
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("ArrGetScalar");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_arrget_scalar");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_arrget_scalar");
  ASSERT_TRUE(ok) << "emit must succeed";

  std::ifstream h_ifs(
      "/tmp/hirct_genmodel_arrget_scalar/cmodel/ArrGetScalar.h");
  std::string h_content((std::istreambuf_iterator<char>(h_ifs)),
                        std::istreambuf_iterator<char>());
  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_arrget_scalar/cmodel/ArrGetScalar.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(h_content.find("inp[4]"), std::string::npos)
      << "array input must be C array; header:\n" << h_content;

  EXPECT_NE(cpp_content.find("elem"), std::string::npos)
      << "eval_comb must assign scalar output; cpp:\n" << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_arrget_scalar/cmodel "
      "/tmp/hirct_genmodel_arrget_scalar/cmodel/ArrGetScalar.cpp 2>&1");
  EXPECT_EQ(rc, 0)
      << "array_get from array input must produce compilable C++";

  std::system("rm -rf /tmp/hirct_genmodel_arrget_scalar");
}

TEST_F(CModelEmitterFixture, GenModel_ArrayInputViaArcCall) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @pass_arr(%arg0: !hw.array<4xi8>) -> !hw.array<4xi8> {
        arc.output %arg0 : !hw.array<4xi8>
      }
      hw.module @ArrViaArc(in %inp : !hw.array<4xi8>, out q : !hw.array<4xi8>) {
        %0 = arc.call @pass_arr(%inp) : (!hw.array<4xi8>) -> !hw.array<4xi8>
        hw.output %0 : !hw.array<4xi8>
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("ArrViaArc");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_arrin_arccall");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_arrin_arccall");
  ASSERT_TRUE(ok) << "emit must succeed";

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_arrin_arccall/cmodel/ArrViaArc.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("q["), std::string::npos)
      << "eval_comb must assign array output via arc.call; cpp:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_arrin_arccall/cmodel "
      "/tmp/hirct_genmodel_arrin_arccall/cmodel/ArrViaArc.cpp 2>&1");
  EXPECT_EQ(rc, 0)
      << "array input via arc.call must produce compilable C++";

  std::system("rm -rf /tmp/hirct_genmodel_arrin_arccall");
}

TEST_F(CModelEmitterFixture, GenModel_ArrayInputArcCallGetScalar) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @get_elem(%arg0: !hw.array<4xi8>, %arg1: i2) -> i8 {
        %0 = hw.array_get %arg0[%arg1] : !hw.array<4xi8>, i2
        arc.output %0 : i8
      }
      hw.module @ArrArcGet(in %inp : !hw.array<4xi8>, out elem : i8) {
        %idx = hw.constant 1 : i2
        %0 = arc.call @get_elem(%inp, %idx) : (!hw.array<4xi8>, i2) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("ArrArcGet");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_arrin_arcget");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_arrin_arcget");
  ASSERT_TRUE(ok) << "emit must succeed";

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_arrin_arcget/cmodel/ArrArcGet.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("elem"), std::string::npos)
      << "eval_comb must assign scalar output from arc.call; cpp:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_arrin_arcget/cmodel "
      "/tmp/hirct_genmodel_arrin_arcget/cmodel/ArrArcGet.cpp 2>&1");
  EXPECT_EQ(rc, 0)
      << "array input -> arc.call(array_get) -> scalar output must compile";

  std::system("rm -rf /tmp/hirct_genmodel_arrin_arcget");
}

TEST_F(CModelEmitterFixture, GenModel_ArrayInputMixedComb) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @ArrMixComb(in %inp : !hw.array<4xi8>, in %x : i8,
                            out q : !hw.array<4xi8>) {
        %arr = hw.array_create %x, %x, %x, %x : i8
        %c0 = hw.constant 0 : i1
        %out = comb.mux %c0, %inp, %arr : !hw.array<4xi8>
        hw.output %out : !hw.array<4xi8>
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("ArrMixComb");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_arrmix");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_arrmix");
  ASSERT_TRUE(ok) << "emit must succeed";

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_arrmix/cmodel/ArrMixComb.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("q["), std::string::npos)
      << "eval_comb must assign array output from mux; cpp:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_arrmix/cmodel "
      "/tmp/hirct_genmodel_arrmix/cmodel/ArrMixComb.cpp 2>&1");
  EXPECT_EQ(rc, 0)
      << "array input + array_create mux must produce compilable C++";

  std::system("rm -rf /tmp/hirct_genmodel_arrmix");
}

TEST_F(CModelEmitterFixture, GenModel_ArrayInputToInstance) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @ArrChild(in %inp : !hw.array<4xi8>, out q : !hw.array<4xi8>) {
        hw.output %inp : !hw.array<4xi8>
      }
      hw.module @ArrParent(in %d : !hw.array<4xi8>, out q : !hw.array<4xi8>) {
        %0 = hw.instance "child0" @ArrChild(inp: %d: !hw.array<4xi8>) -> (q: !hw.array<4xi8>)
        hw.output %0 : !hw.array<4xi8>
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  std::system("rm -rf /tmp/hirct_genmodel_arrin_inst");
  std::system("mkdir -p /tmp/hirct_genmodel_arrin_inst");

  auto childMod =
      module->lookupSymbol<circt::hw::HWModuleOp>("ArrChild");
  ASSERT_TRUE(childMod);
  {
    hirct::GenModel genChild(childMod, *module);
    bool cOk = genChild.emit("/tmp/hirct_genmodel_arrin_inst/ArrChild");
    ASSERT_TRUE(cOk) << "child emit must succeed";
  }

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("ArrParent");
  ASSERT_TRUE(hwModule);
  {
    hirct::GenModel gen(hwModule, *module);
    bool ok = gen.emit("/tmp/hirct_genmodel_arrin_inst/ArrParent");
    ASSERT_TRUE(ok) << "parent emit must succeed";
  }

  // Place child header where parent include expects it (sibling dir layout).
  std::system(
      "mkdir -p /tmp/hirct_genmodel_arrin_inst/ArrParent/cmodel/../ArrChild/cmodel && "
      "cp /tmp/hirct_genmodel_arrin_inst/ArrChild/cmodel/ArrChild.h "
      "/tmp/hirct_genmodel_arrin_inst/ArrParent/ArrChild/cmodel/ArrChild.h");

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_arrin_inst/ArrParent/cmodel/ArrParent.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());
  EXPECT_NE(cpp_content.find("q["), std::string::npos)
      << "eval_comb must assign array output; cpp:\n" << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_arrin_inst/ArrParent/cmodel "
      "/tmp/hirct_genmodel_arrin_inst/ArrParent/cmodel/ArrParent.cpp 2>&1");
  EXPECT_EQ(rc, 0)
      << "array input forwarded to child instance must produce compilable C++";

  std::system("rm -rf /tmp/hirct_genmodel_arrin_inst");
}

TEST_F(CModelEmitterFixture, GenModel_AggregateOutputConstant) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @AggOutConst(out q : !hw.array<4xi8>) {
        %c0 = hw.constant 0 : i8
        %c1 = hw.constant 1 : i8
        %c2 = hw.constant 2 : i8
        %c3 = hw.constant 3 : i8
        %arr = hw.array_create %c0, %c1, %c2, %c3 : i8
        hw.output %arr : !hw.array<4xi8>
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggOutConst");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_aggout_const");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_aggout_const");
  ASSERT_TRUE(ok) << "emit must succeed; reason: " << gen.last_error_reason();

  std::ifstream h_ifs(
      "/tmp/hirct_genmodel_aggout_const/cmodel/AggOutConst.h");
  std::string h_content((std::istreambuf_iterator<char>(h_ifs)),
                        std::istreambuf_iterator<char>());
  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_aggout_const/cmodel/AggOutConst.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(h_content.find("q[4]"), std::string::npos)
      << "constant array output must be array; header:\n" << h_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_aggout_const/cmodel "
      "/tmp/hirct_genmodel_aggout_const/cmodel/AggOutConst.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "constant array output must compile";

  std::system("rm -rf /tmp/hirct_genmodel_aggout_const");
}

// ---------------------------------------------------------------------------
// Batch: IndexedUpdate / ElementwiseUpdate + reset/enable hardening
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GenModel_IndexedUpdateResetRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @IdxArc(%old: !hw.array<4xi8>, %idx: i2, %val: i8) -> !hw.array<4xi8> {
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %c2 = hw.constant 2 : i2
        %c3 = hw.constant 3 : i2
        %e0 = hw.array_get %old[%c0] : !hw.array<4xi8>, i2
        %e1 = hw.array_get %old[%c1] : !hw.array<4xi8>, i2
        %e2 = hw.array_get %old[%c2] : !hw.array<4xi8>, i2
        %e3 = hw.array_get %old[%c3] : !hw.array<4xi8>, i2
        %sel0 = comb.icmp eq %idx, %c0 : i2
        %sel1 = comb.icmp eq %idx, %c1 : i2
        %sel2 = comb.icmp eq %idx, %c2 : i2
        %sel3 = comb.icmp eq %idx, %c3 : i2
        %n0 = comb.mux %sel0, %val, %e0 : i8
        %n1 = comb.mux %sel1, %val, %e1 : i8
        %n2 = comb.mux %sel2, %val, %e2 : i8
        %n3 = comb.mux %sel3, %val, %e3 : i8
        %result = hw.array_create %n3, %n2, %n1, %n0 : i8
        arc.output %result : !hw.array<4xi8>
      }
      hw.module @IdxRst(in %clk : i1, in %rst : i1, in %idx : i2,
                        in %val : i8, out q0 : i8, out q1 : i8) {
        %c = seq.to_clock %clk
        %s = arc.state @IdxArc(%s, %idx, %val) clock %c reset %rst latency 1
              {names = ["arr"]} : (!hw.array<4xi8>, i2, i8) -> !hw.array<4xi8>
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %q0 = hw.array_get %s[%c0] : !hw.array<4xi8>, i2
        %q1 = hw.array_get %s[%c1] : !hw.array<4xi8>, i2
        hw.output %q0, %q1 : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("IdxRst");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_idxrst");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_idxrst");
  ASSERT_TRUE(ok) << "emit must succeed";

  std::ifstream cpp_ifs("/tmp/hirct_idxrst/cmodel/IdxRst.cpp");
  std::string cpp_src((std::istreambuf_iterator<char>(cpp_ifs)),
                      std::istreambuf_iterator<char>());

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_idxrst/cmodel /tmp/hirct_idxrst/cmodel/IdxRst.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "IndexedUpdate+reset must compile; src:\n" << cpp_src;

  // Write runtime driver
  {
    std::ofstream drv("/tmp/hirct_idxrst/driver.cpp");
    drv << R"(
#include "IdxRst.h"
#include <cassert>
#include <cstdio>
int main() {
  IdxRst m;
  m.do_reset();
  // After reset: all elements should be 0
  assert(m.q0 == 0 && "after reset q0==0");
  assert(m.q1 == 0 && "after reset q1==0");

  // Write 42 to index 0, rst=0
  m.rst = 0; m.idx = 0; m.val = 42;
  m.step();
  assert(m.q0 == 42 && "idx0 updated to 42");
  assert(m.q1 == 0 && "idx1 still 0");

  // Write 99 to index 1
  m.idx = 1; m.val = 99;
  m.step();
  assert(m.q0 == 42 && "idx0 still 42");
  assert(m.q1 == 99 && "idx1 updated to 99");

  // Assert reset: all should go to 0
  m.rst = 1;
  m.step();
  assert(m.q0 == 0 && "after rst q0==0");
  assert(m.q1 == 0 && "after rst q1==0");

  printf("PASS: IndexedUpdate+reset\n");
  return 0;
}
)";
  }
  rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_idxrst/driver "
      "-I/tmp/hirct_idxrst/cmodel "
      "/tmp/hirct_idxrst/cmodel/IdxRst.cpp /tmp/hirct_idxrst/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "IndexedUpdate+reset driver must compile";

  if (rc == 0) {
    int run = std::system("/tmp/hirct_idxrst/driver");
    EXPECT_EQ(run, 0) << "IndexedUpdate+reset runtime must pass";
  }

  std::system("rm -rf /tmp/hirct_idxrst");
}

TEST_F(CModelEmitterFixture, GenModel_IndexedUpdateEnableRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @IdxArc(%old: !hw.array<4xi8>, %idx: i2, %val: i8) -> !hw.array<4xi8> {
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %c2 = hw.constant 2 : i2
        %c3 = hw.constant 3 : i2
        %e0 = hw.array_get %old[%c0] : !hw.array<4xi8>, i2
        %e1 = hw.array_get %old[%c1] : !hw.array<4xi8>, i2
        %e2 = hw.array_get %old[%c2] : !hw.array<4xi8>, i2
        %e3 = hw.array_get %old[%c3] : !hw.array<4xi8>, i2
        %sel0 = comb.icmp eq %idx, %c0 : i2
        %sel1 = comb.icmp eq %idx, %c1 : i2
        %sel2 = comb.icmp eq %idx, %c2 : i2
        %sel3 = comb.icmp eq %idx, %c3 : i2
        %n0 = comb.mux %sel0, %val, %e0 : i8
        %n1 = comb.mux %sel1, %val, %e1 : i8
        %n2 = comb.mux %sel2, %val, %e2 : i8
        %n3 = comb.mux %sel3, %val, %e3 : i8
        %result = hw.array_create %n3, %n2, %n1, %n0 : i8
        arc.output %result : !hw.array<4xi8>
      }
      hw.module @IdxEn(in %clk : i1, in %en : i1, in %idx : i2,
                       in %val : i8, out q0 : i8, out q1 : i8) {
        %c = seq.to_clock %clk
        %s = arc.state @IdxArc(%s, %idx, %val) clock %c enable %en latency 1
              {names = ["arr"]} : (!hw.array<4xi8>, i2, i8) -> !hw.array<4xi8>
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %q0 = hw.array_get %s[%c0] : !hw.array<4xi8>, i2
        %q1 = hw.array_get %s[%c1] : !hw.array<4xi8>, i2
        hw.output %q0, %q1 : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("IdxEn");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_idxen");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_idxen");
  ASSERT_TRUE(ok) << "emit must succeed";

  {
    std::ofstream drv("/tmp/hirct_idxen/driver.cpp");
    drv << R"(
#include "IdxEn.h"
#include <cassert>
#include <cstdio>
int main() {
  IdxEn m;
  m.do_reset();
  assert(m.q0 == 0 && m.q1 == 0);

  // en=1: write 42 to idx 0
  m.en = 1; m.idx = 0; m.val = 42;
  m.step();
  assert(m.q0 == 42 && "en=1 idx0 updated");
  assert(m.q1 == 0 && "idx1 still 0");

  // en=0: should hold
  m.en = 0; m.idx = 0; m.val = 99;
  m.step();
  assert(m.q0 == 42 && "en=0 q0 held");
  assert(m.q1 == 0 && "en=0 q1 held");

  // en=1: write 77 to idx 1
  m.en = 1; m.idx = 1; m.val = 77;
  m.step();
  assert(m.q0 == 42 && "idx0 still 42");
  assert(m.q1 == 77 && "en=1 idx1 updated");

  printf("PASS: IndexedUpdate+enable\n");
  return 0;
}
)";
  }
  int rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_idxen/driver "
      "-I/tmp/hirct_idxen/cmodel "
      "/tmp/hirct_idxen/cmodel/IdxEn.cpp /tmp/hirct_idxen/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "IndexedUpdate+enable driver must compile";

  if (rc == 0) {
    int run = std::system("/tmp/hirct_idxen/driver");
    EXPECT_EQ(run, 0) << "IndexedUpdate+enable runtime must pass";
  }

  std::system("rm -rf /tmp/hirct_idxen");
}

TEST_F(CModelEmitterFixture, GenModel_IndexedUpdateResetEnableRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @IdxArc(%old: !hw.array<4xi8>, %idx: i2, %val: i8) -> !hw.array<4xi8> {
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %c2 = hw.constant 2 : i2
        %c3 = hw.constant 3 : i2
        %e0 = hw.array_get %old[%c0] : !hw.array<4xi8>, i2
        %e1 = hw.array_get %old[%c1] : !hw.array<4xi8>, i2
        %e2 = hw.array_get %old[%c2] : !hw.array<4xi8>, i2
        %e3 = hw.array_get %old[%c3] : !hw.array<4xi8>, i2
        %sel0 = comb.icmp eq %idx, %c0 : i2
        %sel1 = comb.icmp eq %idx, %c1 : i2
        %sel2 = comb.icmp eq %idx, %c2 : i2
        %sel3 = comb.icmp eq %idx, %c3 : i2
        %n0 = comb.mux %sel0, %val, %e0 : i8
        %n1 = comb.mux %sel1, %val, %e1 : i8
        %n2 = comb.mux %sel2, %val, %e2 : i8
        %n3 = comb.mux %sel3, %val, %e3 : i8
        %result = hw.array_create %n3, %n2, %n1, %n0 : i8
        arc.output %result : !hw.array<4xi8>
      }
      hw.module @IdxRstEn(in %clk : i1, in %rst : i1, in %en : i1,
                          in %idx : i2, in %val : i8,
                          out q0 : i8, out q1 : i8) {
        %c = seq.to_clock %clk
        %s = arc.state @IdxArc(%s, %idx, %val) clock %c enable %en reset %rst latency 1
              {names = ["arr"]} : (!hw.array<4xi8>, i2, i8) -> !hw.array<4xi8>
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %q0 = hw.array_get %s[%c0] : !hw.array<4xi8>, i2
        %q1 = hw.array_get %s[%c1] : !hw.array<4xi8>, i2
        hw.output %q0, %q1 : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("IdxRstEn");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_idxrsten");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_idxrsten");
  ASSERT_TRUE(ok) << "emit must succeed";

  {
    std::ofstream drv("/tmp/hirct_idxrsten/driver.cpp");
    drv << R"(
#include "IdxRstEn.h"
#include <cassert>
#include <cstdio>
int main() {
  IdxRstEn m;
  m.do_reset();
  assert(m.q0 == 0 && m.q1 == 0);

  // en=1, rst=0: write 42 to idx 0
  m.rst = 0; m.en = 1; m.idx = 0; m.val = 42;
  m.step();
  assert(m.q0 == 42 && "idx0 updated");

  // en=0: hold
  m.en = 0; m.idx = 0; m.val = 99;
  m.step();
  assert(m.q0 == 42 && "en=0 hold");

  // rst=1 takes priority over en=1
  m.rst = 1; m.en = 1; m.idx = 0; m.val = 55;
  m.step();
  assert(m.q0 == 0 && "rst beats en");
  assert(m.q1 == 0 && "rst beats en");

  // rst=0, en=1: write again
  m.rst = 0; m.en = 1; m.idx = 1; m.val = 88;
  m.step();
  assert(m.q0 == 0 && "idx0 still 0");
  assert(m.q1 == 88 && "idx1 updated");

  printf("PASS: IndexedUpdate+reset+enable\n");
  return 0;
}
)";
  }
  int rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_idxrsten/driver "
      "-I/tmp/hirct_idxrsten/cmodel "
      "/tmp/hirct_idxrsten/cmodel/IdxRstEn.cpp "
      "/tmp/hirct_idxrsten/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "IndexedUpdate+reset+enable driver must compile";

  if (rc == 0) {
    int run = std::system("/tmp/hirct_idxrsten/driver");
    EXPECT_EQ(run, 0) << "IndexedUpdate+reset+enable runtime must pass";
  }

  std::system("rm -rf /tmp/hirct_idxrsten");
}

TEST_F(CModelEmitterFixture, GenModel_ElementwiseUpdateResetRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @EwArc(%old: !hw.array<4xi8>) -> !hw.array<4xi8> {
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %c2 = hw.constant 2 : i2
        %c3 = hw.constant 3 : i2
        %c1_8 = hw.constant 1 : i8
        %e0 = hw.array_get %old[%c0] : !hw.array<4xi8>, i2
        %e1 = hw.array_get %old[%c1] : !hw.array<4xi8>, i2
        %e2 = hw.array_get %old[%c2] : !hw.array<4xi8>, i2
        %e3 = hw.array_get %old[%c3] : !hw.array<4xi8>, i2
        %n0 = comb.add %e0, %c1_8 : i8
        %n1 = comb.add %e1, %c1_8 : i8
        %n2 = comb.add %e2, %c1_8 : i8
        %n3 = comb.add %e3, %c1_8 : i8
        %result = hw.array_create %n3, %n2, %n1, %n0 : i8
        arc.output %result : !hw.array<4xi8>
      }
      hw.module @EwRst(in %clk : i1, in %rst : i1,
                       out q0 : i8, out q1 : i8) {
        %c = seq.to_clock %clk
        %s = arc.state @EwArc(%s) clock %c reset %rst latency 1
              {names = ["ew"]} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %q0 = hw.array_get %s[%c0] : !hw.array<4xi8>, i2
        %q1 = hw.array_get %s[%c1] : !hw.array<4xi8>, i2
        hw.output %q0, %q1 : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("EwRst");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_ewrst");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_ewrst");
  ASSERT_TRUE(ok) << "emit must succeed";

  {
    std::ofstream drv("/tmp/hirct_ewrst/driver.cpp");
    drv << R"(
#include "EwRst.h"
#include <cassert>
#include <cstdio>
int main() {
  EwRst m;
  m.do_reset();
  assert(m.q0 == 0 && m.q1 == 0);

  // Each step increments all elements by 1
  m.rst = 0;
  m.step();
  assert(m.q0 == 1 && "tick1: 0+1=1");
  assert(m.q1 == 1 && "tick1: 0+1=1");

  m.step();
  assert(m.q0 == 2 && "tick2: 1+1=2");
  assert(m.q1 == 2 && "tick2: 1+1=2");

  // Assert reset
  m.rst = 1;
  m.step();
  assert(m.q0 == 0 && "rst: back to 0");
  assert(m.q1 == 0 && "rst: back to 0");

  // Continue incrementing
  m.rst = 0;
  m.step();
  assert(m.q0 == 1 && "after rst tick: 0+1=1");

  printf("PASS: ElementwiseUpdate+reset\n");
  return 0;
}
)";
  }
  int rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_ewrst/driver "
      "-I/tmp/hirct_ewrst/cmodel "
      "/tmp/hirct_ewrst/cmodel/EwRst.cpp /tmp/hirct_ewrst/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "ElementwiseUpdate+reset driver must compile";

  if (rc == 0) {
    int run = std::system("/tmp/hirct_ewrst/driver");
    EXPECT_EQ(run, 0) << "ElementwiseUpdate+reset runtime must pass";
  }

  std::system("rm -rf /tmp/hirct_ewrst");
}

TEST_F(CModelEmitterFixture, GenModel_ElementwiseUpdateEnableRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @EwArc(%old: !hw.array<4xi8>) -> !hw.array<4xi8> {
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %c2 = hw.constant 2 : i2
        %c3 = hw.constant 3 : i2
        %c1_8 = hw.constant 1 : i8
        %e0 = hw.array_get %old[%c0] : !hw.array<4xi8>, i2
        %e1 = hw.array_get %old[%c1] : !hw.array<4xi8>, i2
        %e2 = hw.array_get %old[%c2] : !hw.array<4xi8>, i2
        %e3 = hw.array_get %old[%c3] : !hw.array<4xi8>, i2
        %n0 = comb.add %e0, %c1_8 : i8
        %n1 = comb.add %e1, %c1_8 : i8
        %n2 = comb.add %e2, %c1_8 : i8
        %n3 = comb.add %e3, %c1_8 : i8
        %result = hw.array_create %n3, %n2, %n1, %n0 : i8
        arc.output %result : !hw.array<4xi8>
      }
      hw.module @EwEn(in %clk : i1, in %en : i1,
                      out q0 : i8, out q1 : i8) {
        %c = seq.to_clock %clk
        %s = arc.state @EwArc(%s) clock %c enable %en latency 1
              {names = ["ew"]} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %q0 = hw.array_get %s[%c0] : !hw.array<4xi8>, i2
        %q1 = hw.array_get %s[%c1] : !hw.array<4xi8>, i2
        hw.output %q0, %q1 : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("EwEn");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_ewen");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_ewen");
  ASSERT_TRUE(ok) << "emit must succeed";

  {
    std::ofstream drv("/tmp/hirct_ewen/driver.cpp");
    drv << R"(
#include "EwEn.h"
#include <cassert>
#include <cstdio>
int main() {
  EwEn m;
  m.do_reset();
  assert(m.q0 == 0 && m.q1 == 0);

  // en=1: increment
  m.en = 1;
  m.step();
  assert(m.q0 == 1 && "en=1 tick1");
  assert(m.q1 == 1 && "en=1 tick1");

  // en=0: hold
  m.en = 0;
  m.step();
  assert(m.q0 == 1 && "en=0 hold");
  assert(m.q1 == 1 && "en=0 hold");

  // en=1: increment again
  m.en = 1;
  m.step();
  assert(m.q0 == 2 && "en=1 tick2");
  assert(m.q1 == 2 && "en=1 tick2");

  printf("PASS: ElementwiseUpdate+enable\n");
  return 0;
}
)";
  }
  int rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_ewen/driver "
      "-I/tmp/hirct_ewen/cmodel "
      "/tmp/hirct_ewen/cmodel/EwEn.cpp /tmp/hirct_ewen/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "ElementwiseUpdate+enable driver must compile";

  if (rc == 0) {
    int run = std::system("/tmp/hirct_ewen/driver");
    EXPECT_EQ(run, 0) << "ElementwiseUpdate+enable runtime must pass";
  }

  std::system("rm -rf /tmp/hirct_ewen");
}

TEST_F(CModelEmitterFixture, GenModel_ElementwiseUpdateResetEnableRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @EwArc(%old: !hw.array<4xi8>) -> !hw.array<4xi8> {
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %c2 = hw.constant 2 : i2
        %c3 = hw.constant 3 : i2
        %c1_8 = hw.constant 1 : i8
        %e0 = hw.array_get %old[%c0] : !hw.array<4xi8>, i2
        %e1 = hw.array_get %old[%c1] : !hw.array<4xi8>, i2
        %e2 = hw.array_get %old[%c2] : !hw.array<4xi8>, i2
        %e3 = hw.array_get %old[%c3] : !hw.array<4xi8>, i2
        %n0 = comb.add %e0, %c1_8 : i8
        %n1 = comb.add %e1, %c1_8 : i8
        %n2 = comb.add %e2, %c1_8 : i8
        %n3 = comb.add %e3, %c1_8 : i8
        %result = hw.array_create %n3, %n2, %n1, %n0 : i8
        arc.output %result : !hw.array<4xi8>
      }
      hw.module @EwRstEn(in %clk : i1, in %rst : i1, in %en : i1,
                         out q0 : i8, out q1 : i8) {
        %c = seq.to_clock %clk
        %s = arc.state @EwArc(%s) clock %c enable %en reset %rst latency 1
              {names = ["ew"]} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %q0 = hw.array_get %s[%c0] : !hw.array<4xi8>, i2
        %q1 = hw.array_get %s[%c1] : !hw.array<4xi8>, i2
        hw.output %q0, %q1 : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("EwRstEn");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_ewrsten");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_ewrsten");
  ASSERT_TRUE(ok) << "emit must succeed";

  {
    std::ofstream drv("/tmp/hirct_ewrsten/driver.cpp");
    drv << R"(
#include "EwRstEn.h"
#include <cassert>
#include <cstdio>
int main() {
  EwRstEn m;
  m.do_reset();
  assert(m.q0 == 0 && m.q1 == 0);

  // en=1: increment
  m.rst = 0; m.en = 1;
  m.step();
  assert(m.q0 == 1 && "en=1 tick1");

  // en=0: hold
  m.en = 0;
  m.step();
  assert(m.q0 == 1 && "en=0 hold");

  // en=1: increment again
  m.en = 1;
  m.step();
  assert(m.q0 == 2 && "en=1 tick2");

  // rst=1 beats en=1
  m.rst = 1; m.en = 1;
  m.step();
  assert(m.q0 == 0 && "rst beats en");
  assert(m.q1 == 0 && "rst beats en");

  // resume
  m.rst = 0; m.en = 1;
  m.step();
  assert(m.q0 == 1 && "after rst resume");

  printf("PASS: ElementwiseUpdate+reset+enable\n");
  return 0;
}
)";
  }
  int rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_ewrsten/driver "
      "-I/tmp/hirct_ewrsten/cmodel "
      "/tmp/hirct_ewrsten/cmodel/EwRstEn.cpp "
      "/tmp/hirct_ewrsten/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "ElementwiseUpdate+reset+enable driver must compile";

  if (rc == 0) {
    int run = std::system("/tmp/hirct_ewrsten/driver");
    EXPECT_EQ(run, 0) << "ElementwiseUpdate+reset+enable runtime must pass";
  }

  std::system("rm -rf /tmp/hirct_ewrsten");
}

// ---------------------------------------------------------------------------
// Batch: Aggregate nonzero initial value
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, GenModel_AggregateStateNonzeroResetCodegen) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @arr_id(%arg0: !hw.array<4xi8>) -> !hw.array<4xi8> {
        arc.output %arg0 : !hw.array<4xi8>
      }
      hw.module @AggNzRstCodegen(in %clk : i1, in %rst : i1, in %d : i8,
                                 out q : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @arr_id(%0) clock %c reset %rst latency 1
              {names = ["arr"], initial_value = 117901063 : i32} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        %idx = hw.constant 0 : i2
        %elem = hw.array_get %0[%idx] : !hw.array<4xi8>, i2
        hw.output %elem : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggNzRstCodegen");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_aggnzrst");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_aggnzrst");
  ASSERT_TRUE(ok);

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_aggnzrst/cmodel/AggNzRstCodegen.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("static_cast<uint8_t>(7)"), std::string::npos)
      << "aggregate nonzero reset value 7 must appear in do_reset; got:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_aggnzrst/cmodel "
      "/tmp/hirct_genmodel_aggnzrst/cmodel/AggNzRstCodegen.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "AggNzRstCodegen must compile";

  std::system("rm -rf /tmp/hirct_genmodel_aggnzrst");
}

TEST_F(CModelEmitterFixture, GenModel_AggregateStateNonzeroResetRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @arr_id(%arg0: !hw.array<4xi8>) -> !hw.array<4xi8> {
        arc.output %arg0 : !hw.array<4xi8>
      }
      hw.module @AggNzRstRun(in %clk : i1, in %rst : i1, in %d : !hw.array<4xi8>,
                             out q0 : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @arr_id(%d) clock %c reset %rst latency 1
              {names = ["arr"], initial_value = 117901063 : i32} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        %idx = hw.constant 0 : i2
        %elem = hw.array_get %0[%idx] : !hw.array<4xi8>, i2
        hw.output %elem : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("AggNzRstRun");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_aggnzrstrun");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_aggnzrstrun");
  ASSERT_TRUE(ok);

  {
    std::ofstream drv("/tmp/hirct_genmodel_aggnzrstrun/driver.cpp");
    drv << R"(
#include "AggNzRstRun.h"
#include <cassert>
#include <cstdio>
int main() {
  AggNzRstRun dut;
  dut.do_reset();
  // After do_reset, each element must be 7
  assert(dut.q0 == 7 && "do_reset must init aggregate elements to 7");

  // Step with rst=1 -> still 7
  dut.rst = 1;
  dut.step();
  assert(dut.q0 == 7 && "reset active must keep elements at 7");

  // Release reset, feed different data
  dut.rst = 0;
  dut.d[0] = 10; dut.d[1] = 20; dut.d[2] = 30; dut.d[3] = 40;
  dut.step();
  assert(dut.q0 == 10 && "after rst release, data must propagate");

  // Re-assert reset -> back to 7
  dut.rst = 1;
  dut.step();
  assert(dut.q0 == 7 && "re-reset must restore to 7, not 0");

  printf("PASS: AggregateStateNonzeroReset\n");
  return 0;
}
)";
  }

  int rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_genmodel_aggnzrstrun/test "
      "-I/tmp/hirct_genmodel_aggnzrstrun/cmodel "
      "/tmp/hirct_genmodel_aggnzrstrun/cmodel/AggNzRstRun.cpp "
      "/tmp/hirct_genmodel_aggnzrstrun/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "AggNzRstRun must compile with driver";

  if (rc == 0) {
    int run_rc = std::system("/tmp/hirct_genmodel_aggnzrstrun/test");
    EXPECT_EQ(run_rc, 0) << "AggNzRstRun runtime assertions failed";
  }

  std::system("rm -rf /tmp/hirct_genmodel_aggnzrstrun");
}

TEST_F(CModelEmitterFixture, GenModel_AggregateStateNonzeroResetEnableRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @arr_id(%arg0: !hw.array<4xi8>) -> !hw.array<4xi8> {
        arc.output %arg0 : !hw.array<4xi8>
      }
      hw.module @AggNzRstEnRun(in %clk : i1, in %rst : i1, in %en : i1,
                               in %d : !hw.array<4xi8>, out q0 : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @arr_id(%d) clock %c enable %en reset %rst latency 1
              {names = ["arr"], initial_value = 84215045 : i32} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        %idx = hw.constant 0 : i2
        %elem = hw.array_get %0[%idx] : !hw.array<4xi8>, i2
        hw.output %elem : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggNzRstEnRun");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_aggnzrsten");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_aggnzrsten");
  ASSERT_TRUE(ok);

  {
    std::ofstream drv("/tmp/hirct_genmodel_aggnzrsten/driver.cpp");
    drv << R"(
#include "AggNzRstEnRun.h"
#include <cassert>
#include <cstdio>
int main() {
  AggNzRstEnRun dut;
  dut.do_reset();
  // After do_reset, each element must be 5
  assert(dut.q0 == 5 && "do_reset must init aggregate elements to 5");

  // rst=1 en=1 -> reset wins
  dut.rst = 1; dut.en = 1;
  dut.d[0] = 99; dut.d[1] = 99; dut.d[2] = 99; dut.d[3] = 99;
  dut.step();
  assert(dut.q0 == 5 && "reset must beat enable");

  // rst=0 en=1 -> data propagates
  dut.rst = 0; dut.en = 1;
  dut.d[0] = 10; dut.d[1] = 20; dut.d[2] = 30; dut.d[3] = 40;
  dut.step();
  assert(dut.q0 == 10 && "enable without reset must propagate data");

  // rst=0 en=0 -> holds
  dut.en = 0;
  dut.d[0] = 77; dut.d[1] = 77; dut.d[2] = 77; dut.d[3] = 77;
  dut.step();
  assert(dut.q0 == 10 && "en=0 must hold previous value");

  // rst=1 en=0 -> reset wins
  dut.rst = 1; dut.en = 0;
  dut.step();
  assert(dut.q0 == 5 && "reset must restore to 5 even with en=0");

  printf("PASS: AggregateStateNonzeroResetEnable\n");
  return 0;
}
)";
  }

  int rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_genmodel_aggnzrsten/test "
      "-I/tmp/hirct_genmodel_aggnzrsten/cmodel "
      "/tmp/hirct_genmodel_aggnzrsten/cmodel/AggNzRstEnRun.cpp "
      "/tmp/hirct_genmodel_aggnzrsten/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "AggNzRstEnRun must compile with driver";

  if (rc == 0) {
    int run_rc = std::system("/tmp/hirct_genmodel_aggnzrsten/test");
    EXPECT_EQ(run_rc, 0) << "AggNzRstEnRun runtime assertions failed";
  }

  std::system("rm -rf /tmp/hirct_genmodel_aggnzrsten");
}

// ---------------------------------------------------------------------------
// Large-width aggregate (>128-bit total) nonzero init tests
// 17 x i8 = 136 bits, every element = 0xAB = 171
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture,
       GenModel_AggregateStateNonzeroResetLargeWidthCodegen) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @arr17_id(%arg0: !hw.array<17xi8>) -> !hw.array<17xi8> {
        arc.output %arg0 : !hw.array<17xi8>
      }
      hw.module @AggNzRstLargeCodegen(in %clk : i1, in %rst : i1,
                                      out q : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @arr17_id(%0) clock %c reset %rst latency 1
              {names = ["arr"], initial_value = 58416474095415694810088967901698373430187 : i136} : (!hw.array<17xi8>) -> !hw.array<17xi8>
        %idx = hw.constant 0 : i5
        %elem = hw.array_get %0[%idx] : !hw.array<17xi8>, i5
        hw.output %elem : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggNzRstLargeCodegen");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_aggnzrstlg");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_aggnzrstlg");
  ASSERT_TRUE(ok);

  std::ifstream cpp_ifs(
      "/tmp/hirct_genmodel_aggnzrstlg/cmodel/AggNzRstLargeCodegen.cpp");
  std::string cpp_content((std::istreambuf_iterator<char>(cpp_ifs)),
                          std::istreambuf_iterator<char>());

  EXPECT_NE(cpp_content.find("static_cast<uint8_t>(171)"), std::string::npos)
      << "large aggregate nonzero reset value 171 (0xAB) must appear in "
         "do_reset; got:\n"
      << cpp_content;

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_genmodel_aggnzrstlg/cmodel "
      "/tmp/hirct_genmodel_aggnzrstlg/cmodel/AggNzRstLargeCodegen.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "AggNzRstLargeCodegen must compile";

  std::system("rm -rf /tmp/hirct_genmodel_aggnzrstlg");
}

TEST_F(CModelEmitterFixture,
       GenModel_AggregateStateNonzeroResetLargeWidthRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @arr17_id(%arg0: !hw.array<17xi8>) -> !hw.array<17xi8> {
        arc.output %arg0 : !hw.array<17xi8>
      }
      hw.module @AggNzRstLargeRun(in %clk : i1, in %rst : i1,
                                  in %d : !hw.array<17xi8>, out q0 : i8,
                                  out q16 : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @arr17_id(%d) clock %c reset %rst latency 1
              {names = ["arr"], initial_value = 58416474095415694810088967901698373430187 : i136} : (!hw.array<17xi8>) -> !hw.array<17xi8>
        %idx0 = hw.constant 0 : i5
        %idx16 = hw.constant 16 : i5
        %e0 = hw.array_get %0[%idx0] : !hw.array<17xi8>, i5
        %e16 = hw.array_get %0[%idx16] : !hw.array<17xi8>, i5
        hw.output %e0, %e16 : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggNzRstLargeRun");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_aggnzrstlgrun");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_aggnzrstlgrun");
  ASSERT_TRUE(ok);

  {
    std::ofstream drv("/tmp/hirct_genmodel_aggnzrstlgrun/driver.cpp");
    drv << R"DRV(
#include "AggNzRstLargeRun.h"
#include <cassert>
#include <cstdio>
int main() {
  AggNzRstLargeRun dut;
  dut.do_reset();
  assert(dut.q0 == 171 && "do_reset: elem[0] must be 171");
  assert(dut.q16 == 171 && "do_reset: elem[16] must be 171");

  dut.rst = 1;
  dut.step();
  assert(dut.q0 == 171 && "reset active: elem[0] must be 171");
  assert(dut.q16 == 171 && "reset active: elem[16] must be 171");

  dut.rst = 0;
  for (int i = 0; i < 17; ++i) dut.d[i] = 42;
  dut.step();
  assert(dut.q0 == 42 && "after rst release: data must propagate");

  dut.rst = 1;
  dut.step();
  assert(dut.q0 == 171 && "re-reset: elem[0] must restore to 171");
  assert(dut.q16 == 171 && "re-reset: elem[16] must restore to 171");

  printf("PASS: AggregateStateNonzeroResetLargeWidth\n");
  return 0;
}
)DRV";
  }

  int rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_genmodel_aggnzrstlgrun/test "
      "-I/tmp/hirct_genmodel_aggnzrstlgrun/cmodel "
      "/tmp/hirct_genmodel_aggnzrstlgrun/cmodel/AggNzRstLargeRun.cpp "
      "/tmp/hirct_genmodel_aggnzrstlgrun/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "AggNzRstLargeRun must compile with driver";

  if (rc == 0) {
    int run_rc = std::system("/tmp/hirct_genmodel_aggnzrstlgrun/test");
    EXPECT_EQ(run_rc, 0) << "AggNzRstLargeRun runtime assertions failed";
  }

  std::system("rm -rf /tmp/hirct_genmodel_aggnzrstlgrun");
}

TEST_F(CModelEmitterFixture,
       GenModel_AggregateStateNonzeroResetLargeWidthEnableRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @arr17_id(%arg0: !hw.array<17xi8>) -> !hw.array<17xi8> {
        arc.output %arg0 : !hw.array<17xi8>
      }
      hw.module @AggNzRstLargeEnRun(in %clk : i1, in %rst : i1, in %en : i1,
                                    in %d : !hw.array<17xi8>, out q0 : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @arr17_id(%d) clock %c enable %en reset %rst latency 1
              {names = ["arr"], initial_value = 58416474095415694810088967901698373430187 : i136} : (!hw.array<17xi8>) -> !hw.array<17xi8>
        %idx = hw.constant 0 : i5
        %elem = hw.array_get %0[%idx] : !hw.array<17xi8>, i5
        hw.output %elem : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggNzRstLargeEnRun");
  ASSERT_TRUE(hwModule);

  std::system("mkdir -p /tmp/hirct_genmodel_aggnzrstlgen");
  hirct::GenModel gen(hwModule, *module);
  bool ok = gen.emit("/tmp/hirct_genmodel_aggnzrstlgen");
  ASSERT_TRUE(ok);

  {
    std::ofstream drv("/tmp/hirct_genmodel_aggnzrstlgen/driver.cpp");
    drv << R"(
#include "AggNzRstLargeEnRun.h"
#include <cassert>
#include <cstdio>
int main() {
  AggNzRstLargeEnRun dut;
  dut.do_reset();
  assert(dut.q0 == 171 && "do_reset: elem[0] must be 171");

  dut.rst = 1; dut.en = 1;
  for (int i = 0; i < 17; ++i) dut.d[i] = 99;
  dut.step();
  assert(dut.q0 == 171 && "reset beats enable");

  dut.rst = 0; dut.en = 1;
  for (int i = 0; i < 17; ++i) dut.d[i] = 55;
  dut.step();
  assert(dut.q0 == 55 && "enable without reset propagates data");

  dut.en = 0;
  for (int i = 0; i < 17; ++i) dut.d[i] = 77;
  dut.step();
  assert(dut.q0 == 55 && "en=0 holds value");

  dut.rst = 1; dut.en = 0;
  dut.step();
  assert(dut.q0 == 171 && "reset restores 171 with en=0");

  printf("PASS: AggregateStateNonzeroResetLargeWidthEnable\n");
  return 0;
}
)";
  }

  int rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_genmodel_aggnzrstlgen/test "
      "-I/tmp/hirct_genmodel_aggnzrstlgen/cmodel "
      "/tmp/hirct_genmodel_aggnzrstlgen/cmodel/AggNzRstLargeEnRun.cpp "
      "/tmp/hirct_genmodel_aggnzrstlgen/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "AggNzRstLargeEnRun must compile with driver";

  if (rc == 0) {
    int run_rc = std::system("/tmp/hirct_genmodel_aggnzrstlgen/test");
    EXPECT_EQ(run_rc, 0) << "AggNzRstLargeEnRun runtime assertions failed";
  }

  std::system("rm -rf /tmp/hirct_genmodel_aggnzrstlgen");
}

// ---------------------------------------------------------------------------
// CModelEmitter: Aggregate nonzero init in _initialize and eval_clock reset
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, CModelEmitter_AggregateNonzeroInitInitialize) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @arr_id(%arg0: !hw.array<4xi8>) -> !hw.array<4xi8> {
        arc.output %arg0 : !hw.array<4xi8>
      }
      hw.module @AggNzInitCM(in %clk : i1, in %rst : i1, in %d : i8,
                             out q : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @arr_id(%0) clock %c reset %rst latency 1
              {names = ["arr"], initial_value = 117901063 : i32} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        %idx = hw.constant 0 : i2
        %elem = hw.array_get %0[%idx] : !hw.array<4xi8>, i2
        hw.output %elem : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggNzInitCM");
  ASSERT_TRUE(hwModule);

  auto model = hirct::semantic::buildModuleModel(*module, "AggNzInitCM");
  ASSERT_TRUE(succeeded(model));

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cm_aggnzinit";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  // _initialize must set arr[0..3] to element-sliced values from 117901063
  // 117901063 = 0x07070707 -> each element is 7
  EXPECT_NE(artifact.implContent.find("arr[0] = (uint8_t)7"),
            std::string::npos)
      << "_initialize must set arr[0] to 7; got:\n"
      << artifact.implContent;
  EXPECT_NE(artifact.implContent.find("arr[3] = (uint8_t)7"),
            std::string::npos)
      << "_initialize must set arr[3] to 7; got:\n"
      << artifact.implContent;
}

TEST_F(CModelEmitterFixture, CModelEmitter_EvalClockAggregateNonzeroResetEnable) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @AggNzEwArc(%old: !hw.array<4xi8>) -> !hw.array<4xi8> {
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %c2 = hw.constant 2 : i2
        %c3 = hw.constant 3 : i2
        %c1_8 = hw.constant 1 : i8
        %e0 = hw.array_get %old[%c0] : !hw.array<4xi8>, i2
        %e1 = hw.array_get %old[%c1] : !hw.array<4xi8>, i2
        %e2 = hw.array_get %old[%c2] : !hw.array<4xi8>, i2
        %e3 = hw.array_get %old[%c3] : !hw.array<4xi8>, i2
        %n0 = comb.add %e0, %c1_8 : i8
        %n1 = comb.add %e1, %c1_8 : i8
        %n2 = comb.add %e2, %c1_8 : i8
        %n3 = comb.add %e3, %c1_8 : i8
        %result = hw.array_create %n3, %n2, %n1, %n0 : i8
        arc.output %result : !hw.array<4xi8>
      }
      hw.module @AggNzRstEnCM(in %clk : i1, in %rst : i1, in %en : i1,
                               out q : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @AggNzEwArc(%0) clock %c enable %en reset %rst latency 1
              {names = ["arr"], initial_value = 84215045 : i32} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        %idx = hw.constant 0 : i2
        %elem = hw.array_get %0[%idx] : !hw.array<4xi8>, i2
        hw.output %elem : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggNzRstEnCM");
  ASSERT_TRUE(hwModule);

  auto model = hirct::semantic::buildModuleModel(*module, "AggNzRstEnCM");
  ASSERT_TRUE(succeeded(model));

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cm_aggnzrsten";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  // eval_clock reset branch must use nonzero init, not zero
  // 84215045 = 0x05050505 -> each element is 5
  auto evalPos = artifact.implContent.find("AggNzRstEnCM_eval_clk(");
  ASSERT_NE(evalPos, std::string::npos) << "must have eval_clk function";
  auto evalBody = artifact.implContent.substr(evalPos);

  EXPECT_NE(evalBody.find("(uint8_t)5"), std::string::npos)
      << "eval_clock reset branch must use nonzero init value 5; got:\n"
      << evalBody;

  auto rstIfPos = evalBody.find("if (s->input_rst)");
  ASSERT_NE(rstIfPos, std::string::npos) << "must have reset if-branch";
  auto resetBranch = evalBody.substr(rstIfPos,
      evalBody.find("} else", rstIfPos) - rstIfPos);
  EXPECT_EQ(resetBranch.find("= (uint8_t)0;"), std::string::npos)
      << "eval_clock reset branch must NOT assign zero for nonzero-init aggregate; got:\n"
      << resetBranch;
}

TEST_F(CModelEmitterFixture, CModelEmitter_AggregateNonzeroInitRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @AggNzRtArc(%old: !hw.array<4xi8>) -> !hw.array<4xi8> {
        %c0 = hw.constant 0 : i2
        %c1 = hw.constant 1 : i2
        %c2 = hw.constant 2 : i2
        %c3 = hw.constant 3 : i2
        %c1_8 = hw.constant 1 : i8
        %e0 = hw.array_get %old[%c0] : !hw.array<4xi8>, i2
        %e1 = hw.array_get %old[%c1] : !hw.array<4xi8>, i2
        %e2 = hw.array_get %old[%c2] : !hw.array<4xi8>, i2
        %e3 = hw.array_get %old[%c3] : !hw.array<4xi8>, i2
        %n0 = comb.add %e0, %c1_8 : i8
        %n1 = comb.add %e1, %c1_8 : i8
        %n2 = comb.add %e2, %c1_8 : i8
        %n3 = comb.add %e3, %c1_8 : i8
        %result = hw.array_create %n3, %n2, %n1, %n0 : i8
        arc.output %result : !hw.array<4xi8>
      }
      hw.module @AggNzRtCM(in %clk : i1, in %rst : i1, in %en : i1,
                            out q0 : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @AggNzRtArc(%0) clock %c enable %en reset %rst latency 1
              {names = ["arr"], initial_value = 84215045 : i32} : (!hw.array<4xi8>) -> !hw.array<4xi8>
        %idx = hw.constant 0 : i2
        %elem = hw.array_get %0[%idx] : !hw.array<4xi8>, i2
        hw.output %elem : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggNzRtCM");
  ASSERT_TRUE(hwModule);

  auto model = hirct::semantic::buildModuleModel(*module, "AggNzRtCM");
  ASSERT_TRUE(succeeded(model));

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cm_aggnzrt";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_cm_aggnzrt");
  {
    std::ofstream hf("/tmp/hirct_cm_aggnzrt/AggNzRtCM.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_cm_aggnzrt/AggNzRtCM.cpp");
    cf << artifact.implContent;
  }
  {
    std::ofstream drv("/tmp/hirct_cm_aggnzrt/driver.cpp");
    drv << R"(
#include "AggNzRtCM.h"
#include <cassert>
#include <cstdio>
int main() {
  AggNzRtCM_state s;
  AggNzRtCM_initialize(&s);

  // After _initialize, arr[0] must be 5 (not 0)
  assert(s.arr[0] == 5 && "initialize must set arr[0] to 5");
  assert(s.arr[1] == 5 && "initialize must set arr[1] to 5");
  assert(s.arr[2] == 5 && "initialize must set arr[2] to 5");
  assert(s.arr[3] == 5 && "initialize must set arr[3] to 5");

  // eval_clk with rst=1 -> reset to init values
  s.input_rst = 1;
  s.input_en = 1;
  AggNzRtCM_eval_clk(&s);
  assert(s.arr[0] == 5 && "reset must restore arr[0] to 5");

  // eval_clk with rst=0 en=0 -> hold
  s.input_rst = 0;
  s.input_en = 0;
  AggNzRtCM_eval_clk(&s);
  assert(s.arr[0] == 5 && "en=0 must hold arr[0]");

  // Re-assert reset
  s.input_rst = 1;
  AggNzRtCM_eval_clk(&s);
  assert(s.arr[0] == 5 && "re-reset must restore to 5, not 0");

  printf("PASS: CModelEmitter AggregateNonzeroInit\n");
  return 0;
}
)";
  }

  int rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_cm_aggnzrt/test "
      "-I/tmp/hirct_cm_aggnzrt "
      "/tmp/hirct_cm_aggnzrt/AggNzRtCM.cpp "
      "/tmp/hirct_cm_aggnzrt/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "AggNzRtCM must compile with driver";

  if (rc == 0) {
    int run_rc = std::system("/tmp/hirct_cm_aggnzrt/test");
    EXPECT_EQ(run_rc, 0) << "AggNzRtCM runtime assertions failed";
  }

  std::system("rm -rf /tmp/hirct_cm_aggnzrt");
}

// ---------------------------------------------------------------------------
// Batch: aggregate indirect reset/enable from arc.call
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, CModelEmitter_EvalClockAggregateResetFromArcCall) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @ctrl_arc(%arg0: i1, %arg1: i1) -> (i1, !seq.clock) {
        %0 = comb.xor %arg0, %arg1 : i1
        %1 = seq.to_clock %arg1
        arc.output %0, %1 : i1, !seq.clock
      }
      arc.define @agg_inc(%old: !hw.array<3xi8>) -> !hw.array<3xi8> {
        %c0 = hw.constant 0 : i2
        %c1_2 = hw.constant 1 : i2
        %c2 = hw.constant 2 : i2
        %c1_8 = hw.constant 1 : i8
        %e0 = hw.array_get %old[%c0] : !hw.array<3xi8>, i2
        %e1 = hw.array_get %old[%c1_2] : !hw.array<3xi8>, i2
        %e2 = hw.array_get %old[%c2] : !hw.array<3xi8>, i2
        %n0 = comb.add %e0, %c1_8 : i8
        %n1 = comb.add %e1, %c1_8 : i8
        %n2 = comb.add %e2, %c1_8 : i8
        %result = hw.array_create %n2, %n1, %n0 : i8
        arc.output %result : !hw.array<3xi8>
      }
      hw.module @AggRstCall(in %rst_n : i1, in %clk_i : i1,
                            out q0 : i8) {
        %ctrl:2 = arc.call @ctrl_arc(%rst_n, %clk_i) : (i1, i1) -> (i1, !seq.clock)
        %s = arc.state @agg_inc(%s) clock %ctrl#1 reset %ctrl#0 latency 1
              {names = ["arr"]} : (!hw.array<3xi8>) -> !hw.array<3xi8>
        %c0 = hw.constant 0 : i2
        %q0 = hw.array_get %s[%c0] : !hw.array<3xi8>, i2
        hw.output %q0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggRstCall");
  ASSERT_TRUE(hwModule);

  auto model = hirct::semantic::buildModuleModel(*module, "AggRstCall");
  ASSERT_TRUE(succeeded(model));
  ASSERT_EQ(model->aggregateStateVars.size(), 1u);

  const auto &agg = model->aggregateStateVars[0];
  EXPECT_TRUE(agg.hasReset);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cm_aggrstcall";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  auto evalClkPos = artifact.implContent.find("AggRstCall_eval_clk_i(");
  ASSERT_NE(evalClkPos, std::string::npos)
      << "must have eval_clk_i function; impl:\n" << artifact.implContent;
  auto afterPos = artifact.implContent.substr(evalClkPos);

  EXPECT_NE(afterPos.find("if ("), std::string::npos)
      << "aggregate eval_clock must have reset branch from arc.call; got:\n"
      << afterPos;
  EXPECT_NE(afterPos.find("s->arr"), std::string::npos)
      << "aggregate eval_clock must reference s->arr for commit; got:\n"
      << afterPos;
  EXPECT_EQ(afterPos.find("/* unresolved"), std::string::npos)
      << "reset signal must be resolved (no unresolved markers); got:\n"
      << afterPos;
}

TEST_F(CModelEmitterFixture, CModelEmitter_EvalClockAggregateEnableFromArcCall) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @en_arc(%arg0: i1, %arg1: i1) -> (i1, !seq.clock) {
        %0 = comb.and %arg0, %arg1 : i1
        %1 = seq.to_clock %arg1
        arc.output %0, %1 : i1, !seq.clock
      }
      arc.define @agg_inc2(%old: !hw.array<2xi8>) -> !hw.array<2xi8> {
        %c0 = hw.constant 0 : i1
        %c1_1 = hw.constant 1 : i1
        %c1_8 = hw.constant 1 : i8
        %e0 = hw.array_get %old[%c0] : !hw.array<2xi8>, i1
        %e1 = hw.array_get %old[%c1_1] : !hw.array<2xi8>, i1
        %n0 = comb.add %e0, %c1_8 : i8
        %n1 = comb.add %e1, %c1_8 : i8
        %result = hw.array_create %n1, %n0 : i8
        arc.output %result : !hw.array<2xi8>
      }
      hw.module @AggEnCall(in %en_i : i1, in %clk_i : i1,
                           out q0 : i8) {
        %ctrl:2 = arc.call @en_arc(%en_i, %clk_i) : (i1, i1) -> (i1, !seq.clock)
        %s = arc.state @agg_inc2(%s) clock %ctrl#1 enable %ctrl#0 latency 1
              {names = ["ew"]} : (!hw.array<2xi8>) -> !hw.array<2xi8>
        %c0 = hw.constant 0 : i1
        %q0 = hw.array_get %s[%c0] : !hw.array<2xi8>, i1
        hw.output %q0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggEnCall");
  ASSERT_TRUE(hwModule);

  auto model = hirct::semantic::buildModuleModel(*module, "AggEnCall");
  ASSERT_TRUE(succeeded(model));
  ASSERT_EQ(model->aggregateStateVars.size(), 1u);

  const auto &agg = model->aggregateStateVars[0];
  EXPECT_TRUE(agg.hasEnable);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cm_aggencall";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  auto evalClkPos = artifact.implContent.find("AggEnCall_eval_clk_i(");
  ASSERT_NE(evalClkPos, std::string::npos)
      << "must have eval_clk_i function; impl:\n" << artifact.implContent;
  auto afterPos = artifact.implContent.substr(evalClkPos);

  EXPECT_NE(afterPos.find("if ("), std::string::npos)
      << "aggregate eval_clock must have enable branch from arc.call; got:\n"
      << afterPos;
  EXPECT_NE(afterPos.find("s->ew"), std::string::npos)
      << "aggregate eval_clock must reference s->ew for commit; got:\n"
      << afterPos;
  EXPECT_EQ(afterPos.find("/* unresolved"), std::string::npos)
      << "enable signal must be resolved (no unresolved markers); got:\n"
      << afterPos;
}

TEST_F(CModelEmitterFixture, CModelEmitter_AggregateIndirectResetEnableRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @ctrl_arc3(%arg0: i1, %arg1: i1, %arg2: i1) -> (i1, i1, !seq.clock) {
        %1 = seq.to_clock %arg2
        arc.output %arg1, %arg0, %1 : i1, i1, !seq.clock
      }
      arc.define @agg_inc3(%old: !hw.array<2xi8>) -> !hw.array<2xi8> {
        %c0 = hw.constant 0 : i1
        %c1_1 = hw.constant 1 : i1
        %c1_8 = hw.constant 1 : i8
        %e0 = hw.array_get %old[%c0] : !hw.array<2xi8>, i1
        %e1 = hw.array_get %old[%c1_1] : !hw.array<2xi8>, i1
        %n0 = comb.add %e0, %c1_8 : i8
        %n1 = comb.add %e1, %c1_8 : i8
        %result = hw.array_create %n1, %n0 : i8
        arc.output %result : !hw.array<2xi8>
      }
      hw.module @AggIndirRE(in %rst_n : i1, in %en : i1, in %clk_i : i1,
                            out q0 : i8, out q1 : i8) {
        %ctrl:3 = arc.call @ctrl_arc3(%rst_n, %en, %clk_i) : (i1, i1, i1) -> (i1, i1, !seq.clock)
        %s = arc.state @agg_inc3(%s) clock %ctrl#2 enable %ctrl#0 reset %ctrl#1 latency 1
              {names = ["ew"]} : (!hw.array<2xi8>) -> !hw.array<2xi8>
        %c0 = hw.constant 0 : i1
        %c1 = hw.constant 1 : i1
        %q0 = hw.array_get %s[%c0] : !hw.array<2xi8>, i1
        %q1 = hw.array_get %s[%c1] : !hw.array<2xi8>, i1
        hw.output %q0, %q1 : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggIndirRE");
  ASSERT_TRUE(hwModule);

  auto model = hirct::semantic::buildModuleModel(*module, "AggIndirRE");
  ASSERT_TRUE(succeeded(model));
  ASSERT_EQ(model->aggregateStateVars.size(), 1u);

  const auto &agg = model->aggregateStateVars[0];
  EXPECT_TRUE(agg.hasReset);
  EXPECT_TRUE(agg.hasEnable);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cm_aggindirre";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_cm_aggindirre");
  {
    std::ofstream hf("/tmp/hirct_cm_aggindirre/AggIndirRE.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_cm_aggindirre/AggIndirRE.cpp");
    cf << artifact.implContent;
  }

  int syntaxRc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_cm_aggindirre "
      "/tmp/hirct_cm_aggindirre/AggIndirRE.cpp 2>&1");
  ASSERT_EQ(syntaxRc, 0)
      << "AggIndirRE must compile; impl:\n" << artifact.implContent;

  auto rtEvalPos = artifact.implContent.find("eval_clk_i(");
  ASSERT_NE(rtEvalPos, std::string::npos)
      << "must have eval_clk_i; impl:\n" << artifact.implContent;
  auto rtAfter = artifact.implContent.substr(rtEvalPos);
  EXPECT_NE(rtAfter.find("if ("), std::string::npos)
      << "runtime test: eval_clk_i must have reset/enable branch; got:\n"
      << rtAfter;

  std::string driverSrc = R"(
#include "AggIndirRE.h"
#include <cstdio>
#include <cassert>
int main() {
  AggIndirRE_state s;
  AggIndirRE_initialize(&s);
  assert(s.ew[0] == 0 && s.ew[1] == 0);

  // rst_n=0, en=1 -> ctrl#1=rst_n=0 (no reset), ctrl#0=en=1 (enable) -> increment
  s.input_rst_n = 0;
  s.input_en = 1;
  s.input_clk_i = 0;
  AggIndirRE_eval_comb(&s);
  AggIndirRE_eval_clk_i(&s);
  assert(s.ew[0] == 1 && s.ew[1] == 1);

  // rst_n=0, en=0 -> no reset, no enable -> hold
  s.input_en = 0;
  AggIndirRE_eval_comb(&s);
  AggIndirRE_eval_clk_i(&s);
  assert(s.ew[0] == 1 && s.ew[1] == 1);

  // rst_n=0, en=1 -> no reset, enable -> increment
  s.input_en = 1;
  AggIndirRE_eval_comb(&s);
  AggIndirRE_eval_clk_i(&s);
  assert(s.ew[0] == 2 && s.ew[1] == 2);

  // rst_n=1, en=1 -> ctrl#1=1 (reset fires, reset > enable)
  s.input_rst_n = 1;
  s.input_en = 1;
  AggIndirRE_eval_comb(&s);
  AggIndirRE_eval_clk_i(&s);
  assert(s.ew[0] == 0 && s.ew[1] == 0);

  // rst_n=0, en=1 -> reset released, enable active -> increment
  s.input_rst_n = 0;
  AggIndirRE_eval_comb(&s);
  AggIndirRE_eval_clk_i(&s);
  assert(s.ew[0] == 1 && s.ew[1] == 1);

  return 0;
}
)";

  {
    std::ofstream df("/tmp/hirct_cm_aggindirre/driver.cpp");
    df << driverSrc;
  }

  int compileRc = std::system(
      "c++ -std=c++17 -O0 -Werror "
      "-I/tmp/hirct_cm_aggindirre "
      "/tmp/hirct_cm_aggindirre/AggIndirRE.cpp "
      "/tmp/hirct_cm_aggindirre/driver.cpp "
      "-o /tmp/hirct_cm_aggindirre/test 2>&1");
  ASSERT_EQ(compileRc, 0)
      << "AggIndirRE driver must compile; impl:\n" << artifact.implContent;

  if (compileRc == 0) {
    int runRc = std::system("/tmp/hirct_cm_aggindirre/test");
    EXPECT_EQ(runRc, 0)
        << "AggIndirRE runtime: indirect reset/enable from arc.call must work";
  }

  std::system("rm -rf /tmp/hirct_cm_aggindirre");
}

// ---------------------------------------------------------------------------
// CModelEmitter: Large-width aggregate (>128-bit total) nonzero init coverage
// 17 x i8 = 136 bits, every element = 0xAB = 171
// packed decimal = 58416474095415694810088967901698373430187
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture,
       CModelEmitter_AggregateNonzeroInitLargeWidthInitialize) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @arr17_id(%arg0: !hw.array<17xi8>) -> !hw.array<17xi8> {
        arc.output %arg0 : !hw.array<17xi8>
      }
      hw.module @AggNzLgInitCM(in %clk : i1, in %rst : i1,
                                out q : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @arr17_id(%0) clock %c reset %rst latency 1
              {names = ["arr"], initial_value = 58416474095415694810088967901698373430187 : i136} : (!hw.array<17xi8>) -> !hw.array<17xi8>
        %idx = hw.constant 0 : i5
        %elem = hw.array_get %0[%idx] : !hw.array<17xi8>, i5
        hw.output %elem : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggNzLgInitCM");
  ASSERT_TRUE(hwModule);

  auto model =
      hirct::semantic::buildModuleModel(*module, "AggNzLgInitCM");
  ASSERT_TRUE(succeeded(model));

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cm_aggnzlginit";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.implContent.find("arr[0] = (uint8_t)171"),
            std::string::npos)
      << "_initialize must set arr[0] to 171 (0xAB); got:\n"
      << artifact.implContent;
  EXPECT_NE(artifact.implContent.find("arr[16] = (uint8_t)171"),
            std::string::npos)
      << "_initialize must set arr[16] to 171 (0xAB); got:\n"
      << artifact.implContent;

  for (unsigned k = 0; k < 17; ++k) {
    std::string needle =
        "arr[" + std::to_string(k) + "] = (uint8_t)171";
    EXPECT_NE(artifact.implContent.find(needle), std::string::npos)
        << "_initialize must set arr[" << k << "] to 171; got:\n"
        << artifact.implContent;
  }
}

TEST_F(CModelEmitterFixture,
       CModelEmitter_EvalClockAggregateNonzeroInitLargeWidthReset) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @AggNzLgEwArc(%old: !hw.array<17xi8>) -> !hw.array<17xi8> {
        %c0 = hw.constant 0 : i5
        %c1 = hw.constant 1 : i5
        %c2 = hw.constant 2 : i5
        %c3 = hw.constant 3 : i5
        %c4 = hw.constant 4 : i5
        %c5 = hw.constant 5 : i5
        %c6 = hw.constant 6 : i5
        %c7 = hw.constant 7 : i5
        %c8 = hw.constant 8 : i5
        %c9 = hw.constant 9 : i5
        %c10 = hw.constant 10 : i5
        %c11 = hw.constant 11 : i5
        %c12 = hw.constant 12 : i5
        %c13 = hw.constant 13 : i5
        %c14 = hw.constant 14 : i5
        %c15 = hw.constant 15 : i5
        %c16 = hw.constant 16 : i5
        %e0 = hw.array_get %old[%c0] : !hw.array<17xi8>, i5
        %e1 = hw.array_get %old[%c1] : !hw.array<17xi8>, i5
        %e2 = hw.array_get %old[%c2] : !hw.array<17xi8>, i5
        %e3 = hw.array_get %old[%c3] : !hw.array<17xi8>, i5
        %e4 = hw.array_get %old[%c4] : !hw.array<17xi8>, i5
        %e5 = hw.array_get %old[%c5] : !hw.array<17xi8>, i5
        %e6 = hw.array_get %old[%c6] : !hw.array<17xi8>, i5
        %e7 = hw.array_get %old[%c7] : !hw.array<17xi8>, i5
        %e8 = hw.array_get %old[%c8] : !hw.array<17xi8>, i5
        %e9 = hw.array_get %old[%c9] : !hw.array<17xi8>, i5
        %e10 = hw.array_get %old[%c10] : !hw.array<17xi8>, i5
        %e11 = hw.array_get %old[%c11] : !hw.array<17xi8>, i5
        %e12 = hw.array_get %old[%c12] : !hw.array<17xi8>, i5
        %e13 = hw.array_get %old[%c13] : !hw.array<17xi8>, i5
        %e14 = hw.array_get %old[%c14] : !hw.array<17xi8>, i5
        %e15 = hw.array_get %old[%c15] : !hw.array<17xi8>, i5
        %e16 = hw.array_get %old[%c16] : !hw.array<17xi8>, i5
        %result = hw.array_create %e16, %e15, %e14, %e13, %e12, %e11, %e10, %e9, %e8, %e7, %e6, %e5, %e4, %e3, %e2, %e1, %e0 : i8
        arc.output %result : !hw.array<17xi8>
      }
      hw.module @AggNzLgRstCM(in %clk : i1, in %rst : i1,
                                out q : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @AggNzLgEwArc(%0) clock %c reset %rst latency 1
              {names = ["arr"], initial_value = 58416474095415694810088967901698373430187 : i136} : (!hw.array<17xi8>) -> !hw.array<17xi8>
        %idx = hw.constant 0 : i5
        %elem = hw.array_get %0[%idx] : !hw.array<17xi8>, i5
        hw.output %elem : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggNzLgRstCM");
  ASSERT_TRUE(hwModule);

  auto model =
      hirct::semantic::buildModuleModel(*module, "AggNzLgRstCM");
  ASSERT_TRUE(succeeded(model));

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cm_aggnzlgrst";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  auto evalPos = artifact.implContent.find("AggNzLgRstCM_eval_clk(");
  ASSERT_NE(evalPos, std::string::npos) << "must have eval_clk function";
  auto evalBody = artifact.implContent.substr(evalPos);

  EXPECT_NE(evalBody.find("(uint8_t)171"), std::string::npos)
      << "eval_clock reset branch must use nonzero init value 171; got:\n"
      << evalBody;

  auto rstIfPos = evalBody.find("if (s->input_rst)");
  ASSERT_NE(rstIfPos, std::string::npos) << "must have reset if-branch";
  auto resetBranch =
      evalBody.substr(rstIfPos, evalBody.find("} else", rstIfPos) - rstIfPos);
  EXPECT_EQ(resetBranch.find("__k] = (uint8_t)0;"), std::string::npos)
      << "eval_clock reset branch must NOT zero-assign for nonzero-init "
         "large-width aggregate; got:\n"
      << resetBranch;

  for (unsigned k = 0; k < 17; ++k) {
    std::string needle =
        "arr[" + std::to_string(k) + "] = (uint8_t)171";
    EXPECT_NE(resetBranch.find(needle), std::string::npos)
        << "eval_clock reset must set arr[" << k << "] to 171; got:\n"
        << resetBranch;
  }
}

TEST_F(CModelEmitterFixture,
       CModelEmitter_AggregateNonzeroInitLargeWidthRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @AggNzLgRtArc(%old: !hw.array<17xi8>) -> !hw.array<17xi8> {
        %c0 = hw.constant 0 : i5
        %c1_8 = hw.constant 1 : i8
        %e0 = hw.array_get %old[%c0] : !hw.array<17xi8>, i5
        %n0 = comb.add %e0, %c1_8 : i8
        %c1 = hw.constant 1 : i5
        %e1 = hw.array_get %old[%c1] : !hw.array<17xi8>, i5
        %n1 = comb.add %e1, %c1_8 : i8
        %c2 = hw.constant 2 : i5
        %e2 = hw.array_get %old[%c2] : !hw.array<17xi8>, i5
        %n2 = comb.add %e2, %c1_8 : i8
        %c3 = hw.constant 3 : i5
        %e3 = hw.array_get %old[%c3] : !hw.array<17xi8>, i5
        %n3 = comb.add %e3, %c1_8 : i8
        %c4 = hw.constant 4 : i5
        %e4 = hw.array_get %old[%c4] : !hw.array<17xi8>, i5
        %n4 = comb.add %e4, %c1_8 : i8
        %c5 = hw.constant 5 : i5
        %e5 = hw.array_get %old[%c5] : !hw.array<17xi8>, i5
        %n5 = comb.add %e5, %c1_8 : i8
        %c6 = hw.constant 6 : i5
        %e6 = hw.array_get %old[%c6] : !hw.array<17xi8>, i5
        %n6 = comb.add %e6, %c1_8 : i8
        %c7 = hw.constant 7 : i5
        %e7 = hw.array_get %old[%c7] : !hw.array<17xi8>, i5
        %n7 = comb.add %e7, %c1_8 : i8
        %c8 = hw.constant 8 : i5
        %e8 = hw.array_get %old[%c8] : !hw.array<17xi8>, i5
        %n8 = comb.add %e8, %c1_8 : i8
        %c9 = hw.constant 9 : i5
        %e9 = hw.array_get %old[%c9] : !hw.array<17xi8>, i5
        %n9 = comb.add %e9, %c1_8 : i8
        %c10 = hw.constant 10 : i5
        %e10 = hw.array_get %old[%c10] : !hw.array<17xi8>, i5
        %n10 = comb.add %e10, %c1_8 : i8
        %c11 = hw.constant 11 : i5
        %e11 = hw.array_get %old[%c11] : !hw.array<17xi8>, i5
        %n11 = comb.add %e11, %c1_8 : i8
        %c12 = hw.constant 12 : i5
        %e12 = hw.array_get %old[%c12] : !hw.array<17xi8>, i5
        %n12 = comb.add %e12, %c1_8 : i8
        %c13 = hw.constant 13 : i5
        %e13 = hw.array_get %old[%c13] : !hw.array<17xi8>, i5
        %n13 = comb.add %e13, %c1_8 : i8
        %c14 = hw.constant 14 : i5
        %e14 = hw.array_get %old[%c14] : !hw.array<17xi8>, i5
        %n14 = comb.add %e14, %c1_8 : i8
        %c15 = hw.constant 15 : i5
        %e15 = hw.array_get %old[%c15] : !hw.array<17xi8>, i5
        %n15 = comb.add %e15, %c1_8 : i8
        %c16 = hw.constant 16 : i5
        %e16 = hw.array_get %old[%c16] : !hw.array<17xi8>, i5
        %n16 = comb.add %e16, %c1_8 : i8
        %result = hw.array_create %n16, %n15, %n14, %n13, %n12, %n11, %n10, %n9, %n8, %n7, %n6, %n5, %n4, %n3, %n2, %n1, %n0 : i8
        arc.output %result : !hw.array<17xi8>
      }
      hw.module @AggNzLgRtCM(in %clk : i1, in %rst : i1, in %en : i1,
                               out q0 : i8, out q16 : i8) {
        %c = seq.to_clock %clk
        %0 = arc.state @AggNzLgRtArc(%0) clock %c enable %en reset %rst latency 1
              {names = ["arr"], initial_value = 58416474095415694810088967901698373430187 : i136} : (!hw.array<17xi8>) -> !hw.array<17xi8>
        %idx0 = hw.constant 0 : i5
        %idx16 = hw.constant 16 : i5
        %e0 = hw.array_get %0[%idx0] : !hw.array<17xi8>, i5
        %e16 = hw.array_get %0[%idx16] : !hw.array<17xi8>, i5
        hw.output %e0, %e16 : i8, i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto hwModule =
      module->lookupSymbol<circt::hw::HWModuleOp>("AggNzLgRtCM");
  ASSERT_TRUE(hwModule);

  auto model =
      hirct::semantic::buildModuleModel(*module, "AggNzLgRtCM");
  ASSERT_TRUE(succeeded(model));

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cm_aggnzlgrt";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_cm_aggnzlgrt");
  {
    std::ofstream hf("/tmp/hirct_cm_aggnzlgrt/AggNzLgRtCM.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_cm_aggnzlgrt/AggNzLgRtCM.cpp");
    cf << artifact.implContent;
  }
  {
    std::ofstream drv("/tmp/hirct_cm_aggnzlgrt/driver.cpp");
    drv << R"DRV(
#include "AggNzLgRtCM.h"
#include <cassert>
#include <cstdio>
int main() {
  AggNzLgRtCM_state s;
  AggNzLgRtCM_initialize(&s);

  // After _initialize, all 17 elements must be 171 (0xAB)
  for (int i = 0; i < 17; ++i)
    assert(s.arr[i] == 171 && "initialize must set arr[i] to 171");

  // eval_clk with rst=1 -> reset to init values
  s.input_rst = 1;
  s.input_en = 1;
  AggNzLgRtCM_eval_clk(&s);
  assert(s.arr[0] == 171 && "reset: arr[0] must be 171");
  assert(s.arr[16] == 171 && "reset: arr[16] must be 171");

  // rst=0, en=1 -> update (each element increments by 1)
  s.input_rst = 0;
  s.input_en = 1;
  AggNzLgRtCM_eval_clk(&s);
  assert(s.arr[0] == 172 && "en=1: arr[0] must be 172");
  assert(s.arr[16] == 172 && "en=1: arr[16] must be 172");

  // rst=0, en=0 -> hold
  s.input_en = 0;
  AggNzLgRtCM_eval_clk(&s);
  assert(s.arr[0] == 172 && "en=0: arr[0] must hold at 172");
  assert(s.arr[16] == 172 && "en=0: arr[16] must hold at 172");

  // Re-assert reset
  s.input_rst = 1;
  s.input_en = 1;
  AggNzLgRtCM_eval_clk(&s);
  assert(s.arr[0] == 171 && "re-reset: arr[0] must restore to 171");
  assert(s.arr[16] == 171 && "re-reset: arr[16] must restore to 171");

  printf("PASS: CModelEmitter AggregateNonzeroInitLargeWidthRuntime\n");
  return 0;
}
)DRV";
  }

  int rc = std::system(
      "c++ -std=c++17 -o /tmp/hirct_cm_aggnzlgrt/test "
      "-I/tmp/hirct_cm_aggnzlgrt "
      "/tmp/hirct_cm_aggnzlgrt/AggNzLgRtCM.cpp "
      "/tmp/hirct_cm_aggnzlgrt/driver.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "AggNzLgRtCM must compile with driver";

  if (rc == 0) {
    int run_rc = std::system("/tmp/hirct_cm_aggnzlgrt/test");
    EXPECT_EQ(run_rc, 0) << "AggNzLgRtCM runtime assertions failed";
  }

  std::system("rm -rf /tmp/hirct_cm_aggnzlgrt");
}

// ---------------------------------------------------------------------------
// Exporter API contract: artifact layout
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, CModelEmitter_ArtifactLayoutContract) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_layout_test";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  // Artifact must carry module name metadata
  EXPECT_EQ(artifact.moduleName, "Counter");

  // Artifact must carry output root metadata
  EXPECT_EQ(artifact.outputRoot, "/tmp/hirct_layout_test");

  // Layout contract: <outputRoot>/<moduleName><suffix>
  EXPECT_EQ(artifact.headerPath, "/tmp/hirct_layout_test/Counter.h");
  EXPECT_EQ(artifact.implPath, "/tmp/hirct_layout_test/Counter.cpp");

  // Paths must be consistent with metadata
  std::string expectedHeader =
      artifact.outputRoot + "/" + artifact.moduleName + ".h";
  std::string expectedImpl =
      artifact.outputRoot + "/" + artifact.moduleName + ".cpp";
  EXPECT_EQ(artifact.headerPath, expectedHeader);
  EXPECT_EQ(artifact.implPath, expectedImpl);
}

TEST_F(CModelEmitterFixture, CModelEmitter_ArtifactBundleCarriesMetadata) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/out/cmodel";
  opts.headerSuffix = ".hh";
  opts.implSuffix = ".cc";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  // Metadata must be present regardless of suffix choice
  EXPECT_EQ(artifact.moduleName, "Counter");
  EXPECT_EQ(artifact.outputRoot, "/out/cmodel");

  // Custom suffixes must be honored in layout
  EXPECT_EQ(artifact.headerPath, "/out/cmodel/Counter.hh");
  EXPECT_EQ(artifact.implPath, "/out/cmodel/Counter.cc");

  // Content must not be empty
  EXPECT_FALSE(artifact.headerContent.empty());
  EXPECT_FALSE(artifact.implContent.empty());
}

TEST_F(CModelEmitterFixture, CModelEmitter_LayoutDeterministic) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/deterministic";
  hirct::CModelEmitter emitter1(model, opts);
  auto a1 = emitter1.emit();

  hirct::CModelEmitter emitter2(model, opts);
  auto a2 = emitter2.emit();

  EXPECT_EQ(a1.moduleName, a2.moduleName);
  EXPECT_EQ(a1.outputRoot, a2.outputRoot);
  EXPECT_EQ(a1.headerPath, a2.headerPath);
  EXPECT_EQ(a1.implPath, a2.implPath);
  EXPECT_EQ(a1.headerContent, a2.headerContent);
  EXPECT_EQ(a1.implContent, a2.implContent);
}

TEST_F(CModelEmitterFixture, CModelEmitter_EmitToLayoutCompiles) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_layout_compile";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_layout_compile");

  // Write using artifact's own path metadata
  {
    std::ofstream hf(artifact.headerPath);
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf(artifact.implPath);
    cf << artifact.implContent;
  }

  // Impl must include header by moduleName (layout contract)
  std::string expectedInclude =
      std::string("#include \"") + artifact.moduleName + ".h\"";
  EXPECT_NE(artifact.implContent.find(expectedInclude), std::string::npos)
      << "impl must include header using moduleName-based filename";

  std::string compileCmd =
      "c++ -std=c++17 -fsyntax-only -Werror -I/tmp/hirct_layout_compile " +
      artifact.implPath + " 2>&1";
  int result = std::system(compileCmd.c_str());
  EXPECT_EQ(result, 0) << "artifact written via layout contract must compile";

  std::system("rm -rf /tmp/hirct_layout_compile");
}

TEST_F(CModelEmitterFixture, CModelEmitter_WriteArtifactHelper) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_write_helper";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_write_helper");

  bool ok = hirct::writeArtifact(artifact);
  EXPECT_TRUE(ok) << "writeArtifact must succeed for valid artifact";

  // Files must exist at layout paths
  EXPECT_TRUE(std::filesystem::exists(artifact.headerPath))
      << "header file must exist at layout path";
  EXPECT_TRUE(std::filesystem::exists(artifact.implPath))
      << "impl file must exist at layout path";

  // Content must match
  {
    std::ifstream hf(artifact.headerPath);
    std::string content((std::istreambuf_iterator<char>(hf)),
                        std::istreambuf_iterator<char>());
    EXPECT_EQ(content, artifact.headerContent);
  }
  {
    std::ifstream cf(artifact.implPath);
    std::string content((std::istreambuf_iterator<char>(cf)),
                        std::istreambuf_iterator<char>());
    EXPECT_EQ(content, artifact.implContent);
  }

  // Written artifact must compile
  std::string compileCmd =
      "c++ -std=c++17 -fsyntax-only -Werror -I/tmp/hirct_write_helper " +
      artifact.implPath + " 2>&1";
  int result = std::system(compileCmd.c_str());
  EXPECT_EQ(result, 0) << "written artifact must compile";

  std::system("rm -rf /tmp/hirct_write_helper");
}

TEST_F(CModelEmitterFixture, CModelEmitter_WriteArtifactFailsOnBadDir) {
  auto model = buildCounterModel();
  hirct::CModelOptions opts;
  opts.outputDir = "/nonexistent/deeply/nested/path";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  bool ok = hirct::writeArtifact(artifact);
  EXPECT_FALSE(ok) << "writeArtifact must return false for nonexistent dir";
}

// ---------------------------------------------------------------------------
// Runtime Smoke Tests: host-facing core C model seam
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, CModelEmitter_RuntimeSmoke_CombOnly) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @CombAdd(in %a : i8, in %b : i8, out sum : i8) {
        %0 = comb.add %a, %b : i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "CombAdd");
  ASSERT_TRUE(succeeded(model));
  auto hwModule = (*module).lookupSymbol<circt::hw::HWModuleOp>("CombAdd");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_rt_combonly";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_rt_combonly");
  ASSERT_TRUE(hirct::writeArtifact(artifact));

  // Host driver: exercises the canonical call sequence for comb-only modules:
  //   1. include generated header
  //   2. allocate state struct
  //   3. initialize
  //   4. set inputs via generated setters
  //   5. eval_comb
  //   6. read outputs via generated getters
  {
    std::ofstream df("/tmp/hirct_rt_combonly/driver.cpp");
    df << R"DRV(
#include "CombAdd.h"
#include <cassert>
#include <cstdio>
int main() {
  // Step 1: allocate state
  CombAdd_state s;

  // Step 2: initialize -- zeroes all fields
  CombAdd_initialize(&s);
  assert(CombAdd_get_sum(&s) == 0 && "after init, output must be 0");

  // Step 3: set inputs via generated setters
  CombAdd_set_a(&s, 10);
  CombAdd_set_b(&s, 20);

  // Step 4: evaluate combinational logic
  CombAdd_eval_comb(&s);

  // Step 5: read output via generated getter
  assert(CombAdd_get_sum(&s) == 30 && "10 + 20 must be 30");

  // Step 6: change inputs and re-evaluate
  CombAdd_set_a(&s, 200);
  CombAdd_set_b(&s, 55);
  CombAdd_eval_comb(&s);
  assert(CombAdd_get_sum(&s) == 255 && "200 + 55 must be 255");

  // Step 7: overflow behavior (uint8_t wraps)
  CombAdd_set_a(&s, 200);
  CombAdd_set_b(&s, 100);
  CombAdd_eval_comb(&s);
  assert(CombAdd_get_sum(&s) == 44 && "200 + 100 must wrap to 44");

  printf("PASS: CModelEmitter_RuntimeSmoke_CombOnly\n");
  return 0;
}
)DRV";
  }

  int rc = std::system(
      "c++ -std=c++17 -O0 -Werror "
      "-I/tmp/hirct_rt_combonly "
      "/tmp/hirct_rt_combonly/CombAdd.cpp "
      "/tmp/hirct_rt_combonly/driver.cpp "
      "-o /tmp/hirct_rt_combonly/test 2>&1");
  ASSERT_EQ(rc, 0) << "CombOnly smoke must compile";

  int run_rc = std::system("/tmp/hirct_rt_combonly/test");
  EXPECT_EQ(run_rc, 0) << "CombOnly runtime: init->set->eval_comb->get";

  std::system("rm -rf /tmp/hirct_rt_combonly");
}

TEST_F(CModelEmitterFixture, CModelEmitter_RuntimeSmoke_CounterLike) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @fc_ctr(%arg0: i8, %arg1: i1, %arg2: i1) -> i8 {
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
      hw.module @SmkCtr(in %clk : i1, in %rst : i1, in %en : i1,
                        out count : i8) {
        %clock = arc.call @clk_arc(%clk) : (i1) -> !seq.clock
        %0 = arc.state @fc_ctr(%0, %en, %rst) clock %clock latency 1 {names = ["count_reg"]} : (i8, i1, i1) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "SmkCtr");
  ASSERT_TRUE(succeeded(model));
  auto hwModule = (*module).lookupSymbol<circt::hw::HWModuleOp>("SmkCtr");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_rt_ctrlike";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_rt_ctrlike");
  ASSERT_TRUE(hirct::writeArtifact(artifact));

  // Host driver: exercises the canonical call sequence for sequential modules:
  //   1. include generated header
  //   2. allocate state struct
  //   3. initialize
  //   4. set inputs
  //   5. eval_comb (update pre-edge combinational outputs)
  //   6. eval_<clock> (advance sequential state)
  //   7. eval_comb again (safe host sequence for post-edge outputs)
  //   8. read outputs
  {
    std::ofstream df("/tmp/hirct_rt_ctrlike/driver.cpp");
    df << R"DRV(
#include "SmkCtr.h"
#include <cassert>
#include <cstdio>
int main() {
  // Allocate + initialize
  SmkCtr_state s;
  SmkCtr_initialize(&s);
  assert(s.count_reg == 0 && "state must init to 0");
  assert(SmkCtr_get_count(&s) == 0 && "output must init to 0");

  // --- Cycle 1: rst=0, en=0 -> counter increments ---
  SmkCtr_set_rst(&s, 0);
  SmkCtr_set_en(&s, 0);
  SmkCtr_eval_comb(&s);       // pre-edge combinational evaluation
  SmkCtr_eval_clk(&s);        // advance state: count_reg = 0+1 = 1
  SmkCtr_eval_comb(&s);       // post-edge combinational evaluation
  assert(s.count_reg == 1 && "cycle 1: count_reg must be 1");
  assert(SmkCtr_get_count(&s) == 1 && "cycle 1: output must be 1");

  // --- Cycle 2: still counting ---
  SmkCtr_eval_clk(&s);
  SmkCtr_eval_comb(&s);
  assert(s.count_reg == 2 && "cycle 2: count_reg must be 2");

  // --- Cycle 3: en=1 -> hold ---
  SmkCtr_set_en(&s, 1);
  SmkCtr_eval_clk(&s);
  SmkCtr_eval_comb(&s);
  assert(s.count_reg == 2 && "cycle 3: hold, count_reg must stay 2");

  // --- Cycle 4: rst=1 -> reset ---
  SmkCtr_set_rst(&s, 1);
  SmkCtr_eval_clk(&s);
  SmkCtr_eval_comb(&s);
  assert(s.count_reg == 0 && "cycle 4: reset, count_reg must be 0");
  assert(SmkCtr_get_count(&s) == 0 && "cycle 4: output must be 0");

  // --- Cycle 5: release reset, resume counting ---
  SmkCtr_set_rst(&s, 0);
  SmkCtr_set_en(&s, 0);
  SmkCtr_eval_clk(&s);
  SmkCtr_eval_comb(&s);
  assert(s.count_reg == 1 && "cycle 5: resumed, count_reg must be 1");

  printf("PASS: CModelEmitter_RuntimeSmoke_CounterLike\n");
  return 0;
}
)DRV";
  }

  int rc = std::system(
      "c++ -std=c++17 -O0 -Werror "
      "-I/tmp/hirct_rt_ctrlike "
      "/tmp/hirct_rt_ctrlike/SmkCtr.cpp "
      "/tmp/hirct_rt_ctrlike/driver.cpp "
      "-o /tmp/hirct_rt_ctrlike/test 2>&1");
  ASSERT_EQ(rc, 0) << "CounterLike smoke must compile";

  int run_rc = std::system("/tmp/hirct_rt_ctrlike/test");
  EXPECT_EQ(run_rc, 0) << "CounterLike runtime: full call sequence";

  std::system("rm -rf /tmp/hirct_rt_ctrlike");
}

TEST_F(CModelEmitterFixture, CModelEmitter_RuntimeSmoke_HostApiContract) {
  // This test verifies the host-facing API contract:
  //   - writeArtifact() produces files that form a self-contained compilation unit
  //   - The host only needs: #include "<Module>.h"
  //   - The host calls: <Module>_initialize, <Module>_set_*, <Module>_eval_comb,
  //     <Module>_eval_<clk>, <Module>_get_*
  //   - No other headers or libraries are required
  auto module = parseInline(R"mlir(
    module {
      hw.module @ApiChk(in %x : i16, in %y : i16, out result : i16) {
        %0 = comb.add %x, %y : i16
        hw.output %0 : i16
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "ApiChk");
  ASSERT_TRUE(succeeded(model));
  auto hwModule = (*module).lookupSymbol<circt::hw::HWModuleOp>("ApiChk");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_rt_apichk";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  // Verify API surface in header
  EXPECT_NE(artifact.headerContent.find("ApiChk_state"), std::string::npos)
      << "header must declare state struct";
  EXPECT_NE(artifact.headerContent.find("ApiChk_initialize"), std::string::npos)
      << "header must declare initialize";
  EXPECT_NE(artifact.headerContent.find("ApiChk_set_x"), std::string::npos)
      << "header must declare input setter";
  EXPECT_NE(artifact.headerContent.find("ApiChk_set_y"), std::string::npos)
      << "header must declare input setter";
  EXPECT_NE(artifact.headerContent.find("ApiChk_eval_comb"), std::string::npos)
      << "header must declare eval_comb";
  EXPECT_NE(artifact.headerContent.find("ApiChk_get_result"), std::string::npos)
      << "header must declare output getter";

  // Write via writeArtifact and compile+run
  std::system("mkdir -p /tmp/hirct_rt_apichk");
  ASSERT_TRUE(hirct::writeArtifact(artifact));

  {
    std::ofstream df("/tmp/hirct_rt_apichk/driver.cpp");
    df << R"DRV(
#include "ApiChk.h"
#include <cassert>
#include <cstdio>
int main() {
  ApiChk_state s;
  ApiChk_initialize(&s);
  ApiChk_set_x(&s, 1000);
  ApiChk_set_y(&s, 2000);
  ApiChk_eval_comb(&s);
  assert(ApiChk_get_result(&s) == 3000 && "1000+2000 must be 3000");
  printf("PASS: HostApiContract\n");
  return 0;
}
)DRV";
  }

  int rc = std::system(
      "c++ -std=c++17 -O0 -Werror "
      "-I/tmp/hirct_rt_apichk "
      "/tmp/hirct_rt_apichk/ApiChk.cpp "
      "/tmp/hirct_rt_apichk/driver.cpp "
      "-o /tmp/hirct_rt_apichk/test 2>&1");
  ASSERT_EQ(rc, 0) << "HostApiContract: artifact bundle must compile standalone";

  int run_rc = std::system("/tmp/hirct_rt_apichk/test");
  EXPECT_EQ(run_rc, 0) << "HostApiContract: compiled artifact must run correctly";

  std::system("rm -rf /tmp/hirct_rt_apichk");
}

TEST_F(CModelEmitterFixture, CModelEmitter_RuntimeSmoke_AggregateCompileOnly) {
  // Verify aggregate path artifacts compile alongside scalar path
  // without runtime execution to keep this lightweight
  hirct::semantic::ModuleModel model;
  model.moduleName = "AggSmoke";
  model.inputPorts.push_back({"clk", true, 1});
  model.inputPorts.push_back({"rst", true, 1});
  model.inputPorts.push_back({"din", true, 8});
  model.outputPorts.push_back({"dout", false, 8});

  hirct::semantic::StateVar sv;
  sv.stableName = "scalar_reg";
  sv.width = 8;
  sv.clockDomain = "clk";
  sv.hasConstantInit = false;
  sv.stateOpIndex = 0;
  model.stateVars.push_back(sv);

  hirct::semantic::AggregateStateVar av;
  av.stableName = "arr";
  av.numElements = 4;
  av.elementWidth = 8;
  av.width = 32;
  av.clockDomain = "clk";
  av.hasReset = false;
  av.hasEnable = false;
  av.hasConstantInit = false;
  model.aggregateStateVars.push_back(av);

  model.clockDomains.push_back("clk");

  hirct::semantic::OutputBinding ob;
  ob.visibility = hirct::semantic::OutputVisibility::Edge;
  ob.sourceEntity = "scalar_reg";
  model.outputs.push_back(ob);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_rt_aggsmk";
  hirct::CModelEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  // Struct must contain both scalar and aggregate storage
  EXPECT_NE(artifact.headerContent.find("scalar_reg"), std::string::npos);
  EXPECT_NE(artifact.headerContent.find("arr[4]"), std::string::npos);

  // Must compile
  std::system("mkdir -p /tmp/hirct_rt_aggsmk");
  ASSERT_TRUE(hirct::writeArtifact(artifact));

  int rc = std::system(
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_rt_aggsmk "
      "/tmp/hirct_rt_aggsmk/AggSmoke.cpp 2>&1");
  EXPECT_EQ(rc, 0) << "Aggregate + scalar mixed artifact must compile";

  std::system("rm -rf /tmp/hirct_rt_aggsmk");
}

// ---------------------------------------------------------------------------
// F-1: comb.icmp signed predicate correctness
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, IcmpSignedSltCodegen) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @SltMod(in %a : i8, in %b : i8, out lt : i1) {
        %0 = comb.icmp slt %a, %b : i8
        hw.output %0 : i1
      }
    }
  )mlir");
  auto result = hirct::semantic::buildModuleModel(*module, "SltMod");
  ASSERT_TRUE(succeeded(result));
  auto model = *result;
  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("SltMod");
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_slt";
  hirct::CModelEmitter emitter(model, opts, hwModule);
  auto artifact = emitter.emit();

  // signed slt must emit a signed cast, not bare '<' on unsigned types
  EXPECT_NE(artifact.implContent.find("int8_t"), std::string::npos)
      << "slt codegen must cast operands to signed type;\n"
      << artifact.implContent;
}

TEST_F(CModelEmitterFixture, IcmpSignedSltRuntime) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @SltRt(in %a : i8, in %b : i8, out lt : i1) {
        %0 = comb.icmp slt %a, %b : i8
        hw.output %0 : i1
      }
    }
  )mlir");
  auto result = hirct::semantic::buildModuleModel(*module, "SltRt");
  ASSERT_TRUE(succeeded(result));
  auto model = *result;
  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("SltRt");
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_sltrt";
  hirct::CModelEmitter emitter(model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_test_sltrt");
  ASSERT_TRUE(hirct::writeArtifact(artifact));

  // Write a tiny main() that exercises signed comparison:
  //   a = 0xff (i.e. -1 as int8_t), b = 0x00
  //   slt(-1, 0) must be true (1)
  {
    std::ofstream f("/tmp/hirct_test_sltrt/main.cpp");
    f << "#include \"SltRt.h\"\n"
      << "#include <cstdio>\n"
      << "#include <cstdlib>\n"
      << "int main() {\n"
      << "  SltRt_state s;\n"
      << "  SltRt_initialize(&s);\n"
      << "  SltRt_set_a(&s, 0xff);\n"  // -1 in signed i8
      << "  SltRt_set_b(&s, 0x00);\n"  // 0
      << "  SltRt_eval_comb(&s);\n"
      << "  uint8_t lt = SltRt_get_lt(&s);\n"
      << "  printf(\"slt(0xff, 0x00) = %u\\n\", lt);\n"
      << "  if (lt != 1) {\n"
      << "    fprintf(stderr, \"FAIL: slt(-1, 0) should be 1, got %u\\n\", lt);\n"
      << "    return 1;\n"
      << "  }\n"
      << "  return 0;\n"
      << "}\n";
  }

  int rc = std::system(
      "c++ -std=c++17 -O0 -o /tmp/hirct_test_sltrt/run "
      "-I/tmp/hirct_test_sltrt "
      "/tmp/hirct_test_sltrt/SltRt.cpp "
      "/tmp/hirct_test_sltrt/main.cpp 2>&1");
  ASSERT_EQ(rc, 0) << "slt artifact must compile";

  rc = std::system("/tmp/hirct_test_sltrt/run");
  EXPECT_EQ(rc, 0) << "slt(-1, 0) must return true (exit 0)";

  std::system("rm -rf /tmp/hirct_test_sltrt");
}

TEST_F(CModelEmitterFixture, IcmpSignedSgeCodegen) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @SgeMod(in %a : i8, in %b : i8, out ge : i1) {
        %0 = comb.icmp sge %a, %b : i8
        hw.output %0 : i1
      }
    }
  )mlir");
  auto result = hirct::semantic::buildModuleModel(*module, "SgeMod");
  ASSERT_TRUE(succeeded(result));
  auto model = *result;
  auto hwModule = module->lookupSymbol<circt::hw::HWModuleOp>("SgeMod");
  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_sge";
  hirct::CModelEmitter emitter(model, opts, hwModule);
  auto artifact = emitter.emit();

  // signed sge must emit a signed cast
  EXPECT_NE(artifact.implContent.find("int8_t"), std::string::npos)
      << "sge codegen must cast operands to signed type;\n"
      << artifact.implContent;
}

// ---------------------------------------------------------------------------
// Host C ABI Seam Tests
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, HostCAbi_PortableCompileFlagsMatchPlatformPolicy) {
#if defined(__linux__)
  EXPECT_EQ(getHostCAbiPortableCompileFlags(), " -fPIE");
#else
  EXPECT_TRUE(getHostCAbiPortableCompileFlags().empty());
#endif
}

TEST_F(CModelEmitterFixture, HostCAbi_HeaderIsCCompatible) {
  auto module = parseInline(R"mlir(
    module {
      hw.module @CCompat(in %a : i8, in %b : i8, out sum : i8) {
        %0 = comb.add %a, %b : i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "CCompat");
  ASSERT_TRUE(succeeded(model));
  auto hwModule = (*module).lookupSymbol<circt::hw::HWModuleOp>("CCompat");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cabi_compat";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  // Generated header must use C-compatible includes (stdint.h not cstdint)
  EXPECT_NE(artifact.headerContent.find("<stdint.h>"), std::string::npos)
      << "header must use <stdint.h> for C compatibility";
  EXPECT_NE(artifact.headerContent.find("<stddef.h>"), std::string::npos)
      << "header must use <stddef.h> for C compatibility";
  EXPECT_NE(artifact.headerContent.find("<string.h>"), std::string::npos)
      << "header must use <string.h> for C compatibility";

  // Must have extern "C" guards
  EXPECT_NE(artifact.headerContent.find("extern \"C\""), std::string::npos)
      << "header must have extern \"C\" linkage guards";
  EXPECT_NE(artifact.headerContent.find("#ifdef __cplusplus"), std::string::npos)
      << "header must have __cplusplus guard";
}

TEST_F(CModelEmitterFixture, HostCAbi_CombOnly_CompileAsC) {
  // Verify the generated artifact compiles as pure C (not C++)
  auto module = parseInline(R"mlir(
    module {
      hw.module @CAdd(in %a : i8, in %b : i8, out sum : i8) {
        %0 = comb.add %a, %b : i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "CAdd");
  ASSERT_TRUE(succeeded(model));
  auto hwModule = (*module).lookupSymbol<circt::hw::HWModuleOp>("CAdd");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cabi_cadd";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_cabi_cadd");
  ASSERT_TRUE(hirct::writeArtifact(artifact));
  const std::string hostCAbiCompileFlags = getHostCAbiPortableCompileFlags();

  // Write a pure C driver
  {
    std::ofstream df("/tmp/hirct_cabi_cadd/driver.c");
    df << R"DRV(
#include "CAdd.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  struct CAdd_state s;
  CAdd_initialize(&s);
  assert(CAdd_get_sum(&s) == 0);

  CAdd_set_a(&s, 10);
  CAdd_set_b(&s, 20);
  CAdd_eval_comb(&s);
  assert(CAdd_get_sum(&s) == 30);

  printf("PASS: HostCAbi_CombOnly_CompileAsC\n");
  return 0;
}
)DRV";
  }

  // Compile generated .cpp as C++ object, then link with C driver
  // The header must be includable from C
  int rc_obj = std::system(
      ("c++ -std=c++17 -O0 -Werror" + hostCAbiCompileFlags + " -c "
       "-I/tmp/hirct_cabi_cadd "
       "/tmp/hirct_cabi_cadd/CAdd.cpp "
       "-o /tmp/hirct_cabi_cadd/CAdd.o 2>&1")
          .c_str());
  ASSERT_EQ(rc_obj, 0) << "C model must compile as C++ object";

  int rc_drv = std::system(
      ("cc -std=c11 -O0 -Werror" + hostCAbiCompileFlags + " -c "
       "-I/tmp/hirct_cabi_cadd "
       "/tmp/hirct_cabi_cadd/driver.c "
       "-o /tmp/hirct_cabi_cadd/driver.o 2>&1")
          .c_str());
  ASSERT_EQ(rc_drv, 0) << "C driver must compile with C compiler";

  int rc_link = std::system(
      "c++ -o /tmp/hirct_cabi_cadd/test "
      "/tmp/hirct_cabi_cadd/CAdd.o "
      "/tmp/hirct_cabi_cadd/driver.o 2>&1");
  ASSERT_EQ(rc_link, 0) << "C driver + C++ model must link";

  int run_rc = std::system("/tmp/hirct_cabi_cadd/test");
  EXPECT_EQ(run_rc, 0) << "HostCAbi_CombOnly: C driver runtime";

  std::system("rm -rf /tmp/hirct_cabi_cadd");
}

TEST_F(CModelEmitterFixture, HostCAbi_SingleClock_CompileAsC) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @fc_cabi_ctr(%arg0: i8, %arg1: i1, %arg2: i1) -> i8 {
        %c1_i8 = hw.constant 1 : i8
        %c0_i8 = hw.constant 0 : i8
        %0 = comb.add %arg0, %c1_i8 : i8
        %1 = comb.mux %arg1, %arg0, %0 : i8
        %2 = comb.mux %arg2, %c0_i8, %1 : i8
        arc.output %2 : i8
      }
      arc.define @clk_arc_cabi(%arg0: i1) -> !seq.clock {
        %0 = seq.to_clock %arg0
        arc.output %0 : !seq.clock
      }
      hw.module @CAbiCtr(in %clk : i1, in %rst : i1, in %en : i1,
                         out count : i8) {
        %clock = arc.call @clk_arc_cabi(%clk) : (i1) -> !seq.clock
        %0 = arc.state @fc_cabi_ctr(%0, %en, %rst) clock %clock latency 1 {names = ["count_reg"]} : (i8, i1, i1) -> i8
        hw.output %0 : i8
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "CAbiCtr");
  ASSERT_TRUE(succeeded(model));
  auto hwModule = (*module).lookupSymbol<circt::hw::HWModuleOp>("CAbiCtr");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cabi_ctr";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_cabi_ctr");
  ASSERT_TRUE(hirct::writeArtifact(artifact));
  const std::string hostCAbiCompileFlags = getHostCAbiPortableCompileFlags();

  // Pure C driver exercising init/set/eval_comb/eval_clk/get
  {
    std::ofstream df("/tmp/hirct_cabi_ctr/driver.c");
    df << R"DRV(
#include "CAbiCtr.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  struct CAbiCtr_state s;
  CAbiCtr_initialize(&s);
  assert(CAbiCtr_get_count(&s) == 0);

  /* Cycle 1: rst=0, en=0 -> count increments */
  CAbiCtr_set_rst(&s, 0);
  CAbiCtr_set_en(&s, 0);
  CAbiCtr_eval_comb(&s);
  CAbiCtr_eval_clk(&s);
  CAbiCtr_eval_comb(&s);
  assert(s.count_reg == 1);
  assert(CAbiCtr_get_count(&s) == 1);

  /* Cycle 2: still counting */
  CAbiCtr_eval_clk(&s);
  CAbiCtr_eval_comb(&s);
  assert(s.count_reg == 2);

  /* Cycle 3: en=1 -> hold */
  CAbiCtr_set_en(&s, 1);
  CAbiCtr_eval_clk(&s);
  CAbiCtr_eval_comb(&s);
  assert(s.count_reg == 2);

  /* Cycle 4: rst=1 -> reset */
  CAbiCtr_set_rst(&s, 1);
  CAbiCtr_eval_clk(&s);
  CAbiCtr_eval_comb(&s);
  assert(s.count_reg == 0);

  printf("PASS: HostCAbi_SingleClock_CompileAsC\n");
  return 0;
}
)DRV";
  }

  int rc_obj = std::system(
      ("c++ -std=c++17 -O0 -Werror" + hostCAbiCompileFlags + " -c "
       "-I/tmp/hirct_cabi_ctr "
       "/tmp/hirct_cabi_ctr/CAbiCtr.cpp "
       "-o /tmp/hirct_cabi_ctr/CAbiCtr.o 2>&1")
          .c_str());
  ASSERT_EQ(rc_obj, 0) << "C model must compile as C++ object";

  int rc_drv = std::system(
      ("cc -std=c11 -O0 -Werror" + hostCAbiCompileFlags + " -c "
       "-I/tmp/hirct_cabi_ctr "
       "/tmp/hirct_cabi_ctr/driver.c "
       "-o /tmp/hirct_cabi_ctr/driver.o 2>&1")
          .c_str());
  ASSERT_EQ(rc_drv, 0) << "C driver must compile with C compiler";

  int rc_link = std::system(
      "c++ -o /tmp/hirct_cabi_ctr/test "
      "/tmp/hirct_cabi_ctr/CAbiCtr.o "
      "/tmp/hirct_cabi_ctr/driver.o 2>&1");
  ASSERT_EQ(rc_link, 0) << "C driver + C++ model must link";

  int run_rc = std::system("/tmp/hirct_cabi_ctr/test");
  EXPECT_EQ(run_rc, 0) << "HostCAbi_SingleClock: C driver runtime";

  std::system("rm -rf /tmp/hirct_cabi_ctr");
}

TEST_F(CModelEmitterFixture, HostCAbi_ArtifactCompile_CppBridge) {
  // Verify: generated artifact compiles when included from both C and C++
  auto module = parseInline(R"mlir(
    module {
      hw.module @Bridge(in %x : i16, out y : i16) {
        hw.output %x : i16
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "Bridge");
  ASSERT_TRUE(succeeded(model));
  auto hwModule = (*module).lookupSymbol<circt::hw::HWModuleOp>("Bridge");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_cabi_bridge";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_cabi_bridge");
  ASSERT_TRUE(hirct::writeArtifact(artifact));
  const std::string hostCAbiCompileFlags = getHostCAbiPortableCompileFlags();

  // C++ driver including the same header
  {
    std::ofstream df("/tmp/hirct_cabi_bridge/cpp_driver.cpp");
    df << R"DRV(
#include "Bridge.h"
#include <cassert>
#include <cstdio>
int main() {
  Bridge_state s;
  Bridge_initialize(&s);
  Bridge_set_x(&s, 42);
  Bridge_eval_comb(&s);
  assert(Bridge_get_y(&s) == 42);
  printf("PASS: HostCAbi_ArtifactCompile_CppBridge\n");
  return 0;
}
)DRV";
  }

  // C driver including the same header
  {
    std::ofstream df("/tmp/hirct_cabi_bridge/c_driver.c");
    df << R"DRV(
#include "Bridge.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  struct Bridge_state s;
  Bridge_initialize(&s);
  Bridge_set_x(&s, 42);
  Bridge_eval_comb(&s);
  assert(Bridge_get_y(&s) == 42);
  printf("PASS: HostCAbi_ArtifactCompile_CppBridge (C side)\n");
  return 0;
}
)DRV";
  }

  // Both must compile and link
  int rc_cpp = std::system(
      ("c++ -std=c++17 -O0 -Werror" + hostCAbiCompileFlags + " "
       "-I/tmp/hirct_cabi_bridge "
       "/tmp/hirct_cabi_bridge/Bridge.cpp "
       "/tmp/hirct_cabi_bridge/cpp_driver.cpp "
       "-o /tmp/hirct_cabi_bridge/test_cpp 2>&1")
          .c_str());
  ASSERT_EQ(rc_cpp, 0) << "C++ bridge compile must work";

  int run_cpp = std::system("/tmp/hirct_cabi_bridge/test_cpp");
  EXPECT_EQ(run_cpp, 0) << "C++ bridge runtime must pass";

  int rc_c_obj = std::system(
      ("cc -std=c11 -O0 -Werror" + hostCAbiCompileFlags + " -c "
       "-I/tmp/hirct_cabi_bridge "
       "/tmp/hirct_cabi_bridge/c_driver.c "
       "-o /tmp/hirct_cabi_bridge/c_driver.o 2>&1")
          .c_str());
  ASSERT_EQ(rc_c_obj, 0) << "C bridge compile must work";

  int rc_model_obj = std::system(
      ("c++ -std=c++17 -O0 -Werror" + hostCAbiCompileFlags + " -c "
       "-I/tmp/hirct_cabi_bridge "
       "/tmp/hirct_cabi_bridge/Bridge.cpp "
       "-o /tmp/hirct_cabi_bridge/Bridge.o 2>&1")
          .c_str());
  ASSERT_EQ(rc_model_obj, 0) << "C++ model object compile must work";

  int rc_link = std::system(
      "c++ -o /tmp/hirct_cabi_bridge/test_c "
      "/tmp/hirct_cabi_bridge/Bridge.o "
      "/tmp/hirct_cabi_bridge/c_driver.o 2>&1");
  ASSERT_EQ(rc_link, 0) << "C driver + C++ model must link";

  int run_c = std::system("/tmp/hirct_cabi_bridge/test_c");
  EXPECT_EQ(run_c, 0) << "C bridge runtime must pass";

  std::system("rm -rf /tmp/hirct_cabi_bridge");
}

// ---------------------------------------------------------------------------
// CModelEmitter: firmem read-before-write (read_latency=1) semantics
// ---------------------------------------------------------------------------

TEST_F(CModelEmitterFixture, CModelEmitter_FirmemReadBeforeWrite) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @ReadId(%arg0: i16) -> i16 {
        arc.output %arg0 : i16
      }
      hw.module @FirmemRBW(in %clk : !seq.clock, in %wr_addr : i4,
                           in %wr_data : i16, in %wr_en : i1,
                           in %rd_addr : i4, out rd_data : i16) {
        %mem = seq.firmem 0, 0, undefined, undefined : <16 x 16>
        %0 = seq.firmem.read_port %mem[%rd_addr], clock %clk : <16 x 16>
        seq.firmem.write_port %mem[%wr_addr] = %wr_data, clock %clk enable %wr_en : <16 x 16>
        hw.output %0 : i16
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "FirmemRBW");
  ASSERT_TRUE(succeeded(model));

  auto hwModule = (*module).lookupSymbol<circt::hw::HWModuleOp>("FirmemRBW");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_firmem_rbw";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  auto evalClkBody = artifact.implContent;
  auto snapPos = evalClkBody.find("snap_mem_rd_");
  EXPECT_NE(snapPos, std::string::npos)
      << "eval_clock must snapshot memory reads before writes";

  auto writePos = evalClkBody.find("if (s->input_wr_en)");
  if (writePos == std::string::npos)
    writePos = evalClkBody.find("s->memory_0[");
  ASSERT_NE(writePos, std::string::npos)
      << "eval_clock must have memory write";

  EXPECT_LT(snapPos, writePos)
      << "memory read snapshot must appear before memory write";
}

TEST_F(CModelEmitterFixture, CModelEmitter_FirmemReadBeforeWriteRuntime) {
  auto module = parseInline(R"mlir(
    module {
      arc.define @ReadId(%arg0: i16) -> i16 {
        arc.output %arg0 : i16
      }
      hw.module @FirmemRBWRT(in %clk : !seq.clock, in %wr_addr : i4,
                             in %wr_data : i16, in %wr_en : i1,
                             in %rd_addr : i4, out rd_data : i16) {
        %mem = seq.firmem 0, 0, undefined, undefined : <16 x 16>
        %0 = seq.firmem.read_port %mem[%rd_addr], clock %clk : <16 x 16>
        seq.firmem.write_port %mem[%wr_addr] = %wr_data, clock %clk enable %wr_en : <16 x 16>
        hw.output %0 : i16
      }
    }
  )mlir");
  ASSERT_TRUE(module);

  auto model = hirct::semantic::buildModuleModel(*module, "FirmemRBWRT");
  ASSERT_TRUE(succeeded(model));

  auto hwModule = (*module).lookupSymbol<circt::hw::HWModuleOp>("FirmemRBWRT");
  ASSERT_TRUE(hwModule);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_test_firmem_rbw_rt";
  hirct::CModelEmitter emitter(*model, opts, hwModule);
  auto artifact = emitter.emit();

  std::system("mkdir -p /tmp/hirct_test_firmem_rbw_rt");
  {
    std::ofstream hf("/tmp/hirct_test_firmem_rbw_rt/FirmemRBWRT.h");
    hf << artifact.headerContent;
  }
  {
    std::ofstream cf("/tmp/hirct_test_firmem_rbw_rt/FirmemRBWRT.cpp");
    cf << artifact.implContent;
  }

  std::string driver = R"cpp(
#include "FirmemRBWRT.h"
#include <stdio.h>
#include <assert.h>
int main() {
  FirmemRBWRT_state s;
  FirmemRBWRT_initialize(&s);
  // Write 0xABCD to address 3
  FirmemRBWRT_set_wr_addr(&s, 3);
  FirmemRBWRT_set_wr_data(&s, 0xABCD);
  FirmemRBWRT_set_wr_en(&s, 1);
  FirmemRBWRT_set_rd_addr(&s, 5); // different address
  FirmemRBWRT_eval_comb(&s);
  FirmemRBWRT_eval_clk(&s);
  // Now read address 3, simultaneously write 0x1234 to address 3
  FirmemRBWRT_set_rd_addr(&s, 3);
  FirmemRBWRT_set_wr_addr(&s, 3);
  FirmemRBWRT_set_wr_data(&s, 0x1234);
  FirmemRBWRT_set_wr_en(&s, 1);
  FirmemRBWRT_eval_comb(&s);
  FirmemRBWRT_eval_clk(&s);
  uint16_t rd = FirmemRBWRT_get_rd_data(&s);
  // read_latency=1: must see OLD value 0xABCD, not new write 0x1234
  if (rd != 0xABCD) {
    printf("FAIL: rd=0x%04X expected 0xABCD (read-before-write)\n", rd);
    return 1;
  }
  printf("PASS: CModelEmitter firmem read-before-write\n");
  return 0;
}
)cpp";
  {
    std::ofstream df("/tmp/hirct_test_firmem_rbw_rt/driver.cpp");
    df << driver;
  }

  int rc = std::system(
      "c++ -std=c++17 -O0 -o /tmp/hirct_test_firmem_rbw_rt/test "
      "-I/tmp/hirct_test_firmem_rbw_rt "
      "/tmp/hirct_test_firmem_rbw_rt/driver.cpp "
      "/tmp/hirct_test_firmem_rbw_rt/FirmemRBWRT.cpp 2>&1");
  ASSERT_EQ(rc, 0) << "firmem read-before-write test must compile";

  int run = std::system("/tmp/hirct_test_firmem_rbw_rt/test");
  EXPECT_EQ(run, 0) << "firmem read-before-write runtime must pass";

  std::system("rm -rf /tmp/hirct_test_firmem_rbw_rt");
}

} // namespace
