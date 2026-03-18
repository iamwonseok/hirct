#include "hirct/Support/VerilogLoader.h"
#include "hirct/Transforms/Passes.h"
#include "circt/Conversion/ImportVerilog.h"
#include "circt/Dialect/Comb/CombDialect.h"
#include "circt/Dialect/HW/HWDialect.h"
#include "circt/Dialect/LLHD/LLHDDialect.h"
#include "circt/Dialect/Moore/MooreDialect.h"
#include "circt/Dialect/SV/SVDialect.h"
#include "circt/Dialect/Seq/SeqDialect.h"
#include "circt/Dialect/Sim/SimDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"

namespace hirct {

VerilogLoadResult load_verilog(mlir::MLIRContext &ctx,
                               const VerilogLoadOptions &opts) {
  VerilogLoadResult result;

  ctx.loadDialect<circt::hw::HWDialect>();
  ctx.loadDialect<circt::comb::CombDialect>();
  ctx.loadDialect<circt::seq::SeqDialect>();
  ctx.loadDialect<circt::llhd::LLHDDialect>();
  ctx.loadDialect<circt::sv::SVDialect>();
  ctx.loadDialect<circt::sim::SimDialect>();
  ctx.loadDialect<circt::moore::MooreDialect>();

  llvm::SourceMgr source_mgr;
  for (const auto &file : opts.input_files) {
    auto buf = llvm::MemoryBuffer::getFile(file);
    if (!buf) {
      result.error_message = "cannot open: " + file;
      return result;
    }
    source_mgr.AddNewSourceBuffer(std::move(*buf), llvm::SMLoc());
  }

  circt::ImportVerilogOptions import_opts;
  import_opts.libDirs = opts.lib_dirs;
  import_opts.includeDirs = opts.include_dirs;
  import_opts.libraryFiles = opts.lib_files;
  if (!opts.top_module.empty())
    import_opts.topModules.push_back(opts.top_module);

  result.module = mlir::ModuleOp::create(mlir::UnknownLoc::get(&ctx));
  mlir::TimingScope ts;
  if (mlir::failed(circt::importVerilog(source_mgr, &ctx, ts,
                                        *result.module, &import_opts))) {
    result.error_message = "importVerilog failed";
    result.module.release();
    return result;
  }

  {
    mlir::PassManager pm(&ctx);
    if (opts.enable_timing)
      pm.enableTiming();
    circt::populateMooreToCorePipeline(pm);
    circt::LlhdToCorePipelineOptions llhd_opts;
    llhd_opts.detectMemories = false;
    llhd_opts.sroa = false;
    circt::populateLlhdToCorePipeline(pm, llhd_opts);

    hirct::populate_hirct_lowering_pipeline(pm);

    if (mlir::failed(pm.run(*result.module))) {
      result.error_message = "moore/llhd-to-core lowering failed";
      result.module.release();
      return result;
    }
  }

  if (opts.canonicalize) {
    mlir::PassManager pm(&ctx);
    if (opts.enable_timing)
      pm.enableTiming();
    pm.addPass(mlir::createCanonicalizerPass());
    if (mlir::failed(pm.run(*result.module))) {
      result.error_message = "canonicalize pass failed";
      result.module.release();
      return result;
    }
  }

  result.success = true;
  return result;
}

} // namespace hirct
