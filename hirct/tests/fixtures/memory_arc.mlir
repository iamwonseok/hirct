module {
  arc.define @mem_write(%arg0: i10, %arg1: i32, %arg2: i1) -> (i10, i32, i1) {
    arc.output %arg0, %arg1, %arg2 : i10, i32, i1
  }
  arc.define @simple_mem2_arc(%arg0: i1) -> !seq.clock {
    %0 = seq.to_clock %arg0
    arc.output %0 : !seq.clock
  }
  hw.module @simple_mem2(in %clk : i1, in %wr_en : i1, in %wr_addr : i10,
                          in %wr_data : i32, in %rd_addr : i10, out rd_data : i32) {
    %mem = arc.memory <1024 x i32, i10>
    %rd_val = arc.memory_read_port %mem[%rd_addr] : <1024 x i32, i10>
    %clk_val = arc.call @simple_mem2_arc(%clk) : (i1) -> !seq.clock
    arc.memory_write_port %mem, @mem_write(%wr_addr, %wr_data, %wr_en) clock %clk_val enable latency 1 : <1024 x i32, i10>, i10, i32, i1
    hw.output %rd_val : i32
  }
}
