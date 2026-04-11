// Fixture: multi-clock module with a wide scalar register (width > 64).
// The pclk domain has a 128-bit register; sclk domain has an 8-bit register.
// This forces is_multi_clock == true so emit_domain_step is exercised.
module {
  hw.module @MultiClockWideReg(in %pclk : i1, in %sclk : i1,
                                in %d128 : i128, in %d8 : i8,
                                out out128 : i128, out out8 : i8) {
    %pclk_c = seq.to_clock %pclk
    %sclk_c = seq.to_clock %sclk
    %c0_i128 = hw.constant 0 : i128
    %c0_i8 = hw.constant 0 : i8
    %reg128 = seq.compreg %d128, %pclk_c reset %pclk, %c0_i128 : i128
    %reg8 = seq.compreg %d8, %sclk_c reset %sclk, %c0_i8 : i8
    hw.output %reg128, %reg8 : i128, i8
  }
}
