#ifndef HIRCT_TARGET_GENTB_H
#define HIRCT_TARGET_GENTB_H

#include "circt/Dialect/HW/HWOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/SymbolTable.h"
#include <fstream>
#include <memory>
#include <string>

namespace hirct {

struct ApbPortNames {
  std::string paddr;
  std::string pwdata;
  std::string prdata;
  std::string pwrite;
  std::string psel;
  std::string penable;
  std::string pready;
};

class GenTB {
public:
  GenTB(circt::hw::HWModuleOp hw_module, mlir::ModuleOp mlir_module);
  bool emit(const std::string &output_dir);
  bool emit_cosim(const std::string &output_dir);

private:
  circt::hw::HWModuleOp hw_module_;
  mlir::ModuleOp mlir_module_;
  std::unique_ptr<mlir::SymbolTable> symbol_table_;

  static bool is_preset_port(const std::string &name);

  void emit_cosim_port_signals(std::ofstream &ofs);
  void emit_cosim_rtl_instance(std::ofstream &ofs);
  void emit_cosim_dpi_instance(std::ofstream &ofs);
  void emit_cosim_clock_gen(std::ofstream &ofs, const std::string &clock_name);
  void emit_cosim_waveform_dump(std::ofstream &ofs, const std::string &tb_name);
  void emit_cosim_stimulus(std::ofstream &ofs, const std::string &clock_name,
                           const std::string &reset_name, bool active_low,
                           const std::string &preset_name);
  void emit_cosim_comparison(std::ofstream &ofs, const std::string &clock_name);
};

} // namespace hirct

#endif // HIRCT_TARGET_GENTB_H
