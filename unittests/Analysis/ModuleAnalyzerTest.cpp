#include "hirct/Analysis/ModuleAnalyzer.h"
#include <gtest/gtest.h>

namespace {

const char *SIMPLE_MODULE_MLIR = R"(
hw.module @Adder(in %a : i8, in %b : i8, out sum : i8) {
  %0 = comb.add %a, %b : i8
  hw.output %0 : i8
}
)";

const char *MULTI_PORT_MLIR = R"(
hw.module @ALU(in %a : i32, in %b : i32, in %sel : i1, out result : i32) {
  %0 = comb.mux %sel, %a, %b : i32
  hw.output %0 : i32
}
)";

const char *MODULE_WITH_CONSTANT_MLIR = R"(
hw.module @ConstModule(in %a : i8, out out : i8) {
  %c1 = hw.constant 1 : i8
  %0 = comb.add %a, %c1 : i8
  hw.output %0 : i8
}
)";

TEST(ModuleAnalyzerTest, ParsesSimpleModule) {
  hirct::ModuleAnalyzer analyzer(SIMPLE_MODULE_MLIR);
  EXPECT_TRUE(analyzer.is_valid());
  EXPECT_EQ(analyzer.module_name(), "Adder");
}

TEST(ModuleAnalyzerTest, ParsesPorts) {
  hirct::ModuleAnalyzer analyzer(SIMPLE_MODULE_MLIR);
  ASSERT_TRUE(analyzer.is_valid());

  EXPECT_EQ(analyzer.ports().size(), 3u);
  EXPECT_EQ(analyzer.input_ports().size(), 2u);
  EXPECT_EQ(analyzer.output_ports().size(), 1u);

  auto inputs = analyzer.input_ports();
  EXPECT_EQ(inputs[0].name, "a");
  EXPECT_EQ(inputs[0].width, 8);
  EXPECT_EQ(inputs[1].name, "b");
  EXPECT_EQ(inputs[1].width, 8);
}

TEST(ModuleAnalyzerTest, ParsesMultiplePortTypes) {
  hirct::ModuleAnalyzer analyzer(MULTI_PORT_MLIR);
  ASSERT_TRUE(analyzer.is_valid());

  EXPECT_EQ(analyzer.module_name(), "ALU");
  EXPECT_EQ(analyzer.input_ports().size(), 3u);
  EXPECT_EQ(analyzer.output_ports().size(), 1u);

  auto out = analyzer.output_ports();
  EXPECT_EQ(out[0].name, "result");
  EXPECT_EQ(out[0].width, 32);
}

TEST(ModuleAnalyzerTest, ParsesOperations) {
  hirct::ModuleAnalyzer analyzer(SIMPLE_MODULE_MLIR);
  ASSERT_TRUE(analyzer.is_valid());

  EXPECT_GE(analyzer.operations().size(), 1u);
}

TEST(ModuleAnalyzerTest, ParsesConstants) {
  hirct::ModuleAnalyzer analyzer(MODULE_WITH_CONSTANT_MLIR);
  ASSERT_TRUE(analyzer.is_valid());

  EXPECT_EQ(analyzer.constants().size(), 1u);
  EXPECT_EQ(analyzer.constants()[0].name, "%c1");
  EXPECT_EQ(analyzer.constants()[0].type, "i8");
}

TEST(ModuleAnalyzerTest, DetectsNoRegisters) {
  hirct::ModuleAnalyzer analyzer(SIMPLE_MODULE_MLIR);
  ASSERT_TRUE(analyzer.is_valid());
  EXPECT_FALSE(analyzer.has_registers());
}

TEST(ModuleAnalyzerTest, DetectsNoInstances) {
  hirct::ModuleAnalyzer analyzer(SIMPLE_MODULE_MLIR);
  ASSERT_TRUE(analyzer.is_valid());
  EXPECT_FALSE(analyzer.has_instances());
}

TEST(ModuleAnalyzerTest, InvalidInputNotValid) {
  hirct::ModuleAnalyzer analyzer("this is not MLIR");
  EXPECT_FALSE(analyzer.is_valid());
}

TEST(ModuleAnalyzerTest, EmptyInputNotValid) {
  hirct::ModuleAnalyzer analyzer("");
  EXPECT_FALSE(analyzer.is_valid());
}

TEST(ModuleAnalyzerTest, ValueWidth) {
  hirct::ModuleAnalyzer analyzer(SIMPLE_MODULE_MLIR);
  ASSERT_TRUE(analyzer.is_valid());
  EXPECT_EQ(analyzer.value_width("%a"), 8);
  EXPECT_EQ(analyzer.value_width("%b"), 8);
  EXPECT_EQ(analyzer.value_width("%nonexistent"), 0);
}

} // namespace
