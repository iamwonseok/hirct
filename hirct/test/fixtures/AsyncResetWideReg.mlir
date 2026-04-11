// Fixture: async reset combined with wide scalar registers (> 64 bits).
// Single-clock module with:
//   - 128-bit firreg with async reset and non-zero reset value
//   - 128-bit compreg with sync reset and non-zero reset value (control case)
module {
  hw.module @AsyncResetWideReg(in %clock : i1, in %reset : i1,
                                in %d128 : i128,
                                out out_async : i128, out out_sync : i128) {
    %clk = seq.to_clock %clock
    // i128 reset value: lower word = 0xCAFEBABE12345678, upper word = 0xAA
    %c_rst = hw.constant 0xAACAFEBABE12345678 : i128
    %c_rst2 = hw.constant 0xBBDEADBEEF00000001 : i128
    %reg_async = seq.firreg %d128 clock %clk reset async %reset, %c_rst : i128
    %reg_sync = seq.compreg %d128, %clk reset %reset, %c_rst2 : i128
    hw.output %reg_async, %reg_sync : i128, i128
  }
}
