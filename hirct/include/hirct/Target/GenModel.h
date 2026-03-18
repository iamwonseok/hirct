#ifndef HIRCT_TARGET_GENMODEL_H
#define HIRCT_TARGET_GENMODEL_H

#include "hirct/Analysis/IRAnalysis.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/LLHD/LLHDOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace hirct {

class GenModel {
public:
  GenModel(circt::hw::HWModuleOp hw_module, mlir::ModuleOp mlir_module);
  bool emit(const std::string &output_dir, bool is_submodule = false);
  const std::string &last_error_reason() const { return last_error_reason_; }

private:
  circt::hw::HWModuleOp hw_module_;
  mlir::ModuleOp mlir_module_;
  std::unique_ptr<mlir::SymbolTable> symbol_table_;
  std::string last_error_reason_;
  int flatten_budget_ = 1'000'000;
  std::string inst_include_prefix_;

  std::string cpp_type_for_width(int width) const;
  std::string ssa_to_ident(const std::string &ssa_name) const;
  static std::string sanitize_inst_name(const std::string &name);

  bool emit_header(const std::string &dir);
  bool emit_impl(const std::string &dir);
  void emit_reset(std::ofstream &ofs, const std::string &name);
  void emit_eval_comb(std::ofstream &ofs, const std::string &name);
  void emit_step(std::ofstream &ofs, const std::string &name);
  void emit_domain_step(std::ofstream &ofs, const std::string &name,
                        const hirct::ClockDomainView &domain);
  void emit_save_old_ssa(std::ofstream &ofs, const std::string &name);

  struct FlattenResult {
    std::vector<std::string> values;
    bool is_array = false;
    unsigned array_depth = 0;
    unsigned array_elem_width = 0;
  };

  FlattenResult flatten_process(
      circt::llhd::ProcessOp proc,
      llvm::DenseMap<mlir::Value, std::string> &val,
      std::ofstream &ofs, int &tmp_cnt);

  using ArrayInfoMap = std::map<std::string, std::pair<unsigned, unsigned>>;

  std::vector<std::string> flatten_block(
      mlir::Block *block, mlir::Block *wait_block,
      llvm::DenseMap<mlir::Value, std::string> &val,
      ArrayInfoMap &array_info,
      llvm::SmallPtrSet<mlir::Block *, 32> &path,
      std::ofstream &ofs, int &tmp_cnt, int depth);

  bool try_unroll_loop(
      mlir::Block *header, mlir::Block *body_block,
      mlir::Block *exit_target,
      llvm::SmallVector<mlir::Value> exit_args,
      mlir::Block *wait_block,
      llvm::DenseMap<mlir::Value, std::string> &val,
      ArrayInfoMap &array_info,
      llvm::SmallPtrSet<mlir::Block *, 32> &path,
      std::ofstream &ofs, int &tmp_cnt,
      std::vector<std::string> &results);
};

} // namespace hirct

#endif // HIRCT_TARGET_GENMODEL_H
