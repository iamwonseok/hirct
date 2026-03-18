#include "hirct/Analysis/FSMAnalysis.h"
#include "circt/Dialect/Comb/CombOps.h"
#include "circt/Dialect/Seq/SeqOps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include <algorithm>
#include <functional>

namespace hirct {

static bool verify_mux_chain_feedback(
    mlir::Value next_input,
    const llvm::SmallVector<mlir::Value> &icmp_results) {
  llvm::SmallVector<mlir::Value, 16> worklist;
  worklist.push_back(next_input);
  llvm::DenseSet<mlir::Value> visited;

  while (!worklist.empty()) {
    mlir::Value v = worklist.pop_back_val();
    if (!visited.insert(v).second)
      continue;

    if (auto mux = v.getDefiningOp<circt::comb::MuxOp>()) {
      for (auto icmp : icmp_results) {
        if (mux.getCond() == icmp)
          return true;
      }
      worklist.push_back(mux.getTrueValue());
      worklist.push_back(mux.getFalseValue());
    }
  }
  return false;
}

static void extract_transitions(
    mlir::Value next_val, const std::vector<FSMStateView> &states,
    std::vector<FSMTransitionView> &transitions) {
  llvm::DenseMap<mlir::Value, uint64_t> state_cond_map;
  for (const auto &s : states)
    state_cond_map[s.icmp_result] = s.encoding;

  std::function<void(mlir::Value, std::optional<uint64_t>)> walk_mux;
  walk_mux = [&](mlir::Value v, std::optional<uint64_t> from_state) {
    auto mux = v.getDefiningOp<circt::comb::MuxOp>();
    if (!mux) {
      if (from_state.has_value()) {
        if (auto c = v.getDefiningOp<circt::hw::ConstantOp>())
          transitions.push_back(
              {*from_state, c.getValue().getZExtValue(), mlir::Value()});
      }
      return;
    }

    auto it = state_cond_map.find(mux.getCond());
    if (it != state_cond_map.end()) {
      uint64_t state_enc = it->second;

      auto inner_mux = mux.getTrueValue().getDefiningOp<circt::comb::MuxOp>();
      if (inner_mux) {
        if (auto tc =
                inner_mux.getTrueValue().getDefiningOp<circt::hw::ConstantOp>())
          transitions.push_back(
              {state_enc, tc.getValue().getZExtValue(), inner_mux.getCond()});
        if (auto fc =
                inner_mux.getFalseValue().getDefiningOp<circt::hw::ConstantOp>())
          transitions.push_back(
              {state_enc, fc.getValue().getZExtValue(), mlir::Value()});
      } else if (auto tc =
                     mux.getTrueValue().getDefiningOp<circt::hw::ConstantOp>()) {
        transitions.push_back(
            {state_enc, tc.getValue().getZExtValue(), mlir::Value()});
      }

      walk_mux(mux.getFalseValue(), std::nullopt);
    } else if (from_state.has_value()) {
      walk_mux(mux.getTrueValue(), from_state);
      walk_mux(mux.getFalseValue(), from_state);
    } else {
      walk_mux(mux.getTrueValue(), std::nullopt);
      walk_mux(mux.getFalseValue(), std::nullopt);
    }
  };

  walk_mux(next_val, std::nullopt);
}

/// Walk the mux chain of a data register's next value and extract per-state
/// update entries. For each mux whose condition is an FSM state comparison, we
/// record the state encoding, an optional inner condition, and the resolved
/// new value.
static void extract_data_reg_updates(
    mlir::Value next_val, const std::vector<FSMStateView> &states,
    size_t reg_idx, const std::string &reg_name,
    std::vector<DataRegUpdate> &updates) {
  llvm::DenseMap<mlir::Value, uint64_t> state_cond_map;
  for (const auto &s : states)
    state_cond_map[s.icmp_result] = s.encoding;

  std::function<void(mlir::Value, std::optional<uint64_t>, mlir::Value)>
      walk_mux;
  walk_mux = [&](mlir::Value v, std::optional<uint64_t> cur_state,
                 mlir::Value cur_cond) {
    auto mux = v.getDefiningOp<circt::comb::MuxOp>();
    if (!mux) {
      if (cur_state.has_value())
        updates.push_back({*cur_state, reg_idx, cur_cond, v, reg_name});
      return;
    }

    auto it = state_cond_map.find(mux.getCond());
    if (it != state_cond_map.end()) {
      uint64_t state_enc = it->second;
      auto inner_mux = mux.getTrueValue().getDefiningOp<circt::comb::MuxOp>();
      if (inner_mux &&
          state_cond_map.find(inner_mux.getCond()) == state_cond_map.end()) {
        updates.push_back({state_enc, reg_idx, inner_mux.getCond(),
                           inner_mux.getTrueValue(), reg_name});
        updates.push_back({state_enc, reg_idx, mlir::Value(),
                           inner_mux.getFalseValue(), reg_name});
      } else {
        updates.push_back(
            {state_enc, reg_idx, mlir::Value(), mux.getTrueValue(), reg_name});
      }
      walk_mux(mux.getFalseValue(), std::nullopt, mlir::Value());
    } else if (cur_state.has_value()) {
      walk_mux(mux.getTrueValue(), cur_state, cur_cond);
      walk_mux(mux.getFalseValue(), cur_state, cur_cond);
    } else {
      walk_mux(mux.getTrueValue(), std::nullopt, mlir::Value());
      walk_mux(mux.getFalseValue(), std::nullopt, mlir::Value());
    }
  };

  walk_mux(next_val, std::nullopt, mlir::Value());
}

static void extract_output_assignments(
    mlir::Value out_val, const std::string &port_name,
    const std::vector<FSMStateView> &states,
    std::vector<OutputAssignment> &assignments) {
  llvm::DenseMap<mlir::Value, uint64_t> state_cond_map;
  for (const auto &s : states)
    state_cond_map[s.icmp_result] = s.encoding;

  if (!out_val.getDefiningOp<circt::comb::MuxOp>()) {
    bool refs_state = false;
    llvm::SmallVector<mlir::Value, 8> worklist;
    llvm::DenseSet<mlir::Value> visited;
    worklist.push_back(out_val);
    while (!worklist.empty()) {
      mlir::Value vv = worklist.pop_back_val();
      if (!visited.insert(vv).second)
        continue;
      if (state_cond_map.count(vv)) {
        refs_state = true;
        break;
      }
      if (auto *def = vv.getDefiningOp()) {
        for (auto operand : def->getOperands())
          worklist.push_back(operand);
      }
    }
    if (refs_state) {
      for (const auto &s : states)
        assignments.push_back(
            {port_name, s.encoding, out_val});
    }
    return;
  }

  std::function<void(mlir::Value, std::optional<uint64_t>)> walk_mux;
  walk_mux = [&](mlir::Value v, std::optional<uint64_t> cur_state) {
    auto mux = v.getDefiningOp<circt::comb::MuxOp>();
    if (!mux) {
      if (cur_state.has_value())
        assignments.push_back(
            {port_name, *cur_state, v});
      return;
    }

    auto it = state_cond_map.find(mux.getCond());
    if (it != state_cond_map.end()) {
      uint64_t state_enc = it->second;
      assignments.push_back(
          {port_name, state_enc, mux.getTrueValue()});
      walk_mux(mux.getFalseValue(), std::nullopt);
    } else if (cur_state.has_value()) {
      walk_mux(mux.getTrueValue(), cur_state);
      walk_mux(mux.getFalseValue(), cur_state);
    } else {
      walk_mux(mux.getTrueValue(), std::nullopt);
      walk_mux(mux.getFalseValue(), std::nullopt);
    }
  };

  walk_mux(out_val, std::nullopt);
}

std::vector<FSMView> identify_fsm_registers(circt::hw::HWModuleOp module) {
  auto regs = collect_registers(module);
  std::vector<FSMView> result;

  for (const auto &reg : regs) {
    std::vector<FSMStateView> states;
    llvm::SmallVector<mlir::Value> icmp_results;
    llvm::DenseSet<uint64_t> seen_encodings;

    mlir::Value reg_result = reg.op->getResult(0);

    for (auto *user : reg_result.getUsers()) {
      auto icmp = mlir::dyn_cast<circt::comb::ICmpOp>(user);
      if (!icmp)
        continue;
      if (icmp.getPredicate() != circt::comb::ICmpPredicate::eq)
        continue;

      mlir::Value other;
      if (icmp.getLhs() == reg_result)
        other = icmp.getRhs();
      else if (icmp.getRhs() == reg_result)
        other = icmp.getLhs();
      else
        continue;

      auto const_op = other.getDefiningOp<circt::hw::ConstantOp>();
      if (!const_op)
        continue;

      uint64_t encoding = const_op.getValue().getZExtValue();
      if (!seen_encodings.insert(encoding).second)
        continue;
      states.push_back({encoding, icmp.getResult()});
      icmp_results.push_back(icmp.getResult());
    }

    if (states.size() < 2)
      continue;

    mlir::Value next_input;
    if (auto compreg = mlir::dyn_cast<circt::seq::CompRegOp>(reg.op))
      next_input = compreg.getInput();
    else if (auto firreg = mlir::dyn_cast<circt::seq::FirRegOp>(reg.op))
      next_input = firreg.getNext();

    if (!next_input)
      continue;

    if (!verify_mux_chain_feedback(next_input, icmp_results))
      continue;

    std::sort(states.begin(), states.end(),
              [](const FSMStateView &a, const FSMStateView &b) {
                return a.encoding < b.encoding;
              });

    FSMView fsm;
    fsm.state_reg = reg;
    fsm.states = std::move(states);
    extract_transitions(next_input, fsm.states, fsm.transitions);

    std::sort(fsm.transitions.begin(), fsm.transitions.end(),
              [](const FSMTransitionView &a, const FSMTransitionView &b) {
                if (a.from_encoding != b.from_encoding)
                  return a.from_encoding < b.from_encoding;
                return a.to_encoding < b.to_encoding;
              });

    llvm::DenseSet<mlir::Value> state_icmp_set;
    for (const auto &s : fsm.states)
      state_icmp_set.insert(s.icmp_result);

    for (const auto &other_reg : regs) {
      if (other_reg.op == reg.op)
        continue;

      mlir::Value other_next;
      if (auto cr = mlir::dyn_cast<circt::seq::CompRegOp>(other_reg.op))
        other_next = cr.getInput();
      else if (auto fr = mlir::dyn_cast<circt::seq::FirRegOp>(other_reg.op))
        other_next = fr.getNext();

      if (!other_next)
        continue;

      bool uses_fsm_state = false;
      llvm::SmallVector<mlir::Value, 16> worklist;
      llvm::DenseSet<mlir::Value> visited;
      worklist.push_back(other_next);
      while (!worklist.empty()) {
        mlir::Value v = worklist.pop_back_val();
        if (!visited.insert(v).second)
          continue;
        if (state_icmp_set.count(v)) {
          uses_fsm_state = true;
          break;
        }
        if (auto *def = v.getDefiningOp()) {
          for (auto operand : def->getOperands())
            worklist.push_back(operand);
        }
      }

      if (uses_fsm_state) {
        size_t reg_idx = fsm.data_regs.size();
        fsm.data_regs.push_back(other_reg);
        extract_data_reg_updates(other_next, fsm.states, reg_idx,
                                 other_reg.name, fsm.data_reg_updates);
      }
    }

    // Extract output assignments from hw.output operands
    auto output_op = module.getBodyBlock()->getTerminator();
    if (output_op) {
      auto port_list = module.getPortList();
      unsigned out_port_idx = 0;
      for (auto &port : port_list) {
        if (!port.isOutput())
          continue;
        if (out_port_idx < output_op->getNumOperands()) {
          mlir::Value out_val = output_op->getOperand(out_port_idx);
          extract_output_assignments(out_val, port.getName().str(), fsm.states,
                                     fsm.output_assignments);
        }
        ++out_port_idx;
      }
    }

    result.push_back(std::move(fsm));
  }

  return result;
}

} // namespace hirct
