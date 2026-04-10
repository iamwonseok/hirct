module {
  arc.define @ncs_hp_first_sig_gen_arc(%arg0: i4, %arg1: i4) -> i1 {
    %0 = comb.icmp ule %arg0, %arg1 : i4
    arc.output %0 : i1
  }
  arc.define @ncs_hp_first_sig_gen_arc_0(%arg0: i4, %arg1: i4, %arg2: i3, %arg3: i4, %arg4: i4, %arg5: i1, %arg6: i4, %arg7: i4, %arg8: i4, %arg9: i4, %arg10: i4, %arg11: i4, %arg12: i4, %arg13: i4, %arg14: i4, %arg15: i4, %arg16: i4, %arg17: i4) -> i4 {
    %0 = comb.icmp ceq %arg0, %arg1 : i4
    %1 = comb.concat %0, %arg2 : i1, i3
    %2 = comb.icmp ceq %arg0, %arg3 : i4
    %3 = comb.icmp ceq %arg0, %arg4 : i4
    %4 = comb.xor %3, %arg5 : i1
    %5 = comb.icmp ceq %arg0, %arg6 : i4
    %6 = comb.xor %5, %arg5 : i1
    %7 = comb.and %6, %4 : i1
    %8 = comb.icmp ceq %arg0, %arg7 : i4
    %9 = comb.xor %8, %arg5 : i1
    %10 = comb.and %9, %7 : i1
    %11 = comb.icmp ceq %arg0, %arg8 : i4
    %12 = comb.xor %11, %arg5 : i1
    %13 = comb.and %12, %10 : i1
    %14 = comb.icmp ceq %arg0, %arg9 : i4
    %15 = comb.xor %14, %arg5 : i1
    %16 = comb.and %15, %13 : i1
    %17 = comb.icmp ceq %arg0, %arg10 : i4
    %18 = comb.xor %17, %arg5 : i1
    %19 = comb.and %18, %16 : i1
    %20 = comb.icmp ceq %arg0, %arg11 : i4
    %21 = comb.xor %20, %arg5 : i1
    %22 = comb.and %21, %19 : i1
    %23 = comb.icmp ceq %arg0, %arg12 : i4
    %24 = comb.xor %23, %arg5 : i1
    %25 = comb.and %24, %22 : i1
    %26 = comb.icmp ceq %arg0, %arg13 : i4
    %27 = comb.xor %26, %arg5 : i1
    %28 = comb.and %27, %25 : i1
    %29 = comb.icmp ceq %arg0, %arg14 : i4
    %30 = comb.xor %29, %arg5 : i1
    %31 = comb.and %30, %28 : i1
    %32 = comb.icmp ceq %arg0, %arg15 : i4
    %33 = comb.xor %32, %arg5 : i1
    %34 = comb.and %33, %31 : i1
    %35 = comb.icmp ceq %arg0, %arg16 : i4
    %36 = comb.xor %35, %arg5 : i1
    %37 = comb.and %36, %34 : i1
    %38 = comb.icmp ceq %arg0, %arg17 : i4
    %39 = comb.xor %38, %arg5 : i1
    %40 = comb.and %39, %37, %2 : i1
    %41 = comb.mux %40, %arg1, %1 : i4
    %42 = comb.and %37, %38 : i1
    %43 = comb.mux %42, %arg3, %41 : i4
    %44 = comb.and %34, %35 : i1
    %45 = comb.mux %44, %arg17, %43 : i4
    %46 = comb.and %31, %32 : i1
    %47 = comb.mux %46, %arg16, %45 : i4
    %48 = comb.and %28, %29 : i1
    %49 = comb.mux %48, %arg15, %47 : i4
    %50 = comb.and %25, %26 : i1
    %51 = comb.mux %50, %arg14, %49 : i4
    %52 = comb.and %22, %23 : i1
    %53 = comb.mux %52, %arg13, %51 : i4
    %54 = comb.and %19, %20 : i1
    %55 = comb.mux %54, %arg12, %53 : i4
    %56 = comb.and %16, %17 : i1
    %57 = comb.mux %56, %arg11, %55 : i4
    %58 = comb.and %13, %14 : i1
    %59 = comb.mux %58, %arg10, %57 : i4
    %60 = comb.and %10, %11 : i1
    %61 = comb.mux %60, %arg9, %59 : i4
    %62 = comb.and %7, %8 : i1
    %63 = comb.mux %62, %arg8, %61 : i4
    %64 = comb.and %4, %5 : i1
    %65 = comb.mux %64, %arg7, %63 : i4
    %66 = comb.mux %3, %arg6, %65 : i4
    arc.output %66 : i4
  }
  arc.define @ncs_hp_first_sig_gen_arc_1(%arg0: i1, %arg1: i1, %arg2: i1) -> (i1, !seq.clock) {
    %0 = comb.xor %arg0, %arg1 : i1
    %1 = seq.to_clock %arg2
    arc.output %0, %1 : i1, !seq.clock
  }
  hw.module @ncs_hp_first_sig_gen(in %RESET_N_I : i1, in %CLK_I : i1, in %THRES_I : i4, out HP_FIRST_O : i1) {
    %c0_i3 = hw.constant 0 : i3
    %true = hw.constant true
    %false = hw.constant false
    %c-7_i4 = hw.constant -7 : i4
    %c-5_i4 = hw.constant -5 : i4
    %c-6_i4 = hw.constant -6 : i4
    %c-2_i4 = hw.constant -2 : i4
    %c-1_i4 = hw.constant -1 : i4
    %c-3_i4 = hw.constant -3 : i4
    %c-4_i4 = hw.constant -4 : i4
    %c4_i4 = hw.constant 4 : i4
    %c5_i4 = hw.constant 5 : i4
    %c7_i4 = hw.constant 7 : i4
    %c6_i4 = hw.constant 6 : i4
    %c2_i4 = hw.constant 2 : i4
    %c3_i4 = hw.constant 3 : i4
    %c1_i4 = hw.constant 1 : i4
    %c0_i4 = hw.constant 0 : i4
    %0 = arc.state @ncs_hp_first_sig_gen_arc(%1, %THRES_I) clock %2#1 reset %2#0 latency 1 {names = ["HP_FIRST_O"]} : (i4, i4) -> i1
    %1 = arc.state @ncs_hp_first_sig_gen_arc_0(%1, %c-7_i4, %c0_i3, %c-5_i4, %c0_i4, %true, %c1_i4, %c3_i4, %c2_i4, %c6_i4, %c7_i4, %c5_i4, %c4_i4, %c-4_i4, %c-3_i4, %c-1_i4, %c-2_i4, %c-6_i4) clock %2#1 reset %2#0 latency 1 {names = ["r_gray_cnt"]} : (i4, i4, i3, i4, i4, i1, i4, i4, i4, i4, i4, i4, i4, i4, i4, i4, i4, i4) -> i4
    %2:2 = arc.call @ncs_hp_first_sig_gen_arc_1(%RESET_N_I, %true, %CLK_I) : (i1, i1, i1) -> (i1, !seq.clock)
    hw.output %0 : i1
  }
}

