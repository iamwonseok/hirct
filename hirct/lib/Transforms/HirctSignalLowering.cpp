#include "hirct/Transforms/Passes.h"
#include "circt/Dialect/Comb/CombOps.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/LLHD/LLHDOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "hirct-signal-lowering"

namespace {

struct HirctSignalLoweringPass
    : public mlir::OperationPass<circt::hw::HWModuleOp> {

  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(HirctSignalLoweringPass)

  HirctSignalLoweringPass()
      : mlir::OperationPass<circt::hw::HWModuleOp>(
            mlir::TypeID::get<HirctSignalLoweringPass>()) {}

  llvm::StringRef getName() const override { return "HirctSignalLowering"; }

  llvm::StringRef getDescription() const override {
    return "Lower LLHD signal operations to hw/comb/seq primitives";
  }

  std::unique_ptr<mlir::Pass> clonePass() const override {
    return std::make_unique<HirctSignalLoweringPass>();
  }

  void runOnOperation() override;
};

struct SignalInfo {
  circt::llhd::SignalOp sig;
  llvm::SmallVector<circt::llhd::DriveOp> drives;
  llvm::SmallVector<circt::llhd::ProbeOp> probes;
};

mlir::Value compute_final_value(mlir::OpBuilder &builder, SignalInfo &info) {
  mlir::Value init = info.sig.getInit();

  if (info.drives.empty())
    return init;

  bool hasConditionalDrive =
      llvm::any_of(info.drives, [](circt::llhd::DriveOp drv) {
        return static_cast<bool>(drv.getEnable());
      });
  bool isArraySignal =
      static_cast<bool>(mlir::dyn_cast<circt::hw::ArrayType>(init.getType()));

  // Conditional signal drives must retain the current signal contents when the
  // enable is low. Seeding from the init value collapses array-style holds into
  // zero on disabled cycles.
  mlir::Value result =
      (isArraySignal && hasConditionalDrive && !info.probes.empty())
          ? info.probes.front().getResult()
          : init;
  for (auto drv : info.drives) {
    mlir::Value drive_val = drv.getValue();
    mlir::Value enable = drv.getEnable();
    if (enable) {
      builder.setInsertionPoint(drv);
      auto mux = circt::comb::MuxOp::create(builder, drv.getLoc(), enable,
                                             drive_val, result);
      result = mux.getResult();
    } else {
      result = drive_val;
    }
  }
  return result;
}

} // namespace

void HirctSignalLoweringPass::runOnOperation() {
  auto module_op = getOperation();

  llvm::SmallVector<SignalInfo> signals;
  llvm::DenseMap<mlir::Value, size_t> sig_to_idx;

  module_op.walk([&](circt::llhd::SignalOp sig) {
    size_t idx = signals.size();
    sig_to_idx[sig.getResult()] = idx;
    signals.push_back({sig, {}, {}});
  });

  module_op.walk([&](circt::llhd::DriveOp drv) {
    auto it = sig_to_idx.find(drv.getSignal());
    if (it != sig_to_idx.end())
      signals[it->second].drives.push_back(drv);
  });

  module_op.walk([&](circt::llhd::ProbeOp prb) {
    auto it = sig_to_idx.find(prb.getSignal());
    if (it != sig_to_idx.end())
      signals[it->second].probes.push_back(prb);
  });

  mlir::OpBuilder builder(module_op.getContext());

  for (auto &info : signals) {
    mlir::Value sig_val = info.sig.getResult();
    mlir::Block *sig_block = info.sig->getBlock();

    bool can_lower = true;
    for (auto *user : sig_val.getUsers()) {
      if (!mlir::isa<circt::llhd::DriveOp, circt::llhd::ProbeOp>(user)) {
        can_lower = false;
        break;
      }
      if (user->getBlock() != sig_block) {
        can_lower = false;
        break;
      }
    }

    if (!can_lower)
      continue;

    mlir::Value final_val = compute_final_value(builder, info);

    for (auto prb : info.probes)
      prb.getResult().replaceAllUsesWith(final_val);

    for (auto drv : info.drives)
      drv.erase();
    for (auto prb : info.probes)
      prb.erase();
    info.sig.erase();
  }
}

std::unique_ptr<mlir::Pass> hirct::create_signal_lowering_pass() {
  return std::make_unique<HirctSignalLoweringPass>();
}
