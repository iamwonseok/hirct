// Fixture: reset signal derived from internal combinational logic, not a module port.
// The reset signal is produced by comb.and of the port %reset and a constant,
// so reg.reset is NOT a BlockArgument.
module {
  hw.module @NonPortResetSig(in %clock : i1, in %reset : i1,
                              in %d8 : i8, in %d128 : i128,
                              out out_narrow : i8, out out_wide : i128) {
    %clk = seq.to_clock %clock
    %true = hw.constant true
    %internal_rst = comb.and bin %reset, %true : i1

    %c42_i8 = hw.constant 42 : i8
    %reg_narrow = seq.compreg %d8, %clk reset %internal_rst, %c42_i8 : i8

    %c_wide = hw.constant 0xAABBCCDD00112233 : i128
    %reg_wide = seq.compreg %d128, %clk reset %internal_rst, %c_wide : i128

    hw.output %reg_narrow, %reg_wide : i8, i128
  }
}
