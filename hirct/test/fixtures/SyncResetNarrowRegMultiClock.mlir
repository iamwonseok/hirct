module {
  hw.module @SyncResetNarrowRegMultiClock(in %clkA : i1, in %clkB : i1, in %reset : i1, in %dA : i8, in %dB : i16, out outA : i8, out outB : i16) {
    %c42_i8 = hw.constant 42 : i8
    %c1000_i16 = hw.constant 1000 : i16
    %ckA = seq.to_clock %clkA
    %ckB = seq.to_clock %clkB
    %regA = seq.compreg %dA, %ckA reset %reset, %c42_i8 : i8
    %regB = seq.compreg %dB, %ckB reset %reset, %c1000_i16 : i16
    hw.output %regA, %regB : i8, i16
  }
}
