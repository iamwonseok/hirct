module {
  hw.module @MultiModule(in %x : i8, in %y : i8, in %z : i8, out result : i8) {
    %add1_sum = hw.instance "add1" @Adder8(a: %x: i8, b: %y: i8) -> (sum: i8)
    %add2_sum = hw.instance "add2" @Adder8(a: %add1_sum: i8, b: %z: i8) -> (sum: i8)
    hw.output %add2_sum : i8
  }
  hw.module private @Adder8(in %a : i8, in %b : i8, out sum : i8) {
    %0 = comb.add %a, %b : i8
    hw.output %0 : i8
  }
}
