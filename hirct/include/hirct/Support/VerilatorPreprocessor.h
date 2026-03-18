#ifndef HIRCT_SUPPORT_VERILATORPREPROCESSOR_H
#define HIRCT_SUPPORT_VERILATORPREPROCESSOR_H

#include <string>
#include <vector>

namespace hirct {

struct PreprocessOptions {
  std::vector<std::string> input_files;
  std::vector<std::string> defines;
  std::vector<std::string> inc_dirs;
  std::vector<std::string> lib_dirs;
  std::vector<std::string> lib_files;
  std::string output_path;
  std::string verilator_path;
  bool verbose = false;
};

struct PreprocessResult {
  std::string output_file;
  std::string error_message;
  int exit_code = -1;
  bool success = false;
};

PreprocessResult run_verilator_preprocess(const PreprocessOptions &opts);

} // namespace hirct

#endif // HIRCT_SUPPORT_VERILATORPREPROCESSOR_H
