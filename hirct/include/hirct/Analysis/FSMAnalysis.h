#ifndef HIRCT_ANALYSIS_FSMANALYSIS_H
#define HIRCT_ANALYSIS_FSMANALYSIS_H

#include "hirct/Analysis/IRAnalysis.h"
#include <vector>

namespace hirct {

struct FSMStateView {
  uint64_t encoding;
  mlir::Value icmp_result;
};

struct FSMTransitionView {
  uint64_t from_encoding;
  uint64_t to_encoding;
  /// Transition guard condition. Empty Value means unconditional transition.
  mlir::Value condition;
};

/// A single update entry for a data register: in which FSM state the register
/// takes a new value, under what optional condition, and what the new value is.
struct DataRegUpdate {
  /// Encoding of the FSM state in which this update occurs.
  uint64_t state_encoding;
  /// Index into FSMView::data_regs identifying which register this entry
  /// belongs to.
  size_t reg_idx;
  /// Guard condition within the state; empty Value means unconditional.
  mlir::Value condition;
  /// The value the register takes in this state.
  mlir::Value new_value;
  /// Name of the data register this update belongs to.
  std::string reg_name;
};

/// Output port assignment derived from FSM state: in a given state, an output
/// port is assigned a specific combinational value.
struct OutputAssignment {
  /// Name of the output port.
  std::string port_name;
  /// FSM state encoding in which this assignment applies.
  uint64_t state_encoding;
  /// Combinational value driving this output in this state.
  mlir::Value value;
};

struct FSMView {
  RegisterView state_reg;
  std::vector<FSMStateView> states;
  std::vector<FSMTransitionView> transitions;
  /// Registers whose next-value depends on FSM state comparisons.
  std::vector<RegisterView> data_regs;
  /// Per-state update entries for all data registers.
  /// States not represented here retain their current value (hold).
  std::vector<DataRegUpdate> data_reg_updates;
  /// Per-state output port assignments derived from combinational output logic.
  std::vector<OutputAssignment> output_assignments;
};

/// Identify FSM patterns within an HW module.
///
/// Scans all registers for FSM state register candidates (≥2 eq comparisons
/// with constants, mux-chain feedback). For each identified FSM, also
/// classifies remaining registers as data registers if their next-value
/// computation references FSM state comparisons.
///
/// \returns empty vector if no FSM patterns are found (e.g. pure combinational
///          modules with no registers).
std::vector<FSMView> identify_fsm_registers(circt::hw::HWModuleOp module);

} // namespace hirct

#endif
