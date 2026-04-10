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
  arc.define @clk_arc(%arg0: i1) -> !seq.clock {
    %0 = seq.to_clock %arg0
    arc.output %0 : !seq.clock
  }
  hw.module @fc_counter(in %clk : i1, in %rst : i1, in %en : i1,
                        out count : i8, out doubled : i8, out tripled : i8) {
    %clock = arc.call @clk_arc(%clk) : (i1) -> !seq.clock
    %cnt = arc.state @fc_counter_arc(%cnt, %en, %rst) clock %clock enable %en latency 1 {names = ["cnt"]} : (i8, i1, i1) -> i8
    %doubled = arc.state @fc_counter_arc(%doubled, %en, %rst) clock %clock enable %en latency 1 {names = ["doubled"]} : (i8, i1, i1) -> i8
    %tripled = arc.state @fc_counter_arc(%tripled, %en, %rst) clock %clock enable %en latency 1 {names = ["tripled"]} : (i8, i1, i1) -> i8
    hw.output %cnt, %doubled, %tripled : i8, i8, i8
  }
}
