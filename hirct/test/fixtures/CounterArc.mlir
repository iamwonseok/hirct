module {
  arc.define @counter_body(%arg0: i8, %arg1: i1) -> i8 {
    %c1_i8 = hw.constant 1 : i8
    %c0_i8 = hw.constant 0 : i8
    %0 = comb.add %arg0, %c1_i8 : i8
    %1 = comb.mux %arg1, %c0_i8, %0 : i8
    arc.output %1 : i8
  }
  arc.define @clk_conv(%arg0: i1) -> !seq.clock {
    %0 = seq.to_clock %arg0
    arc.output %0 : !seq.clock
  }
  hw.module @CounterArc(in %clk : i1, in %rst : i1, out count : i8) {
    %clock = arc.call @clk_conv(%clk) : (i1) -> !seq.clock
    %cnt = arc.state @counter_body(%cnt, %rst) clock %clock latency 1 {names = ["cnt"]} : (i8, i1) -> i8
    hw.output %cnt : i8
  }
}
