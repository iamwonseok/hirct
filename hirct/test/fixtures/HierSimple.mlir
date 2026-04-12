// HierSimple: minimal hierarchy — root Top + child Adder8.
// Used by M3 Batch 1 to verify reachable module artifact emission.
module {
  hw.module @Top(in %a : i8, in %b : i8, out sum : i8) {
    %s = hw.instance "add0" @Adder8(a: %a: i8, b: %b: i8) -> (sum: i8)
    hw.output %s : i8
  }
  hw.module private @Adder8(in %a : i8, in %b : i8, out sum : i8) {
    %0 = comb.add %a, %b : i8
    hw.output %0 : i8
  }
}
