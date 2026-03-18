module {
  hw.module @AsyncResetSSA(in %clock : i1, in %reset : i1, in %data_in : i8, out data_out : i8) {
    %c0_i8 = hw.constant 0 : i8
    %true = hw.constant true
    %rst_inv = comb.xor %reset, %true : i1
    %clk = seq.to_clock %clock
    %reg = seq.firreg %data_in clock %clk reset async %rst_inv, %c0_i8 : i8
    hw.output %reg : i8
  }
}
