module {
  arc.define @ncs_core_evt_log_if_arc(%arg0: i1, %arg1: i1, %arg2: i1, %arg3: i1, %arg4: i1, %arg5: i1) -> (i1, !seq.clock, i1) {
    %0 = comb.xor %arg0, %arg1 : i1
    %1 = seq.to_clock %arg2
    %2 = comb.mux %arg3, %arg4, %arg5 : i1
    arc.output %0, %1, %2 : i1, !seq.clock, i1
  }
  arc.define @ncs_core_evt_log_if_arc_0(%arg0: i512, %arg1: i1, %arg2: i1, %arg3: i1) -> (i31, i1, i1) {
    %0 = comb.extract %arg0 from 30 : (i512) -> i31
    %1 = comb.and %arg1, %arg2 : i1
    %2 = comb.xor %arg2, %arg3 : i1
    %3 = comb.and %arg1, %2 : i1
    arc.output %0, %1, %3 : i31, i1, i1
  }
  arc.define @ncs_core_evt_log_if_arc_1(%arg0: i1, %arg1: i1, %arg2: i1, %arg3: i1, %arg4: i1) -> (i1, i1) {
    %0 = comb.icmp ceq %arg0, %arg1 : i1
    %1 = comb.mux %0, %arg2, %arg3 : i1
    %2 = comb.and %arg4, %1 : i1
    arc.output %0, %2 : i1, i1
  }
  arc.define @ncs_core_evt_log_if_arc_2(%arg0: i1) -> i1 {
    arc.output %arg0 : i1
  }
  hw.module @ncs_core_evt_log_if(in %RESET_N_I : i1, in %CLK_I : i1, in %RD_MPM_RD_REQ_VALID_I : i1, out RD_MPM_RD_REQ_READY_O : i1, in %RD_MPM_RD_REQ_I : i512, out WR_LOG_DRAM_DATA_VALID_O : i1, in %WR_LOG_DRAM_DATA_READY_I : i1, out WR_LOG_DRAM_DATA_O : i512, out WR_LOG_DRAM_DATA_LAST_O : i1, out WR_LOG_REQ_VALID_O : i1, in %WR_LOG_REQ_READY_I : i1, out WR_LOG_REQ_64B_ADDR_O : i31, out DBG_EVT_LOG_IF_O : i1) {
    %true = hw.constant true
    %false = hw.constant false
    %0:3 = arc.call @ncs_core_evt_log_if_arc(%RESET_N_I, %true, %CLK_I, %2#1, %2#0, %3) : (i1, i1, i1, i1, i1, i1) -> (i1, !seq.clock, i1)
    %1:3 = arc.call @ncs_core_evt_log_if_arc_0(%RD_MPM_RD_REQ_I, %2#1, %2#0, %true) : (i512, i1, i1, i1) -> (i31, i1, i1)
    %2:2 = arc.call @ncs_core_evt_log_if_arc_1(%3, %false, %WR_LOG_REQ_READY_I, %WR_LOG_DRAM_DATA_READY_I, %RD_MPM_RD_REQ_VALID_I) : (i1, i1, i1, i1, i1) -> (i1, i1)
    %3 = arc.state @ncs_core_evt_log_if_arc_2(%0#2) clock %0#1 reset %0#0 latency 1 {names = ["r_current_state"]} : (i1) -> i1
    hw.output %2#1, %1#2, %RD_MPM_RD_REQ_I, %true, %1#1, %1#0, %3 : i1, i1, i512, i1, i1, i31, i1
  }
}

