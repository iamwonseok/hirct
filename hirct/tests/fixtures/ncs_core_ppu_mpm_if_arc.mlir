module {
  arc.define @ncs_core_ppu_mpm_if_arc(%arg0: i1, %arg1: i1, %arg2: i1, %arg3: i1, %arg4: i1, %arg5: i1) -> (i1, !seq.clock, i1) {
    %0 = comb.xor %arg0, %arg1 : i1
    %1 = seq.to_clock %arg2
    %2 = comb.mux %arg3, %arg4, %arg5 : i1
    arc.output %0, %1, %2 : i1, !seq.clock, i1
  }
  arc.define @ncs_core_ppu_mpm_if_arc_0(%arg0: i1, %arg1: i1, %arg2: i1, %arg3: i32, %arg4: i4, %arg5: i35) -> (i1, i36, i1) {
    %0 = comb.xor %arg0, %arg1 : i1
    %1 = comb.and %arg2, %0 : i1
    %2 = comb.extract %arg3 from 0 : (i32) -> i31
    %3 = comb.concat %arg4, %2 : i4, i31
    %4 = comb.add %arg5, %3 : i35
    %5 = comb.extract %arg3 from 31 : (i32) -> i1
    %6 = comb.concat %5, %4 : i1, i35
    %7 = comb.and %arg2, %arg0 : i1
    arc.output %1, %6, %7 : i1, i36, i1
  }
  arc.define @ncs_core_ppu_mpm_if_arc_1(%arg0: i1, %arg1: i1, %arg2: i1, %arg3: i1, %arg4: i1) -> (i1, i1) {
    %0 = comb.icmp ceq %arg0, %arg1 : i1
    %1 = comb.mux %0, %arg2, %arg3 : i1
    %2 = comb.and %arg4, %1 : i1
    arc.output %0, %2 : i1, i1
  }
  arc.define @ncs_core_ppu_mpm_if_arc_2(%arg0: i1) -> i1 {
    arc.output %arg0 : i1
  }
  hw.module @ncs_core_ppu_mpm_if(in %RESET_N_I : i1, in %CLK_I : i1, in %PPU_WRITE_DW_BASE_I : i35, in %RD_PPU_REQ_VALID_I : i1, out RD_PPU_REQ_READY_O : i1, in %RD_PPU_REQ_I : i32, out WR_PBM_WR_MPM_REQ_VALID_O : i1, in %WR_PBM_WR_MPM_REQ_READY_I : i1, out WR_PBM_WR_MPM_REQ_DW_ADDR_O : i36, out WR_PBM_WR_MPM_DATA_VALID_O : i1, in %WR_PBM_WR_MPM_DATA_READY_I : i1, out WR_PBM_WR_MPM_DATA_O : i32) {
    %true = hw.constant true
    %false = hw.constant false
    %c0_i4 = hw.constant 0 : i4
    %0:3 = arc.call @ncs_core_ppu_mpm_if_arc(%RESET_N_I, %true, %CLK_I, %2#1, %2#0, %3) : (i1, i1, i1, i1, i1, i1) -> (i1, !seq.clock, i1)
    %1:3 = arc.call @ncs_core_ppu_mpm_if_arc_0(%2#0, %true, %2#1, %RD_PPU_REQ_I, %c0_i4, %PPU_WRITE_DW_BASE_I) : (i1, i1, i1, i32, i4, i35) -> (i1, i36, i1)
    %2:2 = arc.call @ncs_core_ppu_mpm_if_arc_1(%3, %false, %WR_PBM_WR_MPM_REQ_READY_I, %WR_PBM_WR_MPM_DATA_READY_I, %RD_PPU_REQ_VALID_I) : (i1, i1, i1, i1, i1) -> (i1, i1)
    %3 = arc.state @ncs_core_ppu_mpm_if_arc_2(%0#2) clock %0#1 reset %0#0 latency 1 {names = ["r_current_state"]} : (i1) -> i1
    hw.output %2#1, %1#2, %1#1, %1#0, %RD_PPU_REQ_I : i1, i1, i36, i1, i32
  }
}

