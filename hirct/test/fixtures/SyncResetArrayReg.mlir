module {
  hw.module @SyncResetArrayReg(in %clock : i1, in %reset : i1, in %d0 : i8, in %d1 : i8, out out0 : i8, out out1 : i8) {
    %c42_i8 = hw.constant 42 : i8
    %c0_i8 = hw.constant 0 : i8
    %arr_d = hw.array_create %d1, %d0 : i8
    %arr_rst = hw.array_create %c0_i8, %c42_i8 : i8
    %clk = seq.to_clock %clock
    %arr_reg = seq.compreg %arr_d, %clk reset %reset, %arr_rst : !hw.array<2xi8>
    %idx0 = hw.constant 0 : i1
    %idx1 = hw.constant 1 : i1
    %o0 = hw.array_get %arr_reg[%idx0] : !hw.array<2xi8>, i1
    %o1 = hw.array_get %arr_reg[%idx1] : !hw.array<2xi8>, i1
    hw.output %o0, %o1 : i8, i8
  }
}
