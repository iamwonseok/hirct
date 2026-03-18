module {
  hw.module @FirmemBasic(in %clock : i1, in %reset : i1, in %wr_addr : i4, in %wr_data : i16, in %wr_en : i1, in %rd_addr : i4, out rd_data : i16) {
    %clk = seq.to_clock %clock
    %mem = seq.firmem 0, 0, undefined, undefined : <16 x 16>
    %0 = seq.firmem.read_port %mem[%rd_addr], clock %clk : <16 x 16>
    seq.firmem.write_port %mem[%wr_addr] = %wr_data, clock %clk enable %wr_en : <16 x 16>
    hw.output %0 : i16
  }
}
