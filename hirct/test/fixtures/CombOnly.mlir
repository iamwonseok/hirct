module {
  hw.module @CombOnly(in %a : i8, in %b : i8, in %sel : i1, out y : i8) {
    %0 = comb.mux %sel, %a, %b : i8
    hw.output %0 : i8
  }
}
