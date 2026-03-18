module {
  hw.module @InstanceCrossRef(in %a : i8, in %b : i8, out result : i8) {
    %c1_i8 = hw.constant 1 : i8
    %inc = comb.add %a, %c1_i8 : i8
    %add_sum = hw.instance "add" @Adder8(a: %inc: i8, b: %b: i8) -> (sum: i8)
    %mask = comb.and %add_sum, %b : i8
    hw.output %mask : i8
  }
  hw.module private @Adder8(in %a : i8, in %b : i8, out sum : i8) {
    %0 = comb.add %a, %b : i8
    hw.output %0 : i8
  }
}
