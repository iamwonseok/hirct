#include "hirct/Transforms/Passes.h"
#include "circt/Dialect/Comb/CombOps.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/LLHD/LLHDOps.h"
#include "mlir/Analysis/CFGLoopInfo.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Dominance.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Matchers.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "hirct-unroll-process-loops"

//===----------------------------------------------------------------------===//
// Utilities
//===----------------------------------------------------------------------===//

static void clone_blocks(mlir::ArrayRef<mlir::Block *> blocks,
                        mlir::Region &region, mlir::Region::iterator before,
                        mlir::IRMapping &mapper) {
  if (blocks.empty())
    return;

  llvm::SmallVector<mlir::Block *> new_blocks;
  new_blocks.reserve(blocks.size());
  for (auto *block : blocks) {
    auto *new_block = new mlir::Block();
    mapper.map(block, new_block);
    for (auto arg : block->getArguments())
      mapper.map(arg, new_block->addArgument(arg.getType(), arg.getLoc()));
    region.getBlocks().insert(before, new_block);
    new_blocks.push_back(new_block);
  }

  auto clone_options =
      mlir::Operation::CloneOptions::all().cloneRegions(false).cloneOperands(
          false);
  for (auto [old_block, new_block] : llvm::zip(blocks, new_blocks))
    for (auto &op : *old_block)
      new_block->push_back(op.clone(mapper, clone_options));

  llvm::SmallVector<mlir::Value> operands;
  for (auto [old_block, new_block] : llvm::zip(blocks, new_blocks)) {
    for (auto [old_op, new_op] : llvm::zip(*old_block, *new_block)) {
      operands.resize(old_op.getNumOperands());
      llvm::transform(
          old_op.getOperands(), operands.begin(),
          [&](mlir::Value operand) { return mapper.lookupOrDefault(operand); });
      new_op.setOperands(operands);
      for (auto [old_region, new_region] :
           llvm::zip(old_op.getRegions(), new_op.getRegions()))
        old_region.cloneInto(&new_region, mapper);
    }
  }
}

//===----------------------------------------------------------------------===//
// Loop Unroller
//===----------------------------------------------------------------------===//

namespace {

class Loop {
public:
  Loop(unsigned loop_id, mlir::CFGLoop &cfg_loop)
      : loop_id_(loop_id), cfg_loop_(cfg_loop) {}
  bool fail_match(const llvm::Twine &msg) const;
  bool match();
  void unroll(mlir::CFGLoopInfo &cfg_loop_info);

  unsigned loop_id_;
  mlir::CFGLoop &cfg_loop_;
  mlir::BlockOperand *exit_edge_ = nullptr;
  mlir::Value exit_condition_;
  bool exit_inverted_;
  mlir::Value ind_var_;
  mlir::Value ind_var_next_;
  circt::comb::ICmpPredicate predicate_;
  llvm::APInt ind_var_increment_;
  llvm::APInt begin_bound_;
  llvm::APInt end_bound_;
  unsigned trip_count_ = 0;
};

} // namespace

static llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Loop &loop) {
  os << "#" << loop.loop_id_ << " from ";
  loop.cfg_loop_.getHeader()->printAsOperand(os);
  os << " to ";
  loop.cfg_loop_.getLoopLatch()->printAsOperand(os);
  return os;
}

bool Loop::fail_match(const llvm::Twine &msg) const {
  LLVM_DEBUG(llvm::dbgs() << "- Ignoring loop " << *this << ": " << msg
                          << "\n");
  return false;
}

bool Loop::match() {
  llvm::SmallVector<mlir::BlockOperand *> exits;
  for (auto *block : cfg_loop_.getBlocks())
    for (auto &edge : block->getTerminator()->getBlockOperands())
      if (!cfg_loop_.contains(edge.get()))
        exits.push_back(&edge);
  if (exits.size() != 1)
    return fail_match("multiple exits");
  exit_edge_ = exits.back();

  auto exit_branch =
      mlir::dyn_cast<mlir::cf::CondBranchOp>(exit_edge_->getOwner());
  if (!exit_branch)
    return fail_match("unsupported exit branch");
  exit_condition_ = exit_branch.getCondition();
  exit_inverted_ = exit_edge_->getOperandNumber() == 1;

  if (auto icmp_op = exit_condition_.getDefiningOp<circt::comb::ICmpOp>()) {
    mlir::IntegerAttr bound_attr;
    if (!mlir::matchPattern(icmp_op.getRhs(), mlir::m_Constant(&bound_attr)))
      return fail_match("non-constant loop bound");
    ind_var_ = icmp_op.getLhs();
    predicate_ = icmp_op.getPredicate();
    end_bound_ = bound_attr.getValue();
  } else {
    return fail_match("unsupported exit condition");
  }

  if (!exit_inverted_)
    predicate_ = circt::comb::ICmpOp::getNegatedPredicate(predicate_);

  auto *header = cfg_loop_.getHeader();
  auto *latch = cfg_loop_.getLoopLatch();
  auto ind_var_arg = mlir::dyn_cast<mlir::BlockArgument>(ind_var_);
  if (!ind_var_arg || ind_var_arg.getOwner() != header)
    return fail_match("induction variable is not a header block argument");

  mlir::IntegerAttr begin_bound_attr;
  for (auto &pred : header->getUses()) {
    auto branch_op =
        mlir::dyn_cast<mlir::BranchOpInterface>(pred.getOwner());
    if (!branch_op)
      return fail_match("header predecessor terminator is not a branch op");
    auto ind_var_value = branch_op.getSuccessorOperands(
        pred.getOperandNumber())[ind_var_arg.getArgNumber()];
    mlir::IntegerAttr bound_attr;
    if (pred.getOwner()->getBlock() == latch) {
      ind_var_next_ = ind_var_value;
    } else if (mlir::matchPattern(ind_var_value,
                                  mlir::m_Constant(&bound_attr))) {
      if (!begin_bound_attr)
        begin_bound_attr = bound_attr;
      else if (bound_attr != begin_bound_attr)
        return fail_match("multiple initial bounds");
    } else {
      return fail_match("unsupported induction variable value");
    }
  }
  if (!begin_bound_attr)
    return fail_match("no initial bound");
  begin_bound_ = begin_bound_attr.getValue();

  if (auto add_op = ind_var_next_.getDefiningOp<circt::comb::AddOp>();
      add_op && add_op.getNumOperands() == 2) {
    if (add_op.getOperand(0) != ind_var_arg)
      return fail_match("increment LHS not the induction variable");
    mlir::IntegerAttr inc_attr;
    if (!mlir::matchPattern(add_op.getOperand(1),
                            mlir::m_Constant(&inc_attr)))
      return fail_match("increment RHS non-constant");
    ind_var_increment_ = inc_attr.getValue();
  } else {
    return fail_match("unsupported increment");
  }

  if (predicate_ == circt::comb::ICmpPredicate::ult &&
      ind_var_increment_ == 1 && begin_bound_ == 0 && end_bound_.ult(1024)) {
    trip_count_ = end_bound_.getZExtValue();
    return true;
  }
  if (predicate_ == circt::comb::ICmpPredicate::slt &&
      ind_var_increment_ == 1 && begin_bound_ == 0 && !end_bound_.isNegative() &&
      end_bound_.slt(1024)) {
    trip_count_ = end_bound_.getZExtValue();
    return true;
  }
  if (predicate_ == circt::comb::ICmpPredicate::eq &&
      ind_var_increment_ == 1 && begin_bound_ == 0 && end_bound_ == 0) {
    trip_count_ = 1;
    return true;
  }
  // for (signed i = N; i > M; i += step) with step < 0, N > M, N-M < 1024
  if (predicate_ == circt::comb::ICmpPredicate::sgt &&
      ind_var_increment_.isNegative() &&
      begin_bound_.sgt(end_bound_) &&
      (begin_bound_ - end_bound_).slt(1024)) {
    llvm::APInt range = begin_bound_ - end_bound_;
    llvm::APInt abs_step = -ind_var_increment_;
    if (abs_step == 1) {
      trip_count_ = range.getZExtValue();
      return true;
    }
  }
  return fail_match("unsupported loop bounds");
}

void Loop::unroll(mlir::CFGLoopInfo &cfg_loop_info) {
  assert(trip_count_ > 0 && "must have valid trip count");
  LLVM_DEBUG(llvm::dbgs() << "- Unrolling loop " << *this << "\n");

  auto *header = cfg_loop_.getHeader();
  llvm::SmallVector<mlir::Block *> ordered_body;
  for (auto &block : *header->getParent())
    if (cfg_loop_.contains(&block))
      ordered_body.push_back(&block);

  auto *latch = cfg_loop_.getLoopLatch();
  mlir::OpBuilder builder(ind_var_.getContext());
  auto ind_value = begin_bound_;

  for (unsigned trip = 0; trip < trip_count_; ++trip) {
    mlir::IRMapping mapper;
    clone_blocks(ordered_body, *header->getParent(), header->getIterator(),
                mapper);
    auto *cloned_header = mapper.lookup(header);
    auto *cloned_tail = mapper.lookup(latch);

    auto iter_ind_var = mapper.lookup(ind_var_);
    builder.setInsertionPointAfterValue(iter_ind_var);
    iter_ind_var.replaceAllUsesWith(
        circt::hw::ConstantOp::create(builder, iter_ind_var.getLoc(),
                                      ind_value));

    for (auto &block_operand : llvm::make_early_inc_range(header->getUses()))
      if (block_operand.getOwner()->getBlock() != latch)
        block_operand.set(cloned_header);

    for (auto &block_operand :
         cloned_tail->getTerminator()->getBlockOperands())
      if (block_operand.get() == cloned_header)
        block_operand.set(header);

    auto *cloned_exit_block =
        mapper.lookup(exit_edge_->getOwner()->getBlock());
    auto exit_branch_op = mlir::cast<mlir::cf::CondBranchOp>(
        cloned_exit_block->getTerminator());
    mlir::Block *continue_dest = exit_branch_op.getTrueDest();
    mlir::ValueRange continue_dest_operands =
        exit_branch_op.getTrueDestOperands();
    if (exit_edge_->getOperandNumber() == 0) {
      continue_dest = exit_branch_op.getFalseDest();
      continue_dest_operands = exit_branch_op.getFalseDestOperands();
    }
    builder.setInsertionPoint(exit_branch_op);
    mlir::cf::BranchOp::create(builder, exit_branch_op.getLoc(),
                                continue_dest, continue_dest_operands);
    exit_branch_op.erase();

    for (auto *block : ordered_body) {
      auto *new_block = mapper.lookup(block);
      cfg_loop_.addBasicBlockToLoop(new_block, cfg_loop_info);
    }

    ind_value += ind_var_increment_;
  }

  builder.setInsertionPointAfterValue(ind_var_);
  ind_var_.replaceAllUsesWith(
      circt::hw::ConstantOp::create(builder, ind_var_.getLoc(), ind_value));
  ind_var_ = {};

  auto exit_branch_op =
      mlir::cast<mlir::cf::CondBranchOp>(exit_edge_->getOwner());
  mlir::Block *exit_dest = exit_branch_op.getTrueDest();
  mlir::ValueRange exit_dest_operands = exit_branch_op.getTrueDestOperands();
  if (exit_edge_->getOperandNumber() == 1) {
    exit_dest = exit_branch_op.getFalseDest();
    exit_dest_operands = exit_branch_op.getFalseDestOperands();
  }
  builder.setInsertionPoint(exit_branch_op);
  mlir::cf::BranchOp::create(builder, exit_branch_op.getLoc(), exit_dest,
                              exit_dest_operands);
  exit_branch_op.erase();
  exit_edge_ = nullptr;

  llvm::SmallPtrSet<mlir::Block *, 8> blocks_to_prune;
  for (auto *block : cfg_loop_.getBlocks())
    if (block->use_empty())
      blocks_to_prune.insert(block);
  while (!blocks_to_prune.empty()) {
    auto *block = *blocks_to_prune.begin();
    blocks_to_prune.erase(block);
    if (!block->use_empty())
      continue;
    for (auto *succ : block->getSuccessors())
      if (cfg_loop_.contains(succ))
        blocks_to_prune.insert(succ);
    block->dropAllDefinedValueUses();
    cfg_loop_info.removeBlock(block);
    block->erase();
  }

  for (auto &block : *header->getParent()) {
    if (!cfg_loop_.contains(&block))
      continue;
    while (true) {
      auto branch_op =
          mlir::dyn_cast<mlir::cf::BranchOp>(block.getTerminator());
      if (!branch_op)
        break;
      auto *other_block = branch_op.getDest();
      if (!cfg_loop_.contains(other_block) ||
          !other_block->getSinglePredecessor())
        break;
      for (auto [block_arg, branch_arg] :
           llvm::zip(other_block->getArguments(),
                     branch_op.getDestOperands()))
        block_arg.replaceAllUsesWith(branch_arg);
      block.getOperations().splice(branch_op->getIterator(),
                                   other_block->getOperations());
      branch_op.erase();
      cfg_loop_info.removeBlock(other_block);
      other_block->erase();
    }
  }
}

//===----------------------------------------------------------------------===//
// Pass
//===----------------------------------------------------------------------===//

namespace {

struct HirctUnrollProcessLoopsPass
    : public mlir::OperationPass<circt::hw::HWModuleOp> {

  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(HirctUnrollProcessLoopsPass)

  HirctUnrollProcessLoopsPass()
      : mlir::OperationPass<circt::hw::HWModuleOp>(
            mlir::TypeID::get<HirctUnrollProcessLoopsPass>()) {}

  llvm::StringRef getName() const override {
    return "HirctUnrollProcessLoops";
  }

  llvm::StringRef getDescription() const override {
    return "Unroll static-bound for loops inside llhd.process ops";
  }

  std::unique_ptr<mlir::Pass> clonePass() const override {
    return std::make_unique<HirctUnrollProcessLoopsPass>();
  }

  void runOnOperation() override;
};

} // namespace

// Returns true if any loops were unrolled in the given region.
static bool try_unroll_region(mlir::Region &region) {
  if (region.hasOneBlock())
    return false;

  mlir::DominanceInfo dom_info(region.getParentOp());
  mlir::CFGLoopInfo loop_info(dom_info.getDomTree(&region));

  llvm::SmallVector<Loop> loops;
  for (auto *cfg_loop : loop_info.getLoopsInPreorder()) {
    auto *header = cfg_loop->getHeader();
    auto *latch = cfg_loop->getLoopLatch();
    if (!latch)
      continue;

    Loop loop(loops.size(), *cfg_loop);

    auto *parent = cfg_loop->getParentLoop();
    while (parent && parent->getHeader() != header)
      parent = parent->getParentLoop();
    if (parent) {
      loop.fail_match("header block shared across multiple loops");
      continue;
    }

    parent = cfg_loop->getParentLoop();
    while (parent && !parent->isLoopLatch(latch))
      parent = parent->getParentLoop();
    if (parent) {
      loop.fail_match("latch block shared across multiple loops");
      continue;
    }

    if (loop.match())
      loops.push_back(std::move(loop));
  }

  if (loops.empty())
    return false;

  for (auto &loop : llvm::reverse(loops))
    loop.unroll(loop_info);
  return true;
}

static bool try_unroll_process(circt::llhd::ProcessOp proc) {
  return try_unroll_region(proc.getBody());
}

static bool try_unroll_combinational(circt::llhd::CombinationalOp comb) {
  return try_unroll_region(comb.getBody());
}

void HirctUnrollProcessLoopsPass::runOnOperation() {
  auto module_op = getOperation();
  // Iterate until convergence: inner loops are unrolled first; on the next
  // iteration their enclosing outer loops become eligible for unrolling.
  static constexpr unsigned kMaxIterations = 64;
  for (auto &op : module_op.getBodyBlock()->getOperations()) {
    if (auto proc = mlir::dyn_cast<circt::llhd::ProcessOp>(op)) {
      for (unsigned iter = 0; iter < kMaxIterations; ++iter)
        if (!try_unroll_process(proc))
          break;
    }
    if (auto comb = mlir::dyn_cast<circt::llhd::CombinationalOp>(op)) {
      for (unsigned iter = 0; iter < kMaxIterations; ++iter)
        if (!try_unroll_combinational(comb))
          break;
    }
  }
}

std::unique_ptr<mlir::Pass> hirct::create_unroll_process_loops_pass() {
  return std::make_unique<HirctUnrollProcessLoopsPass>();
}
