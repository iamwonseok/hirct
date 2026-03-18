#include "hirct/Analysis/FSMAnalysis.h"
#include "circt/Dialect/Comb/CombDialect.h"
#include "circt/Dialect/HW/HWDialect.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/Seq/SeqDialect.h"
#include "mlir/Parser/Parser.h"
#include <gtest/gtest.h>
#include <set>

static const char *FOUR_STATE_FSM_MLIR = R"mlir(
module {
hw.module @FourStateFSM(in %clk : !seq.clock, in %rst : i1,
                         in %start : i1, in %ack : i1,
                         out busy : i1, out done : i1) {
  %c0_i2 = hw.constant 0 : i2
  %c1_i2 = hw.constant 1 : i2
  %c2_i2 = hw.constant 2 : i2
  %c3_i2 = hw.constant 3 : i2
  %true = hw.constant true
  %state = seq.compreg %next_state, %clk reset %rst, %c0_i2 : i2
  %is_idle = comb.icmp eq %state, %c0_i2 : i2
  %is_running = comb.icmp eq %state, %c1_i2 : i2
  %is_waiting = comb.icmp eq %state, %c2_i2 : i2
  %is_done = comb.icmp eq %state, %c3_i2 : i2
  %idle_next = comb.mux %start, %c1_i2, %c0_i2 : i2
  %waiting_next = comb.mux %ack, %c3_i2, %c2_i2 : i2
  %ns3 = comb.mux %is_done, %c0_i2, %state : i2
  %ns2 = comb.mux %is_waiting, %waiting_next, %ns3 : i2
  %ns1 = comb.mux %is_running, %c2_i2, %ns2 : i2
  %next_state = comb.mux %is_idle, %idle_next, %ns1 : i2
  %c1_i8 = hw.constant 1 : i8
  %c0_i8 = hw.constant 0 : i8
  %cnt = seq.compreg %cnt_next_final, %clk reset %rst, %c0_i8 : i8
  %cnt_inc = comb.add %cnt, %c1_i8 : i8
  %cnt_next = comb.mux %is_running, %cnt_inc, %cnt : i8
  %cnt_next_final = comb.mux %is_idle, %c0_i8, %cnt_next : i8
  %not_idle = comb.xor %is_idle, %true : i1
  hw.output %not_idle, %is_done : i1, i1
}
}
)mlir";

class FSMAnalysisTest : public ::testing::Test {
protected:
  void SetUp() override {
    ctx_.allowUnregisteredDialects();
    ctx_.loadDialect<circt::hw::HWDialect, circt::comb::CombDialect,
                     circt::seq::SeqDialect>();
    module_ = mlir::parseSourceString<mlir::ModuleOp>(FOUR_STATE_FSM_MLIR,
                                                      &ctx_);
    ASSERT_TRUE(module_);
    module_->walk([&](circt::hw::HWModuleOp op) {
      if (op.getName() == "FourStateFSM")
        hw_mod_ = op;
    });
    ASSERT_TRUE(hw_mod_);
  }

  mlir::MLIRContext ctx_;
  mlir::OwningOpRef<mlir::ModuleOp> module_;
  circt::hw::HWModuleOp hw_mod_;
};

TEST_F(FSMAnalysisTest, IdentifiesFourStateFSM) {
  auto fsm_views = hirct::identify_fsm_registers(hw_mod_);

  ASSERT_EQ(fsm_views.size(), 1u);
  EXPECT_EQ(fsm_views[0].states.size(), 4u);
  EXPECT_EQ(fsm_views[0].state_reg.width, 2u);
}

TEST_F(FSMAnalysisTest, StateEncodings) {
  auto fsm_views = hirct::identify_fsm_registers(hw_mod_);
  ASSERT_EQ(fsm_views.size(), 1u);

  std::set<uint64_t> encodings;
  for (const auto &s : fsm_views[0].states)
    encodings.insert(s.encoding);
  EXPECT_EQ(encodings, (std::set<uint64_t>{0, 1, 2, 3}));
}

TEST_F(FSMAnalysisTest, TransitionCount) {
  auto fsm_views = hirct::identify_fsm_registers(hw_mod_);
  ASSERT_EQ(fsm_views.size(), 1u);
  EXPECT_GE(fsm_views[0].transitions.size(), 4u);
}

TEST_F(FSMAnalysisTest, DataRegisters) {
  auto fsm_views = hirct::identify_fsm_registers(hw_mod_);
  ASSERT_EQ(fsm_views.size(), 1u);
  EXPECT_EQ(fsm_views[0].data_regs.size(), 1u);
  EXPECT_EQ(fsm_views[0].data_regs[0].width, 8u);
}

TEST_F(FSMAnalysisTest, DataRegUpdatesCount) {
  auto fsm_views = hirct::identify_fsm_registers(hw_mod_);
  ASSERT_EQ(fsm_views.size(), 1u);
  // cnt register: is_idle → 0, is_running → cnt_inc (exactly 2 branches)
  EXPECT_EQ(fsm_views[0].data_reg_updates.size(), 2u);
}

TEST_F(FSMAnalysisTest, DataRegUpdatesStateEncodings) {
  auto fsm_views = hirct::identify_fsm_registers(hw_mod_);
  ASSERT_EQ(fsm_views.size(), 1u);

  std::set<uint64_t> update_states;
  for (const auto &u : fsm_views[0].data_reg_updates)
    update_states.insert(u.state_encoding);
  // idle(0)와 running(1) 상태에서 cnt 갱신
  EXPECT_TRUE(update_states.count(0u));
  EXPECT_TRUE(update_states.count(1u));
}

TEST_F(FSMAnalysisTest, DataRegUpdatesHaveNewValue) {
  auto fsm_views = hirct::identify_fsm_registers(hw_mod_);
  ASSERT_EQ(fsm_views.size(), 1u);

  for (const auto &u : fsm_views[0].data_reg_updates)
    EXPECT_TRUE(u.new_value != nullptr);
}

static const char *PURE_COMB_MLIR = R"mlir(
module {
hw.module @PureComb(in %a : i8, in %b : i8, out sum : i8) {
  %result = comb.add %a, %b : i8
  hw.output %result : i8
}
}
)mlir";

TEST(FSMAnalysis, NoFSMReturnsEmpty) {
  mlir::MLIRContext ctx;
  ctx.allowUnregisteredDialects();
  ctx.loadDialect<circt::hw::HWDialect, circt::comb::CombDialect,
                   circt::seq::SeqDialect>();
  auto module = mlir::parseSourceString<mlir::ModuleOp>(PURE_COMB_MLIR, &ctx);
  ASSERT_TRUE(module);
  circt::hw::HWModuleOp hw_mod;
  module->walk([&](circt::hw::HWModuleOp op) {
    if (op.getName() == "PureComb")
      hw_mod = op;
  });
  ASSERT_TRUE(hw_mod);
  auto fsm_views = hirct::identify_fsm_registers(hw_mod);
  EXPECT_TRUE(fsm_views.empty());
}
