#include "hirct/Transforms/Passes.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/LLHD/LLHDOps.h"
#include "mlir/Pass/Pass.h"

namespace {

struct HirctSimCleanupPass
    : public mlir::OperationPass<circt::hw::HWModuleOp> {

  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(HirctSimCleanupPass)

  HirctSimCleanupPass()
      : mlir::OperationPass<circt::hw::HWModuleOp>(
            mlir::TypeID::get<HirctSimCleanupPass>()) {}

  llvm::StringRef getName() const override { return "HirctSimCleanup"; }

  llvm::StringRef getDescription() const override {
    return "Remove no-result LLHD processes containing llhd.halt";
  }

  std::unique_ptr<mlir::Pass> clonePass() const override {
    return std::make_unique<HirctSimCleanupPass>();
  }

  void runOnOperation() override;
};

} // namespace

void HirctSimCleanupPass::runOnOperation() {
  auto module_op = getOperation();
  llvm::SmallVector<circt::llhd::ProcessOp> to_erase;

  for (auto &op : module_op.getBodyBlock()->getOperations()) {
    auto proc = mlir::dyn_cast<circt::llhd::ProcessOp>(op);
    if (!proc || proc.getNumResults() != 0)
      continue;
    bool has_halt = false;
    proc.walk([&](circt::llhd::HaltOp) { has_halt = true; });
    if (has_halt)
      to_erase.push_back(proc);
  }

  for (auto proc : to_erase)
    proc.erase();
}

std::unique_ptr<mlir::Pass> hirct::create_sim_cleanup_pass() {
  return std::make_unique<HirctSimCleanupPass>();
}
