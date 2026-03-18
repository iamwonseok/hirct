#ifndef HIRCT_TRANSFORMS_PASSES_H
#define HIRCT_TRANSFORMS_PASSES_H

#include "mlir/Pass/Pass.h"
#include <memory>

namespace mlir {
class PassManager;
} // namespace mlir

namespace hirct {

void populate_hirct_lowering_pipeline(mlir::PassManager &pm);

std::unique_ptr<mlir::Pass> create_sim_cleanup_pass();
std::unique_ptr<mlir::Pass> create_unroll_process_loops_pass();
std::unique_ptr<mlir::Pass> create_process_flatten_pass();
std::unique_ptr<mlir::Pass> create_process_deseq_pass();
std::unique_ptr<mlir::Pass> create_signal_lowering_pass();

} // namespace hirct
#endif // HIRCT_TRANSFORMS_PASSES_H
