module {
  hw.module @NonZeroReset(in %clock : i1, in %reset : i1, in %d8 : i8, in %d1 : i1, out out8 : i8, out out1 : i1) {
    %c42_i8 = hw.constant 42 : i8
    %true = hw.constant true
    %clk = seq.to_clock %clock
    %reg8 = seq.compreg %d8, %clk reset %reset, %c42_i8 : i8
    %reg1 = seq.firreg %d1 clock %clk reset async %reset, %true : i1
    hw.output %reg8, %reg1 : i8, i1
  }
}
