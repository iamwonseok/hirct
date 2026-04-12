// HierDedup: root DedupTop instantiates Adder8 twice (add0, add1).
// Adder8 artifact must be generated exactly once.
module {
  hw.module @DedupTop(in %a : i8, in %b : i8, in %c : i8, out sum : i8) {
    %s1 = hw.instance "add0" @Adder8(a: %a: i8, b: %b: i8) -> (sum: i8)
    %s2 = hw.instance "add1" @Adder8(a: %s1: i8, b: %c: i8) -> (sum: i8)
    hw.output %s2 : i8
  }
  hw.module private @Adder8(in %a : i8, in %b : i8, out sum : i8) {
    %0 = comb.add %a, %b : i8
    hw.output %0 : i8
  }
}
