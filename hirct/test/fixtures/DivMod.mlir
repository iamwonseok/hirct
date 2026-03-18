module {
  hw.module @DivMod(in %a : i16, in %b : i16, out divu : i16, out divs : i16, out modu : i16, out mods : i16) {
    %0 = comb.divu %a, %b : i16
    %1 = comb.divs %a, %b : i16
    %2 = comb.modu %a, %b : i16
    %3 = comb.mods %a, %b : i16
    hw.output %0, %1, %2, %3 : i16, i16, i16, i16
  }
}
