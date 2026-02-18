#include "hirct/Analysis/ModuleAnalyzer.h"
#include "hirct/Support/CirctRunner.h"
#include "hirct/Target/GenMakefile.h"
#include "hirct/Target/GenModel.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace {

struct Options {
  std::string input_path;
  std::string output_dir = "output";
  std::string only_filter;
  std::string top_module;
  std::string filelist;
  bool verbose = false;
  bool help = false;
};

void print_usage(const char *prog) {
  std::cout << "Usage: " << prog
            << " [options] <input.v>\n\n"
               "Options:\n"
               "  -o <dir>        Output directory (default: output)\n"
               "  --only <filter> Generate only matching modules\n"
               "  --top <module>  Specify top module name\n"
               "  -f <filelist>   Read input files from filelist\n"
               "  --verbose       Enable verbose output\n"
               "  --help          Show this help message\n";
}

Options parse_args(int argc, char *argv[]) {
  Options opts;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      opts.help = true;
      return opts;
    }
    if (arg == "-o" && i + 1 < argc) {
      opts.output_dir = argv[++i];
    } else if (arg == "--only" && i + 1 < argc) {
      opts.only_filter = argv[++i];
    } else if (arg == "--top" && i + 1 < argc) {
      opts.top_module = argv[++i];
    } else if (arg == "-f" && i + 1 < argc) {
      opts.filelist = argv[++i];
    } else if (arg == "--verbose") {
      opts.verbose = true;
    } else if (arg[0] != '-') {
      opts.input_path = arg;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
    }
  }
  return opts;
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

bool write_meta_json(const std::string &path, const std::string &top,
                     bool mlir_ok, const std::string &reason, bool model_ok,
                     bool makefile_ok) {
  std::ofstream ofs(path);
  if (!ofs) {
    return false;
  }
  ofs << "{\n";
  ofs << "  \"path\": \"" << json_escape(path) << "\",\n";
  ofs << "  \"top\": \"" << json_escape(top) << "\",\n";
  ofs << "  \"mlir\": \"" << (mlir_ok ? "pass" : "fail") << "\",\n";
  ofs << "  \"reason\": \"" << json_escape(reason) << "\",\n";
  ofs << "  \"emitters\": {\n";
  ofs << "    \"GenModel\": \"" << (model_ok ? "pass" : "fail") << "\",\n";
  ofs << "    \"GenMakefile\": \"" << (makefile_ok ? "pass" : "fail") << "\"\n";
  ofs << "  }\n";
  ofs << "}\n";
  return true;
}

} // namespace

int main(int argc, char *argv[]) {
  auto opts = parse_args(argc, argv);

  if (opts.help) {
    print_usage(argv[0]);
    return 0;
  }

  if (opts.input_path.empty() && opts.filelist.empty()) {
    std::cerr << "Error: no input file specified\n";
    print_usage(argv[0]);
    return 1;
  }

  // Determine input files
  std::vector<std::string> inputs;
  if (!opts.filelist.empty()) {
    std::ifstream flist(opts.filelist);
    if (!flist) {
      std::cerr << "Error: cannot open filelist: " << opts.filelist << "\n";
      return 1;
    }
    std::string line;
    while (std::getline(flist, line)) {
      if (!line.empty() && line[0] != '#') {
        inputs.push_back(line);
      }
    }
  }
  if (!opts.input_path.empty()) {
    inputs.push_back(opts.input_path);
  }

  if (opts.verbose) {
    std::cout << "Input files: " << inputs.size() << "\n";
    for (const auto &f : inputs) {
      std::cout << "  " << f << "\n";
    }
  }

  // Run circt-verilog
  hirct::CirctRunner runner;
  hirct::RunResult result;

  if (inputs.size() == 1 && opts.top_module.empty()) {
    result = runner.run_circt_verilog(inputs[0]);
  } else {
    std::string top = opts.top_module.empty() ? "top" : opts.top_module;
    result = runner.run_circt_verilog_multi(inputs, top);
  }

  if (result.exit_code != 0) {
    std::cerr << "Error: circt-verilog failed (exit code " << result.exit_code
              << ")\n";
    if (!result.stderr_str.empty()) {
      std::cerr << result.stderr_str << "\n";
    }
    if (mkdir(opts.output_dir.c_str(), 0755) != 0 && errno != EEXIST) {
      std::cerr << "ERROR: cannot create directory: " << opts.output_dir << ": "
                << strerror(errno) << "\n";
    }
    write_meta_json(opts.output_dir + "/meta.json",
                    opts.top_module.empty() ? "" : opts.top_module, false,
                    "circt-verilog failed", false, false);
    return 1;
  }

  if (opts.verbose) {
    std::cout << "MLIR output size: " << result.stdout_str.size() << " bytes\n";
  }

  // Analyze MLIR
  hirct::ModuleAnalyzer analyzer(result.stdout_str);
  if (!analyzer.is_valid()) {
    std::cerr << "Error: failed to parse MLIR output\n";
    if (mkdir(opts.output_dir.c_str(), 0755) != 0 && errno != EEXIST) {
      std::cerr << "ERROR: cannot create directory: " << opts.output_dir << ": "
                << strerror(errno) << "\n";
    }
    write_meta_json(opts.output_dir + "/meta.json",
                    opts.top_module.empty() ? "" : opts.top_module, false,
                    "MLIR parse failed", false, false);
    return 1;
  }

  if (opts.verbose) {
    std::cout << "Module: " << analyzer.module_name() << "\n";
    std::cout << "Ports: " << analyzer.ports().size() << " ("
              << analyzer.input_ports().size() << " in, "
              << analyzer.output_ports().size() << " out)\n";
    std::cout << "Operations: " << analyzer.operations().size() << "\n";
    std::cout << "Has registers: " << (analyzer.has_registers() ? "yes" : "no")
              << "\n";
  }

  // Determine output directory
  std::string mod_output_dir = opts.output_dir + "/" + analyzer.module_name();
  if (mkdir(opts.output_dir.c_str(), 0755) != 0 && errno != EEXIST) {
    std::cerr << "ERROR: cannot create directory: " << opts.output_dir << ": "
              << strerror(errno) << "\n";
    return 1;
  }
  if (mkdir(mod_output_dir.c_str(), 0755) != 0 && errno != EEXIST) {
    std::cerr << "ERROR: cannot create directory: " << mod_output_dir << ": "
              << strerror(errno) << "\n";
    return 1;
  }

  // Run emitters (filtered by --only if provided)
  auto allowed = parse_only_filter(opts.only_filter);

  bool model_ok = true;
  if (emitter_allowed(allowed, "model")) {
    hirct::GenModel gen_model(analyzer);
    model_ok = gen_model.emit(mod_output_dir);
  }

  bool makefile_ok = true;
  if (emitter_allowed(allowed, "makefile")) {
    hirct::GenMakefile gen_makefile(analyzer);
    makefile_ok = gen_makefile.emit(mod_output_dir);
  }

  // Write meta.json
  write_meta_json(mod_output_dir + "/meta.json", analyzer.module_name(), true,
                  "", model_ok, makefile_ok);

  if (model_ok && makefile_ok) {
    std::cout << "Generated C++ model for " << analyzer.module_name() << " in "
              << mod_output_dir << "/cmodel/\n";
    return 0;
  }

  std::cerr << "Error: one or more emitters failed\n";
  return 1;
}
