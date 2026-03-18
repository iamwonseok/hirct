#include "hirct/Support/VerilatorPreprocessor.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Program.h"

#include <iostream>

namespace hirct {

PreprocessResult run_verilator_preprocess(const PreprocessOptions &opts) {
  PreprocessResult result;

  // Find verilator binary
  std::string verilator = opts.verilator_path;
  if (verilator.empty()) {
    auto found = llvm::sys::findProgramByName("verilator");
    if (!found) {
      result.error_message = "verilator not found in PATH";
      return result;
    }
    verilator = *found;
  }

  // Build argument list; owned_args keeps strings alive for StringRef
  llvm::SmallVector<llvm::StringRef, 32> args;
  args.push_back(verilator);
  args.push_back("-E");
  args.push_back("--pp-comments");

  std::vector<std::string> owned_args;
  for (const auto &def : opts.defines)
    owned_args.push_back("+define+" + def);
  for (const auto &dir : opts.inc_dirs)
    owned_args.push_back("+incdir+" + dir);
  for (const auto &dir : opts.lib_dirs) {
    owned_args.push_back("-y");
    owned_args.push_back(dir);
  }
  for (const auto &file : opts.lib_files) {
    owned_args.push_back("-v");
    owned_args.push_back(file);
  }
  for (const auto &a : owned_args)
    args.push_back(a);
  for (const auto &file : opts.input_files)
    args.push_back(file);

  if (opts.verbose) {
    std::cout << "[preprocess] Running: " << verilator;
    for (size_t i = 1; i < args.size(); ++i)
      std::cout << " " << args[i].str();
    std::cout << "\n";
  }

  // Execute with stdout redirected to output file
  std::optional<llvm::StringRef> redirects[] = {
      std::nullopt,
      llvm::StringRef(opts.output_path),
      std::nullopt,
  };

  std::string err_msg;
  result.exit_code = llvm::sys::ExecuteAndWait(verilator, args, std::nullopt,
                                               redirects, 0, 0, &err_msg);
  result.success = (result.exit_code == 0);

  if (result.success) {
    result.output_file = opts.output_path;
    uint64_t file_size;
    if (llvm::sys::fs::file_size(opts.output_path, file_size) ||
        file_size == 0) {
      result.success = false;
      result.error_message = "verilator -E produced empty output";
    }
  } else {
    result.error_message =
        err_msg.empty() ? "verilator -E failed with exit code " +
                              std::to_string(result.exit_code)
                        : err_msg;
  }

  return result;
}

} // namespace hirct
