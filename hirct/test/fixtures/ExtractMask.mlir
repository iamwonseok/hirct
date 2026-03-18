hw.module @ExtractMask(in %in: i8, out out: i4) {
  %0 = comb.extract %in from 4 : (i8) -> i4
  hw.output %0 : i4
}
