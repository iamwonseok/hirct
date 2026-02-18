module {
  hw.module @Fadu_K2_S5_LevelGateway(in %preset_flops : i1, in %clock : i1, in %reset : i1, in %io_interrupt : i1, out io_plic_valid : i1, in %io_plic_ready : i1, in %io_plic_complete : i1) {
    %true = hw.constant true
    %false = hw.constant false
    %0 = comb.and %io_interrupt, %io_plic_ready : i1
    %1 = comb.xor %inFlight, %true {sv.namehint = "_T_1"} : i1
    %2 = comb.and %io_interrupt, %1 : i1
    %3 = comb.xor %reset, %true : i1
    %4 = comb.and %3, %io_plic_complete : i1
    %5 = comb.xor %4, %true : i1
    %6 = comb.and %3, %5, %0 : i1
    %7 = comb.or %reset, %4, %0 : i1
    %8 = seq.to_clock %clock
    %9 = comb.mux bin %7, %6, %inFlight : i1
    %inFlight = seq.firreg %9 clock %8 reset async %preset_flops, %false : i1
    hw.output %2 : i1
  }
}
