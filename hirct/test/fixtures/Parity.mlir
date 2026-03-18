module {
  hw.module @Parity(in %a : i8, out y : i1) {
    %0 = comb.parity %a : i8
    hw.output %0 : i1
  }
}
