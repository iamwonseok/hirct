#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct VerifyOptions {
  std::string input_path;
  int seeds = 1;
  int cycles = 100;
  bool help = false;
};

void print_usage(const char *prog) {
  std::cout << "Usage: " << prog
            << " [options] <input.v>\n\n"
               "Options:\n"
               "  --seeds <N>   Number of random seeds (default: 1)\n"
               "  --cycles <N>  Number of simulation cycles (default: 100)\n"
               "  --help        Show this help message\n";
}

VerifyOptions parse_args(int argc, char *argv[]) {
  VerifyOptions opts;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      opts.help = true;
      return opts;
    }
    if (arg == "--seeds" && i + 1 < argc) {
      try {
        opts.seeds = std::stoi(argv[++i]);
      } catch (const std::exception &) {
        std::cerr << "Invalid value for --seeds\n";
        opts.help = true;
        return opts;
      }
    } else if (arg == "--cycles" && i + 1 < argc) {
      try {
        opts.cycles = std::stoi(argv[++i]);
      } catch (const std::exception &) {
        std::cerr << "Invalid value for --cycles\n";
        opts.help = true;
        return opts;
      }
    } else if (arg[0] != '-') {
      opts.input_path = arg;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
    }
  }
  return opts;
}

} // namespace

int main(int argc, char *argv[]) {
  auto opts = parse_args(argc, argv);

  if (opts.help) {
    print_usage(argv[0]);
    return 0;
  }

  std::cout << "hirct-verify: not yet implemented\n";
  return 0;
}
