// HierUnreachable: root UnreachTop uses Adder8 but Orphan is unreachable.
// Orphan artifact must NOT be generated.
module {
  hw.module @UnreachTop(in %a : i8, in %b : i8, out sum : i8) {
    %s = hw.instance "add0" @Adder8(a: %a: i8, b: %b: i8) -> (sum: i8)
    hw.output %s : i8
  }
  hw.module private @Adder8(in %a : i8, in %b : i8, out sum : i8) {
    %0 = comb.add %a, %b : i8
    hw.output %0 : i8
  }
  hw.module private @Orphan(in %x : i8, out y : i8) {
    hw.output %x : i8
  }
}
