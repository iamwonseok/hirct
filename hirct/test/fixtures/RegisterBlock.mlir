module {
  hw.module @RegisterBlock(in %clock : i1, in %reset : i1, in %write_en : i1, in %write_data : i32, in %write_addr : i2, out status_reg : i32, out control_reg : i32, out data_reg : i32, out config_reg : i32) {
    %c0_i32 = hw.constant 0 : i32
    %c0_i2 = hw.constant 0 : i2
    %c1_i2 = hw.constant 1 : i2
    %c-2_i2 = hw.constant -2 : i2
    %c-1_i2 = hw.constant -1 : i2
    %clk = seq.to_clock %clock
    %sel0 = comb.icmp eq %write_addr, %c0_i2 : i2
    %sel1 = comb.icmp eq %write_addr, %c1_i2 : i2
    %sel2 = comb.icmp eq %write_addr, %c-2_i2 : i2
    %sel3 = comb.icmp eq %write_addr, %c-1_i2 : i2
    %wen0 = comb.and %write_en, %sel0 : i1
    %wen1 = comb.and %write_en, %sel1 : i1
    %wen2 = comb.and %write_en, %sel2 : i1
    %wen3 = comb.and %write_en, %sel3 : i1
    %next_status = comb.mux %wen0, %write_data, %status : i32
    %next_control = comb.mux %wen1, %write_data, %control : i32
    %next_data = comb.mux %wen2, %write_data, %data : i32
    %next_config = comb.mux %wen3, %write_data, %config : i32
    %status = seq.firreg %next_status clock %clk reset async %reset, %c0_i32 : i32
    %control = seq.firreg %next_control clock %clk reset async %reset, %c0_i32 : i32
    %data = seq.firreg %next_data clock %clk reset async %reset, %c0_i32 : i32
    %config = seq.firreg %next_config clock %clk reset async %reset, %c0_i32 : i32
    hw.output %status, %control, %data, %config : i32, i32, i32, i32
  }
}
