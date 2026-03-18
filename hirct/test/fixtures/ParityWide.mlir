hw.module @ParityWide(in %in: i128, out out: i1) {
  %0 = comb.parity %in : i128
  hw.output %0 : i1
}
