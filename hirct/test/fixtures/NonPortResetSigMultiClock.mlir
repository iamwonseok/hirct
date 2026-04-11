// Fixture: non-port reset in multi-clock module.
// Reset is derived from comb.and, so reg.reset is NOT a BlockArgument.
module {
  hw.module @NonPortResetSigMultiClock(in %clkA : i1, in %clkB : i1, in %reset : i1,
                                        in %dA : i8, in %dB : i128,
                                        out outA : i8, out outB : i128) {
    %true = hw.constant true
    %internal_rst = comb.and bin %reset, %true : i1

    %ckA = seq.to_clock %clkA
    %ckB = seq.to_clock %clkB

    %c42_i8 = hw.constant 42 : i8
    %regA = seq.compreg %dA, %ckA reset %internal_rst, %c42_i8 : i8

    %c_wide = hw.constant 0xAABBCCDD00112233 : i128
    %regB = seq.compreg %dB, %ckB reset %internal_rst, %c_wide : i128

    hw.output %regA, %regB : i8, i128
  }
}
