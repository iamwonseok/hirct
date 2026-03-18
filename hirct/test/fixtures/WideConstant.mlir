// Fixture: wide constants (>= 64 bits) in hw.ConstantOp and register reset.
// Used by wide-constant-crash.test to verify no SIGABRT on APInt::getZExtValue().
module {
  hw.module @WideConstant(in %clock : i1, in %reset : i1,
                           in %d128 : i128, in %d384 : i384,
                           out out128 : i128, out out384 : i384) {
    // i128 constant: lower 64 bits = 0xDEADBEEFCAFEBABE, upper 64 = 0
    %c_i128 = hw.constant 0xDEADBEEFCAFEBABE : i128
    // i384 constant: non-zero wide value
    %c_i384 = hw.constant 0x0102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F20 : i384
    %clk = seq.to_clock %clock
    %reg128 = seq.compreg %d128, %clk reset %reset, %c_i128 : i128
    %reg384 = seq.compreg %d384, %clk reset %reset, %c_i384 : i384
    hw.output %reg128, %reg384 : i128, i384
  }
}
