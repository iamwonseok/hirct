// HierCrossRef: parent uses child output in its own combinational logic.
// Used by M3 Batch 2 to verify staged binding + cross-module reference.
module {
  hw.module @Top(in %a : i8, in %b : i8, out result : i8) {
    %s = hw.instance "add0" @Adder8(a: %a: i8, b: %b: i8) -> (sum: i8)
    %c42 = hw.constant 42 : i8
    %r = comb.add %s, %c42 : i8
    hw.output %r : i8
  }
  hw.module private @Adder8(in %a : i8, in %b : i8, out sum : i8) {
    %0 = comb.add %a, %b : i8
    hw.output %0 : i8
  }
}
