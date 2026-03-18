module {
  hw.module @WideConcat(in %hi : i64, in %lo : i16, out result : i64) {
    %cat = comb.concat %hi, %lo : i64, i16
    %trunc = comb.extract %cat from 0 : (i80) -> i64
    hw.output %trunc : i64
  }
}
