module {
  arc.define @Id8(%arg0: i8) -> i8 {
    arc.output %arg0 : i8
  }
  hw.module @DualClk(in %clk_a : !seq.clock, in %clk_b : !seq.clock,
                      in %d : i8, out qa : i8, out qb : i8) {
    %qa = arc.state @Id8(%d) clock %clk_a latency 1 {names = ["reg_a"]} : (i8) -> i8
    %qb = arc.state @Id8(%d) clock %clk_b latency 1 {names = ["reg_b"]} : (i8) -> i8
    hw.output %qa, %qb : i8, i8
  }
}
