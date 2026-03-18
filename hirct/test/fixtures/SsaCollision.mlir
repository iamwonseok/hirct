module {
  hw.module @SsaCollision(in %a : i8, in %b : i8, out y : i8, out z : i8) {
    %a.b = comb.add %a, %b : i8
    %a_b = comb.sub %a, %b : i8
    hw.output %a.b, %a_b : i8, i8
  }
}
