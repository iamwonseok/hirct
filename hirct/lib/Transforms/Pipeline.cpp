#include "hirct/Transforms/Passes.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/LLHD/LLHDPasses.h"
#include "mlir/Transforms/Passes.h"

void hirct::populate_hirct_lowering_pipeline(mlir::PassManager &pm) {
  auto &module_pm = pm.nest<circt::hw::HWModuleOp>();
  module_pm.addPass(hirct::create_sim_cleanup_pass());
  module_pm.addPass(hirct::create_unroll_process_loops_pass());
  // RemoveControlFlow converts cf.br/cf.cond_br into combinational form;
  // Canonicalizer folds the resulting trivial ops before ProcessFlatten.
  module_pm.addPass(circt::llhd::createRemoveControlFlowPass());
  module_pm.addPass(mlir::createCanonicalizerPass());
  module_pm.addPass(hirct::create_process_flatten_pass());
  module_pm.addPass(hirct::create_process_deseq_pass());
  module_pm.addPass(hirct::create_signal_lowering_pass());
  module_pm.addPass(mlir::createCSEPass());
  module_pm.addPass(mlir::createCanonicalizerPass());
}
