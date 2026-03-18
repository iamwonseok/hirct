#include "hirct/Support/PathUtils.h"
#include "hirct/Support/VerilogLoader.h"
#include "hirct/Target/GenMakefile.h"
#include "hirct/Target/GenModel.h"
#include "hirct/Target/GenVerify.h"

#include "circt/Dialect/HW/HWOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"

#include "llvm/Support/Program.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace {

struct VerifyOptions {
  std::string input_path;
  std::string output_dir = "output";
  std::vector<std::string> lib_dirs;
  int seeds = 1;
  int cycles = 100;
  bool help = false;
  bool parse_error = false;
};

void print_usage(const char *prog) {
  std::cout << "Usage: " << prog
            << " [options] <input.v>\n\n"
               "Options:\n"
               "  -o <path>       Output directory (default: output)\n"
               "  --lib-dir <dir> Add Verilator library search directory "
               "(-y), repeatable\n"
               "  --seeds <N>     Number of random seeds (default: 1)\n"
               "  --cycles <N>    Number of simulation cycles (default: 100)\n"
               "  --help          Show this help message\n";
}

VerifyOptions parse_args(int argc, char *argv[]) {
  VerifyOptions opts;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      opts.help = true;
      return opts;
    }
    if (arg == "-o" && i + 1 < argc) {
      opts.output_dir = argv[++i];
    } else if (arg == "--seeds" && i + 1 < argc) {
      try {
        opts.seeds = std::stoi(argv[++i]);
        if (opts.seeds <= 0) {
          throw std::runtime_error("seeds must be positive");
        }
      } catch (const std::exception &) {
        std::cerr << "Invalid value for --seeds\n";
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
    } else if (arg == "--cycles" && i + 1 < argc) {
      try {
        opts.cycles = std::stoi(argv[++i]);
        if (opts.cycles <= 0) {
          throw std::runtime_error("cycles must be positive");
        }
      } catch (const std::exception &) {
        std::cerr << "Invalid value for --cycles\n";
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
    } else if (arg == "--lib-dir" && i + 1 < argc) {
      opts.lib_dirs.push_back(argv[++i]);
    } else if (arg[0] != '-') {
      opts.input_path = arg;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      opts.help = true;
      opts.parse_error = true;
      return opts;
    }
  }
  return opts;
}

} // namespace

int main(int argc, char *argv[]) {
  auto opts = parse_args(argc, argv);

  if (opts.help) {
    print_usage(argv[0]);
    return opts.parse_error ? 1 : 0;
  }

  if (opts.input_path.empty()) {
    std::cerr << "Error: input file is required\n";
    print_usage(argv[0]);
    return 1;
  }

  bool is_mlir_input =
      opts.input_path.size() >= 5 &&
      opts.input_path.compare(opts.input_path.size() - 5, 5, ".mlir") == 0;
  if (is_mlir_input) {
    std::cerr << "Error: hirct-verify requires Verilog input.\n"
              << "MLIR input cannot provide RTL source for Verilator "
                 "co-simulation.\n"
              << "Use a .v or .sv file instead.\n";
    return 1;
  }

  mlir::MLIRContext mlir_ctx;
  hirct::VerilogLoadOptions load_opts;
  load_opts.input_files = {opts.input_path};
  load_opts.lib_dirs = opts.lib_dirs;

  auto load_result = hirct::load_verilog(mlir_ctx, load_opts);
  if (!load_result.success) {
    std::cerr << "Error: " << load_result.error_message << "\n";
    return 1;
  }

  circt::hw::HWModuleOp top_hw;
  load_result.module->walk(
      [&](circt::hw::HWModuleOp m) { top_hw = m; });
  if (!top_hw) {
    std::cerr << "Error: no hw.module found\n";
    return 1;
  }

  std::string module_name = top_hw.getSymName().str();

  std::string out_root = opts.output_dir;
  std::string mod_output_dir = out_root + "/" + module_name;
  if (mkdir(out_root.c_str(), 0755) != 0 && errno != EEXIST) {
    std::cerr << "Error: cannot create output directory: " << out_root << ": "
              << strerror(errno) << "\n";
    return 1;
  }
  if (mkdir(mod_output_dir.c_str(), 0755) != 0 && errno != EEXIST) {
    std::cerr << "Error: cannot create module output directory: "
              << mod_output_dir << ": " << strerror(errno) << "\n";
    return 1;
  }

  hirct::GenModel gen_model(top_hw, *load_result.module);
  if (!gen_model.emit(mod_output_dir)) {
    std::cerr << "Error: GenModel failed";
    if (!gen_model.last_error_reason().empty())
      std::cerr << " (" << gen_model.last_error_reason() << ")";
    std::cerr << "\n";
    return 1;
  }

  hirct::GenVerify gen_verify(top_hw);
  if (!gen_verify.emit(mod_output_dir)) {
    std::cerr << "Error: GenVerify failed\n";
    return 1;
  }

  std::string rtl_src = hirct::to_absolute_path(opts.input_path);
  hirct::GenMakefile gen_makefile(top_hw, rtl_src, opts.lib_dirs);
  if (!gen_makefile.emit(mod_output_dir)) {
    std::cerr << "Error: GenMakefile failed\n";
    return 1;
  }

  llvm::ErrorOr<std::string> make_path_or =
      llvm::sys::findProgramByName("make");
  if (!make_path_or) {
    std::cerr << "Error: cannot find 'make' in PATH\n";
    return 1;
  }
  std::string make_exe = *make_path_or;

  for (int seed = 1; seed <= opts.seeds; ++seed) {
    std::cout << "[hirct-verify] running seed " << seed << "/" << opts.seeds
              << " (cycles=" << opts.cycles << ")\n";
    std::string seed_str = "SEED=" + std::to_string(seed);
    std::string cycles_str = "CYCLES=" + std::to_string(opts.cycles);
    llvm::SmallVector<llvm::StringRef, 8> args = {
        make_exe, "-C", mod_output_dir, "test-verify", seed_str, cycles_str};
    std::string err_msg;
    bool exec_failed = false;
    int rc = llvm::sys::ExecuteAndWait(make_exe, args,
                                       /*Env=*/std::nullopt,
                                       /*Redirects=*/{},
                                       /*SecondsToWait=*/300,
                                       /*MemoryLimit=*/0,
                                       &err_msg,
                                       &exec_failed);
    if (rc != 0) {
      std::cerr << "Error: verification failed for seed " << seed << " (exit "
                << rc << ")\n";
      if (exec_failed && !err_msg.empty()) {
        std::cerr << err_msg << "\n";
      }
      return rc;
    }
  }

  std::cout << "hirct-verify: PASS (" << opts.seeds << " seeds, " << opts.cycles
            << " cycles)\n";
  return 0;
}
