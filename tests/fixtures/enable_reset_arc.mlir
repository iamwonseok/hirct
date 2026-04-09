module {
  arc.define @IdArc(%arg0: i32) -> i32 {
    arc.output %arg0 : i32
  }
  hw.module @RegBank(in %clock : !seq.clock, in %a : i32, in %en : i1, in %rst : i1,
                     out q0 : i32, out q1 : i32) {
    %q0 = arc.state @IdArc(%a) clock %clock enable %en latency 1 {names = ["q0"]} : (i32) -> i32
    %q1 = arc.state @IdArc(%q0) clock %clock enable %en reset %rst latency 1 {names = ["q1"]} : (i32) -> i32
    hw.output %q0, %q1 : i32, i32
  }
}
