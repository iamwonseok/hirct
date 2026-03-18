// Fixture: combinational ops with forward references (use before define in IR order).
// Models the pattern seen in DW_apb_uart_bcm23 where %0 uses %10 which is defined later.
module {
  hw.module @ForwardRefComb(in %clk : i1, in %rst_n : i1, in %a : i1, in %b : i1, out y : i1) {
    %true = hw.constant true
    %false = hw.constant false
    // %0 uses %2, which is defined *after* this line in IR order (forward reference)
    %0 = comb.and %a, %2 : i1
    %clk_s = seq.to_clock %clk
    %rst_inv = comb.xor %rst_n, %true : i1
    %reg = seq.firreg %0 clock %clk_s reset async %rst_inv, %false : i1
    // %2 is defined here — after %0 used it
    %2 = comb.xor %b, %reg : i1
    hw.output %reg : i1
  }
}
