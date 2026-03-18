module {
  hw.module @NegConst64(in %a : i64, out y : i64, out z : i4) {
    %c_neg3_i64 = hw.constant -3 : i64
    %c_neg3_i4 = hw.constant -3 : i4
    %0 = comb.add %a, %c_neg3_i64 : i64
    hw.output %0, %c_neg3_i4 : i64, i4
  }
}
