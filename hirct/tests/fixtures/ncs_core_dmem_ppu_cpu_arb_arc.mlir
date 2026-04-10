module {
  arc.define @ncs_core_dmem_ppu_cpu_arb_arc(%arg0: i1, %arg1: i1, %arg2: i1, %arg3: i1, %arg4: i1) -> (i1, !seq.clock, i1) {
    %0 = comb.xor %arg0, %arg1 : i1
    %1 = seq.to_clock %arg2
    %2 = comb.and %arg3, %arg4 : i1
    arc.output %0, %1, %2 : i1, !seq.clock, i1
  }
  arc.define @ncs_core_dmem_ppu_cpu_arb_arc_0(%arg0: i1, %arg1: i32, %arg2: i32, %arg3: i13, %arg4: i13, %arg5: i1, %arg6: i1, %arg7: i1, %arg8: i1) -> (i32, i13, i1, i1, i1) {
    %0 = comb.mux %arg0, %arg1, %arg2 : i32
    %1 = comb.mux %arg0, %arg3, %arg4 : i13
    %2 = comb.mux %arg0, %arg5, %arg6 : i1
    %3 = comb.xor %arg0, %arg7 : i1
    %4 = comb.and %3, %arg8 : i1
    %5 = comb.or %arg0, %4 : i1
    %6 = comb.xor %5, %arg7 : i1
    %7 = comb.or %6, %2 : i1
    arc.output %0, %1, %3, %6, %7 : i32, i13, i1, i1, i1
  }
  arc.define @ncs_core_dmem_ppu_cpu_arb_arc_1(%arg0: i1, %arg1: i1, %arg2: i1, %arg3: i1) -> i1 {
    %0 = comb.xor %arg0, %arg1 : i1
    %1 = comb.and %arg2, %0, %arg3 : i1
    arc.output %1 : i1
  }
  arc.define @ncs_core_dmem_ppu_cpu_arb_arc_2(%arg0: i1) -> i1 {
    arc.output %arg0 : i1
  }
  hw.module @ncs_core_dmem_ppu_cpu_arb(in %RESET_N_I : i1, in %CLK_I : i1, in %RD_CPU_REQ_VALID_I : i1, out RD_CPU_REQ_READY_O : i1, in %RD_CPU_REQ_RnW_I : i1, in %RD_CPU_REQ_ADDR_I : i13, in %RD_CPU_REQ_DATA_I : i32, out WR_CPU_DATA_VALID_O : i1, in %WR_CPU_DATA_READY_I : i1, out WR_CPU_DATA_O : i32, in %RD_CELL_REQ_VALID_I : i1, out RD_CELL_REQ_READY_O : i1, in %RD_CELL_REQ_RnW_I : i1, in %RD_CELL_REQ_ADDR_I : i13, in %RD_CELL_REQ_DATA_I : i32, out WR_CELL_DATA_O : i32, out MEM_CEN_O : i1, out MEM_WEN_O : i1, out MEM_ADDR_O : i13, out MEM_WR_DATA_O : i32, in %MEM_RD_DATA_I : i32) {
    %true = hw.constant true
    %false = hw.constant false
    %0:3 = arc.call @ncs_core_dmem_ppu_cpu_arb_arc(%RESET_N_I, %true, %CLK_I, %2, %RD_CPU_REQ_RnW_I) : (i1, i1, i1, i1, i1) -> (i1, !seq.clock, i1)
    %1:5 = arc.call @ncs_core_dmem_ppu_cpu_arb_arc_0(%2, %RD_CPU_REQ_DATA_I, %RD_CELL_REQ_DATA_I, %RD_CPU_REQ_ADDR_I, %RD_CELL_REQ_ADDR_I, %RD_CPU_REQ_RnW_I, %RD_CELL_REQ_RnW_I, %true, %RD_CELL_REQ_VALID_I) : (i1, i32, i32, i13, i13, i1, i1, i1, i1) -> (i32, i13, i1, i1, i1)
    %2 = arc.call @ncs_core_dmem_ppu_cpu_arb_arc_1(%3, %true, %RD_CPU_REQ_VALID_I, %WR_CPU_DATA_READY_I) : (i1, i1, i1, i1) -> i1
    %3 = arc.state @ncs_core_dmem_ppu_cpu_arb_arc_2(%0#2) clock %0#1 reset %0#0 latency 1 {names = ["WR_CPU_DATA_VALID_O"]} : (i1) -> i1
    hw.output %2, %3, %MEM_RD_DATA_I, %1#2, %MEM_RD_DATA_I, %1#3, %1#4, %1#1, %1#0 : i1, i1, i32, i1, i32, i1, i1, i13, i32
  }
}

