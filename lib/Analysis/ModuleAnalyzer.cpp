#include "hirct/Analysis/ModuleAnalyzer.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <regex>
#include <set>
#include <sstream>

namespace hirct {

ModuleAnalyzer::ModuleAnalyzer(const std::string &mlir_text) {
  parse(mlir_text);
}

std::vector<PortInfo> ModuleAnalyzer::input_ports() const {
  std::vector<PortInfo> result;
  for (const auto &p : ports_) {
    if (p.direction == "in") {
      result.push_back(p);
    }
  }
  return result;
}

std::vector<PortInfo> ModuleAnalyzer::output_ports() const {
  std::vector<PortInfo> result;
  for (const auto &p : ports_) {
    if (p.direction == "out") {
      result.push_back(p);
    }
  }
  return result;
}

int ModuleAnalyzer::value_width(const std::string &ssa_name) const {
  auto it = ssa_widths_.find(ssa_name);
  if (it != ssa_widths_.end()) {
    return it->second;
  }
  return 0;
}

namespace {

const std::regex MODULE_RE(R"(hw\.module\s+@(\w+)\(([^)]*)\))");

int extract_bit_width(const std::string &type_str) {
  static const std::regex width_re(R"(i(\d+))");
  std::smatch m;
  if (std::regex_search(type_str, m, width_re)) {
    return std::stoi(m[1].str());
  }
  return 0;
}

std::vector<std::string> split_lines(const std::string &text) {
  std::vector<std::string> lines;
  std::istringstream iss(text);
  std::string line;
  while (std::getline(iss, line)) {
    lines.push_back(line);
  }
  return lines;
}

std::string trim(const std::string &s) {
  auto start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  auto end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

std::vector<std::string> extract_operands(const std::string &operand_str) {
  std::vector<std::string> operands;
  static const std::regex ssa_re(R"(%[\w.]+)");
  auto begin =
      std::sregex_iterator(operand_str.begin(), operand_str.end(), ssa_re);
  auto end = std::sregex_iterator();
  for (auto it = begin; it != end; ++it) {
    operands.push_back(it->str());
  }
  return operands;
}

} // namespace

void ModuleAnalyzer::parse(const std::string &mlir_text) {
  auto lines = split_lines(mlir_text);
  if (lines.empty()) {
    return;
  }

  std::string module_header;
  std::string module_body;
  bool in_module = false;
  int brace_depth = 0;

  for (const auto &line : lines) {
    std::smatch m;
    if (!in_module && std::regex_search(line, m, MODULE_RE)) {
      module_header = line;
      in_module = true;
      for (char c : line) {
        if (c == '{') {
          brace_depth++;
        }
        if (c == '}') {
          brace_depth--;
        }
      }
      continue;
    }
    if (in_module) {
      for (char c : line) {
        if (c == '{') {
          brace_depth++;
        }
        if (c == '}') {
          brace_depth--;
        }
      }
      if (brace_depth <= 0) {
        in_module = false;
        break;
      }
      module_body += line + "\n";
    }
  }

  if (module_header.empty()) {
    return;
  }

  parse_module(module_header);
  parse_body(module_body);
  topological_sort();
  valid_ = !has_combinational_loops_;
}

void ModuleAnalyzer::parse_module(const std::string &header) {
  std::smatch m;
  if (!std::regex_search(header, m, MODULE_RE)) {
    return;
  }

  module_name_ = m[1].str();
  std::string port_str = m[2].str();

  static const std::regex port_re(R"((in|out)\s+(%?\w+)\s*:\s*(\S+))");
  auto begin = std::sregex_iterator(port_str.begin(), port_str.end(), port_re);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    PortInfo port;
    port.direction = (*it)[1].str();
    port.name = (*it)[2].str();
    if (port.name[0] == '%') {
      port.name = port.name.substr(1);
    }
    port.width = extract_bit_width((*it)[3].str());

    ports_.push_back(port);
    if (port.direction == "in") {
      ssa_widths_["%" + port.name] = port.width;
    }
  }
}

void ModuleAnalyzer::parse_body(const std::string &body) {
  auto lines = split_lines(body);

  static const std::regex op_re(
      R"((%[\w.]+)\s*=\s*([\w.]+)\s+(.+?)\s*:\s*(.+))");
  static const std::regex const_re(
      R"((%[\w.]+)\s*=\s*hw\.constant\s+(.+?)\s*:\s*(i\d+))");
  static const std::regex output_re(R"(hw\.output\s+(.+))");

  for (const auto &raw_line : lines) {
    std::string line = trim(raw_line);
    if (line.empty()) {
      continue;
    }

    // Check for registers
    if (line.find("seq.firreg") != std::string::npos ||
        line.find("seq.compreg") != std::string::npos) {
      has_registers_ = true;
    }

    // Check for instances
    if (line.find("hw.instance") != std::string::npos) {
      has_instances_ = true;
    }

    // Parse constants
    std::smatch cm;
    if (std::regex_search(line, cm, const_re)) {
      ConstantInfo ci;
      ci.name = cm[1].str();
      ci.value = cm[2].str();
      ci.type = cm[3].str();
      constants_.push_back(ci);
      ssa_widths_[ci.name] = extract_bit_width(ci.type);
      continue;
    }

    // Parse hw.output
    std::smatch om;
    if (std::regex_search(line, om, output_re)) {
      std::string vals = om[1].str();
      output_values_ = extract_operands(vals);
      continue;
    }

    // Parse general operations
    std::smatch opm;
    if (std::regex_search(line, opm, op_re)) {
      OpInfo op;
      op.result_name = opm[1].str();
      op.op_name = opm[2].str();
      op.result_type = opm[4].str();
      op.operands = extract_operands(opm[3].str());

      operations_.push_back(op);
      ssa_widths_[op.result_name] = extract_bit_width(op.result_type);
    }
  }
}

void ModuleAnalyzer::topological_sort() {
  if (operations_.empty()) {
    return;
  }

  // Build name → index map
  std::map<std::string, size_t> name_to_idx;
  for (size_t i = 0; i < operations_.size(); ++i) {
    name_to_idx[operations_[i].result_name] = i;
  }

  // Cut points: register outputs don't count as dependencies
  std::set<std::string> cut_points;
  for (const auto &op : operations_) {
    if (op.op_name == "seq.firreg" || op.op_name == "seq.compreg") {
      cut_points.insert(op.result_name);
    }
  }

  // Build adjacency and in-degree
  size_t n = operations_.size();
  std::vector<std::vector<size_t>> adj(n);
  std::vector<int> in_degree(n, 0);

  for (size_t i = 0; i < n; ++i) {
    for (const auto &operand : operations_[i].operands) {
      if (cut_points.count(operand)) {
        continue;
      }
      auto it = name_to_idx.find(operand);
      if (it != name_to_idx.end() && it->second != i) {
        adj[it->second].push_back(i);
        in_degree[i]++;
      }
    }
  }

  // Kahn's algorithm
  std::queue<size_t> q;
  for (size_t i = 0; i < n; ++i) {
    if (in_degree[i] == 0) {
      q.push(i);
    }
  }

  std::vector<OpInfo> sorted;
  sorted.reserve(n);

  while (!q.empty()) {
    size_t cur = q.front();
    q.pop();
    sorted.push_back(operations_[cur]);
    for (size_t next : adj[cur]) {
      in_degree[next]--;
      if (in_degree[next] == 0) {
        q.push(next);
      }
    }
  }

  if (sorted.size() < n) {
    has_combinational_loops_ = true;

    std::set<size_t> cycle_indices;
    for (size_t i = 0; i < n; ++i) {
      if (in_degree[i] > 0) {
        cycle_indices.insert(i);
      }
    }

    std::string cycle_path;
    if (!cycle_indices.empty()) {
      size_t start = *cycle_indices.begin();
      std::vector<size_t> trace;
      std::set<size_t> visited;
      size_t cur = start;

      while (visited.find(cur) == visited.end()) {
        visited.insert(cur);
        trace.push_back(cur);
        bool found = false;
        for (size_t nxt : adj[cur]) {
          if (cycle_indices.count(nxt)) {
            cur = nxt;
            found = true;
            break;
          }
        }
        if (!found) {
          break;
        }
      }

      bool in_cycle = false;
      for (size_t idx : trace) {
        if (idx == cur) {
          in_cycle = true;
        }
        if (in_cycle) {
          if (!cycle_path.empty()) {
            cycle_path += " -> ";
          }
          cycle_path += operations_[idx].result_name;
        }
      }
      cycle_path += " -> " + operations_[cur].result_name;
    }

    std::cerr << "ERROR: combinational loop detected in module " << module_name_
              << " (" << (n - sorted.size()) << " nodes in cycle)";
    if (!cycle_path.empty()) {
      std::cerr << ": " << cycle_path;
    }
    std::cerr << "\n";
  }

  operations_ = std::move(sorted);
}

} // namespace hirct
