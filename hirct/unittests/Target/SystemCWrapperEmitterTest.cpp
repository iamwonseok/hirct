//===- SystemCWrapperEmitterTest.cpp - SystemC wrapper tests ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "hirct/Target/SystemCWrapperEmitter.h"
#include "hirct/Target/CModelEmitter.h"
#include "hirct/SemanticModel/Builder.h"
#include "hirct/SemanticModel/Model.h"

#include "circt/Dialect/Arc/ArcDialect.h"
#include "circt/Dialect/Comb/CombDialect.h"
#include "circt/Dialect/HW/HWDialect.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/Seq/SeqDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace {

constexpr const char *kSystemCStub = R"STUB(
#ifndef SYSTEMC_STUB_H
#define SYSTEMC_STUB_H
#include <cstdint>
#include <string>

namespace sc_core {
  class sc_module_name {
  public:
    sc_module_name(const char *) {}
  };

  struct sc_sensitive_proxy {
    template<typename T>
    sc_sensitive_proxy &operator<<(const T &) { return *this; }
  };

  class sc_module {
  public:
    sc_sensitive_proxy sensitive;
    sc_module() {}
    sc_module(sc_module_name) {}
    void dont_initialize() {}
  };
}

using sc_core::sc_module;
using sc_core::sc_module_name;

namespace sc_dt {
  template<int W> struct sc_uint {
    uint64_t val_ = 0;
    sc_uint() = default;
    sc_uint(uint64_t v) : val_(v) {}
    operator uint64_t() const { return val_; }
    sc_uint &operator=(uint64_t v) { val_ = v; return *this; }
  };
  template<int W> struct sc_biguint {
    uint64_t val_ = 0;
    sc_biguint() = default;
    sc_biguint(uint64_t v) : val_(v) {}
    operator uint64_t() const { return val_; }
  };
}

template<typename T> struct sc_in {
  T val_;
  T read() const { return val_; }
  void write(T v) { val_ = v; }
};

template<typename T> struct sc_out {
  T val_;
  T read() const { return val_; }
  void write(T v) { val_ = v; }
};

struct sc_in_clk {
  bool val_ = false;
  bool read() const { return val_; }
  struct pos_edge_t {};
  pos_edge_t pos() const { return {}; }
};

#define SC_MODULE(name) struct name : public sc_module
#define SC_METHOD(func)
#define SC_CTOR(name) name(sc_module_name nm = sc_module_name(#name)) : sc_module(nm)

#endif
)STUB";

static void writeSystemCStub(const std::filesystem::path &dir) {
  std::filesystem::create_directories(dir);
  std::ofstream f(dir / "systemc");
  f << kSystemCStub;
}

class SystemCWrapperFixture : public ::testing::Test {
protected:
  mlir::MLIRContext ctx_;

  SystemCWrapperFixture() {
    ctx_.getOrLoadDialect<circt::hw::HWDialect>();
    ctx_.getOrLoadDialect<circt::comb::CombDialect>();
    ctx_.getOrLoadDialect<circt::seq::SeqDialect>();
    ctx_.getOrLoadDialect<circt::arc::ArcDialect>();
  }

  mlir::OwningOpRef<mlir::ModuleOp> parseInline(llvm::StringRef src) {
    return mlir::parseSourceString<mlir::ModuleOp>(src, &ctx_);
  }
};

// ---------------------------------------------------------------------------
// Contract: wrapper artifact struct carries expected metadata
// ---------------------------------------------------------------------------

TEST_F(SystemCWrapperFixture, WrapperArtifact_HasModuleName) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "Adder";
  model.inputPorts.push_back({"a", true, 8});
  model.inputPorts.push_back({"b", true, 8});
  model.outputPorts.push_back({"sum", false, 8});

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_sc_test";

  hirct::SystemCWrapperEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_EQ(artifact.moduleName, "Adder");
  EXPECT_FALSE(artifact.wrapperHeaderContent.empty());
  EXPECT_FALSE(artifact.wrapperImplContent.empty());
}

TEST_F(SystemCWrapperFixture, WrapperArtifact_PathLayout) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "Foo";
  model.inputPorts.push_back({"x", true, 8});
  model.outputPorts.push_back({"y", false, 8});

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_sc_layout";

  hirct::SystemCWrapperEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_EQ(artifact.wrapperHeaderPath, "/tmp/hirct_sc_layout/Foo_sc_wrapper.h");
  EXPECT_EQ(artifact.wrapperImplPath, "/tmp/hirct_sc_layout/Foo_sc_wrapper.cpp");
}

// ---------------------------------------------------------------------------
// Contract: wrapper header includes C model header
// ---------------------------------------------------------------------------

TEST_F(SystemCWrapperFixture, WrapperHeader_IncludesCModel) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "Dut";
  model.inputPorts.push_back({"in0", true, 8});
  model.outputPorts.push_back({"out0", false, 8});

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_sc_inc";

  hirct::SystemCWrapperEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.wrapperHeaderContent.find("#include \"Dut.h\""),
            std::string::npos)
      << "wrapper must include C model header";
}

// ---------------------------------------------------------------------------
// Contract: wrapper declares SC_MODULE with correct name
// ---------------------------------------------------------------------------

TEST_F(SystemCWrapperFixture, WrapperHeader_DeclaresScModule) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "Counter";
  model.inputPorts.push_back({"clk", true, 1});
  model.inputPorts.push_back({"rst", true, 1});
  model.outputPorts.push_back({"count", false, 8});
  model.clockDomains.push_back("clk");

  hirct::semantic::StateVar sv;
  sv.stableName = "count_reg";
  sv.width = 8;
  sv.clockDomain = "clk";
  sv.stateOpIndex = 0;
  model.stateVars.push_back(sv);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_sc_mod";

  hirct::SystemCWrapperEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.wrapperHeaderContent.find("SC_MODULE"),
            std::string::npos)
      << "wrapper header must declare SC_MODULE";
  EXPECT_NE(artifact.wrapperHeaderContent.find("Counter_sc_wrapper"),
            std::string::npos)
      << "SC_MODULE name must be <Module>_sc_wrapper";
}

// ---------------------------------------------------------------------------
// Contract: wrapper owns C model state
// ---------------------------------------------------------------------------

TEST_F(SystemCWrapperFixture, WrapperHeader_OwnsCModelState) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "Buf";
  model.inputPorts.push_back({"d", true, 8});
  model.outputPorts.push_back({"q", false, 8});

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_sc_own";

  hirct::SystemCWrapperEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.wrapperHeaderContent.find("Buf_state"),
            std::string::npos)
      << "wrapper must own C model state struct";
}

// ---------------------------------------------------------------------------
// Contract: wrapper has eval delegation points
// ---------------------------------------------------------------------------

TEST_F(SystemCWrapperFixture, WrapperImpl_DelegatesEvalComb) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "CombDut";
  model.inputPorts.push_back({"a", true, 8});
  model.outputPorts.push_back({"y", false, 8});

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_sc_eval";

  hirct::SystemCWrapperEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.wrapperImplContent.find("CombDut_eval_comb"),
            std::string::npos)
      << "wrapper impl must call eval_comb";
}

TEST_F(SystemCWrapperFixture, WrapperImpl_DelegatesEvalClock) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "SeqDut";
  model.inputPorts.push_back({"clk", true, 1});
  model.inputPorts.push_back({"d", true, 8});
  model.outputPorts.push_back({"q", false, 8});
  model.clockDomains.push_back("clk");

  hirct::semantic::StateVar sv;
  sv.stableName = "r";
  sv.width = 8;
  sv.clockDomain = "clk";
  sv.stateOpIndex = 0;
  model.stateVars.push_back(sv);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_sc_clk";

  hirct::SystemCWrapperEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.wrapperImplContent.find("SeqDut_eval_clk"),
            std::string::npos)
      << "wrapper impl must call eval_<clock>";
}

TEST_F(SystemCWrapperFixture, WrapperHeader_UsesActualClockPortName) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "NamedClk";
  model.inputPorts.push_back({"i_clk", true, 1});
  model.inputPorts.push_back({"d", true, 8});
  model.outputPorts.push_back({"q", false, 8});
  model.clockDomains.push_back("i_clk");

  hirct::semantic::StateVar sv;
  sv.stableName = "r";
  sv.width = 8;
  sv.clockDomain = "i_clk";
  sv.stateOpIndex = 0;
  model.stateVars.push_back(sv);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_sc_namedclk";

  hirct::SystemCWrapperEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.wrapperHeaderContent.find("sc_in_clk i_clk;"),
            std::string::npos)
      << "wrapper must preserve the actual clock port name";
  EXPECT_EQ(artifact.wrapperHeaderContent.find("sc_in_clk clk;"),
            std::string::npos)
      << "wrapper must not hardcode clock port name to clk";
}

TEST_F(SystemCWrapperFixture, WrapperImpl_SetsClockInputShadowBeforeEvalClock) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "ClkShadow";
  model.inputPorts.push_back({"i_clk", true, 1});
  model.inputPorts.push_back({"rst", true, 1});
  model.outputPorts.push_back({"q", false, 8});
  model.clockDomains.push_back("i_clk");

  hirct::semantic::StateVar sv;
  sv.stableName = "r";
  sv.width = 8;
  sv.clockDomain = "i_clk";
  sv.stateOpIndex = 0;
  model.stateVars.push_back(sv);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_sc_clkshadow";

  hirct::SystemCWrapperEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  auto setPos = artifact.wrapperImplContent.find(
      "ClkShadow_set_i_clk(&state_, i_clk.read());");
  auto evalPos = artifact.wrapperImplContent.find("ClkShadow_eval_i_clk(&state_);");
  ASSERT_NE(setPos, std::string::npos)
      << "clock_method must mirror the clock input into the C model state";
  ASSERT_NE(evalPos, std::string::npos)
      << "clock_method must call the clock-domain eval function";
  EXPECT_LT(setPos, evalPos)
      << "clock input shadow must be updated before eval_<clock>";
}

TEST_F(SystemCWrapperFixture, WrapperImpl_EvalMethodDoesNotTriggerOnClock) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "NoClockSens";
  model.inputPorts.push_back({"i_clk", true, 1});
  model.inputPorts.push_back({"d", true, 8});
  model.outputPorts.push_back({"q", false, 8});
  model.clockDomains.push_back("i_clk");

  hirct::semantic::StateVar sv;
  sv.stableName = "r";
  sv.width = 8;
  sv.clockDomain = "i_clk";
  sv.stateOpIndex = 0;
  model.stateVars.push_back(sv);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_sc_noclocksens";

  hirct::SystemCWrapperEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_EQ(artifact.wrapperImplContent.find("sensitive << i_clk;\n"),
            std::string::npos)
      << "eval_method must not be directly sensitive to the clock signal";
  EXPECT_NE(artifact.wrapperImplContent.find("sensitive << i_clk.pos();"),
            std::string::npos)
      << "clock_method must keep posedge sensitivity";
}

// ---------------------------------------------------------------------------
// Contract: wrapper exposes SC ports for each input/output
// ---------------------------------------------------------------------------

TEST_F(SystemCWrapperFixture, WrapperHeader_ExposesScPorts) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "IoTest";
  model.inputPorts.push_back({"clk", true, 1});
  model.inputPorts.push_back({"data_in", true, 16});
  model.outputPorts.push_back({"data_out", false, 16});
  model.clockDomains.push_back("clk");

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_sc_ports";

  hirct::SystemCWrapperEmitter emitter(model, opts);
  auto artifact = emitter.emit();

  EXPECT_NE(artifact.wrapperHeaderContent.find("sc_in<"),
            std::string::npos)
      << "wrapper must have sc_in ports";
  EXPECT_NE(artifact.wrapperHeaderContent.find("sc_out<"),
            std::string::npos)
      << "wrapper must have sc_out ports";
  EXPECT_NE(artifact.wrapperHeaderContent.find("sc_in_clk"),
            std::string::npos)
      << "clock input must use sc_in_clk";
}

// ---------------------------------------------------------------------------
// Smoke: SingleClock wrapper + C model compile together
// ---------------------------------------------------------------------------

TEST_F(SystemCWrapperFixture,
       SystemCWrapper_CompileSmoke_SingleClock) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "SmkClk";
  model.inputPorts.push_back({"clk", true, 1});
  model.inputPorts.push_back({"rst", true, 1});
  model.inputPorts.push_back({"din", true, 8});
  model.outputPorts.push_back({"dout", false, 8});
  model.clockDomains.push_back("clk");

  hirct::semantic::StateVar sv;
  sv.stableName = "r";
  sv.width = 8;
  sv.clockDomain = "clk";
  sv.stateOpIndex = 0;
  model.stateVars.push_back(sv);

  hirct::semantic::OutputBinding ob;
  ob.visibility = hirct::semantic::OutputVisibility::Edge;
  ob.sourceEntity = "r";
  model.outputs.push_back(ob);

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_sc_smk_clk";

  hirct::CModelEmitter cEmitter(model, opts);
  auto cArtifact = cEmitter.emit();

  hirct::SystemCWrapperEmitter wEmitter(model, opts);
  auto wArtifact = wEmitter.emit();

  auto tmpDir = std::filesystem::path("/tmp/hirct_sc_smk_clk");
  std::filesystem::create_directories(tmpDir);
  writeSystemCStub(tmpDir);

  ASSERT_TRUE(hirct::writeArtifact(cArtifact));

  {
    std::ofstream hdr(wArtifact.wrapperHeaderPath);
    hdr << wArtifact.wrapperHeaderContent;
    std::ofstream impl(wArtifact.wrapperImplPath);
    impl << wArtifact.wrapperImplContent;
  }

  std::string cmd =
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_sc_smk_clk "
      "/tmp/hirct_sc_smk_clk/SmkClk.cpp "
      "/tmp/hirct_sc_smk_clk/SmkClk_sc_wrapper.cpp 2>&1";
  int rc = std::system(cmd.c_str());
  EXPECT_EQ(rc, 0) << "SingleClock wrapper + C model must compile together";

  std::filesystem::remove_all(tmpDir);
}

// ---------------------------------------------------------------------------
// Contract: multi-clock module is rejected by wrapper v1
// ---------------------------------------------------------------------------

TEST_F(SystemCWrapperFixture, WrapperV1_RejectsMultiClock) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "DualClk";
  model.inputPorts.push_back({"clk_a", true, 1});
  model.inputPorts.push_back({"clk_b", true, 1});
  model.inputPorts.push_back({"d", true, 8});
  model.outputPorts.push_back({"qa", false, 8});
  model.outputPorts.push_back({"qb", false, 8});
  model.clockDomains.push_back("clk_a");
  model.clockDomains.push_back("clk_b");

  auto reason = hirct::getWrapperV1UnsupportedReason(model);
  ASSERT_TRUE(reason.has_value())
      << "multi-clock module must be rejected by wrapper v1";
  EXPECT_NE(reason->find("single-clock"), std::string::npos)
      << "rejection reason must mention single-clock constraint";
}

// ---------------------------------------------------------------------------
// Contract: wide (>64-bit) port module is rejected by wrapper v1
// ---------------------------------------------------------------------------

TEST_F(SystemCWrapperFixture, WrapperV1_RejectsWideInputPort) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "WideIn";
  model.inputPorts.push_back({"din", true, 128});
  model.outputPorts.push_back({"dout", false, 8});

  auto reason = hirct::getWrapperV1UnsupportedReason(model);
  ASSERT_TRUE(reason.has_value())
      << ">64-bit input port must be rejected by wrapper v1";
  EXPECT_NE(reason->find("64 bits"), std::string::npos)
      << "rejection reason must mention 64-bit constraint";
}

TEST_F(SystemCWrapperFixture, WrapperV1_RejectsWideOutputPort) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "WideOut";
  model.inputPorts.push_back({"din", true, 8});
  model.outputPorts.push_back({"dout", false, 256});

  auto reason = hirct::getWrapperV1UnsupportedReason(model);
  ASSERT_TRUE(reason.has_value())
      << ">64-bit output port must be rejected by wrapper v1";
  EXPECT_NE(reason->find("64 bits"), std::string::npos)
      << "rejection reason must mention 64-bit constraint";
}

// ---------------------------------------------------------------------------
// Contract: supported single-clock <=64-bit model passes v1 check
// ---------------------------------------------------------------------------

TEST_F(SystemCWrapperFixture, WrapperV1_AcceptsSingleClockNarrow) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "GoodDut";
  model.inputPorts.push_back({"clk", true, 1});
  model.inputPorts.push_back({"d", true, 64});
  model.outputPorts.push_back({"q", false, 64});
  model.clockDomains.push_back("clk");

  auto reason = hirct::getWrapperV1UnsupportedReason(model);
  EXPECT_FALSE(reason.has_value())
      << "single-clock <=64-bit model must be accepted: "
      << reason.value_or("");
}

TEST_F(SystemCWrapperFixture, WrapperV1_AcceptsCombOnly) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "CombDut";
  model.inputPorts.push_back({"a", true, 32});
  model.outputPorts.push_back({"y", false, 32});

  auto reason = hirct::getWrapperV1UnsupportedReason(model);
  EXPECT_FALSE(reason.has_value())
      << "comb-only <=64-bit model must be accepted: "
      << reason.value_or("");
}

// ---------------------------------------------------------------------------
// Smoke: CombOnly wrapper + C model compile together
// ---------------------------------------------------------------------------

TEST_F(SystemCWrapperFixture,
       SystemCWrapper_CompileSmoke_CombOnly) {
  hirct::semantic::ModuleModel model;
  model.moduleName = "SmkComb";
  model.inputPorts.push_back({"a", true, 8});
  model.inputPorts.push_back({"b", true, 8});
  model.outputPorts.push_back({"sum", false, 8});

  hirct::CModelOptions opts;
  opts.outputDir = "/tmp/hirct_sc_smk_comb";

  hirct::CModelEmitter cEmitter(model, opts);
  auto cArtifact = cEmitter.emit();

  hirct::SystemCWrapperEmitter wEmitter(model, opts);
  auto wArtifact = wEmitter.emit();

  auto tmpDir = std::filesystem::path("/tmp/hirct_sc_smk_comb");
  std::filesystem::create_directories(tmpDir);
  writeSystemCStub(tmpDir);

  ASSERT_TRUE(hirct::writeArtifact(cArtifact));

  {
    std::ofstream hdr(wArtifact.wrapperHeaderPath);
    hdr << wArtifact.wrapperHeaderContent;
    std::ofstream impl(wArtifact.wrapperImplPath);
    impl << wArtifact.wrapperImplContent;
  }

  std::string cmd =
      "c++ -std=c++17 -fsyntax-only -Werror "
      "-I/tmp/hirct_sc_smk_comb "
      "/tmp/hirct_sc_smk_comb/SmkComb.cpp "
      "/tmp/hirct_sc_smk_comb/SmkComb_sc_wrapper.cpp 2>&1";
  int rc = std::system(cmd.c_str());
  EXPECT_EQ(rc, 0) << "CombOnly wrapper + C model must compile together";

  std::filesystem::remove_all(tmpDir);
}

} // namespace
