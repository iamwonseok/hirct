#ifndef HIRCT_SUPPORT_CIRCTRUNNER_H
#define HIRCT_SUPPORT_CIRCTRUNNER_H

#include <string>
#include <vector>

namespace hirct {

struct RunResult {
  int exit_code;
  std::string stdout_str;
  std::string stderr_str;
};

class CirctRunner {
public:
  CirctRunner() = default;

  RunResult run_circt_verilog(const std::string &input_path);
  RunResult run_circt_verilog_multi(const std::vector<std::string> &inputs,
                                    const std::string &top,
                                    const std::string &timescale = "1ns/1ps");
  RunResult run_circt_opt(const std::string &mlir_content,
                          const std::vector<std::string> &passes);

  void set_timeout(int seconds) { timeout_ = seconds; }
  void set_canonicalize(bool enable) { canonicalize_ = enable; }

private:
  int timeout_ = 60;
  bool canonicalize_ = false;

  RunResult run_process(const std::vector<std::string> &args,
                        const std::string &stdin_data = "");
};

} // namespace hirct

#endif // HIRCT_SUPPORT_CIRCTRUNNER_H
