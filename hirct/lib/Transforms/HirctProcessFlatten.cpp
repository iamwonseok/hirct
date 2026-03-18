#include "hirct/Transforms/Passes.h"
#include "circt/Dialect/Comb/CombOps.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/LLHD/LLHDOps.h"
#include "circt/Dialect/Seq/SeqOps.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Dominance.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Matchers.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

namespace {

struct HirctProcessFlattenPass
    : public mlir::OperationPass<circt::hw::HWModuleOp> {

  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(HirctProcessFlattenPass)

  HirctProcessFlattenPass()
      : mlir::OperationPass<circt::hw::HWModuleOp>(
            mlir::TypeID::get<HirctProcessFlattenPass>()) {}

  llvm::StringRef getName() const override { return "HirctProcessFlatten"; }

  llvm::StringRef getDescription() const override {
    return "Flatten LLHD processes into combinational/sequential logic";
  }

  std::unique_ptr<mlir::Pass> clonePass() const override {
    return std::make_unique<HirctProcessFlattenPass>();
  }

  void runOnOperation() override;
};

enum class ProcessPattern { COMBINATIONAL, SEQUENTIAL, UNKNOWN };

struct ProcessAnalysis {
  circt::llhd::ProcessOp proc;
  ProcessPattern pattern;
  mlir::Block *wait_block;
  circt::llhd::WaitOp wait_op;
  mlir::Block *wakeup_block;
};

ProcessAnalysis analyze_process(circt::llhd::ProcessOp proc) {
  ProcessAnalysis result;
  result.proc = proc;
  result.pattern = ProcessPattern::UNKNOWN;
  result.wait_block = nullptr;
  result.wakeup_block = nullptr;

  for (auto &block : proc.getBody().getBlocks()) {
    if (auto w =
            mlir::dyn_cast<circt::llhd::WaitOp>(block.getTerminator())) {
      result.wait_block = &block;
      result.wait_op = w;
      break;
    }
  }

  if (!result.wait_block || !result.wait_op.getDest())
    return result;

  result.wakeup_block = result.wait_op.getDest();

  bool has_edge_detect = false;
  for (auto &op : result.wakeup_block->getOperations()) {
    if (auto xor_op = mlir::dyn_cast<circt::comb::XorOp>(op)) {
      for (auto *user : xor_op.getResult().getUsers()) {
        if (mlir::isa<circt::comb::AndOp>(user)) {
          has_edge_detect = true;
          break;
        }
      }
      if (has_edge_detect) break;
    }
  }

  if (has_edge_detect) {
    result.pattern = ProcessPattern::SEQUENTIAL;
    return result;
  }

  result.pattern = ProcessPattern::COMBINATIONAL;
  return result;
}

//===----------------------------------------------------------------------===//
// Condition helper — ported from CIRCT RemoveControlFlow.cpp
//===----------------------------------------------------------------------===//

/// Tracks a boolean condition as constant false, constant true, or SSA value.
struct Condition {
  Condition() {}
  Condition(mlir::Value value) : pair(value, 0) {
    if (value) {
      if (mlir::matchPattern(value, mlir::m_One()))
        *this = Condition(true);
      if (mlir::matchPattern(value, mlir::m_Zero()))
        *this = Condition(false);
    }
  }
  Condition(bool konst) : pair(nullptr, konst ? 1 : 2) {}

  explicit operator bool() const {
    return pair.getPointer() != nullptr || pair.getInt() != 0;
  }

  bool isTrue() const { return !pair.getPointer() && pair.getInt() == 1; }
  bool isFalse() const { return !pair.getPointer() && pair.getInt() == 2; }
  mlir::Value getValue() const { return pair.getPointer(); }

  mlir::Value materialize(mlir::OpBuilder &builder, mlir::Location loc) const {
    if (isTrue())
      return circt::hw::ConstantOp::create(builder, loc, llvm::APInt(1, 1));
    if (isFalse())
      return circt::hw::ConstantOp::create(builder, loc, llvm::APInt(1, 0));
    return pair.getPointer();
  }

  Condition orWith(Condition other, mlir::OpBuilder &builder) const {
    if (isTrue() || other.isTrue())
      return true;
    if (isFalse())
      return other;
    if (other.isFalse())
      return *this;
    return builder.createOrFold<circt::comb::OrOp>(getValue().getLoc(),
                                                    getValue(),
                                                    other.getValue());
  }

  Condition andWith(Condition other, mlir::OpBuilder &builder) const {
    if (isFalse() || other.isFalse())
      return false;
    if (isTrue())
      return other;
    if (other.isTrue())
      return *this;
    return builder.createOrFold<circt::comb::AndOp>(getValue().getLoc(),
                                                     getValue(),
                                                     other.getValue());
  }

  Condition inverted(mlir::OpBuilder &builder) const {
    if (isTrue())
      return false;
    if (isFalse())
      return true;
    return circt::comb::createOrFoldNot(getValue().getLoc(), getValue(),
                                        builder);
  }

private:
  llvm::PointerIntPair<mlir::Value, 2> pair;
};

//===----------------------------------------------------------------------===//
// Topological-order process flattening
//===----------------------------------------------------------------------===//

using ValueVec = llvm::SmallVector<mlir::Value>;

/// Flatten a combinational ProcessOp into straight-line mux logic using an
/// O(n) topological-order algorithm. Each block is visited exactly once,
/// and block arguments are resolved into muxes based on branch conditions
/// accumulated from the entry (wakeup_block).
///
/// Returns the yield values (process results) defined outside the process,
/// or an empty vector on failure.
ValueVec flatten_process(circt::llhd::ProcessOp proc,
                         mlir::Block *wait_block,
                         mlir::Block *wakeup_block,
                         mlir::OpBuilder &builder, mlir::Location loc) {
  // 1. Topological sort via inverse post-order from wakeup_block.
  //    wait_block is placed in the visited set so it is never traversed.
  llvm::SmallVector<mlir::Block *> sorted_blocks;
  llvm::SmallPtrSet<mlir::Block *, 8> visited, ipo_set;
  ipo_set.insert(wait_block);
  visited.insert(wait_block);

  // Manual reverse postorder: DFS from wakeup_block, collect in post-order,
  // then reverse. Avoids reliance on llvm::inverse_post_order_ext which
  // may not traverse correctly in all LLVM/MLIR builds.
  {
    llvm::SmallVector<mlir::Block *> po_order;
    llvm::SmallVector<std::pair<mlir::Block *, mlir::Block::succ_iterator>> stk;
    ipo_set.insert(wakeup_block);
    stk.push_back({wakeup_block, wakeup_block->succ_begin()});
    while (!stk.empty()) {
      auto &[block, it] = stk.back();
      if (it != block->succ_end()) {
        mlir::Block *child = *it;
        ++it;
        if (ipo_set.insert(child).second)
          stk.push_back({child, child->succ_begin()});
      } else {
        po_order.push_back(block);
        stk.pop_back();
      }
    }
    // Reverse post-order = topological order
    for (auto it = po_order.rbegin(); it != po_order.rend(); ++it) {
      auto *block = *it;
      if (!llvm::all_of(block->getPredecessors(),
                        [&](auto *pred) { return visited.contains(pred); }))
        return {};
      visited.insert(block);
      sorted_blocks.push_back(block);
    }
  }

  if (sorted_blocks.empty())
    return {};

  for (auto *block : sorted_blocks) {
    if (!mlir::isa<mlir::cf::BranchOp, mlir::cf::CondBranchOp>(
            block->getTerminator()))
      return {};
  }

  // 2. Prepare value mapping: clone ops into builder position (before proc).
  auto wait_op =
      mlir::cast<circt::llhd::WaitOp>(wait_block->getTerminator());
  mlir::IRMapping mapping;
  for (auto [wa, d] :
       llvm::zip(wakeup_block->getArguments(), wait_op.getDestOperands()))
    mapping.map(wa, d);

  // 3. Incremental reachability condition per block (mapped domain).
  llvm::DenseMap<mlir::Block *, Condition> block_conds;
  block_conds[sorted_blocks.front()] = Condition(true);

  // Helper: merge block arguments (or wait_block virtual args) for a target.
  // Iterates over predecessors within sorted_blocks and produces muxed values.
  auto mergeBlockArgs =
      [&](mlir::Block *target) -> llvm::SmallVector<mlir::Value> {
    llvm::SmallVector<mlir::Value> merged;
    llvm::SmallPtrSet<mlir::Block *, 4> seen;

    for (auto *pred : target->getPredecessors()) {
      if (pred == wait_block || !visited.contains(pred))
        continue;
      if (!seen.insert(pred).second)
        continue;

      auto pcond = block_conds.lookup(pred);
      auto *term = pred->getTerminator();
      Condition decision;
      llvm::SmallVector<mlir::Value> mapped_args;

      if (auto cb = mlir::dyn_cast<mlir::cf::CondBranchOp>(term)) {
        auto mapped_cond =
            Condition(mapping.lookupOrDefault(cb.getCondition()));

        if (cb.getTrueDest() == cb.getFalseDest()) {
          decision = pcond;
          auto mc = mapping.lookupOrDefault(cb.getCondition());
          for (auto [t, f] : llvm::zip(cb.getTrueDestOperands(),
                                        cb.getFalseDestOperands()))
            mapped_args.push_back(builder.createOrFold<circt::comb::MuxOp>(
                loc, mc, mapping.lookupOrDefault(t),
                mapping.lookupOrDefault(f)));
        } else if (cb.getTrueDest() == target) {
          decision = pcond.andWith(mapped_cond, builder);
          for (auto a : cb.getTrueDestOperands())
            mapped_args.push_back(mapping.lookupOrDefault(a));
        } else {
          decision = pcond.andWith(mapped_cond.inverted(builder), builder);
          for (auto a : cb.getFalseDestOperands())
            mapped_args.push_back(mapping.lookupOrDefault(a));
        }
      } else {
        auto br = mlir::cast<mlir::cf::BranchOp>(term);
        decision = pcond;
        for (auto a : br.getDestOperands())
          mapped_args.push_back(mapping.lookupOrDefault(a));
      }

      if (merged.empty()) {
        merged = std::move(mapped_args);
        continue;
      }

      for (auto [m, a] : llvm::zip(merged, mapped_args)) {
        if (m == a)
          continue;
        if (decision.isTrue())
          m = a;
        else if (decision.isFalse())
          continue;
        else
          m = builder.createOrFold<circt::comb::MuxOp>(
              loc, decision.materialize(builder, loc), a, m);
      }
    }

    return merged;
  };

  // 4. Process each block in topological order: compute condition, resolve
  //    block args, clone non-terminator ops.
  for (auto *block : sorted_blocks) {
    if (block != sorted_blocks.front()) {
      auto cond = Condition(false);
      for (auto *pred : block->getPredecessors()) {
        if (pred == wait_block || !visited.contains(pred))
          continue;
        auto pcond = block_conds.lookup(pred);

        if (auto cb = mlir::dyn_cast<mlir::cf::CondBranchOp>(
                pred->getTerminator())) {
          auto mapped_cond =
              Condition(mapping.lookupOrDefault(cb.getCondition()));
          if (cb.getTrueDest() == cb.getFalseDest())
            cond = cond.orWith(pcond, builder);
          else if (cb.getTrueDest() == block)
            cond = cond.orWith(pcond.andWith(mapped_cond, builder), builder);
          else
            cond = cond.orWith(
                pcond.andWith(mapped_cond.inverted(builder), builder), builder);
        } else {
          cond = cond.orWith(pcond, builder);
        }
      }
      block_conds[block] = cond;
    }

    if (block != sorted_blocks.front() && block->getNumArguments() > 0) {
      auto merged = mergeBlockArgs(block);
      for (auto [ba, ma] : llvm::zip(block->getArguments(), merged))
        mapping.map(ba, ma);
    }

    for (auto &op : block->getOperations()) {
      if (op.hasTrait<mlir::OpTrait::IsTerminator>())
        break;
      builder.clone(op, mapping);
    }
  }

  // 5. Collect yield values: treat wait_block as a virtual final merge target.
  auto merged_wait_args = mergeBlockArgs(wait_block);
  for (auto [wa, mwa] :
       llvm::zip(wait_block->getArguments(), merged_wait_args))
    mapping.map(wa, mwa);

  for (auto &op : wait_block->getOperations()) {
    if (op.hasTrait<mlir::OpTrait::IsTerminator>())
      break;
    builder.clone(op, mapping);
  }

  ValueVec result;
  for (auto yv : wait_op.getYieldOperands())
    result.push_back(mapping.lookupOrDefault(yv));
  return result;
}

} // namespace

void HirctProcessFlattenPass::runOnOperation() {
  auto module_op = getOperation();

  llvm::SmallVector<ProcessAnalysis> worklist;
  for (auto &op : module_op.getBodyBlock()->getOperations()) {
    auto proc = mlir::dyn_cast<circt::llhd::ProcessOp>(op);
    if (!proc || proc.getNumResults() == 0)
      continue;
    auto analysis = analyze_process(proc);
    if (analysis.pattern == ProcessPattern::SEQUENTIAL)
      continue;
    if (analysis.pattern == ProcessPattern::UNKNOWN) {
      proc.emitWarning("HirctProcessFlatten: unrecognized process pattern, "
                       "skipping");
      continue;
    }
    worklist.push_back(std::move(analysis));
  }

  for (auto &info : worklist) {
    auto proc = info.proc;
    mlir::Location loc = proc.getLoc();
    mlir::OpBuilder builder(proc);

    auto flattened = flatten_process(proc, info.wait_block, info.wakeup_block,
                                    builder, loc);

    if (flattened.empty()) {
      proc.emitWarning("HirctProcessFlatten: flatten failed, skipping");
      continue;
    }

    unsigned num_results = proc.getNumResults();
    if (flattened.size() != num_results) {
      proc.emitWarning("HirctProcessFlatten: flattened count mismatch");
      continue;
    }
    for (unsigned k = 0; k < num_results; ++k)
      proc.getResult(k).replaceAllUsesWith(flattened[k]);
    proc.erase();
  }
}

std::unique_ptr<mlir::Pass> hirct::create_process_flatten_pass() {
  return std::make_unique<HirctProcessFlattenPass>();
}
