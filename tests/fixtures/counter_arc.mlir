module {
  func.func private @exit(i32)
  arc.define @fc_counter_arc(%arg0: i8, %arg1: i1, %arg2: i1) -> i8 {
    %c1_i8 = hw.constant 1 : i8
    %c0_i8 = hw.constant 0 : i8
    %0 = comb.add %arg0, %c1_i8 : i8
    %1 = comb.mux %arg1, %arg0, %0 : i8
    %2 = comb.mux %arg2, %c0_i8, %1 : i8
    arc.output %2 : i8
  }
  arc.define @fc_counter_arc_0(%arg0: i1, %arg1: i1, %arg2: i1) -> i1 {
    %0 = comb.and %arg0, %arg1, %arg2 : i1
    arc.output %0 : i1
  }
  arc.define @fc_counter_arc_1(%arg0: i1, %arg1: i1, %arg2: i1) -> i1 {
    %true = hw.constant true
    %0 = comb.xor %arg0, %true : i1
    %1 = comb.and %arg1, %arg2, %0 : i1
    arc.output %1 : i1
  }
  arc.define @fc_counter_arc_2_split_0() -> i1 {
    %true = hw.constant true
    arc.output %true : i1
  }
  arc.define @fc_counter_arc_2_split_1(%arg0: i1) -> !seq.clock {
    %0 = seq.to_clock %arg0
    arc.output %0 : !seq.clock
  }
  arc.define @fc_counter_arc_2_split_2(%arg0: i1, %arg1: i1, %arg2: i1) -> i1 {
    %0 = comb.xor %arg0, %arg1 : i1
    %1 = comb.and %arg2, %0 : i1
    arc.output %1 : i1
  }
  arc.define @fc_counter_arc_2_split_3(%arg0: i1, %arg1: i1) -> i1 {
    %0 = comb.xor %arg0, %arg1 : i1
    arc.output %0 : i1
  }
  arc.define @fc_counter_arc_2_split_5(%arg0: i1, %arg1: i1) -> i1 {
    %0 = comb.or %arg0, %arg1 : i1
    arc.output %0 : i1
  }
  arc.define @fc_counter_arc_3(%arg0: i8, %arg1: i8) -> i1 {
    %0 = comb.icmp eq %arg0, %arg1 : i8
    arc.output %0 : i1
  }
  hw.module @fc_counter(in %clk : i1, in %rst_n : i1, in %en : i1, in %thresh : i8, out count : i8, out overflow : i1, out valid : i1) {
    %0 = arc.state @fc_counter_arc(%0, %5, %7) clock %4 enable %8 latency 1 : (i8, i1, i1) -> i8
    %1 = arc.state @fc_counter_arc_0(%rst_n, %6, %9) clock %4 enable %8 latency 1 : (i1, i1, i1) -> i1
    %2 = arc.state @fc_counter_arc_1(%9, %rst_n, %6) clock %4 enable %8 latency 1 : (i1, i1, i1) -> i1
    %3 = arc.call @fc_counter_arc_2_split_0() : () -> i1
    %4 = arc.call @fc_counter_arc_2_split_1(%clk) : (i1) -> !seq.clock
    %5 = arc.call @fc_counter_arc_2_split_2(%en, %3, %rst_n) : (i1, i1, i1) -> i1
    %6 = arc.call @fc_counter_arc_2_split_3(%5, %3) : (i1, i1) -> i1
    %7 = arc.call @fc_counter_arc_2_split_3(%rst_n, %3) : (i1, i1) -> i1
    %8 = arc.call @fc_counter_arc_2_split_5(%7, %6) : (i1, i1) -> i1
    %9 = arc.call @fc_counter_arc_3(%0, %thresh) : (i8, i8) -> i1
    hw.output %0, %1, %2 : i8, i1, i1
  }
}
