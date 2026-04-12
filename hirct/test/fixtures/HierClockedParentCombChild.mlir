// HierClockedParentCombChild: parent has a clock domain, child is comb-only.
// Used to verify that parent's eval_{clock} does NOT call {Child}_eval_{clock}
// when the child has no such clock domain.
module {
  arc.define @arc_acc(%arg0 : i8, %arg1 : i8) -> i8 {
    %r = comb.add %arg0, %arg1 : i8
    arc.output %r : i8
  }
  arc.define @clk_conv(%arg0: i1) -> !seq.clock {
    %0 = seq.to_clock %arg0
    arc.output %0 : !seq.clock
  }
  hw.module @Top(in %clk : i1, in %a : i8, in %b : i8, out sum : i8, out acc : i8) {
    %clock = arc.call @clk_conv(%clk) : (i1) -> !seq.clock
    %s = hw.instance "add0" @Adder8(a: %a: i8, b: %b: i8) -> (sum: i8)
    %acc_reg = arc.state @arc_acc(%a, %s) clock %clock latency 1 {names = ["acc"]} : (i8, i8) -> i8
    hw.output %s, %acc_reg : i8, i8
  }
  hw.module private @Adder8(in %a : i8, in %b : i8, out sum : i8) {
    %0 = comb.add %a, %b : i8
    hw.output %0 : i8
  }
}
