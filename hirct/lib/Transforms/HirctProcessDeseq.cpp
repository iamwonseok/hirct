#include "hirct/Transforms/Passes.h"
#include "circt/Dialect/Comb/CombOps.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/LLHD/LLHDOps.h"
#include "circt/Dialect/Seq/SeqOps.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Matchers.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

namespace {

struct HirctProcessDeseqPass
    : public mlir::OperationPass<circt::hw::HWModuleOp> {

  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(HirctProcessDeseqPass)

  HirctProcessDeseqPass()
      : mlir::OperationPass<circt::hw::HWModuleOp>(
            mlir::TypeID::get<HirctProcessDeseqPass>()) {}

  llvm::StringRef getName() const override { return "HirctProcessDeseq"; }

  llvm::StringRef getDescription() const override {
    return "Convert clock-edge sequential processes to seq.compreg registers";
  }

  std::unique_ptr<mlir::Pass> clonePass() const override {
    return std::make_unique<HirctProcessDeseqPass>();
  }

  void runOnOperation() override;
};

//===----------------------------------------------------------------------===//
// Condition helper — tracks a boolean condition as constant or SSA value.
// Ported from HirctProcessFlatten.cpp (originally from CIRCT).
//===----------------------------------------------------------------------===//

struct Condition {
  Condition() {}
  Condition(mlir::Value value) : pair_(value, 0) {
    if (value) {
      if (mlir::matchPattern(value, mlir::m_One()))
        *this = Condition(true);
      else if (mlir::matchPattern(value, mlir::m_Zero()))
        *this = Condition(false);
    }
  }
  Condition(bool konst) : pair_(nullptr, konst ? 1 : 2) {}

  explicit operator bool() const {
    return pair_.getPointer() != nullptr || pair_.getInt() != 0;
  }

  bool is_true() const { return !pair_.getPointer() && pair_.getInt() == 1; }
  bool is_false() const { return !pair_.getPointer() && pair_.getInt() == 2; }
  mlir::Value value() const { return pair_.getPointer(); }

  mlir::Value materialize(mlir::OpBuilder &builder, mlir::Location loc) const {
    if (is_true())
      return circt::hw::ConstantOp::create(builder, loc, llvm::APInt(1, 1));
    if (is_false())
      return circt::hw::ConstantOp::create(builder, loc, llvm::APInt(1, 0));
    return pair_.getPointer();
  }

  Condition or_with(Condition other, mlir::OpBuilder &builder) const {
    if (is_true() || other.is_true())
      return true;
    if (is_false())
      return other;
    if (other.is_false())
      return *this;
    return builder.createOrFold<circt::comb::OrOp>(value().getLoc(), value(),
                                                    other.value());
  }

  Condition and_with(Condition other, mlir::OpBuilder &builder) const {
    if (is_false() || other.is_false())
      return false;
    if (is_true())
      return other;
    if (other.is_true())
      return *this;
    return builder.createOrFold<circt::comb::AndOp>(value().getLoc(), value(),
                                                     other.value());
  }

  Condition inverted(mlir::OpBuilder &builder) const {
    if (is_true())
      return false;
    if (is_false())
      return true;
    return circt::comb::createOrFoldNot(value().getLoc(), value(), builder);
  }

private:
  llvm::PointerIntPair<mlir::Value, 2> pair_;
};

//===----------------------------------------------------------------------===//
// Sequential process pattern matching
//===----------------------------------------------------------------------===//

struct SeqProcessMatch {
  circt::llhd::ProcessOp proc;
  mlir::Block *wait_block;
  circt::llhd::WaitOp wait_op;
  mlir::Block *edge_block;
  mlir::Block *reset_block;
  mlir::Block *logic_entry;
  mlir::Value clk_signal;
  mlir::Value rst_signal;
  mlir::Value rst_condition;
  unsigned state_arg_offset;
  llvm::SmallVector<mlir::Value> reset_values;
  llvm::SmallVector<mlir::Block *> reset_intermediate_blocks;
};

// Follow a chain of unconditional branches from `start` to find a block
// that branches to `target`. Returns the terminal BranchOp if found.
static mlir::cf::BranchOp
trace_unconditional_to(mlir::Block *start, mlir::Block *target,
                       unsigned max_depth = 8) {
  mlir::Block *cur = start;
  for (unsigned i = 0; i < max_depth; ++i) {
    auto br = mlir::dyn_cast<mlir::cf::BranchOp>(cur->getTerminator());
    if (!br)
      return nullptr;
    if (br.getDest() == target)
      return br;
    cur = br.getDest();
  }
  return nullptr;
}

std::optional<SeqProcessMatch>
match_sequential(circt::llhd::ProcessOp proc) {
  SeqProcessMatch m;
  m.proc = proc;
  auto &body = proc.getBody();
  if (body.getBlocks().size() < 4)
    return std::nullopt;

  mlir::Block *bb0 = &body.front();
  auto br0 = mlir::dyn_cast<mlir::cf::BranchOp>(bb0->getTerminator());
  if (!br0)
    return std::nullopt;

  m.wait_block = br0.getDest();
  m.wait_op =
      mlir::dyn_cast<circt::llhd::WaitOp>(m.wait_block->getTerminator());
  if (!m.wait_op)
    return std::nullopt;

  auto observed = m.wait_op.getObserved();
  if (observed.size() != 2)
    return std::nullopt;
  m.clk_signal = observed[0];
  m.rst_signal = observed[1];

  m.edge_block = m.wait_op.getDest();
  if (!m.edge_block)
    return std::nullopt;
  auto cb2 =
      mlir::dyn_cast<mlir::cf::CondBranchOp>(m.edge_block->getTerminator());
  if (!cb2)
    return std::nullopt;
  if (cb2.getFalseDest() != m.wait_block)
    return std::nullopt;

  m.reset_block = cb2.getTrueDest();
  auto cb3 =
      mlir::dyn_cast<mlir::cf::CondBranchOp>(m.reset_block->getTerminator());
  if (!cb3)
    return std::nullopt;

  m.rst_condition = cb3.getCondition();

  unsigned bb1_args = m.wait_block->getNumArguments();
  unsigned yield_count = m.wait_op.getYieldOperands().size();
  if (yield_count == 0 || bb1_args < yield_count)
    return std::nullopt;
  m.state_arg_offset = bb1_args - yield_count;

  // Reset path: cb3 TRUE may go directly to wait_block or through
  // intermediate blocks (e.g., unrolled reset initialization loop).
  mlir::OperandRange reset_ops = cb3.getTrueOperands();
  if (cb3.getTrueDest() == m.wait_block) {
    m.logic_entry = cb3.getFalseDest();
    if (reset_ops.size() != bb1_args)
      return std::nullopt;
  } else {
    auto reset_terminal =
        trace_unconditional_to(cb3.getTrueDest(), m.wait_block);
    if (!reset_terminal)
      return std::nullopt;
    m.logic_entry = cb3.getFalseDest();
    reset_ops = reset_terminal.getDestOperands();
    if (reset_ops.size() != bb1_args)
      return std::nullopt;
    m.reset_intermediate_blocks.push_back(cb3.getTrueDest());
    mlir::Block *cur = cb3.getTrueDest();
    while (cur != m.wait_block) {
      auto br = mlir::dyn_cast<mlir::cf::BranchOp>(cur->getTerminator());
      if (!br)
        break;
      if (br.getDest() != m.wait_block)
        m.reset_intermediate_blocks.push_back(br.getDest());
      cur = br.getDest();
    }
  }

  for (unsigned i = m.state_arg_offset; i < reset_ops.size(); ++i)
    m.reset_values.push_back(reset_ops[i]);

  return m;
}

//===----------------------------------------------------------------------===//
// Logic body flattening (adapted from HirctProcessFlatten)
//===----------------------------------------------------------------------===//

using ValueVec = llvm::SmallVector<mlir::Value>;

ValueVec flatten_logic_body(SeqProcessMatch &match, mlir::OpBuilder &builder,
                            mlir::Location loc) {
  mlir::Block *logic_entry = match.logic_entry;
  mlir::Block *wait_block = match.wait_block;

  // Reverse-postorder (topological sort) from logic_entry; wait_block is the
  // barrier that stops traversal.
  llvm::SmallVector<mlir::Block *> sorted;
  llvm::SmallPtrSet<mlir::Block *, 8> visited, ipo_set;
  ipo_set.insert(wait_block);
  visited.insert(wait_block);
  for (auto *rb : match.reset_intermediate_blocks) {
    ipo_set.insert(rb);
    visited.insert(rb);
  }

  {
    llvm::SmallVector<mlir::Block *> po;
    llvm::SmallVector<std::pair<mlir::Block *, mlir::Block::succ_iterator>> stk;
    ipo_set.insert(logic_entry);
    stk.push_back({logic_entry, logic_entry->succ_begin()});
    while (!stk.empty()) {
      auto &[block, it] = stk.back();
      if (it != block->succ_end()) {
        mlir::Block *child = *it;
        ++it;
        if (ipo_set.insert(child).second)
          stk.push_back({child, child->succ_begin()});
      } else {
        po.push_back(block);
        stk.pop_back();
      }
    }
    for (auto it = po.rbegin(); it != po.rend(); ++it) {
      auto *block = *it;
      for (auto *pred : block->getPredecessors()) {
        if (pred == wait_block || !ipo_set.contains(pred))
          continue;
        if (!visited.contains(pred))
          return {};
      }
      visited.insert(block);
      sorted.push_back(block);
    }
  }

  if (sorted.empty())
    return {};

  for (auto *block : sorted) {
    if (!mlir::isa<mlir::cf::BranchOp, mlir::cf::CondBranchOp>(
            block->getTerminator()))
      return {};
  }

  // Map logic_entry block args from reset_block's false-branch operands.
  mlir::IRMapping mapping;
  if (logic_entry->getNumArguments() > 0) {
    auto cb3 = mlir::cast<mlir::cf::CondBranchOp>(
        match.reset_block->getTerminator());
    for (auto [ba, op] :
         llvm::zip(logic_entry->getArguments(), cb3.getFalseOperands()))
      mapping.map(ba, op);
  }

  llvm::DenseMap<mlir::Block *, Condition> block_conds;
  block_conds[sorted.front()] = Condition(true);

  // Merge arguments from visited predecessors of `target`.
  auto merge_args =
      [&](mlir::Block *target) -> llvm::SmallVector<mlir::Value> {
    llvm::SmallVector<mlir::Value> merged;
    llvm::SmallPtrSet<mlir::Block *, 4> seen;

    for (auto *pred : target->getPredecessors()) {
      if (pred == wait_block || !visited.contains(pred))
        continue;
      if (!block_conds.count(pred))
        continue;
      if (!seen.insert(pred).second)
        continue;

      auto pcond = block_conds.lookup(pred);
      auto *term = pred->getTerminator();
      Condition decision;
      llvm::SmallVector<mlir::Value> mapped;

      if (auto cb = mlir::dyn_cast<mlir::cf::CondBranchOp>(term)) {
        auto mc = Condition(mapping.lookupOrDefault(cb.getCondition()));

        if (cb.getTrueDest() == cb.getFalseDest()) {
          decision = pcond;
          auto cv = mapping.lookupOrDefault(cb.getCondition());
          for (auto [t, f] : llvm::zip(cb.getTrueDestOperands(),
                                        cb.getFalseDestOperands()))
            mapped.push_back(builder.createOrFold<circt::comb::MuxOp>(
                loc, cv, mapping.lookupOrDefault(t),
                mapping.lookupOrDefault(f)));
        } else if (cb.getTrueDest() == target) {
          decision = pcond.and_with(mc, builder);
          for (auto a : cb.getTrueDestOperands())
            mapped.push_back(mapping.lookupOrDefault(a));
        } else {
          decision = pcond.and_with(mc.inverted(builder), builder);
          for (auto a : cb.getFalseDestOperands())
            mapped.push_back(mapping.lookupOrDefault(a));
        }
      } else {
        auto br = mlir::cast<mlir::cf::BranchOp>(term);
        decision = pcond;
        for (auto a : br.getDestOperands())
          mapped.push_back(mapping.lookupOrDefault(a));
      }

      if (merged.empty()) {
        merged = std::move(mapped);
        continue;
      }

      for (auto &&[m, a] : llvm::zip(merged, mapped)) {
        if (m == a)
          continue;
        if (decision.is_true())
          m = a;
        else if (decision.is_false())
          continue;
        else
          m = builder.createOrFold<circt::comb::MuxOp>(
              loc, decision.materialize(builder, loc), a, m);
      }
    }
    return merged;
  };

  // Process each block: compute condition, resolve args, clone ops.
  for (auto *block : sorted) {
    if (block != sorted.front()) {
      Condition cond(false);
      for (auto *pred : block->getPredecessors()) {
        if (pred == wait_block || !visited.contains(pred))
          continue;
        if (!block_conds.count(pred))
          continue;
        auto pcond = block_conds.lookup(pred);
        if (auto cb = mlir::dyn_cast<mlir::cf::CondBranchOp>(
                pred->getTerminator())) {
          auto mc = Condition(mapping.lookupOrDefault(cb.getCondition()));
          if (cb.getTrueDest() == cb.getFalseDest())
            cond = cond.or_with(pcond, builder);
          else if (cb.getTrueDest() == block)
            cond = cond.or_with(pcond.and_with(mc, builder), builder);
          else
            cond = cond.or_with(
                pcond.and_with(mc.inverted(builder), builder), builder);
        } else {
          cond = cond.or_with(pcond, builder);
        }
      }
      block_conds[block] = cond;
    }

    if (block != sorted.front() && block->getNumArguments() > 0) {
      auto merged = merge_args(block);
      for (auto [ba, ma] : llvm::zip(block->getArguments(), merged))
        mapping.map(ba, ma);
    }

    for (auto &op : block->getOperations()) {
      if (op.hasTrait<mlir::OpTrait::IsTerminator>())
        break;
      builder.clone(op, mapping);
    }
  }

  // Merge back-edges to wait_block from logic body.
  auto merged_wait = merge_args(wait_block);
  if (merged_wait.size() != match.wait_block->getNumArguments())
    return {};

  ValueVec result;
  for (unsigned i = match.state_arg_offset; i < merged_wait.size(); ++i)
    result.push_back(merged_wait[i]);
  return result;
}

} // namespace

void HirctProcessDeseqPass::runOnOperation() {
  auto module_op = getOperation();

  llvm::SmallVector<SeqProcessMatch> worklist;
  for (auto &op : module_op.getBodyBlock()->getOperations()) {
    auto proc = mlir::dyn_cast<circt::llhd::ProcessOp>(op);
    if (!proc || proc.getNumResults() == 0)
      continue;
    auto m = match_sequential(proc);
    if (m)
      worklist.push_back(std::move(*m));
  }

  for (auto &match : worklist) {
    auto proc = match.proc;
    mlir::Location loc = proc.getLoc();
    mlir::OpBuilder builder(proc);

    auto next_values = flatten_logic_body(match, builder, loc);
    if (next_values.empty())
      continue;

    unsigned num_results = proc.getNumResults();
    if (next_values.size() != num_results) {
      proc.emitWarning("HirctProcessDeseq: state count mismatch, skipping");
      continue;
    }

    // Clone reset block ops (and any intermediate reset blocks) so values
    // defined inside the process region become available in the module body.
    mlir::IRMapping rst_mapping;
    for (auto &op : match.reset_block->getOperations()) {
      if (op.hasTrait<mlir::OpTrait::IsTerminator>())
        break;
      builder.clone(op, rst_mapping);
    }
    for (unsigned i = 0; i < match.reset_intermediate_blocks.size(); ++i) {
      auto *iblock = match.reset_intermediate_blocks[i];
      mlir::OperandRange prev_ops =
          (i == 0)
              ? mlir::cast<mlir::cf::CondBranchOp>(
                    match.reset_block->getTerminator())
                    .getTrueOperands()
              : mlir::cast<mlir::cf::BranchOp>(
                    match.reset_intermediate_blocks[i - 1]->getTerminator())
                    .getDestOperands();
      for (auto [ba, op] : llvm::zip(iblock->getArguments(), prev_ops))
        rst_mapping.map(ba, rst_mapping.lookupOrDefault(op));
      for (auto &op : iblock->getOperations()) {
        if (op.hasTrait<mlir::OpTrait::IsTerminator>())
          break;
        builder.clone(op, rst_mapping);
      }
    }
    mlir::Value rst_cond =
        rst_mapping.lookupOrDefault(match.rst_condition);
    llvm::SmallVector<mlir::Value> reset_vals;
    for (auto rv : match.reset_values)
      reset_vals.push_back(rst_mapping.lookupOrDefault(rv));

    auto clk = circt::seq::ToClockOp::create(builder, loc, match.clk_signal);
    for (unsigned i = 0; i < num_results; ++i) {
      auto reg = circt::seq::CompRegOp::create(builder, loc, next_values[i],
                                               clk, rst_cond, reset_vals[i]);
      proc.getResult(i).replaceAllUsesWith(reg.getResult());
    }

    proc.erase();
  }
}

std::unique_ptr<mlir::Pass> hirct::create_process_deseq_pass() {
  return std::make_unique<HirctProcessDeseqPass>();
}
