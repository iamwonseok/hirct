module {
  arc.define @toggle_gen_arc(%arg0: i1, %arg1: i1, %arg2: i1, %arg3: i1) -> i1 {
    %0 = comb.xor %arg0, %arg1, %arg2 : i1
    %1 = comb.and %arg3, %0 : i1
    %2 = comb.mux bin %arg0, %arg1, %1 : i1
    arc.output %2 : i1
  }
  arc.define @toggle_gen_arc_0(%arg0: i16, %arg1: i16, %arg2: i1, %arg3: i16) -> i16 {
    %0 = comb.add %arg0, %arg1 : i16
    %1 = comb.mux %arg2, %0, %arg3 : i16
    arc.output %1 : i16
  }
  arc.define @toggle_gen_arc_1(%arg0: i1, %arg1: i1, %arg2: i1, %arg3: i16, %arg4: i16, %arg5: i1) -> (i1, !seq.clock, i1) {
    %0 = comb.xor %arg0, %arg1 : i1
    %1 = seq.to_clock %arg2
    %2 = comb.icmp ne %arg3, %arg4 : i16
    %3 = comb.and %2, %arg5 : i1
    arc.output %0, %1, %3 : i1, !seq.clock, i1
  }
  hw.module @toggle_gen(in %rst_n : i1, in %clk : i1, in %enable : i1, in %period_0b : i16, out toggle : i1) {
    %true = hw.constant true
    %c1_i16 = hw.constant 1 : i16
    %c0_i16 = hw.constant 0 : i16
    %false = hw.constant false
    %0 = arc.state @toggle_gen_arc(%2#2, %0, %true, %enable) clock %2#1 reset %2#0 latency 1 {names = ["toggle"]} : (i1, i1, i1, i1) -> i1
    %1 = arc.state @toggle_gen_arc_0(%1, %c1_i16, %2#2, %c0_i16) clock %2#1 reset %2#0 latency 1 {names = ["cnt"]} : (i16, i16, i1, i16) -> i16
    %2:3 = arc.call @toggle_gen_arc_1(%rst_n, %true, %clk, %1, %period_0b, %enable) : (i1, i1, i1, i16, i16, i1) -> (i1, !seq.clock, i1)
    hw.output %0 : i1
  }
}

