module {
  hw.module @SyncResetArrayRegMultiClock(in %clkA : i1, in %clkB : i1, in %reset : i1, in %dA0 : i8, in %dA1 : i8, in %dB0 : i16, in %dB1 : i16, out outA0 : i8, out outA1 : i8, out outB0 : i16, out outB1 : i16) {
    %c42_i8 = hw.constant 42 : i8
    %c0_i8 = hw.constant 0 : i8
    %arr_dA = hw.array_create %dA1, %dA0 : i8
    %arr_rstA = hw.array_create %c0_i8, %c42_i8 : i8
    %ckA = seq.to_clock %clkA
    %arr_regA = seq.compreg %arr_dA, %ckA reset %reset, %arr_rstA : !hw.array<2xi8>
    %c1000_i16 = hw.constant 1000 : i16
    %c0_i16 = hw.constant 0 : i16
    %arr_dB = hw.array_create %dB1, %dB0 : i16
    %arr_rstB = hw.array_create %c0_i16, %c1000_i16 : i16
    %ckB = seq.to_clock %clkB
    %arr_regB = seq.compreg %arr_dB, %ckB reset %reset, %arr_rstB : !hw.array<2xi16>
    %idx0 = hw.constant 0 : i1
    %idx1 = hw.constant 1 : i1
    %oA0 = hw.array_get %arr_regA[%idx0] : !hw.array<2xi8>, i1
    %oA1 = hw.array_get %arr_regA[%idx1] : !hw.array<2xi8>, i1
    %oB0 = hw.array_get %arr_regB[%idx0] : !hw.array<2xi16>, i1
    %oB1 = hw.array_get %arr_regB[%idx1] : !hw.array<2xi16>, i1
    hw.output %oA0, %oA1, %oB0, %oB1 : i8, i8, i16, i16
  }
}
