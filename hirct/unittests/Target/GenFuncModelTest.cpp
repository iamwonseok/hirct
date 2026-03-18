#include "hirct/Target/GenFuncModel.h"
#include "circt/Dialect/Comb/CombDialect.h"
#include "circt/Dialect/HW/HWDialect.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/Seq/SeqDialect.h"
#include "mlir/Parser/Parser.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

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

class GenFuncModelTest : public ::testing::Test {
protected:
  void SetUp() override {
    ctx_.allowUnregisteredDialects();
    ctx_.loadDialect<circt::hw::HWDialect, circt::comb::CombDialect,
                     circt::seq::SeqDialect>();
    module_ =
        mlir::parseSourceString<mlir::ModuleOp>(FOUR_STATE_FSM_MLIR, &ctx_);
    ASSERT_TRUE(module_);
    module_->walk([&](circt::hw::HWModuleOp op) {
      if (op.getName() == "FourStateFSM")
        hw_mod_ = op;
    });
    ASSERT_TRUE(hw_mod_);
    tmp_dir_ = std::filesystem::temp_directory_path() / "genfuncmodel_test";
    std::filesystem::create_directories(tmp_dir_);
  }
  void TearDown() override { std::filesystem::remove_all(tmp_dir_); }
  std::string read_file(const std::filesystem::path &p) {
    std::ifstream ifs(p);
    return {std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>()};
  }

  mlir::MLIRContext ctx_;
  mlir::OwningOpRef<mlir::ModuleOp> module_;
  circt::hw::HWModuleOp hw_mod_;
  std::filesystem::path tmp_dir_;
};

TEST_F(GenFuncModelTest, EmitsHeaderWithStateEnum) {
  hirct::GenFuncModel gen(hw_mod_, *module_);
  ASSERT_TRUE(gen.emit(tmp_dir_.string()));
  auto content = read_file(tmp_dir_ / "func_model" / "FourStateFSM.h");
  EXPECT_NE(content.find("enum class State"), std::string::npos);
  EXPECT_NE(content.find("void tick("), std::string::npos);
}

TEST_F(GenFuncModelTest, EmitsTickWithSwitchCase) {
  hirct::GenFuncModel gen(hw_mod_, *module_);
  ASSERT_TRUE(gen.emit(tmp_dir_.string()));
  auto content = read_file(tmp_dir_ / "func_model" / "FourStateFSM.cpp");
  EXPECT_NE(content.find("switch (state_)"), std::string::npos);
  EXPECT_NE(content.find("case State::S0"), std::string::npos);
}

// PureComb fixture: seq.compreg가 없는 순수 조합 모듈 → FSM 없음
static const char *PURE_COMB_MLIR = R"mlir(
module {
hw.module @PureComb(in %a : i1, in %b : i1, out y : i1) {
  %and = comb.and %a, %b : i1
  hw.output %and : i1
}
}
)mlir";

class PureCombTest : public ::testing::Test {
protected:
  void SetUp() override {
    ctx_.allowUnregisteredDialects();
    ctx_.loadDialect<circt::hw::HWDialect, circt::comb::CombDialect,
                     circt::seq::SeqDialect>();
    module_ = mlir::parseSourceString<mlir::ModuleOp>(PURE_COMB_MLIR, &ctx_);
    ASSERT_TRUE(module_);
    module_->walk([&](circt::hw::HWModuleOp op) {
      if (op.getName() == "PureComb")
        hw_mod_ = op;
    });
    ASSERT_TRUE(hw_mod_);
    tmp_dir_ = std::filesystem::temp_directory_path() / "purecomb_test";
    std::filesystem::create_directories(tmp_dir_);
  }
  void TearDown() override { std::filesystem::remove_all(tmp_dir_); }

  mlir::MLIRContext ctx_;
  mlir::OwningOpRef<mlir::ModuleOp> module_;
  circt::hw::HWModuleOp hw_mod_;
  std::filesystem::path tmp_dir_;
};

TEST_F(PureCombTest, NoFSMReturnsFalse) {
  hirct::GenFuncModel gen(hw_mod_, *module_);
  EXPECT_FALSE(gen.emit(tmp_dir_.string()));
  EXPECT_EQ(gen.last_error_reason(), "no FSM found");
}

TEST_F(GenFuncModelTest, EmitsTransitionConditions) {
  hirct::GenFuncModel gen(hw_mod_, *module_);
  ASSERT_TRUE(gen.emit(tmp_dir_.string()));
  auto content = read_file(tmp_dir_ / "func_model" / "FourStateFSM.cpp");
  // Waiting 상태(S2)에서 ack 조건부 전이 + default 전이 → if/else 체인 생성 확인
  // (Task 1 fix: cond_trans 단일 포인터 덮어쓰기 버그 수정 후 올바른 if/else 생성)
  EXPECT_NE(content.find("if (io.ack)"), std::string::npos);
  EXPECT_NE(content.find("else\n"), std::string::npos);
}

TEST_F(GenFuncModelTest, EmitsDataRegMember) {
  hirct::GenFuncModel gen(hw_mod_, *module_);
  ASSERT_TRUE(gen.emit(tmp_dir_.string()));
  auto content = read_file(tmp_dir_ / "func_model" / "FourStateFSM.h");
  // cnt data register → cnt_ 멤버 생성 확인
  EXPECT_NE(content.find("cnt_"), std::string::npos);
}

TEST_F(GenFuncModelTest, EmitsResetToInitialState) {
  hirct::GenFuncModel gen(hw_mod_, *module_);
  ASSERT_TRUE(gen.emit(tmp_dir_.string()));
  auto content = read_file(tmp_dir_ / "func_model" / "FourStateFSM.cpp");
  // reset() 메서드 내 초기 상태 대입 확인
  EXPECT_NE(content.find("state_ = State::S"), std::string::npos);
}

// CompoundCond fixture: comb.and 를 전이 조건으로 사용하는 2-state FSM
// FSM 탐지 조건: %state에 icmp eq가 2개 이상(is_s0, is_s1)
static const char *COMPOUND_COND_MLIR = R"mlir(
module {
hw.module @CompoundCond(in %clk : !seq.clock, in %rst : i1,
                         in %en : i1, in %valid : i1,
                         out busy : i1) {
  %c0_i2 = hw.constant 0 : i2
  %c1_i2 = hw.constant 1 : i2
  %state = seq.compreg %next_state, %clk reset %rst, %c0_i2 : i2
  %is_s0 = comb.icmp eq %state, %c0_i2 : i2
  %is_s1 = comb.icmp eq %state, %c1_i2 : i2
  %and_cond = comb.and %en, %valid : i1
  %s0_next = comb.mux %and_cond, %c1_i2, %c0_i2 : i2
  %next_state = comb.mux %is_s0, %s0_next, %c0_i2 : i2
  hw.output %is_s1 : i1
}
}
)mlir";

class CompoundCondTest : public ::testing::Test {
protected:
  void SetUp() override {
    ctx_.allowUnregisteredDialects();
    ctx_.loadDialect<circt::hw::HWDialect, circt::comb::CombDialect,
                     circt::seq::SeqDialect>();
    module_ =
        mlir::parseSourceString<mlir::ModuleOp>(COMPOUND_COND_MLIR, &ctx_);
    ASSERT_TRUE(module_);
    module_->walk([&](circt::hw::HWModuleOp op) {
      if (op.getName() == "CompoundCond")
        hw_mod_ = op;
    });
    ASSERT_TRUE(hw_mod_);
    tmp_dir_ = std::filesystem::temp_directory_path() / "compound_cond_test";
    std::filesystem::create_directories(tmp_dir_);
  }
  void TearDown() override { std::filesystem::remove_all(tmp_dir_); }
  std::string read_file(const std::filesystem::path &p) {
    std::ifstream ifs(p);
    return {std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>()};
  }

  mlir::MLIRContext ctx_;
  mlir::OwningOpRef<mlir::ModuleOp> module_;
  circt::hw::HWModuleOp hw_mod_;
  std::filesystem::path tmp_dir_;
};

// comb.and %en, %valid → (io.en) & (io.valid) 형태로 생성
TEST_F(CompoundCondTest, EmitsCombAndCondition) {
  hirct::GenFuncModel gen(hw_mod_, *module_);
  ASSERT_TRUE(gen.emit(tmp_dir_.string()));
  auto content = read_file(tmp_dir_ / "func_model" / "CompoundCond.cpp");
  EXPECT_NE(content.find("io.en"), std::string::npos);
  EXPECT_NE(content.find("io.valid"), std::string::npos);
  // comb.and → & 연산자 생성
  EXPECT_NE(content.find("&"), std::string::npos);
  // unresolved 없어야 함
  EXPECT_EQ(content.find("unresolved"), std::string::npos);
}

// IcmpEqCond fixture: comb.icmp eq + hw.constant 를 전이 조건으로 사용
static const char *ICMP_EQ_COND_MLIR = R"mlir(
module {
hw.module @IcmpEqCond(in %clk : !seq.clock, in %rst : i1,
                       in %mode : i2,
                       out active : i1) {
  %c0_i2 = hw.constant 0 : i2
  %c1_i2 = hw.constant 1 : i2
  %c3_i2 = hw.constant 3 : i2
  %state = seq.compreg %next_state, %clk reset %rst, %c0_i2 : i2
  %is_s0 = comb.icmp eq %state, %c0_i2 : i2
  %is_s1 = comb.icmp eq %state, %c1_i2 : i2
  %icmp_cond = comb.icmp eq %mode, %c3_i2 : i2
  %s0_next = comb.mux %icmp_cond, %c1_i2, %c0_i2 : i2
  %next_state = comb.mux %is_s0, %s0_next, %c0_i2 : i2
  hw.output %is_s1 : i1
}
}
)mlir";

class IcmpEqCondTest : public ::testing::Test {
protected:
  void SetUp() override {
    ctx_.allowUnregisteredDialects();
    ctx_.loadDialect<circt::hw::HWDialect, circt::comb::CombDialect,
                     circt::seq::SeqDialect>();
    module_ =
        mlir::parseSourceString<mlir::ModuleOp>(ICMP_EQ_COND_MLIR, &ctx_);
    ASSERT_TRUE(module_);
    module_->walk([&](circt::hw::HWModuleOp op) {
      if (op.getName() == "IcmpEqCond")
        hw_mod_ = op;
    });
    ASSERT_TRUE(hw_mod_);
    tmp_dir_ = std::filesystem::temp_directory_path() / "icmp_eq_cond_test";
    std::filesystem::create_directories(tmp_dir_);
  }
  void TearDown() override { std::filesystem::remove_all(tmp_dir_); }
  std::string read_file(const std::filesystem::path &p) {
    std::ifstream ifs(p);
    return {std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>()};
  }

  mlir::MLIRContext ctx_;
  mlir::OwningOpRef<mlir::ModuleOp> module_;
  circt::hw::HWModuleOp hw_mod_;
  std::filesystem::path tmp_dir_;
};

// comb.icmp eq %mode, hw.constant 3 → (io.mode) == (3) 형태 생성
TEST_F(IcmpEqCondTest, EmitsCombIcmpEqWithConstant) {
  hirct::GenFuncModel gen(hw_mod_, *module_);
  ASSERT_TRUE(gen.emit(tmp_dir_.string()));
  auto content = read_file(tmp_dir_ / "func_model" / "IcmpEqCond.cpp");
  EXPECT_NE(content.find("io.mode"), std::string::npos);
  EXPECT_NE(content.find("=="), std::string::npos);
  // hw.constant 3 → 정수 리터럴 "3" 포함
  EXPECT_NE(content.find("3"), std::string::npos);
  EXPECT_EQ(content.find("unresolved"), std::string::npos);
}

// Task 3: tick()에 data register 갱신 코드 생성 확인
TEST_F(GenFuncModelTest, EmitsDataRegUpdate) {
  hirct::GenFuncModel gen(hw_mod_, *module_);
  ASSERT_TRUE(gen.emit(tmp_dir_.string()));
  auto content = read_file(tmp_dir_ / "func_model" / "FourStateFSM.cpp");
  // cnt 데이터 레지스터 갱신 코드가 생성되어야 함
  EXPECT_NE(content.find("cnt_"), std::string::npos)
      << "cnt_ assignment not found in:\n" << content;
  // 최소한 대입 연산이 있어야 함
  EXPECT_NE(content.find("cnt_ ="), std::string::npos)
      << "cnt_ = not found in:\n" << content;
}

// Task 4: tick()에 출력 포트 갱신 코드 생성 확인
TEST_F(GenFuncModelTest, EmitsOutputAssignment) {
  hirct::GenFuncModel gen(hw_mod_, *module_);
  ASSERT_TRUE(gen.emit(tmp_dir_.string()));
  auto content = read_file(tmp_dir_ / "func_model" / "FourStateFSM.cpp");
  // busy 또는 done 출력 포트 갱신 코드가 생성되어야 함
  EXPECT_TRUE(content.find("io.busy =") != std::string::npos ||
              content.find("io.done =") != std::string::npos)
      << "No output assignment (io.busy= or io.done=) found in:\n" << content;
}
