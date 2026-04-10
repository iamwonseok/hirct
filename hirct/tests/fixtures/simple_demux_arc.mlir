module {
  arc.define @simple_demux_arc(%arg0: i4, %arg1: i4, %arg2: i32, %arg3: i32, %arg4: i4, %arg5: i4, %arg6: i4, %arg7: i4, %arg8: i4, %arg9: i4, %arg10: i4, %arg11: i4, %arg12: i4, %arg13: i4, %arg14: i4, %arg15: i4, %arg16: i4, %arg17: i4, %arg18: i4) -> i512 {
    %0 = comb.icmp eq %arg0, %arg1 : i4
    %1 = comb.mux %0, %arg2, %arg3 : i32
    %2 = comb.icmp eq %arg0, %arg4 : i4
    %3 = comb.mux %2, %arg2, %arg3 : i32
    %4 = comb.icmp eq %arg0, %arg5 : i4
    %5 = comb.mux %4, %arg2, %arg3 : i32
    %6 = comb.icmp eq %arg0, %arg6 : i4
    %7 = comb.mux %6, %arg2, %arg3 : i32
    %8 = comb.icmp eq %arg0, %arg7 : i4
    %9 = comb.mux %8, %arg2, %arg3 : i32
    %10 = comb.icmp eq %arg0, %arg8 : i4
    %11 = comb.mux %10, %arg2, %arg3 : i32
    %12 = comb.icmp eq %arg0, %arg9 : i4
    %13 = comb.mux %12, %arg2, %arg3 : i32
    %14 = comb.icmp eq %arg0, %arg10 : i4
    %15 = comb.mux %14, %arg2, %arg3 : i32
    %16 = comb.icmp eq %arg0, %arg11 : i4
    %17 = comb.mux %16, %arg2, %arg3 : i32
    %18 = comb.icmp eq %arg0, %arg12 : i4
    %19 = comb.mux %18, %arg2, %arg3 : i32
    %20 = comb.icmp eq %arg0, %arg13 : i4
    %21 = comb.mux %20, %arg2, %arg3 : i32
    %22 = comb.icmp eq %arg0, %arg14 : i4
    %23 = comb.mux %22, %arg2, %arg3 : i32
    %24 = comb.icmp eq %arg0, %arg15 : i4
    %25 = comb.mux %24, %arg2, %arg3 : i32
    %26 = comb.icmp eq %arg0, %arg16 : i4
    %27 = comb.mux %26, %arg2, %arg3 : i32
    %28 = comb.icmp eq %arg0, %arg17 : i4
    %29 = comb.mux %28, %arg2, %arg3 : i32
    %30 = comb.icmp eq %arg0, %arg18 : i4
    %31 = comb.mux %30, %arg2, %arg3 : i32
    %32 = comb.concat %31, %29, %27, %25, %23, %21, %19, %17, %15, %13, %11, %9, %7, %5, %3, %1 : i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32
    arc.output %32 : i512
  }
  hw.module @simple_demux(in %SEL_I : i4, in %DIN_I : i32, out DOUT_O : i512) {
    %c-1_i4 = hw.constant -1 : i4
    %c-2_i4 = hw.constant -2 : i4
    %c-3_i4 = hw.constant -3 : i4
    %c-4_i4 = hw.constant -4 : i4
    %c-5_i4 = hw.constant -5 : i4
    %c-6_i4 = hw.constant -6 : i4
    %c-7_i4 = hw.constant -7 : i4
    %c-8_i4 = hw.constant -8 : i4
    %c7_i4 = hw.constant 7 : i4
    %c6_i4 = hw.constant 6 : i4
    %c5_i4 = hw.constant 5 : i4
    %c4_i4 = hw.constant 4 : i4
    %c3_i4 = hw.constant 3 : i4
    %c2_i4 = hw.constant 2 : i4
    %c1_i4 = hw.constant 1 : i4
    %c0_i4 = hw.constant 0 : i4
    %c0_i32 = hw.constant 0 : i32
    %0 = arc.call @simple_demux_arc(%SEL_I, %c0_i4, %DIN_I, %c0_i32, %c1_i4, %c2_i4, %c3_i4, %c4_i4, %c5_i4, %c6_i4, %c7_i4, %c-8_i4, %c-7_i4, %c-6_i4, %c-5_i4, %c-4_i4, %c-3_i4, %c-2_i4, %c-1_i4) : (i4, i4, i32, i32, i4, i4, i4, i4, i4, i4, i4, i4, i4, i4, i4, i4, i4, i4, i4) -> i512
    hw.output %0 : i512
  }
}

