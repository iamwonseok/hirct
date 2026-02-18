#include "hirct/Support/CirctRunner.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

namespace hirct {

namespace {

// CirctRunner is NOT thread-safe: g_child_pid is a global used by the
// SIGALRM handler to kill a timed-out child process.
volatile sig_atomic_t g_child_pid = 0;

void alarm_handler(int /*sig*/) {
  if (g_child_pid > 0) {
    kill(g_child_pid, SIGKILL);
  }
}

bool write_all(int fd, const char *data, size_t len) {
  while (len > 0) {
    ssize_t n = write(fd, data, len);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return false;
    }
    data += n;
    len -= static_cast<size_t>(n);
  }
  return true;
}

std::string read_fd_to_string(int fd) {
  std::string result;
  char buf[4096];
  ssize_t n;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    result.append(buf, static_cast<size_t>(n));
  }
  return result;
}

std::string read_file_to_string(const std::string &path) {
  std::ifstream ifs(path);
  if (!ifs) {
    return "";
  }
  std::ostringstream ss;
  ss << ifs.rdbuf();
  return ss.str();
}

} // namespace

RunResult CirctRunner::run_process(const std::vector<std::string> &args,
                                   const std::string &stdin_data) {
  RunResult result{-1, "", ""};

  // Temp file for stdout (avoids pipe buffer deadlock with large MLIR)
  char stdout_template[] = "/tmp/hirct_stdout_XXXXXX";
  int stdout_fd = mkstemp(stdout_template);
  if (stdout_fd < 0) {
    result.stderr_str = "failed to create temp file for stdout";
    return result;
  }

  // Pipe for stderr (small, <64KB)
  int stderr_pipe[2];
  if (pipe(stderr_pipe) < 0) {
    close(stdout_fd);
    unlink(stdout_template);
    result.stderr_str = "failed to create stderr pipe";
    return result;
  }

  // Pipe for stdin if needed
  int stdin_pipe[2] = {-1, -1};
  if (!stdin_data.empty()) {
    if (pipe(stdin_pipe) < 0) {
      close(stdout_fd);
      unlink(stdout_template);
      close(stderr_pipe[0]);
      close(stderr_pipe[1]);
      result.stderr_str = "failed to create stdin pipe";
      return result;
    }
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(stdout_fd);
    unlink(stdout_template);
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
    if (stdin_pipe[0] >= 0) {
      close(stdin_pipe[0]);
      close(stdin_pipe[1]);
    }
    result.stderr_str = "fork failed";
    return result;
  }

  if (pid == 0) {
    // Child
    dup2(stdout_fd, STDOUT_FILENO);
    close(stdout_fd);

    close(stderr_pipe[0]);
    dup2(stderr_pipe[1], STDERR_FILENO);
    close(stderr_pipe[1]);

    if (!stdin_data.empty()) {
      close(stdin_pipe[1]);
      dup2(stdin_pipe[0], STDIN_FILENO);
      close(stdin_pipe[0]);
    }

    std::vector<char *> c_args;
    for (const auto &a : args) {
      c_args.push_back(const_cast<char *>(a.c_str()));
    }
    c_args.push_back(nullptr);

    execvp(c_args[0], c_args.data());
    _exit(127);
  }

  // Parent
  close(stdout_fd);
  close(stderr_pipe[1]);

  if (!stdin_data.empty()) {
    close(stdin_pipe[0]);
    if (!write_all(stdin_pipe[1], stdin_data.data(), stdin_data.size())) {
      result.stderr_str = "failed to write stdin data to pipe";
    }
    close(stdin_pipe[1]);
  }

  // Set timeout — save old handler so we restore it afterwards
  g_child_pid = pid;
  struct sigaction sa {
  }, old_sa{};
  sa.sa_handler = alarm_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGALRM, &sa, &old_sa);
  alarm(static_cast<unsigned>(timeout_));

  result.stderr_str = read_fd_to_string(stderr_pipe[0]);
  close(stderr_pipe[0]);

  int status = 0;
  waitpid(pid, &status, 0);
  alarm(0);
  g_child_pid = 0;
  sigaction(SIGALRM, &old_sa, nullptr);

  if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    result.exit_code = 128 + WTERMSIG(status);
    if (WTERMSIG(status) == SIGKILL) {
      result.stderr_str += "\nprocess killed: timeout after " +
                           std::to_string(timeout_) + " seconds";
    }
  }

  result.stdout_str = read_file_to_string(stdout_template);
  unlink(stdout_template);

  return result;
}

RunResult CirctRunner::run_circt_verilog(const std::string &input_path) {
  std::vector<std::string> args = {"circt-verilog", input_path};
  if (canonicalize_) {
    args.insert(args.begin() + 1, "--canonicalize");
  }
  return run_process(args);
}

RunResult
CirctRunner::run_circt_verilog_multi(const std::vector<std::string> &inputs,
                                     const std::string &top,
                                     const std::string &timescale) {
  std::vector<std::string> args = {"circt-verilog"};
  args.push_back("--timescale=" + timescale);
  args.push_back("--top=" + top);
  for (const auto &input : inputs) {
    args.push_back(input);
  }
  if (canonicalize_) {
    args.insert(args.begin() + 1, "--canonicalize");
  }
  return run_process(args);
}

RunResult CirctRunner::run_circt_opt(const std::string &mlir_content,
                                     const std::vector<std::string> &passes) {
  char tmp_template[] = "/tmp/hirct_mlir_XXXXXX";
  int tmp_fd = mkstemp(tmp_template);
  if (tmp_fd < 0) {
    return {-1, "", "failed to create temp file for MLIR input"};
  }
  if (!write_all(tmp_fd, mlir_content.data(), mlir_content.size())) {
    close(tmp_fd);
    unlink(tmp_template);
    return {-1, "", "failed to write MLIR to temp file"};
  }
  close(tmp_fd);

  std::vector<std::string> args = {"circt-opt"};
  for (const auto &pass : passes) {
    args.push_back(pass);
  }
  args.emplace_back(tmp_template);

  auto result = run_process(args);
  unlink(tmp_template);
  return result;
}

} // namespace hirct
