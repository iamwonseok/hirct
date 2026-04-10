module {
  arc.define @ncs_core_timer_arc(%arg0: i18, %arg1: i16, %arg2: i16, %arg3: i2, %arg4: i2, %arg5: i1, %arg6: i1, %arg7: i2, %arg8: i1) -> i16 {
    %0 = comb.extract %arg0 from 2 : (i18) -> i16
    %1 = comb.add %arg1, %arg2 : i16
    %2 = comb.icmp eq %arg3, %arg4 : i2
    %3 = comb.and %2, %arg5 : i1
    %4 = comb.xor %3, %arg6 : i1
    %5 = comb.icmp eq %arg3, %arg7 : i2
    %6 = comb.and %5, %arg8 : i1
    %7 = comb.and %6, %4 : i1
    %8 = comb.mux %7, %1, %0 : i16
    %9 = comb.or %3, %6 : i1
    %10 = comb.mux bin %9, %8, %arg1 : i16
    arc.output %10 : i16
  }
  arc.define @ncs_core_timer_arc_0(%arg0: i2, %arg1: i2, %arg2: i1, %arg3: i1, %arg4: i2, %arg5: i2, %arg6: i2, %arg7: i2, %arg8: i1, %arg9: i16, %arg10: i16, %arg11: i1, %arg12: i1) -> i2 {
    %0 = comb.icmp eq %arg0, %arg1 : i2
    %1 = comb.and %arg2, %0 : i1
    %2 = comb.xor %1, %arg3 : i1
    %3 = comb.icmp ceq %arg4, %arg5 : i2
    %4 = comb.xor %3, %arg3 : i1
    %5 = comb.icmp ceq %arg4, %arg6 : i2
    %6 = comb.xor %5, %arg3 : i1
    %7 = comb.and %6, %4 : i1
    %8 = comb.icmp ceq %arg4, %arg7 : i2
    %9 = comb.and %8, %7, %2 : i1
    %10 = comb.concat %arg8, %9 : i1, i1
    %11 = comb.icmp eq %arg9, %arg10 : i16
    %12 = comb.or %11, %1 : i1
    %13 = comb.xor %12, %arg3 : i1
    %14 = comb.and %5, %4, %13 : i1
    %15 = comb.mux %14, %arg7, %10 : i2
    %16 = comb.mux %3, %arg6, %15 : i2
    %17 = comb.icmp eq %arg0, %arg7 : i2
    %18 = comb.and %arg2, %17 : i1
    %19 = comb.mux %9, %18, %arg11 : i1
    %20 = comb.icmp eq %arg0, %arg6 : i2
    %21 = comb.and %arg2, %20 : i1
    %22 = comb.mux %14, %21, %19 : i1
    %23 = comb.mux %3, %arg12, %22 : i1
    %24 = comb.mux %23, %16, %arg4 : i2
    %25 = comb.xor %8, %arg3 : i1
    %26 = comb.and %25, %7 : i1
    %27 = comb.or %26, %9, %14, %3 : i1
    %28 = comb.mux %27, %24, %arg1 : i2
    arc.output %28 : i2
  }
  arc.define @ncs_core_timer_arc_1(%arg0: i1, %arg1: i1, %arg2: i1, %arg3: i18, %arg4: i2, %arg5: i1) -> (i1, !seq.clock, i2, i1) {
    %0 = comb.xor %arg0, %arg1 : i1
    %1 = seq.to_clock %arg2
    %2 = comb.extract %arg3 from 0 : (i18) -> i2
    %3 = comb.icmp eq %2, %arg4 : i2
    %4 = comb.and %arg5, %3 : i1
    arc.output %0, %1, %2, %4 : i1, !seq.clock, i2, i1
  }
  arc.define @ncs_core_timer_arc_2(%arg0: i2, %arg1: i2) -> i1 {
    %0 = comb.icmp eq %arg0, %arg1 : i2
    arc.output %0 : i1
  }
  hw.module @ncs_core_timer(in %RESET_N_I : i1, in %CLK_I : i1, in %TIMER_TICK_I : i1, in %RD_TIMER_REQ_VALID_I : i1, in %RD_TIMER_REQ_I : i18, out WR_TIMER_CPL_VALID_O : i1, in %WR_TIMER_CPL_READY_I : i1) {
    %true = hw.constant true
    %c-1_i16 = hw.constant -1 : i16
    %c-1_i2 = hw.constant -1 : i2
    %c-2_i2 = hw.constant -2 : i2
    %c1_i2 = hw.constant 1 : i2
    %c0_i2 = hw.constant 0 : i2
    %c0_i16 = hw.constant 0 : i16
    %false = hw.constant false
    %0 = arc.state @ncs_core_timer_arc(%RD_TIMER_REQ_I, %0, %c-1_i16, %1, %c0_i2, %2#3, %true, %c1_i2, %TIMER_TICK_I) clock %2#1 reset %2#0 latency 1 {names = ["r_timer"]} : (i18, i16, i16, i2, i2, i1, i1, i2, i1) -> i16
    %1 = arc.state @ncs_core_timer_arc_0(%2#2, %c-1_i2, %RD_TIMER_REQ_VALID_I, %true, %1, %c0_i2, %c1_i2, %c-2_i2, %false, %0, %c0_i16, %WR_TIMER_CPL_READY_I, %2#3) clock %2#1 reset %2#0 latency 1 {names = ["r_current_state"]} : (i2, i2, i1, i1, i2, i2, i2, i2, i1, i16, i16, i1, i1) -> i2
    %2:4 = arc.call @ncs_core_timer_arc_1(%RESET_N_I, %true, %CLK_I, %RD_TIMER_REQ_I, %c0_i2, %RD_TIMER_REQ_VALID_I) : (i1, i1, i1, i18, i2, i1) -> (i1, !seq.clock, i2, i1)
    %3 = arc.call @ncs_core_timer_arc_2(%1, %c-1_i2) : (i2, i2) -> i1
    hw.output %3 : i1
  }
}

