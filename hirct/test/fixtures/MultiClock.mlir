module {
  hw.module @MultiClock(in %pclk : i1, in %sclk : i1, in %data : i8, out out_a : i8, out out_b : i8) {
    %pclk_c = seq.to_clock %pclk
    %sclk_c = seq.to_clock %sclk
    %c0 = hw.constant 0 : i8
    %regA = seq.compreg %data, %pclk_c : i8
    %regB = seq.compreg %data, %sclk_c : i8
    hw.output %regA, %regB : i8, i8
  }
}
