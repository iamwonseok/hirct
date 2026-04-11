//===- SystemCWrapperEmitter.cpp - SystemC wrapper emitter ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Generates a thin SystemC SC_MODULE wrapper around a CModelEmitter-generated
// C model. The wrapper's only responsibilities are:
//   - Expose sc_in / sc_out ports matching the module's I/O
//   - Own the C model state struct
//   - Drive eval_comb / eval_<clock> at the appropriate SystemC events
//
// Timing accuracy lives in this wrapper layer; the C model core remains
// a pure-combinational / edge-triggered evaluation engine.
//
// Backlog: multi-clock generalization (currently single-clock only).
//
//===----------------------------------------------------------------------===//

#include "hirct/Target/SystemCWrapperEmitter.h"

#include "llvm/Support/raw_ostream.h"

namespace hirct {

namespace {

static std::string scPortType(unsigned width) {
  if (width <= 1)
    return "bool";
  if (width <= 8)
    return "sc_dt::sc_uint<8>";
  if (width <= 16)
    return "sc_dt::sc_uint<16>";
  if (width <= 32)
    return "sc_dt::sc_uint<32>";
  if (width <= 64)
    return "sc_dt::sc_uint<64>";
  return "sc_dt::sc_biguint<" + std::to_string(width) + ">";
}

static bool isClockPort(const semantic::ModuleModel &model,
                        const semantic::PortInfo &port) {
  for (const auto &cd : model.clockDomains)
    if (port.name == cd)
      return true;
  return false;
}

static std::string getPrimaryClockPortName(const semantic::ModuleModel &model) {
  for (const auto &port : model.inputPorts)
    if (isClockPort(model, port))
      return port.name;
  return "";
}

} // namespace

SystemCWrapperEmitter::SystemCWrapperEmitter(
    const semantic::ModuleModel &model, const CModelOptions &options)
    : model_(model), options_(options) {}

SystemCWrapperArtifact SystemCWrapperEmitter::emit() {
  SystemCWrapperArtifact artifact;
  artifact.moduleName = model_.moduleName;

  std::string wrapperName = model_.moduleName + "_sc_wrapper";
  artifact.wrapperHeaderPath =
      options_.outputDir + "/" + wrapperName + ".h";
  artifact.wrapperImplPath =
      options_.outputDir + "/" + wrapperName + ".cpp";

  std::string headerBuf;
  llvm::raw_string_ostream headerOS(headerBuf);
  emitWrapperHeader(headerOS);
  artifact.wrapperHeaderContent = std::move(headerBuf);

  std::string implBuf;
  llvm::raw_string_ostream implOS(implBuf);
  emitWrapperImpl(implOS);
  artifact.wrapperImplContent = std::move(implBuf);

  return artifact;
}

void SystemCWrapperEmitter::emitWrapperHeader(llvm::raw_string_ostream &os) {
  const auto &mod = model_.moduleName;
  std::string wrapperName = mod + "_sc_wrapper";
  std::string clockPortName = getPrimaryClockPortName(model_);
  std::string guardName = mod + "_SC_WRAPPER_H";
  for (auto &c : guardName)
    c = std::toupper(static_cast<unsigned char>(c));

  os << "#ifndef " << guardName << "\n";
  os << "#define " << guardName << "\n\n";

  os << "#include \"" << mod << ".h\"\n";
  os << "#include <systemc>\n\n";

  os << "SC_MODULE(" << wrapperName << ") {\n";

  // Clock port(s)
  bool hasClock = !clockPortName.empty();
  if (hasClock) {
    os << "  sc_in_clk " << clockPortName << ";\n";
  }

  // Input ports (skip the clock input if it matches a clock domain)
  for (const auto &port : model_.inputPorts) {
    if (isClockPort(model_, port))
      continue;
    os << "  sc_in<" << scPortType(port.width) << "> " << port.name << ";\n";
  }

  // Output ports
  for (const auto &port : model_.outputPorts) {
    os << "  sc_out<" << scPortType(port.width) << "> " << port.name << ";\n";
  }

  os << "\n";
  os << "  " << mod << "_state state_;\n\n";

  os << "  void eval_method();\n";
  if (hasClock) {
    os << "  void clock_method();\n";
  }

  os << "\n  " << wrapperName << "(sc_module_name name);\n";
  os << "};\n\n";

  os << "#endif // " << guardName << "\n";
}

void SystemCWrapperEmitter::emitWrapperImpl(llvm::raw_string_ostream &os) {
  const auto &mod = model_.moduleName;
  std::string wrapperName = mod + "_sc_wrapper";
  std::string clockPortName = getPrimaryClockPortName(model_);

  os << "#include \"" << wrapperName << ".h\"\n\n";

  // Constructor
  os << wrapperName << "::" << wrapperName
     << "(sc_module_name name) : sc_module(name) {\n";
  os << "  " << mod << "_initialize(&state_);\n";

  if (!clockPortName.empty()) {
    os << "  SC_METHOD(clock_method);\n";
    os << "  sensitive << " << clockPortName << ".pos();\n";
    os << "  dont_initialize();\n\n";
  }

  os << "  SC_METHOD(eval_method);\n";
  for (const auto &port : model_.inputPorts) {
    if (isClockPort(model_, port))
      continue;
    os << "  sensitive << " << port.name << ";\n";
  }
  os << "}\n\n";

  // eval_method: read SC ports -> C model setters -> eval_comb -> write outputs
  os << "void " << wrapperName << "::eval_method() {\n";
  for (const auto &port : model_.inputPorts) {
    if (isClockPort(model_, port))
      continue;
    os << "  " << mod << "_set_" << port.name << "(&state_, "
       << port.name << ".read());\n";
  }
  os << "  " << mod << "_eval_comb(&state_);\n";
  for (const auto &port : model_.outputPorts) {
    os << "  " << port.name << ".write(state_.output_" << port.name << ");\n";
  }
  os << "}\n\n";

  // clock_method: call eval_<clock> for each clock domain, then re-eval comb
  if (!clockPortName.empty()) {
    os << "void " << wrapperName << "::clock_method() {\n";
    os << "  " << mod << "_set_" << clockPortName << "(&state_, "
       << clockPortName << ".read());\n";
    for (const auto &port : model_.inputPorts) {
      if (isClockPort(model_, port))
        continue;
      os << "  " << mod << "_set_" << port.name << "(&state_, "
         << port.name << ".read());\n";
    }
    for (const auto &cd : model_.clockDomains) {
      os << "  " << mod << "_eval_" << cd << "(&state_);\n";
    }
    os << "  " << mod << "_eval_comb(&state_);\n";
    for (const auto &port : model_.outputPorts) {
      os << "  " << port.name << ".write(state_.output_" << port.name
         << ");\n";
    }
    os << "}\n";
  }
}

} // namespace hirct
