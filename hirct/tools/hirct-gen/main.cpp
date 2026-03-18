#include "hirct/Analysis/IRAnalysis.h"
#include "hirct/Support/PathUtils.h"
#include "hirct/Support/VerilatorPreprocessor.h"
#include "hirct/Support/VerilogLoader.h"
#include "hirct/Target/GenCocotb.h"
#include "hirct/Target/GenDPIC.h"
#include "hirct/Target/GenDoc.h"
#include "hirct/Target/GenFormat.h"
#include "hirct/Target/GenFuncModel.h"
#include "hirct/Target/GenMakefile.h"
#include "hirct/Target/GenModel.h"
#include "hirct/Target/GenRAL.h"
#include "hirct/Target/GenTB.h"
#include "hirct/Target/GenVerify.h"
#include "hirct/Target/GenWrapper.h"
#include "hirct/Transforms/Passes.h"

#include "circt/Dialect/Comb/CombDialect.h"
#include "circt/Dialect/HW/HWDialect.h"
#include "circt/Dialect/HW/HWOps.h"
#include "circt/Dialect/LLHD/LLHDDialect.h"
#include "circt/Dialect/LLHD/LLHDPasses.h"
#include "circt/Dialect/SV/SVDialect.h"
#include "circt/Dialect/Seq/SeqDialect.h"
#include "circt/Dialect/Sim/SimDialect.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

struct Options {
  std::string input_path;
  std::string output_dir = "output";
  std::string only_filter;
  std::string top_module;
  std::string filelist;
  std::string timescale = "1ns/10ps";
  std::string preprocess = "none";
  std::string verilator_path;
  std::string pipeline;
  std::string pipeline_checkpoint_dir;
  std::vector<std::string> lib_dirs;
  bool no_auto_lib = false;
  bool dump_ir = false;
  bool timing = false;
  bool verbose = false;
  bool help = false;
  bool parse_error = false;
};

void print_usage(const char *prog) {
  std::cout << "Usage: " << prog
            << " [options] <input.v>\n\n"
               "Options:\n"
               "  -o <dir>          Output directory (default: output)\n"
               "  --only <filter>   Generate only matching modules\n"
               "  --top <module>    Specify top module name\n"
               "  -f <filelist>     Read input files from filelist\n"
               "  --lib-dir <dir>   Add library search directory (-y) "
               "(repeatable)\n"
               "  --no-auto-lib     Disable automatic library directory "
               "detection\n"
               "  --preprocess <mode>  Preprocessing mode: none, verilator "
               "(default: none)\n"
               "  --verilator-path <p> Path to verilator binary (default: PATH "
               "lookup)\n"
               "  --timescale <ts>  Default timescale (default: 1ns/10ps)\n"
               "  --pipeline <passes>        Pass chain (comma-separated, default: full lowering)\n"
               "                             Passes: sim-cleanup, unroll-process-loops, remove-control-flow,\n"
               "                                     canonicalize, process-flatten, process-deseq,\n"
               "                                     signal-lowering, cse\n"
               "  --pipeline-checkpoint-dir <dir>  Save IR after each pass as {N}_{name}.mlir\n"
               "  --dump-ir                  Print final IR to stdout after pipeline and exit\n"
               "  --timing          Enable PassManager timing statistics\n"
               "  --verbose         Enable verbose output\n"
               "  --help            Show this help message\n";
}

Options parse_args(int argc, char *argv[]) {
  Options opts;

  auto consume_value = [&](const std::string &opt_name, int &i,
                           std::string &target) -> bool {
    if (i + 1 >= argc || argv[i + 1][0] == '-') {
      std::cerr << "error: option " << opt_name
                << " requires a non-option value\n";
      return false;
    }
    target = argv[++i];
    return true;
  };

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      opts.help = true;
      return opts;
    }
    if (arg == "-o") {
      if (!consume_value("-o", i, opts.output_dir)) {
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
    } else if (arg == "--only") {
      if (!consume_value("--only", i, opts.only_filter)) {
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
    } else if (arg == "--top") {
      if (!consume_value("--top", i, opts.top_module)) {
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
    } else if (arg == "-f") {
      if (!consume_value("-f", i, opts.filelist)) {
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
    } else if (arg == "--lib-dir") {
      std::string dir;
      if (!consume_value("--lib-dir", i, dir)) {
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
      opts.lib_dirs.push_back(dir);
    } else if (arg == "--no-auto-lib") {
      opts.no_auto_lib = true;
    } else if (arg == "--dump-ir") {
      opts.dump_ir = true;
    } else if (arg == "--timing") {
      opts.timing = true;
    } else if (arg == "--pipeline") {
      if (!consume_value("--pipeline", i, opts.pipeline)) {
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
    } else if (arg == "--pipeline-checkpoint-dir") {
      if (!consume_value("--pipeline-checkpoint-dir", i,
                         opts.pipeline_checkpoint_dir)) {
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
    } else if (arg == "--timescale") {
      if (!consume_value("--timescale", i, opts.timescale)) {
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
    } else if (arg == "--preprocess") {
      if (!consume_value("--preprocess", i, opts.preprocess)) {
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
      if (opts.preprocess != "none" && opts.preprocess != "verilator") {
        std::cerr << "error: unknown preprocess mode '" << opts.preprocess
                  << "' (expected: none, verilator)\n";
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
    } else if (arg == "--verilator-path") {
      if (!consume_value("--verilator-path", i, opts.verilator_path)) {
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
    } else if (arg == "--verbose") {
      opts.verbose = true;
    } else if (arg[0] != '-') {
      if (!opts.input_path.empty()) {
        std::cerr << "error: multiple positional arguments not supported"
                     " (use -f for filelist)\n";
        opts.help = true;
        opts.parse_error = true;
        return opts;
      }
      opts.input_path = arg;
    } else {
      std::cerr << "error: unknown option: " << arg << "\n";
      opts.help = true;
      opts.parse_error = true;
      return opts;
    }
  }
  return opts;
}

bool is_directory(const std::string &path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

std::string parent_directory(const std::string &path) {
  auto pos = path.find_last_of('/');
  if (pos == std::string::npos)
    return ".";
  if (pos == 0)
    return "/";
  return path.substr(0, pos);
}

// Non-RTL directory names to skip when collecting rtl/ subdirs
static const std::set<std::string> RTL_SKIP_DIRS = {
    "build", "output", "sim", "tb", "testbench", "verification"};

void collect_subdirs(const std::string &base, std::set<std::string> &out,
                     int depth, int max_depth, size_t max_count,
                     bool &max_count_warned) {
  if (depth > max_depth || out.size() >= max_count)
    return;

  DIR *d = opendir(base.c_str());
  if (!d)
    return;

  while (struct dirent *ent = readdir(d)) {
    if (out.size() >= max_count) {
      if (!max_count_warned) {
        std::cerr << "warning: lib-dir auto-detection reached max count ("
                  << max_count << "), stopping search\n";
        max_count_warned = true;
      }
      break;
    }
    std::string name = ent->d_name;
    if (name == "." || name == "..")
      continue;
    if (!name.empty() && name[0] == '.')
      continue; // skip hidden directories
    if (RTL_SKIP_DIRS.count(name))
      continue; // skip non-RTL directories
    std::string full = base + "/" + name;
    if (is_directory(full)) {
      out.insert(full);
      collect_subdirs(full, out, depth + 1, max_depth, max_count,
                      max_count_warned);
    }
  }
  closedir(d);
}

std::vector<std::string>
collect_auto_lib_dirs(const std::vector<std::string> &inputs) {
  std::vector<std::string> result;
  std::set<std::string> seen;

  // Parent dirs of inputs first (preserve input order, first occurrence)
  for (const auto &input : inputs) {
    std::string pdir = parent_directory(input);
    if (is_directory(pdir) && seen.insert(pdir).second)
      result.push_back(pdir);
  }

  if (is_directory("rtl")) {
    if (seen.insert("rtl").second)
      result.push_back("rtl");
    std::set<std::string> rtl_subdirs;
    bool max_count_warned = false;
    collect_subdirs("rtl", rtl_subdirs, 0, 8, 200, max_count_warned);
    for (const auto &d : rtl_subdirs) {
      if (seen.insert(d).second)
        result.push_back(d);
    }
  }

  return result;
}

void warn_module_conflicts(const std::vector<std::string> &lib_dirs) {
  // Map: filename -> list of directories containing it
  std::map<std::string, std::vector<std::string>> file_to_dirs;

  for (const auto &dir : lib_dirs) {
    DIR *d = opendir(dir.c_str());
    if (!d)
      continue;
    while (struct dirent *ent = readdir(d)) {
      std::string name = ent->d_name;
      if (name.size() < 2 || name.compare(name.size() - 2, 2, ".v") != 0)
        continue;
      std::string full = dir + "/" + name;
      if (!is_directory(full))
        file_to_dirs[name].push_back(dir);
    }
    closedir(d);
  }

  for (const auto &p : file_to_dirs) {
    if (p.second.size() > 1) {
      std::cerr << "warning: module conflict: " << p.first
                << " found in multiple -y directories:\n";
      for (const auto &loc : p.second)
        std::cerr << "  " << loc << "\n";
    }
  }
}

std::set<std::string> parse_only_filter(const std::string &filter) {
  std::set<std::string> result;
  if (filter.empty()) {
    return result;
  }
  std::istringstream iss(filter);
  std::string token;
  while (std::getline(iss, token, ',')) {
    auto start = token.find_first_not_of(" \t");
    auto end = token.find_last_not_of(" \t");
    if (start != std::string::npos) {
      result.insert(token.substr(start, end - start + 1));
    }
  }
  return result;
}

bool emitter_allowed(const std::set<std::string> &filter,
                     const std::string &name) {
  return filter.empty() || filter.count(name) > 0;
}

struct FilelistResult {
  std::vector<std::string> input_files;
  std::vector<std::string> inc_dirs;
  std::vector<std::string> lib_dirs;
  std::vector<std::string> lib_files;
  std::vector<std::string> defines; // +define+NAME=VAL → "NAME=VAL"
  bool ok = true;
};

std::string expand_env_vars(const std::string &s) {
  std::string result;
  result.reserve(s.size());
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] != '$') {
      result += s[i];
      continue;
    }
    bool braced = (i + 1 < s.size() && s[i + 1] == '{');
    size_t name_start = braced ? i + 2 : i + 1;
    size_t name_end = name_start;

    while (name_end < s.size() &&
           (std::isalnum(static_cast<unsigned char>(s[name_end])) ||
            s[name_end] == '_')) {
      ++name_end;
    }

    if (name_end == name_start) {
      result += '$';
      continue;
    }

    std::string var_name = s.substr(name_start, name_end - name_start);
    size_t consume_end = name_end;

    if (braced) {
      if (name_end < s.size() && s[name_end] == '}') {
        consume_end = name_end + 1;
      } else {
        result += '$';
        continue;
      }
    }

    const char *val = std::getenv(var_name.c_str());
    if (val) {
      result += val;
    } else {
      std::cerr << "warning: undefined environment variable: " << var_name
                << "\n";
      result.append(s, i, consume_end - i);
    }
    i = consume_end - 1;
  }

  return result;
}

FilelistResult parse_filelist(const std::string &path) {
  FilelistResult fl;
  std::ifstream flist(path);
  if (!flist) {
    std::cerr << "Error: cannot open filelist: " << path << "\n";
    fl.ok = false;
    return fl;
  }

  std::string line;
  while (std::getline(flist, line)) {
    auto start = line.find_first_not_of(" \t\r");
    if (start == std::string::npos)
      continue;
    auto end = line.find_last_not_of(" \t\r");
    line = line.substr(start, end - start + 1);

    if (line.empty() || line[0] == '#')
      continue;
    if (line.size() >= 2 && line[0] == '/' && line[1] == '/')
      continue;

    line = expand_env_vars(line);

    // +define+NAME[=VAL][+NAME[=VAL]...]
    if (line.compare(0, 8, "+define+") == 0) {
      std::istringstream iss(line.substr(8));
      std::string def;
      while (std::getline(iss, def, '+')) {
        if (!def.empty())
          fl.defines.push_back(def);
      }
      continue;
    }

    // +incdir+<path>[+<path>...]
    if (line.compare(0, 8, "+incdir+") == 0) {
      std::istringstream iss(line.substr(8));
      std::string dir;
      while (std::getline(iss, dir, '+')) {
        if (!dir.empty())
          fl.inc_dirs.push_back(dir);
      }
      continue;
    }

    // -v <path> — library file
    if (line.compare(0, 3, "-v ") == 0 || line.compare(0, 3, "-v\t") == 0) {
      std::string rest = line.substr(3);
      auto fstart = rest.find_first_not_of(" \t");
      if (fstart != std::string::npos)
        fl.lib_files.push_back(rest.substr(fstart));
      continue;
    }

    // -y <dir> — library search directory
    if (line.compare(0, 3, "-y ") == 0 || line.compare(0, 3, "-y\t") == 0) {
      std::string rest = line.substr(3);
      auto fstart = rest.find_first_not_of(" \t");
      if (fstart != std::string::npos)
        fl.lib_dirs.push_back(rest.substr(fstart));
      continue;
    }

    // -sverilog — verilator handles SV natively, skip
    if (line == "-sverilog")
      continue;

    // .vh files are include headers — skip as compilation input
    if (line.size() >= 3 && line.compare(line.size() - 3, 3, ".vh") == 0) {
      continue;
    }

    fl.input_files.push_back(line);
  }

  return fl;
}

std::string json_escape(const std::string &s) {
  std::string result;
  result.reserve(s.size());
  for (char c : s) {
    switch (c) {
    case '"':
      result += "\\\"";
      break;
    case '\\':
      result += "\\\\";
      break;
    case '\n':
      result += "\\n";
      break;
    case '\r':
      result += "\\r";
      break;
    case '\t':
      result += "\\t";
      break;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        char buf[8];
        snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
        result += buf;
      } else {
        result += c;
      }
    }
  }
  return result;
}

struct RalMetaInfo {
  bool ok = false;
  bool skipped = false;
  std::string detection = "none";
};

struct EmitterResults {
  bool model_ok = false;
  bool model_skipped = false;
  std::string model_reason;
  bool func_model_ok = false;
  bool func_model_skipped = false;
  std::string func_model_reason;
  bool tb_ok = false;
  bool tb_skipped = false;
  bool makefile_ok = false;
  bool makefile_skipped = false;
  bool verify_ok = false;
  bool verify_skipped = false;
  bool dpic_ok = false;
  bool dpic_skipped = false;
  bool wrapper_ok = false;
  bool wrapper_skipped = false;
  bool format_ok = false;
  bool format_skipped = false;
  bool doc_ok = false;
  bool doc_skipped = false;
  bool cocotb_ok = false;
  bool cocotb_skipped = false;
  RalMetaInfo ral;
  bool ral_skipped = false;
};

bool write_meta_json(const std::string &path, const std::string &top,
                     bool mlir_ok, const std::string &reason,
                     const EmitterResults &results) {
  std::ofstream ofs(path);
  if (!ofs) {
    return false;
  }
  ofs << "{\n";
  ofs << "  \"path\": \"" << json_escape(path) << "\",\n";
  ofs << "  \"top\": \"" << json_escape(top) << "\",\n";
  ofs << "  \"mlir\": \"" << (mlir_ok ? "pass" : "fail") << "\",\n";
  ofs << "  \"reason\": \"" << json_escape(reason) << "\",\n";
  auto status = [](bool ok, bool skipped) -> const char * {
    if (skipped)
      return "skipped";
    return ok ? "pass" : "fail";
  };
  constexpr const char *skip_reason = "not in --only filter";

  ofs << "  \"emitters\": {\n";
  ofs << "    \"gen-model\": {\"result\": \""
      << status(results.model_ok, results.model_skipped) << "\", \"reason\": \""
      << json_escape(results.model_skipped ? skip_reason : results.model_reason)
      << "\"},\n";
  ofs << "    \"gen-tb\": {\"result\": \""
      << status(results.tb_ok, results.tb_skipped) << "\", \"reason\": \""
      << (results.tb_skipped ? skip_reason : "") << "\"},\n";
  ofs << "    \"gen-makefile\": {\"result\": \""
      << status(results.makefile_ok, results.makefile_skipped)
      << "\", \"reason\": \"" << (results.makefile_skipped ? skip_reason : "")
      << "\"},\n";
  ofs << "    \"gen-verify\": {\"result\": \""
      << status(results.verify_ok, results.verify_skipped)
      << "\", \"reason\": \"" << (results.verify_skipped ? skip_reason : "")
      << "\"},\n";
  ofs << "    \"gen-dpic\": {\"result\": \""
      << status(results.dpic_ok, results.dpic_skipped) << "\", \"reason\": \""
      << (results.dpic_skipped ? skip_reason : "") << "\"},\n";
  ofs << "    \"gen-wrapper\": {\"result\": \""
      << status(results.wrapper_ok, results.wrapper_skipped)
      << "\", \"reason\": \"" << (results.wrapper_skipped ? skip_reason : "")
      << "\"},\n";
  ofs << "    \"gen-format\": {\"result\": \""
      << status(results.format_ok, results.format_skipped)
      << "\", \"reason\": \"" << (results.format_skipped ? skip_reason : "")
      << "\"},\n";
  if (results.ral_skipped) {
    ofs << "    \"gen-ral\": {\"result\": \"skipped\", \"detection\": "
           "\"none\", "
           "\"reason\": \""
        << skip_reason << "\"},\n";
  } else if (results.ral.skipped) {
    ofs << "    \"gen-ral\": {\"result\": \"skipped\", \"detection\": "
           "\"none\", "
           "\"reason\": \"no register indicators\"},\n";
  } else {
    ofs << "    \"gen-ral\": {\"result\": \""
        << (results.ral.ok ? "pass" : "fail") << "\", \"detection\": \""
        << results.ral.detection << "\", \"reason\": \"\"},\n";
  }
  ofs << "    \"gen-doc\": {\"result\": \""
      << status(results.doc_ok, results.doc_skipped) << "\", \"reason\": \""
      << (results.doc_skipped ? skip_reason : "") << "\"},\n";
  ofs << "    \"gen-func-model\": {\"result\": \""
      << status(results.func_model_ok, results.func_model_skipped)
      << "\", \"reason\": \""
      << json_escape(results.func_model_skipped ? skip_reason
                                                : results.func_model_reason)
      << "\"},\n";
  ofs << "    \"gen-cocotb\": {\"result\": \""
      << status(results.cocotb_ok, results.cocotb_skipped)
      << "\", \"reason\": \"" << (results.cocotb_skipped ? skip_reason : "")
      << "\"}\n";
  ofs << "  }\n";
  ofs << "}\n";
  return true;
}

} // namespace

static std::string sanitize_pass_name(const std::string &raw) {
  std::string result;
  for (size_t i = 0; i < raw.size(); ++i) {
    char c = raw[i];
    if (std::isupper(static_cast<unsigned char>(c)) && i > 0) {
      bool prev_lower =
          std::islower(static_cast<unsigned char>(raw[i - 1]));
      bool next_lower =
          (i + 1 < raw.size()) &&
          std::islower(static_cast<unsigned char>(raw[i + 1]));
      if (prev_lower || next_lower)
        result += '-';
    }
    result += static_cast<char>(
        std::tolower(static_cast<unsigned char>(c)));
  }
  return result;
}

static std::unique_ptr<mlir::Pass>
create_pass_by_name(const std::string &name) {
  if (name == "sim-cleanup")
    return hirct::create_sim_cleanup_pass();
  if (name == "unroll-process-loops")
    return hirct::create_unroll_process_loops_pass();
  if (name == "process-flatten")
    return hirct::create_process_flatten_pass();
  if (name == "process-deseq")
    return hirct::create_process_deseq_pass();
  if (name == "signal-lowering")
    return hirct::create_signal_lowering_pass();
  if (name == "remove-control-flow")
    return circt::llhd::createRemoveControlFlowPass();
  if (name == "canonicalize")
    return mlir::createCanonicalizerPass();
  if (name == "cse")
    return mlir::createCSEPass();
  return nullptr;
}

int main(int argc, char *argv[]) {
  auto opts = parse_args(argc, argv);

  if (opts.help) {
    print_usage(argv[0]);
    return opts.parse_error ? 1 : 0;
  }

  if (opts.input_path.empty() && opts.filelist.empty()) {
    std::cerr << "Error: no input file specified\n";
    print_usage(argv[0]);
    return 1;
  }

  // Determine input files
  std::vector<std::string> inputs;
  std::vector<std::string> filelist_inc_dirs;
  std::vector<std::string> filelist_lib_files;
  std::vector<std::string> filelist_lib_dirs;
  std::vector<std::string> filelist_defines;
  if (!opts.filelist.empty()) {
    auto fl = parse_filelist(opts.filelist);
    if (!fl.ok) {
      return 1;
    }
    inputs = std::move(fl.input_files);
    filelist_inc_dirs = std::move(fl.inc_dirs);
    filelist_lib_files = std::move(fl.lib_files);
    filelist_lib_dirs = std::move(fl.lib_dirs);
    filelist_defines = std::move(fl.defines);
  }
  if (!opts.input_path.empty()) {
    inputs.push_back(opts.input_path);
  }

  if (opts.verbose) {
    std::cout << "Input files: " << inputs.size() << "\n";
    for (const auto &f : inputs) {
      std::cout << "  " << f << "\n";
    }
    if (!filelist_inc_dirs.empty()) {
      std::cout << "Include dirs (from filelist): " << filelist_inc_dirs.size()
                << "\n";
      for (const auto &d : filelist_inc_dirs) {
        std::cout << "  -I " << d << "\n";
      }
    }
    if (!filelist_lib_files.empty()) {
      std::cout << "Library files (from filelist): "
                << filelist_lib_files.size() << "\n";
      for (const auto &f : filelist_lib_files) {
        std::cout << "  -v " << f << "\n";
      }
    }
    if (!filelist_defines.empty()) {
      std::cout << "Defines (from filelist): " << filelist_defines.size()
                << "\n";
      for (const auto &d : filelist_defines)
        std::cout << "  +define+" << d << "\n";
    }
    if (!filelist_lib_dirs.empty()) {
      std::cout << "Library dirs (from filelist): " << filelist_lib_dirs.size()
                << "\n";
      for (const auto &d : filelist_lib_dirs)
        std::cout << "  -y " << d << "\n";
    }
  }

  bool is_mlir_input = inputs.size() == 1 && inputs[0].size() >= 5 &&
                       inputs[0].compare(inputs[0].size() - 5, 5, ".mlir") == 0;

  if (!opts.pipeline.empty() && !is_mlir_input) {
    std::cerr << "Error: --pipeline requires .mlir input\n";
    return 1;
  }
  if (!opts.pipeline_checkpoint_dir.empty() && !is_mlir_input) {
    std::cerr << "Error: --pipeline-checkpoint-dir requires .mlir input\n";
    return 1;
  }

  // Collect library directories for Verilog inputs
  std::vector<std::string> lib_dirs = opts.lib_dirs;
  for (const auto &d : filelist_lib_dirs)
    lib_dirs.push_back(d);
  if (!is_mlir_input) {
    if (!opts.no_auto_lib) {
      auto auto_dirs = collect_auto_lib_dirs(inputs);
      for (const auto &d : auto_dirs)
        lib_dirs.push_back(d);
    }
    std::set<std::string> seen;
    std::vector<std::string> unique;
    for (const auto &d : lib_dirs) {
      if (seen.insert(d).second)
        unique.push_back(d);
    }
    lib_dirs = std::move(unique);

    if (opts.verbose && !lib_dirs.empty()) {
      std::cout << "Library search dirs (" << lib_dirs.size() << "):\n";
      for (const auto &d : lib_dirs)
        std::cout << "  -y " << d << "\n";
      warn_module_conflicts(lib_dirs);
    }
  }

  // Unified MLIR native path — VerilogLoader for .v, MLIR parser for .mlir
  mlir::MLIRContext mlir_ctx;
  mlir_ctx.allowUnregisteredDialects();
  mlir_ctx.loadDialect<circt::hw::HWDialect, circt::comb::CombDialect,
                        circt::seq::SeqDialect, circt::llhd::LLHDDialect,
                        circt::sv::SVDialect, circt::sim::SimDialect,
                        mlir::cf::ControlFlowDialect,
                        mlir::func::FuncDialect>();

  mlir::OwningOpRef<mlir::ModuleOp> mlir_module;

  if (is_mlir_input) {
    auto buf = llvm::MemoryBuffer::getFile(inputs[0]);
    if (!buf) {
      std::cerr << "Error: cannot read MLIR file: " << inputs[0] << "\n";
      return 1;
    }
    llvm::SourceMgr src_mgr;
    src_mgr.AddNewSourceBuffer(std::move(*buf), llvm::SMLoc());
    mlir_module = mlir::parseSourceFile<mlir::ModuleOp>(src_mgr, &mlir_ctx);
    if (!mlir_module) {
      std::cerr << "Error: failed to parse MLIR file\n";
      hirct::mkdir_p(opts.output_dir);
      write_meta_json(opts.output_dir + "/meta.json",
                      opts.top_module.empty() ? "" : opts.top_module, false,
                      "MLIR parse failed", EmitterResults{});
      return 1;
    }

    {
      bool need_pipeline = opts.dump_ir || !opts.pipeline.empty() ||
                           !opts.pipeline_checkpoint_dir.empty();
      if (need_pipeline) {
        mlir::PassManager pm(&mlir_ctx);
        if (opts.timing)
          pm.enableTiming();

        std::vector<std::pair<std::string, std::unique_ptr<mlir::Pass>>>
            pass_list;

        if (!opts.pipeline.empty()) {
          std::string chain = opts.pipeline;
          size_t pos = 0;
          while (true) {
            size_t comma = chain.find(',', pos);
            std::string pname = chain.substr(
                pos,
                comma == std::string::npos ? std::string::npos : comma - pos);
            {
              auto a = pname.find_first_not_of(" \t");
              auto b = pname.find_last_not_of(" \t");
              pname =
                  (a == std::string::npos) ? "" : pname.substr(a, b - a + 1);
            }
            if (!pname.empty()) {
              auto pass = create_pass_by_name(pname);
              if (!pass) {
                std::cerr << "Error: unknown pass '" << pname
                          << "'. Available: sim-cleanup, "
                             "unroll-process-loops, remove-control-flow, "
                             "canonicalize, process-flatten, process-deseq, "
                             "signal-lowering, cse\n";
                return 1;
              }
              pass_list.emplace_back(pname, std::move(pass));
            }
            if (comma == std::string::npos)
              break;
            pos = comma + 1;
          }
        } else {
          pass_list.emplace_back("sim-cleanup",
                                 hirct::create_sim_cleanup_pass());
          pass_list.emplace_back("unroll-process-loops",
                                 hirct::create_unroll_process_loops_pass());
          pass_list.emplace_back("remove-control-flow",
                                 circt::llhd::createRemoveControlFlowPass());
          pass_list.emplace_back("canonicalize",
                                 mlir::createCanonicalizerPass());
          pass_list.emplace_back("process-flatten",
                                 hirct::create_process_flatten_pass());
          pass_list.emplace_back("process-deseq",
                                 hirct::create_process_deseq_pass());
          pass_list.emplace_back("signal-lowering",
                                 hirct::create_signal_lowering_pass());
          pass_list.emplace_back("cse", mlir::createCSEPass());
          pass_list.emplace_back("canonicalize",
                                 mlir::createCanonicalizerPass());
        }

        if (!opts.pipeline_checkpoint_dir.empty())
          hirct::mkdir_p(opts.pipeline_checkpoint_dir);

        bool pipeline_failed = false;
        for (unsigned i = 0; i < pass_list.size(); ++i) {
          auto &[pname, pass] = pass_list[i];

          if (!opts.pipeline_checkpoint_dir.empty()) {
            std::string path = opts.pipeline_checkpoint_dir + "/" +
                               std::to_string(i) + "_before-" + pname +
                               ".mlir";
            std::error_code ec;
            llvm::raw_fd_ostream os(path, ec);
            if (!ec)
              mlir_module->print(os);
          }

          mlir::PassManager step_pm(&mlir_ctx);
          if (opts.timing)
            step_pm.enableTiming();
          step_pm.nest<circt::hw::HWModuleOp>().addPass(std::move(pass));
          if (mlir::failed(step_pm.run(*mlir_module))) {
            std::cerr << "Error: pass '" << pname << "' failed\n";
            pipeline_failed = true;
            break;
          }
        }

        if (!opts.pipeline_checkpoint_dir.empty() && !pipeline_failed) {
          std::string path = opts.pipeline_checkpoint_dir + "/" +
                             std::to_string(pass_list.size()) + "_final.mlir";
          std::error_code ec;
          llvm::raw_fd_ostream os(path, ec);
          if (!ec)
            mlir_module->print(os);
        }

        if (pipeline_failed)
          return 1;
      }
    }
  } else {
    hirct::VerilogLoadOptions load_opts;

    if (opts.preprocess == "verilator") {
      hirct::PreprocessOptions pp_opts;
      pp_opts.input_files = inputs;
      pp_opts.defines = filelist_defines;
      pp_opts.inc_dirs = filelist_inc_dirs;
      pp_opts.lib_dirs = lib_dirs;
      pp_opts.lib_files = filelist_lib_files;
      pp_opts.output_path = opts.output_dir + "/_preprocessed.v";
      pp_opts.verilator_path = opts.verilator_path;
      pp_opts.verbose = opts.verbose;

      hirct::mkdir_p(opts.output_dir);

      auto pp_result = hirct::run_verilator_preprocess(pp_opts);
      if (!pp_result.success) {
        std::cerr << "error: verilator preprocessing failed: "
                  << pp_result.error_message << "\n";
        return 1;
      }

      if (opts.verbose) {
        std::cout << "Preprocessed output: " << pp_result.output_file << "\n";
      }

      load_opts.input_files = {pp_result.output_file};
    } else {
      load_opts.input_files = inputs;
      load_opts.include_dirs = filelist_inc_dirs;
      load_opts.lib_dirs = lib_dirs;
      load_opts.lib_files = filelist_lib_files;
    }

    if (!opts.top_module.empty())
      load_opts.top_module = opts.top_module;
    load_opts.enable_timing = opts.timing;

    auto load_result = hirct::load_verilog(mlir_ctx, load_opts);
    if (!load_result.success) {
      std::cerr << "Error: " << load_result.error_message << "\n";
      hirct::mkdir_p(opts.output_dir);
      write_meta_json(opts.output_dir + "/meta.json",
                      opts.top_module.empty() ? "" : opts.top_module, false,
                      load_result.error_message, EmitterResults{});
      return 1;
    }
    mlir_module = std::move(load_result.module);
  }

  if (opts.dump_ir) {
    mlir_module->print(llvm::outs());
    return 0;
  }

  mlir::SymbolTable symbol_table(*mlir_module);
  llvm::SmallVector<circt::hw::HWModuleOp> hw_modules;
  mlir_module->walk(
      [&](circt::hw::HWModuleOp m) { hw_modules.push_back(m); });

  if (hw_modules.empty()) {
    std::cerr << "Error: no hw.module found in MLIR\n";
    return 1;
  }

  // Find top module
  circt::hw::HWModuleOp top_hw;
  if (!opts.top_module.empty()) {
    top_hw =
        symbol_table.lookup<circt::hw::HWModuleOp>(opts.top_module);
  }
  if (!top_hw)
    top_hw = hw_modules.back();

  std::string top_name = top_hw.getSymName().str();

  if (opts.verbose) {
    auto ports = hirct::get_ports(top_hw);
    auto in_ports = hirct::get_input_ports(top_hw);
    auto out_ports = hirct::get_output_ports(top_hw);
    auto regs = hirct::collect_registers(top_hw);
    std::cout << "Modules found: " << hw_modules.size() << "\n";
    for (auto m : hw_modules) {
      std::string mn = m.getSymName().str();
      std::cout << "  " << mn << (mn == top_name ? " (top)" : "") << "\n";
    }
    std::cout << "Top module: " << top_name << "\n";
    std::cout << "Ports: " << ports.size() << " (" << in_ports.size()
              << " in, " << out_ports.size() << " out)\n";
    std::cout << "Has registers: " << (regs.empty() ? "no" : "yes") << "\n";

    llvm::SmallVector<circt::hw::InstanceOp> instances;
    top_hw.walk([&](circt::hw::InstanceOp inst) { instances.push_back(inst); });
    std::cout << "Has instances: " << (instances.empty() ? "no" : "yes")
              << "\n";

    if (!instances.empty()) {
      auto topo = hirct::sort_instances_topologically(top_hw);
      std::cerr << "[topo] Instance topological order (" << topo.order.size()
                << " instances):\n";
      for (size_t i = 0; i < topo.order.size(); ++i) {
        auto inst = topo.order[i];
        std::cerr << "[topo]   " << i << ": "
                  << inst.getInstanceName().str() << " (@"
                  << inst.getModuleName().str() << ")\n";
      }
    }
  }

  if (!hirct::mkdir_p(opts.output_dir)) {
    std::cerr << "ERROR: cannot create directory: " << opts.output_dir << ": "
              << strerror(errno) << "\n";
    return 1;
  }

  // Generate sub-modules first (so their headers exist for #include)
  if (hw_modules.size() > 1) {
    for (auto sub_hw : hw_modules) {
      std::string mod_name = sub_hw.getSymName().str();
      if (mod_name == top_name)
        continue;

      std::string sub_dir = opts.output_dir + "/" + mod_name;
      if (!hirct::mkdir_p(sub_dir)) {
        std::cerr << "ERROR: cannot create directory: " << sub_dir << ": "
                  << strerror(errno) << "\n";
        continue;
      }
      hirct::GenModel sub_gen(sub_hw, *mlir_module);
      if (!sub_gen.emit(sub_dir, true)) {
        std::cerr << "Warning: gen-model failed for sub-module " << mod_name
                  << ": " << sub_gen.last_error_reason() << "\n";
      }
    }
  }

  std::string mod_output_dir = opts.output_dir;
  if (!hirct::mkdir_p(mod_output_dir)) {
    std::cerr << "ERROR: cannot create directory: " << mod_output_dir << ": "
              << strerror(errno) << "\n";
    return 1;
  }

  auto allowed = parse_only_filter(opts.only_filter);

  std::string rtl_src_for_makefile =
      (inputs.size() == 1 && !is_mlir_input)
          ? hirct::to_absolute_path(inputs[0])
          : "";

  EmitterResults results;

  if (emitter_allowed(allowed, "model")) {
    hirct::GenModel gen_model(top_hw, *mlir_module);
    results.model_ok = gen_model.emit(mod_output_dir);
    if (!results.model_ok)
      results.model_reason = gen_model.last_error_reason();
  } else {
    results.model_skipped = true;
  }

  if (emitter_allowed(allowed, "func-model")) {
    hirct::GenFuncModel gen_func_model(top_hw, *mlir_module);
    results.func_model_ok = gen_func_model.emit(mod_output_dir);
    if (!results.func_model_ok) {
      results.func_model_reason = gen_func_model.last_error_reason();
      if (results.func_model_reason == "no FSM found")
        results.func_model_skipped = true;
    }
  } else {
    results.func_model_skipped = true;
  }

  if (emitter_allowed(allowed, "makefile")) {
    hirct::GenMakefile gen_makefile(top_hw, rtl_src_for_makefile, opts.lib_dirs,
                                   results.func_model_ok);
    results.makefile_ok = gen_makefile.emit(mod_output_dir);
  } else {
    results.makefile_skipped = true;
  }

  if (emitter_allowed(allowed, "verify")) {
    hirct::GenVerify gen_verify(top_hw);
    results.verify_ok = gen_verify.emit(mod_output_dir);
  } else {
    results.verify_skipped = true;
  }

  if (emitter_allowed(allowed, "dpic")) {
    hirct::GenDPIC gen_dpic(top_hw, *mlir_module);
    results.dpic_ok = gen_dpic.emit(mod_output_dir);
  } else {
    results.dpic_skipped = true;
  }

  if (emitter_allowed(allowed, "wrapper")) {
    hirct::GenWrapper gen_wrapper(top_hw);
    results.wrapper_ok = gen_wrapper.emit(mod_output_dir);
  } else {
    results.wrapper_skipped = true;
  }

  if (emitter_allowed(allowed, "format")) {
    hirct::GenFormat gen_format(top_hw);
    results.format_ok = gen_format.emit(mod_output_dir);
  } else {
    results.format_skipped = true;
  }

  if (emitter_allowed(allowed, "ral")) {
    hirct::GenRAL gen_ral(top_hw);
    if (gen_ral.should_skip()) {
      results.ral.skipped = true;
    } else {
      results.ral.ok = gen_ral.emit(mod_output_dir);
      results.ral.detection = "ir_pattern";
    }
  } else {
    results.ral_skipped = true;
  }

  if (emitter_allowed(allowed, "tb")) {
    hirct::GenTB gen_tb(top_hw, *mlir_module);
    results.tb_ok = gen_tb.emit(mod_output_dir);
  } else {
    results.tb_skipped = true;
  }

  if (emitter_allowed(allowed, "cocotb")) {
    hirct::GenCocotb gen_cocotb(top_hw);
    results.cocotb_ok = gen_cocotb.emit(mod_output_dir);
  } else {
    results.cocotb_skipped = true;
  }

  if (emitter_allowed(allowed, "doc")) {
    hirct::GenDoc gen_doc(top_hw);
    results.doc_ok = gen_doc.emit(mod_output_dir);
  } else {
    results.doc_skipped = true;
  }

  write_meta_json(mod_output_dir + "/meta.json", top_name, true, "", results);

  bool all_ok = true;
  if (emitter_allowed(allowed, "model"))
    all_ok &= results.model_ok;
  if (emitter_allowed(allowed, "func-model"))
    all_ok &= (results.func_model_skipped || results.func_model_ok);
  if (emitter_allowed(allowed, "tb"))
    all_ok &= results.tb_ok;
  if (emitter_allowed(allowed, "makefile"))
    all_ok &= results.makefile_ok;
  if (emitter_allowed(allowed, "verify"))
    all_ok &= results.verify_ok;
  if (emitter_allowed(allowed, "dpic"))
    all_ok &= results.dpic_ok;
  if (emitter_allowed(allowed, "wrapper"))
    all_ok &= results.wrapper_ok;
  if (emitter_allowed(allowed, "format"))
    all_ok &= results.format_ok;
  if (emitter_allowed(allowed, "doc"))
    all_ok &= results.doc_ok;
  if (emitter_allowed(allowed, "ral"))
    all_ok &= (results.ral.skipped || results.ral.ok);
  if (emitter_allowed(allowed, "cocotb"))
    all_ok &= results.cocotb_ok;

  if (all_ok) {
    std::cout << "Generated artifacts for " << top_name << " in "
              << mod_output_dir << "/\n";
    return 0;
  }

  std::cerr << "Error: one or more emitters failed\n";
  return 1;
}
