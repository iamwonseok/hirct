// Fixture: wide register reset in a multi-clock module.
// Two clock domains force emit_domain_step usage:
//   - pclk domain: 128-bit firreg with async reset, non-zero reset value
//   - sclk domain: 128-bit compreg with sync reset, non-zero reset value
module {
  hw.module @AsyncResetWideRegMultiClock(in %pclk : i1, in %sclk : i1,
                                          in %reset : i1,
                                          in %d128a : i128, in %d128b : i128,
                                          out out_async : i128, out out_sync : i128) {
    %pclk_c = seq.to_clock %pclk
    %sclk_c = seq.to_clock %sclk
    // i128 reset value: lower word = 0xCAFEBABE12345678, upper word = 0xAA
    %c_rst = hw.constant 0xAACAFEBABE12345678 : i128
    %c_rst2 = hw.constant 0xBBDEADBEEF00000001 : i128
    %reg128_async = seq.firreg %d128a clock %pclk_c reset async %reset, %c_rst : i128
    %reg128_sync = seq.compreg %d128b, %sclk_c reset %reset, %c_rst2 : i128
    hw.output %reg128_async, %reg128_sync : i128, i128
  }
}
