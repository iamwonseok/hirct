module {
  arc.define @Top_arc(%arg0: i8, %arg1: i8) -> i8 {
    %0 = comb.xor %arg0, %arg1 : i8
    arc.output %0 : i8
  }
  arc.define @Top_arc_clk(%arg0: i1) -> !seq.clock {
    %0 = seq.to_clock %arg0
    arc.output %0 : !seq.clock
  }
  hw.module @Top(in %clock : i1, in %i0 : i8, in %i1 : i8, out out : i8) {
    %clk = arc.call @Top_arc_clk(%clock) : (i1) -> !seq.clock
    %0 = arc.state @Top_arc(%0, %i1) clock %clk latency 1 {names = ["foo"]} : (i8, i8) -> i8
    %1 = arc.state @Top_arc(%0, %i1) clock %clk latency 1 {names = ["bar"]} : (i8, i8) -> i8
    %2 = comb.mul %0, %1 : i8
    hw.output %2 : i8
  }
}
