module {
  hw.module private @DW_apb_uart_bcm00_and(in %a : i8, in %b : i8, out z : i8) {
    %0 = comb.and %a, %b : i8
    hw.output %0 : i8
  }
  hw.module private @DW_apb_uart_bcm00_and_14(in %a : i1, in %b : i1, out z : i1) {
    %0 = comb.and %a, %b : i1
    hw.output %0 : i1
  }
  hw.module private @DW_apb_uart_bcm00_and_15(in %a : i10, in %b : i10, out z : i10) {
    %0 = comb.and %a, %b : i10
    hw.output %0 : i10
  }
  hw.module private @DW_apb_uart_bcm00_and_16(in %a : i16, in %b : i16, out z : i16) {
    %0 = comb.and %a, %b : i16
    hw.output %0 : i16
  }
  hw.module private @DW_apb_uart_bcm00_and_17(in %a : i6, in %b : i6, out z : i6) {
    %0 = comb.and %a, %b : i6
    hw.output %0 : i6
  }
  hw.module private @DW_apb_uart_bcm00_and_18(in %a : i4, in %b : i4, out z : i4) {
    %0 = comb.and %a, %b : i4
    hw.output %0 : i4
  }
  hw.module private @DW_apb_uart_bcm57(in %clk : i1, in %rst_n : i1, in %wr_n : i1, in %data_in : i8, in %wr_addr : i5, in %rd_addr : i5, out data_out : i8) {
    %0 = llhd.constant_time <0ns, 1d, 0e>
    %c0_i27 = hw.constant 0 : i27
    %true = hw.constant true
    %c0_i8 = hw.constant 0 : i8
    %c0_i256 = hw.constant 0 : i256
    %c1_i32 = hw.constant 1 : i32
    %false = hw.constant false
    %c31_i32 = hw.constant 31 : i32
    %c32_i32 = hw.constant 32 : i32
    %c0_i32 = hw.constant 0 : i32
    %c-1_i5 = hw.constant -1 : i5
    %1 = hw.bitcast %c0_i256 : (i256) -> !hw.array<32xi8>
    %mem = llhd.sig %1 : !hw.array<32xi8>
    %2 = llhd.prb %mem : !hw.array<32xi8>
    %3 = comb.sub %c-1_i5, %rd_addr : i5
    %4 = hw.array_get %2[%3] {sv.namehint = "read_data"} : !hw.array<32xi8>, i5
    %5:2 = llhd.process -> !hw.array<32xi8>, i1 {
      cf.br ^bb1(%clk, %rst_n, %1, %false : i1, i1, !hw.array<32xi8>, i1)
    ^bb1(%6: i1, %7: i1, %8: !hw.array<32xi8>, %9: i1):  // 5 preds: ^bb0, ^bb2, ^bb4, ^bb6, ^bb7
      llhd.wait yield (%8, %9 : !hw.array<32xi8>, i1), (%clk, %rst_n : i1, i1), ^bb2(%7, %6 : i1, i1)
    ^bb2(%10: i1, %11: i1):  // pred: ^bb1
      %12 = comb.xor bin %11, %true : i1
      %13 = comb.and bin %12, %clk : i1
      %14 = comb.xor bin %rst_n, %true : i1
      %15 = comb.and bin %10, %14 : i1
      %16 = comb.or bin %13, %15 : i1
      cf.cond_br %16, ^bb3, ^bb1(%clk, %rst_n, %2, %false : i1, i1, !hw.array<32xi8>, i1)
    ^bb3:  // pred: ^bb2
      %17 = comb.xor %rst_n, %true : i1
      cf.cond_br %17, ^bb4(%c0_i32, %2, %false : i32, !hw.array<32xi8>, i1), ^bb6
    ^bb4(%18: i32, %19: !hw.array<32xi8>, %20: i1):  // 2 preds: ^bb3, ^bb5
      %21 = comb.icmp slt %18, %c32_i32 : i32
      cf.cond_br %21, ^bb5, ^bb1(%clk, %rst_n, %19, %20 : i1, i1, !hw.array<32xi8>, i1)
    ^bb5:  // pred: ^bb4
      %22 = comb.sub %c31_i32, %18 : i32
      %23 = comb.extract %22 from 5 : (i32) -> i27
      %24 = comb.icmp eq %23, %c0_i27 : i27
      %25 = comb.extract %22 from 0 : (i32) -> i5
      %26 = comb.mux %24, %25, %c-1_i5 : i5
      %27 = hw.array_inject %19[%26], %c0_i8 : !hw.array<32xi8>, i5
      %28 = comb.add %18, %c1_i32 : i32
      cf.br ^bb4(%28, %27, %true : i32, !hw.array<32xi8>, i1)
    ^bb6:  // pred: ^bb3
      %29 = comb.xor %wr_n, %true : i1
      cf.cond_br %29, ^bb7, ^bb1(%clk, %rst_n, %2, %false : i1, i1, !hw.array<32xi8>, i1)
    ^bb7:  // pred: ^bb6
      %30 = comb.sub %c-1_i5, %wr_addr : i5
      %31 = hw.array_inject %2[%30], %data_in : !hw.array<32xi8>, i5
      cf.br ^bb1(%clk, %rst_n, %31, %true : i1, i1, !hw.array<32xi8>, i1)
    }
    llhd.drv %mem, %5#0 after %0 if %5#1 : !hw.array<32xi8>
    hw.output %4 : i8
  }
  hw.module private @DW_apb_uart_bcm57_0(in %clk : i1, in %rst_n : i1, in %wr_n : i1, in %data_in : i10, in %wr_addr : i5, in %rd_addr : i5, out data_out : i10) {
    %0 = llhd.constant_time <0ns, 1d, 0e>
    %c0_i27 = hw.constant 0 : i27
    %true = hw.constant true
    %c0_i10 = hw.constant 0 : i10
    %c0_i320 = hw.constant 0 : i320
    %c1_i32 = hw.constant 1 : i32
    %false = hw.constant false
    %c31_i32 = hw.constant 31 : i32
    %c32_i32 = hw.constant 32 : i32
    %c0_i32 = hw.constant 0 : i32
    %c-1_i5 = hw.constant -1 : i5
    %1 = hw.bitcast %c0_i320 : (i320) -> !hw.array<32xi10>
    %mem = llhd.sig %1 : !hw.array<32xi10>
    %2 = llhd.prb %mem : !hw.array<32xi10>
    %3 = comb.sub %c-1_i5, %rd_addr : i5
    %4 = hw.array_get %2[%3] {sv.namehint = "read_data"} : !hw.array<32xi10>, i5
    %5:2 = llhd.process -> !hw.array<32xi10>, i1 {
      cf.br ^bb1(%clk, %rst_n, %1, %false : i1, i1, !hw.array<32xi10>, i1)
    ^bb1(%6: i1, %7: i1, %8: !hw.array<32xi10>, %9: i1):  // 5 preds: ^bb0, ^bb2, ^bb4, ^bb6, ^bb7
      llhd.wait yield (%8, %9 : !hw.array<32xi10>, i1), (%clk, %rst_n : i1, i1), ^bb2(%7, %6 : i1, i1)
    ^bb2(%10: i1, %11: i1):  // pred: ^bb1
      %12 = comb.xor bin %11, %true : i1
      %13 = comb.and bin %12, %clk : i1
      %14 = comb.xor bin %rst_n, %true : i1
      %15 = comb.and bin %10, %14 : i1
      %16 = comb.or bin %13, %15 : i1
      cf.cond_br %16, ^bb3, ^bb1(%clk, %rst_n, %2, %false : i1, i1, !hw.array<32xi10>, i1)
    ^bb3:  // pred: ^bb2
      %17 = comb.xor %rst_n, %true : i1
      cf.cond_br %17, ^bb4(%c0_i32, %2, %false : i32, !hw.array<32xi10>, i1), ^bb6
    ^bb4(%18: i32, %19: !hw.array<32xi10>, %20: i1):  // 2 preds: ^bb3, ^bb5
      %21 = comb.icmp slt %18, %c32_i32 : i32
      cf.cond_br %21, ^bb5, ^bb1(%clk, %rst_n, %19, %20 : i1, i1, !hw.array<32xi10>, i1)
    ^bb5:  // pred: ^bb4
      %22 = comb.sub %c31_i32, %18 : i32
      %23 = comb.extract %22 from 5 : (i32) -> i27
      %24 = comb.icmp eq %23, %c0_i27 : i27
      %25 = comb.extract %22 from 0 : (i32) -> i5
      %26 = comb.mux %24, %25, %c-1_i5 : i5
      %27 = hw.array_inject %19[%26], %c0_i10 : !hw.array<32xi10>, i5
      %28 = comb.add %18, %c1_i32 : i32
      cf.br ^bb4(%28, %27, %true : i32, !hw.array<32xi10>, i1)
    ^bb6:  // pred: ^bb3
      %29 = comb.xor %wr_n, %true : i1
      cf.cond_br %29, ^bb7, ^bb1(%clk, %rst_n, %2, %false : i1, i1, !hw.array<32xi10>, i1)
    ^bb7:  // pred: ^bb6
      %30 = comb.sub %c-1_i5, %wr_addr : i5
      %31 = hw.array_inject %2[%30], %data_in : !hw.array<32xi10>, i5
      cf.br ^bb1(%clk, %rst_n, %31, %true : i1, i1, !hw.array<32xi10>, i1)
    }
    llhd.drv %mem, %5#0 after %0 if %5#1 : !hw.array<32xi10>
    hw.output %4 : i10
  }
  hw.module private @DW_apb_uart_bcm25(in %clk_s : i1, in %rst_s_n : i1, in %init_s_n : i1, in %send_s : i1, in %data_s : i8, out empty_s : i1, out full_s : i1, out done_s : i1, in %clk_d : i1, in %rst_d_n : i1, in %init_d_n : i1, out data_avail_d : i1, out data_d : i8) {
    %true = hw.constant true
    %0 = sim.fmt.literal "Information: *** Instance <module> module is using the <Data Bus Synchronizer With Acknowledge (5)> Clock Domain Crossing Method ***\0A"
    %c0_i8 = hw.constant 0 : i8
    %false = hw.constant false
    llhd.process {
      sim.proc.print %0
      llhd.halt
    }
    %U1.ack_s, %U1.busy_s, %U1.event_d = hw.instance "U1" @DW_apb_uart_bcm23(clk_s: %clk_s: i1, rst_s_n: %rst_s_n: i1, init_s_n: %init_s_n: i1, event_s: %send_s: i1, clk_d: %clk_d: i1, rst_d_n: %rst_d_n: i1, init_d_n: %init_d_n: i1) -> (ack_s: i1, busy_s: i1, event_d: i1) {sv.namehint = "ack_s_pas"}
    %1 = comb.xor %init_s_n, %true : i1
    %2 = comb.and %init_s_n, %19 : i1
    %3 = comb.mux %2, %data_s, %c0_i8 : i8
    %4 = comb.or %1, %19 : i1
    %5 = comb.and %init_s_n, %17 : i1
    %6 = comb.and %init_s_n, %22 : i1
    %7 = seq.to_clock %clk_s
    %8 = comb.xor %rst_s_n, %true : i1
    %9 = comb.mux bin %4, %3, %data_s_reg : i8
    %data_s_reg = seq.firreg %9 clock %7 reset async %8, %c0_i8 : i8
    %busy_int = seq.firreg %5 clock %7 reset async %8, %false : i1
    %busy_pnr = seq.firreg %5 clock %7 reset async %8, %false : i1
    %dr_bsy = seq.firreg %6 clock %7 reset async %8, %false : i1
    %U_FORCE_BUS.data_out = hw.instance "U_FORCE_BUS" @DW_apb_uart_bcm03(enable: %U1.event_d: i1, data_in: %data_s_reg: i8) -> (data_out: i8)
    %10 = comb.xor %init_d_n, %true : i1
    %11 = comb.and %init_d_n, %U1.event_d : i1
    %12 = comb.mux %11, %U_FORCE_BUS.data_out, %c0_i8 : i8
    %13 = comb.or %10, %U1.event_d : i1
    %14 = seq.to_clock %clk_d
    %15 = comb.xor %rst_d_n, %true : i1
    %16 = comb.mux bin %13, %12, %data_d_reg : i8
    %data_d_reg = seq.firreg %16 clock %14 reset async %15, %c0_i8 : i8
    %data_avail_reg = seq.firreg %11 clock %14 reset async %15, %false : i1
    %17 = comb.or %send_s, %22 : i1
    %18 = comb.xor %U1.busy_s, %true : i1
    %19 = comb.and %send_s, %18 : i1
    %20 = comb.xor %U1.ack_s, %true : i1
    %21 = comb.and %dr_bsy, %20 : i1
    %22 = comb.or %19, %21 : i1
    hw.output %busy_int, %busy_pnr, %U1.ack_s, %data_avail_reg, %data_d_reg : i1, i1, i1, i1, i8
  }
  hw.module private @DW_apb_uart_bcm25_1(in %clk_s : i1, in %rst_s_n : i1, in %init_s_n : i1, in %send_s : i1, in %data_s : i1, out empty_s : i1, out full_s : i1, out done_s : i1, in %clk_d : i1, in %rst_d_n : i1, in %init_d_n : i1, out data_avail_d : i1, out data_d : i1) {
    %true = hw.constant true
    %0 = sim.fmt.literal "Information: *** Instance <module> module is using the <Data Bus Synchronizer With Acknowledge (5)> Clock Domain Crossing Method ***\0A"
    %false = hw.constant false
    llhd.process {
      sim.proc.print %0
      llhd.halt
    }
    %U1.ack_s, %U1.busy_s, %U1.event_d = hw.instance "U1" @DW_apb_uart_bcm23(clk_s: %clk_s: i1, rst_s_n: %rst_s_n: i1, init_s_n: %init_s_n: i1, event_s: %send_s: i1, clk_d: %clk_d: i1, rst_d_n: %rst_d_n: i1, init_d_n: %init_d_n: i1) -> (ack_s: i1, busy_s: i1, event_d: i1) {sv.namehint = "ack_s_pas"}
    %1 = comb.xor %init_s_n, %true : i1
    %2 = comb.and %init_s_n, %18, %data_s : i1
    %3 = comb.or %1, %18 : i1
    %4 = comb.and %init_s_n, %16 : i1
    %5 = comb.and %init_s_n, %21 : i1
    %6 = seq.to_clock %clk_s
    %7 = comb.xor %rst_s_n, %true : i1
    %8 = comb.mux bin %3, %2, %data_s_reg : i1
    %data_s_reg = seq.firreg %8 clock %6 reset async %7, %false : i1
    %busy_int = seq.firreg %4 clock %6 reset async %7, %false : i1
    %busy_pnr = seq.firreg %4 clock %6 reset async %7, %false : i1
    %dr_bsy = seq.firreg %5 clock %6 reset async %7, %false : i1
    %U_FORCE_BUS.data_out = hw.instance "U_FORCE_BUS" @DW_apb_uart_bcm03_7(enable: %U1.event_d: i1, data_in: %data_s_reg: i1) -> (data_out: i1)
    %9 = comb.xor %init_d_n, %true : i1
    %10 = comb.and %init_d_n, %U1.event_d, %U_FORCE_BUS.data_out : i1
    %11 = comb.or %9, %U1.event_d : i1
    %12 = comb.and %init_d_n, %U1.event_d : i1
    %13 = seq.to_clock %clk_d
    %14 = comb.xor %rst_d_n, %true : i1
    %15 = comb.mux bin %11, %10, %data_d_reg : i1
    %data_d_reg = seq.firreg %15 clock %13 reset async %14, %false : i1
    %data_avail_reg = seq.firreg %12 clock %13 reset async %14, %false : i1
    %16 = comb.or %send_s, %21 : i1
    %17 = comb.xor %U1.busy_s, %true : i1
    %18 = comb.and %send_s, %17 : i1
    %19 = comb.xor %U1.ack_s, %true : i1
    %20 = comb.and %dr_bsy, %19 : i1
    %21 = comb.or %18, %20 : i1
    hw.output %busy_int, %busy_pnr, %U1.ack_s, %data_avail_reg, %data_d_reg : i1, i1, i1, i1, i1
  }
  hw.module private @DW_apb_uart_bcm25_2(in %clk_s : i1, in %rst_s_n : i1, in %init_s_n : i1, in %send_s : i1, in %data_s : i10, out empty_s : i1, out full_s : i1, out done_s : i1, in %clk_d : i1, in %rst_d_n : i1, in %init_d_n : i1, out data_avail_d : i1, out data_d : i10) {
    %true = hw.constant true
    %0 = sim.fmt.literal "Information: *** Instance <module> module is using the <Data Bus Synchronizer With Acknowledge (5)> Clock Domain Crossing Method ***\0A"
    %c0_i10 = hw.constant 0 : i10
    %false = hw.constant false
    llhd.process {
      sim.proc.print %0
      llhd.halt
    }
    %U1.ack_s, %U1.busy_s, %U1.event_d = hw.instance "U1" @DW_apb_uart_bcm23(clk_s: %clk_s: i1, rst_s_n: %rst_s_n: i1, init_s_n: %init_s_n: i1, event_s: %send_s: i1, clk_d: %clk_d: i1, rst_d_n: %rst_d_n: i1, init_d_n: %init_d_n: i1) -> (ack_s: i1, busy_s: i1, event_d: i1) {sv.namehint = "ack_s_pas"}
    %1 = comb.xor %init_s_n, %true : i1
    %2 = comb.and %init_s_n, %19 : i1
    %3 = comb.mux %2, %data_s, %c0_i10 : i10
    %4 = comb.or %1, %19 : i1
    %5 = comb.and %init_s_n, %17 : i1
    %6 = comb.and %init_s_n, %22 : i1
    %7 = seq.to_clock %clk_s
    %8 = comb.xor %rst_s_n, %true : i1
    %9 = comb.mux bin %4, %3, %data_s_reg : i10
    %data_s_reg = seq.firreg %9 clock %7 reset async %8, %c0_i10 : i10
    %busy_int = seq.firreg %5 clock %7 reset async %8, %false : i1
    %busy_pnr = seq.firreg %5 clock %7 reset async %8, %false : i1
    %dr_bsy = seq.firreg %6 clock %7 reset async %8, %false : i1
    %U_FORCE_BUS.data_out = hw.instance "U_FORCE_BUS" @DW_apb_uart_bcm03_8(enable: %U1.event_d: i1, data_in: %data_s_reg: i10) -> (data_out: i10)
    %10 = comb.xor %init_d_n, %true : i1
    %11 = comb.and %init_d_n, %U1.event_d : i1
    %12 = comb.mux %11, %U_FORCE_BUS.data_out, %c0_i10 : i10
    %13 = comb.or %10, %U1.event_d : i1
    %14 = seq.to_clock %clk_d
    %15 = comb.xor %rst_d_n, %true : i1
    %16 = comb.mux bin %13, %12, %data_d_reg : i10
    %data_d_reg = seq.firreg %16 clock %14 reset async %15, %c0_i10 : i10
    %data_avail_reg = seq.firreg %11 clock %14 reset async %15, %false : i1
    %17 = comb.or %send_s, %22 : i1
    %18 = comb.xor %U1.busy_s, %true : i1
    %19 = comb.and %send_s, %18 : i1
    %20 = comb.xor %U1.ack_s, %true : i1
    %21 = comb.and %dr_bsy, %20 : i1
    %22 = comb.or %19, %21 : i1
    hw.output %busy_int, %busy_pnr, %U1.ack_s, %data_avail_reg, %data_d_reg : i1, i1, i1, i1, i10
  }
  hw.module private @DW_apb_uart_bcm25_3(in %clk_s : i1, in %rst_s_n : i1, in %init_s_n : i1, in %send_s : i1, in %data_s : i16, out empty_s : i1, out full_s : i1, out done_s : i1, in %clk_d : i1, in %rst_d_n : i1, in %init_d_n : i1, out data_avail_d : i1, out data_d : i16) {
    %true = hw.constant true
    %0 = sim.fmt.literal "Information: *** Instance <module> module is using the <Data Bus Synchronizer With Acknowledge (5)> Clock Domain Crossing Method ***\0A"
    %false = hw.constant false
    %c0_i16 = hw.constant 0 : i16
    llhd.process {
      sim.proc.print %0
      llhd.halt
    }
    %U1.ack_s, %U1.busy_s, %U1.event_d = hw.instance "U1" @DW_apb_uart_bcm23(clk_s: %clk_s: i1, rst_s_n: %rst_s_n: i1, init_s_n: %init_s_n: i1, event_s: %51: i1, clk_d: %clk_d: i1, rst_d_n: %rst_d_n: i1, init_d_n: %init_d_n: i1) -> (ack_s: i1, busy_s: i1, event_d: i1) {sv.namehint = "ack_s_pas"}
    %1 = comb.xor %init_s_n, %true : i1
    %2 = comb.and %init_s_n, %49 : i1
    %3 = comb.mux %2, %52, %c0_i16 : i16
    %4 = comb.or %1, %49 : i1
    %5 = comb.and %init_s_n, %41 : i1
    %6 = comb.and %init_s_n, %38 : i1
    %7 = comb.and %init_s_n, %55 : i1
    %8 = seq.to_clock %clk_s
    %9 = comb.xor %rst_s_n, %true : i1
    %10 = comb.mux bin %4, %3, %data_s_reg : i16
    %data_s_reg = seq.firreg %10 clock %8 reset async %9, %c0_i16 : i16
    %busy_int = seq.firreg %5 clock %8 reset async %9, %false : i1
    %busy_pnr = seq.firreg %6 clock %8 reset async %9, %false : i1
    %dr_bsy = seq.firreg %7 clock %8 reset async %9, %false : i1
    %U_FORCE_BUS.data_out = hw.instance "U_FORCE_BUS" @DW_apb_uart_bcm03_9(enable: %U1.event_d: i1, data_in: %data_s_reg: i16) -> (data_out: i16)
    %11 = comb.xor %init_d_n, %true : i1
    %12 = comb.and %init_d_n, %U1.event_d : i1
    %13 = comb.mux %12, %U_FORCE_BUS.data_out, %c0_i16 : i16
    %14 = comb.or %11, %U1.event_d : i1
    %15 = seq.to_clock %clk_d
    %16 = comb.xor %rst_d_n, %true : i1
    %17 = comb.mux bin %14, %13, %data_d_reg : i16
    %data_d_reg = seq.firreg %17 clock %15 reset async %16, %c0_i16 : i16
    %data_avail_reg = seq.firreg %12 clock %15 reset async %16, %false : i1
    %18 = comb.and %41, %send_s : i1
    %19 = comb.and %init_s_n, %18 : i1
    %20 = comb.mux %19, %data_s, %c0_i16 : i16
    %21 = comb.or %1, %18 : i1
    %22 = comb.and %init_s_n, %33 : i1
    %23 = comb.mux bin %21, %20, %GEN_PM1.data_s_pnd : i16
    %GEN_PM1.data_s_pnd = seq.firreg %23 clock %8 reset async %9, %c0_i16 : i16
    %GEN_PM1.pr_bsy = seq.firreg %22 clock %8 reset async %9, %false : i1
    %24 = comb.xor %GEN_PM1.pr_bsy, %true : i1
    %25 = comb.and %send_s, %24 : i1
    %26 = comb.and %25, %dr_bsy : i1
    %27 = comb.xor %U1.ack_s, %true : i1
    %28 = comb.and %GEN_PM1.pr_bsy, %27, %dr_bsy : i1
    %29 = comb.and %send_s, %U1.ack_s, %dr_bsy : i1
    %30 = comb.or %26, %28, %29 : i1
    %31 = comb.and %25, %U1.ack_s : i1
    %32 = comb.xor %31, %true : i1
    %33 = comb.and %30, %32 : i1
    %34 = comb.and %dr_bsy, %33, %27 : i1
    %35 = comb.xor %U1.busy_s, %true : i1
    %36 = comb.and %51, %55, %GEN_PM1.pr_bsy, %35 : i1
    %37 = comb.and %send_s, %dr_bsy, %GEN_PM1.pr_bsy, %U1.ack_s : i1
    %38 = comb.or %34, %36, %37 : i1
    %39 = comb.and %27, %busy_int : i1
    %40 = comb.and %U1.ack_s, %GEN_PM1.pr_bsy : i1
    %41 = comb.or %send_s, %51, %39, %40 : i1
    %42 = comb.xor %dr_bsy, %true : i1
    %43 = comb.and %send_s, %42 : i1
    %44 = comb.xor %busy_int, %true : i1
    %45 = comb.and %43, %44 : i1
    %46 = comb.and %42, %GEN_PM1.pr_bsy : i1
    %47 = comb.and %46, %27 : i1
    %48 = comb.and %29, %24 : i1
    %49 = comb.or %45, %40, %47, %48 : i1
    %50 = comb.and %dr_bsy, %35 : i1
    %51 = comb.or %43, %50 {sv.namehint = "send_en"} : i1
    %52 = comb.mux %GEN_PM1.pr_bsy, %GEN_PM1.data_s_pnd, %data_s : i16
    %53 = comb.and %51, %35 : i1
    %54 = comb.and %dr_bsy, %27 : i1
    %55 = comb.or %53, %54, %40, %46, %48 : i1
    hw.output %busy_int, %busy_pnr, %U1.ack_s, %data_avail_reg, %data_d_reg : i1, i1, i1, i1, i16
  }
  hw.module private @DW_apb_uart_bcm25_4(in %clk_s : i1, in %rst_s_n : i1, in %init_s_n : i1, in %send_s : i1, in %data_s : i6, out empty_s : i1, out full_s : i1, out done_s : i1, in %clk_d : i1, in %rst_d_n : i1, in %init_d_n : i1, out data_avail_d : i1, out data_d : i6) {
    %true = hw.constant true
    %0 = sim.fmt.literal "Information: *** Instance <module> module is using the <Data Bus Synchronizer With Acknowledge (5)> Clock Domain Crossing Method ***\0A"
    %false = hw.constant false
    %c0_i6 = hw.constant 0 : i6
    llhd.process {
      sim.proc.print %0
      llhd.halt
    }
    %U1.ack_s, %U1.busy_s, %U1.event_d = hw.instance "U1" @DW_apb_uart_bcm23(clk_s: %clk_s: i1, rst_s_n: %rst_s_n: i1, init_s_n: %init_s_n: i1, event_s: %51: i1, clk_d: %clk_d: i1, rst_d_n: %rst_d_n: i1, init_d_n: %init_d_n: i1) -> (ack_s: i1, busy_s: i1, event_d: i1) {sv.namehint = "ack_s_pas"}
    %1 = comb.xor %init_s_n, %true : i1
    %2 = comb.and %init_s_n, %49 : i1
    %3 = comb.mux %2, %52, %c0_i6 : i6
    %4 = comb.or %1, %49 : i1
    %5 = comb.and %init_s_n, %41 : i1
    %6 = comb.and %init_s_n, %38 : i1
    %7 = comb.and %init_s_n, %55 : i1
    %8 = seq.to_clock %clk_s
    %9 = comb.xor %rst_s_n, %true : i1
    %10 = comb.mux bin %4, %3, %data_s_reg : i6
    %data_s_reg = seq.firreg %10 clock %8 reset async %9, %c0_i6 : i6
    %busy_int = seq.firreg %5 clock %8 reset async %9, %false : i1
    %busy_pnr = seq.firreg %6 clock %8 reset async %9, %false : i1
    %dr_bsy = seq.firreg %7 clock %8 reset async %9, %false : i1
    %U_FORCE_BUS.data_out = hw.instance "U_FORCE_BUS" @DW_apb_uart_bcm03_10(enable: %U1.event_d: i1, data_in: %data_s_reg: i6) -> (data_out: i6)
    %11 = comb.xor %init_d_n, %true : i1
    %12 = comb.and %init_d_n, %U1.event_d : i1
    %13 = comb.mux %12, %U_FORCE_BUS.data_out, %c0_i6 : i6
    %14 = comb.or %11, %U1.event_d : i1
    %15 = seq.to_clock %clk_d
    %16 = comb.xor %rst_d_n, %true : i1
    %17 = comb.mux bin %14, %13, %data_d_reg : i6
    %data_d_reg = seq.firreg %17 clock %15 reset async %16, %c0_i6 : i6
    %data_avail_reg = seq.firreg %12 clock %15 reset async %16, %false : i1
    %18 = comb.and %41, %send_s : i1
    %19 = comb.and %init_s_n, %18 : i1
    %20 = comb.mux %19, %data_s, %c0_i6 : i6
    %21 = comb.or %1, %18 : i1
    %22 = comb.and %init_s_n, %33 : i1
    %23 = comb.mux bin %21, %20, %GEN_PM1.data_s_pnd : i6
    %GEN_PM1.data_s_pnd = seq.firreg %23 clock %8 reset async %9, %c0_i6 : i6
    %GEN_PM1.pr_bsy = seq.firreg %22 clock %8 reset async %9, %false : i1
    %24 = comb.xor %GEN_PM1.pr_bsy, %true : i1
    %25 = comb.and %send_s, %24 : i1
    %26 = comb.and %25, %dr_bsy : i1
    %27 = comb.xor %U1.ack_s, %true : i1
    %28 = comb.and %GEN_PM1.pr_bsy, %27, %dr_bsy : i1
    %29 = comb.and %send_s, %U1.ack_s, %dr_bsy : i1
    %30 = comb.or %26, %28, %29 : i1
    %31 = comb.and %25, %U1.ack_s : i1
    %32 = comb.xor %31, %true : i1
    %33 = comb.and %30, %32 : i1
    %34 = comb.and %dr_bsy, %33, %27 : i1
    %35 = comb.xor %U1.busy_s, %true : i1
    %36 = comb.and %51, %55, %GEN_PM1.pr_bsy, %35 : i1
    %37 = comb.and %send_s, %dr_bsy, %GEN_PM1.pr_bsy, %U1.ack_s : i1
    %38 = comb.or %34, %36, %37 : i1
    %39 = comb.and %27, %busy_int : i1
    %40 = comb.and %U1.ack_s, %GEN_PM1.pr_bsy : i1
    %41 = comb.or %send_s, %51, %39, %40 : i1
    %42 = comb.xor %dr_bsy, %true : i1
    %43 = comb.and %send_s, %42 : i1
    %44 = comb.xor %busy_int, %true : i1
    %45 = comb.and %43, %44 : i1
    %46 = comb.and %42, %GEN_PM1.pr_bsy : i1
    %47 = comb.and %46, %27 : i1
    %48 = comb.and %29, %24 : i1
    %49 = comb.or %45, %40, %47, %48 : i1
    %50 = comb.and %dr_bsy, %35 : i1
    %51 = comb.or %43, %50 {sv.namehint = "send_en"} : i1
    %52 = comb.mux %GEN_PM1.pr_bsy, %GEN_PM1.data_s_pnd, %data_s : i6
    %53 = comb.and %51, %35 : i1
    %54 = comb.and %dr_bsy, %27 : i1
    %55 = comb.or %53, %54, %40, %46, %48 : i1
    hw.output %busy_int, %busy_pnr, %U1.ack_s, %data_avail_reg, %data_d_reg : i1, i1, i1, i1, i6
  }
  hw.module private @DW_apb_uart_bcm25_5(in %clk_s : i1, in %rst_s_n : i1, in %init_s_n : i1, in %send_s : i1, in %data_s : i1, out empty_s : i1, out full_s : i1, out done_s : i1, in %clk_d : i1, in %rst_d_n : i1, in %init_d_n : i1, out data_avail_d : i1, out data_d : i1) {
    %true = hw.constant true
    %0 = sim.fmt.literal "Information: *** Instance <module> module is using the <Data Bus Synchronizer With Acknowledge (5)> Clock Domain Crossing Method ***\0A"
    %false = hw.constant false
    llhd.process {
      sim.proc.print %0
      llhd.halt
    }
    %U1.ack_s, %U1.busy_s, %U1.event_d = hw.instance "U1" @DW_apb_uart_bcm23(clk_s: %clk_s: i1, rst_s_n: %rst_s_n: i1, init_s_n: %init_s_n: i1, event_s: %49: i1, clk_d: %clk_d: i1, rst_d_n: %rst_d_n: i1, init_d_n: %init_d_n: i1) -> (ack_s: i1, busy_s: i1, event_d: i1) {sv.namehint = "ack_s_pas"}
    %1 = comb.xor %init_s_n, %true : i1
    %2 = comb.and %init_s_n, %47, %50 : i1
    %3 = comb.or %1, %47 : i1
    %4 = comb.and %init_s_n, %39 : i1
    %5 = comb.and %init_s_n, %36 : i1
    %6 = comb.and %init_s_n, %53 : i1
    %7 = seq.to_clock %clk_s
    %8 = comb.xor %rst_s_n, %true : i1
    %9 = comb.mux bin %3, %2, %data_s_reg : i1
    %data_s_reg = seq.firreg %9 clock %7 reset async %8, %false : i1
    %busy_int = seq.firreg %4 clock %7 reset async %8, %false : i1
    %busy_pnr = seq.firreg %5 clock %7 reset async %8, %false : i1
    %dr_bsy = seq.firreg %6 clock %7 reset async %8, %false : i1
    %U_FORCE_BUS.data_out = hw.instance "U_FORCE_BUS" @DW_apb_uart_bcm03_7(enable: %U1.event_d: i1, data_in: %data_s_reg: i1) -> (data_out: i1)
    %10 = comb.xor %init_d_n, %true : i1
    %11 = comb.and %init_d_n, %U1.event_d, %U_FORCE_BUS.data_out : i1
    %12 = comb.or %10, %U1.event_d : i1
    %13 = comb.and %init_d_n, %U1.event_d : i1
    %14 = seq.to_clock %clk_d
    %15 = comb.xor %rst_d_n, %true : i1
    %16 = comb.mux bin %12, %11, %data_d_reg : i1
    %data_d_reg = seq.firreg %16 clock %14 reset async %15, %false : i1
    %data_avail_reg = seq.firreg %13 clock %14 reset async %15, %false : i1
    %17 = comb.and %39, %send_s : i1
    %18 = comb.and %init_s_n, %17, %data_s : i1
    %19 = comb.or %1, %17 : i1
    %20 = comb.and %init_s_n, %31 : i1
    %21 = comb.mux bin %19, %18, %GEN_PM1.data_s_pnd : i1
    %GEN_PM1.data_s_pnd = seq.firreg %21 clock %7 reset async %8, %false : i1
    %GEN_PM1.pr_bsy = seq.firreg %20 clock %7 reset async %8, %false : i1
    %22 = comb.xor %GEN_PM1.pr_bsy, %true : i1
    %23 = comb.and %send_s, %22 : i1
    %24 = comb.and %23, %dr_bsy : i1
    %25 = comb.xor %U1.ack_s, %true : i1
    %26 = comb.and %GEN_PM1.pr_bsy, %25, %dr_bsy : i1
    %27 = comb.and %send_s, %U1.ack_s, %dr_bsy : i1
    %28 = comb.or %24, %26, %27 : i1
    %29 = comb.and %23, %U1.ack_s : i1
    %30 = comb.xor %29, %true : i1
    %31 = comb.and %28, %30 : i1
    %32 = comb.and %dr_bsy, %31, %25 : i1
    %33 = comb.xor %U1.busy_s, %true : i1
    %34 = comb.and %49, %53, %GEN_PM1.pr_bsy, %33 : i1
    %35 = comb.and %send_s, %dr_bsy, %GEN_PM1.pr_bsy, %U1.ack_s : i1
    %36 = comb.or %32, %34, %35 : i1
    %37 = comb.and %25, %busy_int : i1
    %38 = comb.and %U1.ack_s, %GEN_PM1.pr_bsy : i1
    %39 = comb.or %send_s, %49, %37, %38 : i1
    %40 = comb.xor %dr_bsy, %true : i1
    %41 = comb.and %send_s, %40 : i1
    %42 = comb.xor %busy_int, %true : i1
    %43 = comb.and %41, %42 : i1
    %44 = comb.and %40, %GEN_PM1.pr_bsy : i1
    %45 = comb.and %44, %25 : i1
    %46 = comb.and %27, %22 : i1
    %47 = comb.or %43, %38, %45, %46 : i1
    %48 = comb.and %dr_bsy, %33 : i1
    %49 = comb.or %41, %48 {sv.namehint = "send_en"} : i1
    %50 = comb.mux %GEN_PM1.pr_bsy, %GEN_PM1.data_s_pnd, %data_s : i1
    %51 = comb.and %49, %33 : i1
    %52 = comb.and %dr_bsy, %25 : i1
    %53 = comb.or %51, %52, %38, %44, %46 : i1
    hw.output %busy_int, %busy_pnr, %U1.ack_s, %data_avail_reg, %data_d_reg : i1, i1, i1, i1, i1
  }
  hw.module private @DW_apb_uart_bcm25_6(in %clk_s : i1, in %rst_s_n : i1, in %init_s_n : i1, in %send_s : i1, in %data_s : i4, out empty_s : i1, out full_s : i1, out done_s : i1, in %clk_d : i1, in %rst_d_n : i1, in %init_d_n : i1, out data_avail_d : i1, out data_d : i4) {
    %true = hw.constant true
    %0 = sim.fmt.literal "Information: *** Instance <module> module is using the <Data Bus Synchronizer With Acknowledge (5)> Clock Domain Crossing Method ***\0A"
    %c0_i4 = hw.constant 0 : i4
    %false = hw.constant false
    llhd.process {
      sim.proc.print %0
      llhd.halt
    }
    %U1.ack_s, %U1.busy_s, %U1.event_d = hw.instance "U1" @DW_apb_uart_bcm23(clk_s: %clk_s: i1, rst_s_n: %rst_s_n: i1, init_s_n: %init_s_n: i1, event_s: %send_s: i1, clk_d: %clk_d: i1, rst_d_n: %rst_d_n: i1, init_d_n: %init_d_n: i1) -> (ack_s: i1, busy_s: i1, event_d: i1) {sv.namehint = "ack_s_pas"}
    %1 = comb.xor %init_s_n, %true : i1
    %2 = comb.and %init_s_n, %19 : i1
    %3 = comb.mux %2, %data_s, %c0_i4 : i4
    %4 = comb.or %1, %19 : i1
    %5 = comb.and %init_s_n, %17 : i1
    %6 = comb.and %init_s_n, %22 : i1
    %7 = seq.to_clock %clk_s
    %8 = comb.xor %rst_s_n, %true : i1
    %9 = comb.mux bin %4, %3, %data_s_reg : i4
    %data_s_reg = seq.firreg %9 clock %7 reset async %8, %c0_i4 : i4
    %busy_int = seq.firreg %5 clock %7 reset async %8, %false : i1
    %busy_pnr = seq.firreg %5 clock %7 reset async %8, %false : i1
    %dr_bsy = seq.firreg %6 clock %7 reset async %8, %false : i1
    %U_FORCE_BUS.data_out = hw.instance "U_FORCE_BUS" @DW_apb_uart_bcm03_11(enable: %U1.event_d: i1, data_in: %data_s_reg: i4) -> (data_out: i4)
    %10 = comb.xor %init_d_n, %true : i1
    %11 = comb.and %init_d_n, %U1.event_d : i1
    %12 = comb.mux %11, %U_FORCE_BUS.data_out, %c0_i4 : i4
    %13 = comb.or %10, %U1.event_d : i1
    %14 = seq.to_clock %clk_d
    %15 = comb.xor %rst_d_n, %true : i1
    %16 = comb.mux bin %13, %12, %data_d_reg : i4
    %data_d_reg = seq.firreg %16 clock %14 reset async %15, %c0_i4 : i4
    %data_avail_reg = seq.firreg %11 clock %14 reset async %15, %false : i1
    %17 = comb.or %send_s, %22 : i1
    %18 = comb.xor %U1.busy_s, %true : i1
    %19 = comb.and %send_s, %18 : i1
    %20 = comb.xor %U1.ack_s, %true : i1
    %21 = comb.and %dr_bsy, %20 : i1
    %22 = comb.or %19, %21 : i1
    hw.output %busy_int, %busy_pnr, %U1.ack_s, %data_avail_reg, %data_d_reg : i1, i1, i1, i1, i4
  }
  hw.module private @DW_apb_uart_rst(in %presetn : i1, in %s_rst_n : i1, in %scan_mode : i1, out new_presetn : i1, out new_s_rst_n : i1) {
    hw.output %presetn, %s_rst_n : i1, i1
  }
  hw.module private @DW_apb_uart_bcm41(in %clk_d : i1, in %rst_d_n : i1, in %data_s : i1, out data_d : i1) {
    %true = hw.constant true
    %0 = comb.xor %data_s, %true {sv.namehint = "data_s_int"} : i1
    %U_SYNC.data_d = hw.instance "U_SYNC" @DW_apb_uart_bcm21_12(clk_d: %clk_d: i1, rst_d_n: %rst_d_n: i1, data_s: %0: i1) -> (data_d: i1) {sv.namehint = "data_d_int"}
    %1 = comb.xor %U_SYNC.data_d, %true : i1
    hw.output %1 : i1
  }
  hw.module private @DW_apb_uart_bcm21(in %clk_d : i1, in %rst_d_n : i1, in %data_s : i1, out data_d : i1) {
    %true = hw.constant true
    %0 = sim.fmt.literal "Information: *** Instance <module> module is using the <Double Register Synchronizer (1)> Clock Domain Crossing Method ***\0A"
    %false = hw.constant false
    llhd.process {
      sim.proc.print %0
      llhd.halt
    }
    %1 = seq.to_clock %clk_d
    %2 = comb.xor %rst_d_n, %true : i1
    %GEN_FST2.sample_meta = seq.firreg %data_s clock %1 reset async %2, %false : i1
    %GEN_FST2.sample_syncl = seq.firreg %GEN_FST2.sample_meta clock %1 reset async %2, %false : i1
    hw.output %GEN_FST2.sample_syncl : i1
  }
  hw.module private @DW_apb_uart_bcm21_12(in %clk_d : i1, in %rst_d_n : i1, in %data_s : i1, out data_d : i1) {
    %true = hw.constant true
    %false = hw.constant false
    %0 = seq.to_clock %clk_d
    %1 = comb.xor %rst_d_n, %true : i1
    %GEN_FST2.sample_meta = seq.firreg %data_s clock %0 reset async %1, %false : i1
    %GEN_FST2.sample_syncl = seq.firreg %GEN_FST2.sample_meta clock %0 reset async %1, %false : i1
    hw.output %GEN_FST2.sample_syncl : i1
  }
  hw.module private @DW_apb_uart_bcm21_13(in %clk_d : i1, in %rst_d_n : i1, in %data_s : i1, out data_d : i1) {
    %true = hw.constant true
    %false = hw.constant false
    %0 = seq.to_clock %clk_d
    %1 = comb.xor %rst_d_n, %true : i1
    %GEN_FST2.sample_meta = seq.firreg %data_s clock %0 reset async %1, %false : i1
    %GEN_FST2.sample_syncl = seq.firreg %GEN_FST2.sample_meta clock %0 reset async %1, %false : i1
    hw.output %GEN_FST2.sample_syncl : i1
  }
  hw.module private @DW_apb_uart_bcm03(in %enable : i1, in %data_in : i8, out data_out : i8) {
    %0 = comb.replicate %enable {sv.namehint = "enable_int"} : (i1) -> i8
    %GEN_FV0.U_AND.z = hw.instance "GEN_FV0.U_AND" @DW_apb_uart_bcm00_and(a: %data_in: i8, b: %0: i8) -> (z: i8)
    hw.output %GEN_FV0.U_AND.z : i8
  }
  hw.module private @DW_apb_uart_bcm03_7(in %enable : i1, in %data_in : i1, out data_out : i1) {
    %GEN_FV0.U_AND.z = hw.instance "GEN_FV0.U_AND" @DW_apb_uart_bcm00_and_14(a: %data_in: i1, b: %enable: i1) -> (z: i1)
    hw.output %GEN_FV0.U_AND.z : i1
  }
  hw.module private @DW_apb_uart_bcm03_8(in %enable : i1, in %data_in : i10, out data_out : i10) {
    %0 = comb.replicate %enable {sv.namehint = "enable_int"} : (i1) -> i10
    %GEN_FV0.U_AND.z = hw.instance "GEN_FV0.U_AND" @DW_apb_uart_bcm00_and_15(a: %data_in: i10, b: %0: i10) -> (z: i10)
    hw.output %GEN_FV0.U_AND.z : i10
  }
  hw.module private @DW_apb_uart_bcm03_9(in %enable : i1, in %data_in : i16, out data_out : i16) {
    %0 = comb.replicate %enable {sv.namehint = "enable_int"} : (i1) -> i16
    %GEN_FV0.U_AND.z = hw.instance "GEN_FV0.U_AND" @DW_apb_uart_bcm00_and_16(a: %data_in: i16, b: %0: i16) -> (z: i16)
    hw.output %GEN_FV0.U_AND.z : i16
  }
  hw.module private @DW_apb_uart_bcm03_10(in %enable : i1, in %data_in : i6, out data_out : i6) {
    %0 = comb.replicate %enable {sv.namehint = "enable_int"} : (i1) -> i6
    %GEN_FV0.U_AND.z = hw.instance "GEN_FV0.U_AND" @DW_apb_uart_bcm00_and_17(a: %data_in: i6, b: %0: i6) -> (z: i6)
    hw.output %GEN_FV0.U_AND.z : i6
  }
  hw.module private @DW_apb_uart_bcm03_11(in %enable : i1, in %data_in : i4, out data_out : i4) {
    %0 = comb.replicate %enable {sv.namehint = "enable_int"} : (i1) -> i4
    %GEN_FV0.U_AND.z = hw.instance "GEN_FV0.U_AND" @DW_apb_uart_bcm00_and_18(a: %data_in: i4, b: %0: i4) -> (z: i4)
    hw.output %GEN_FV0.U_AND.z : i4
  }
  hw.module private @DW_apb_uart_biu(in %pclk : i1, in %presetn : i1, in %psel : i1, in %penable : i1, in %pwrite : i1, in %paddr : i8, in %pwdata : i32, out prdata : i32, out wr_en : i1, out wr_enx : i1, out rd_en : i1, out byte_en : i4, out reg_addr : i6, out ipwdata : i32, in %iprdata : i32) {
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %true = hw.constant true
    %c0_i32 = hw.constant 0 : i32
    %false = hw.constant false
    %c-1_i4 = hw.constant -1 : i4
    %ipwdata = llhd.sig %c0_i32 : i32
    %1 = comb.and %psel, %penable, %pwrite : i1
    %2 = comb.xor %penable, %true : i1
    %3 = comb.and %psel, %2 : i1
    %4 = comb.xor %pwrite, %true : i1
    %5 = comb.and %3, %4 : i1
    %6 = comb.and %3, %pwrite : i1
    %7 = comb.extract %paddr from 2 : (i8) -> i6
    %8:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%13: i32, %14: i1):  // 2 preds: ^bb0, ^bb2
      llhd.wait yield (%13, %14 : i32, i1), (%pwdata : i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb1(%pwdata, %true : i32, i1)
    }
    llhd.drv %ipwdata, %8#0 after %0 if %8#1 : i32
    %9 = seq.to_clock %pclk
    %10 = comb.xor %presetn, %true : i1
    %11 = comb.mux bin %5, %iprdata, %prdata : i32
    %prdata = seq.firreg %11 clock %9 reset async %10, %c0_i32 : i32
    %12 = llhd.prb %ipwdata : i32
    hw.output %prdata, %1, %6, %5, %c-1_i4, %7, %12 : i32, i1, i1, i1, i4, i6, i32
  }
  hw.module private @DW_apb_uart_bcm23(in %clk_s : i1, in %rst_s_n : i1, in %init_s_n : i1, in %event_s : i1, out ack_s : i1, out busy_s : i1, in %clk_d : i1, in %rst_d_n : i1, in %init_d_n : i1, out event_d : i1) {
    %true = hw.constant true
    %false = hw.constant false
    %0 = comb.and %init_s_n, %10 : i1
    %1 = comb.and %init_s_n, %13 : i1
    %2 = comb.and %init_s_n, %U_DW_SYNC_R.data_d : i1
    %3 = seq.to_clock %clk_s
    %4 = comb.xor %rst_s_n, %true : i1
    %tgl_s_event_q = seq.firreg %0 clock %3 reset async %4, %false : i1
    %tgl_s_evnt_nfb_cdc = seq.firreg %0 clock %3 reset async %4, %false : i1
    %busy_state = seq.firreg %1 clock %3 reset async %4, %false : i1
    %tgl_s_ack_q = seq.firreg %2 clock %3 reset async %4, %false : i1
    %U_DW_SYNC_F.data_d = hw.instance "U_DW_SYNC_F" @DW_apb_uart_bcm21_13(clk_d: %clk_d: i1, rst_d_n: %rst_d_n: i1, data_s: %tgl_s_evnt_nfb_cdc: i1) -> (data_d: i1)
    %U_DW_SYNC_R.data_d = hw.instance "U_DW_SYNC_R" @DW_apb_uart_bcm21_13(clk_d: %clk_s: i1, rst_d_n: %rst_s_n: i1, data_s: %GEN_AKDLY1.tgl_d_event_nfb_cdc: i1) -> (data_d: i1)
    %5 = comb.and %init_d_n, %U_DW_SYNC_F.data_d : i1
    %6 = seq.to_clock %clk_d
    %7 = comb.xor %rst_d_n, %true : i1
    %tgl_d_event_q = seq.firreg %5 clock %6 reset async %7, %false : i1
    %8 = comb.xor %busy_state, %true : i1
    %9 = comb.and %event_s, %8 : i1
    %10 = comb.xor %tgl_s_event_q, %9 : i1
    %11 = comb.xor %U_DW_SYNC_F.data_d, %tgl_d_event_q {sv.namehint = "tgl_d_event_dx"} : i1
    %12 = comb.xor %U_DW_SYNC_R.data_d, %tgl_s_ack_q {sv.namehint = "tgl_s_ack_x"} : i1
    %13 = comb.xor %10, %U_DW_SYNC_R.data_d : i1
    %GEN_AKDLY1.tgl_d_event_nfb_cdc = seq.firreg %5 clock %6 reset async %7, %false : i1
    hw.output %12, %busy_state, %11 : i1, i1, i1
  }
  hw.module private @DW_apb_uart_mc_sync(in %clk : i1, in %resetn : i1, in %cts_n : i1, in %dsr_n : i1, in %dcd_n : i1, in %ri_n : i1, out sync_cts_n : i1, out sync_dsr_n : i1, out sync_dcd_n : i1, out sync_ri_n : i1) {
    %U_DW_apb_uart_bcm21_async2ckl_cts_n_cksyzr.data_d = hw.instance "U_DW_apb_uart_bcm21_async2ckl_cts_n_cksyzr" @DW_apb_uart_bcm21(clk_d: %clk: i1, rst_d_n: %resetn: i1, data_s: %cts_n: i1) -> (data_d: i1) {sv.namehint = "sasync2ckl_sync_cts_n"}
    %U_DW_apb_uart_bcm21_async2ckl_dsr_n_cksyzr.data_d = hw.instance "U_DW_apb_uart_bcm21_async2ckl_dsr_n_cksyzr" @DW_apb_uart_bcm21(clk_d: %clk: i1, rst_d_n: %resetn: i1, data_s: %dsr_n: i1) -> (data_d: i1) {sv.namehint = "sasync2ckl_sync_dsr_n"}
    %U_DW_apb_uart_bcm21_async2ckl_dcd_n_cksyzr.data_d = hw.instance "U_DW_apb_uart_bcm21_async2ckl_dcd_n_cksyzr" @DW_apb_uart_bcm21(clk_d: %clk: i1, rst_d_n: %resetn: i1, data_s: %dcd_n: i1) -> (data_d: i1) {sv.namehint = "sasync2ckl_sync_dcd_n"}
    %U_DW_apb_uart_bcm21_async2ckl_ri_n_cksyzr.data_d = hw.instance "U_DW_apb_uart_bcm21_async2ckl_ri_n_cksyzr" @DW_apb_uart_bcm21(clk_d: %clk: i1, rst_d_n: %resetn: i1, data_s: %ri_n: i1) -> (data_d: i1) {sv.namehint = "sasync2ckl_sync_ri_n"}
    hw.output %U_DW_apb_uart_bcm21_async2ckl_cts_n_cksyzr.data_d, %U_DW_apb_uart_bcm21_async2ckl_dsr_n_cksyzr.data_d, %U_DW_apb_uart_bcm21_async2ckl_dcd_n_cksyzr.data_d, %U_DW_apb_uart_bcm21_async2ckl_ri_n_cksyzr.data_d : i1, i1, i1, i1
  }
  hw.module private @DW_apb_uart_bclk_gen(in %sclk : i1, in %s_rst_n : i1, in %divisor : i16, in %divisor_wd : i1, in %uart_lp_req : i1, in %dlf : i4, in %dlf_wd : i1, out bclk : i1) {
    %true = hw.constant true
    %c0_i2 = hw.constant 0 : i2
    %c-1_i16 = hw.constant -1 : i16
    %c1_i16 = hw.constant 1 : i16
    %c-1_i4 = hw.constant -1 : i4
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %c0_i3 = hw.constant 0 : i3
    %c0_i4 = hw.constant 0 : i4
    %c0_i16 = hw.constant 0 : i16
    %c2_i16 = hw.constant 2 : i16
    %false = hw.constant false
    %next_bclk = llhd.sig %false : i1
    %1 = comb.add %divisor, %c1_i16 : i16
    %2 = comb.mux %56, %1, %divisor : i16
    %3 = seq.to_clock %sclk
    %4 = comb.xor %s_rst_n, %true : i1
    %divisor_int = seq.firreg %2 clock %3 reset async %4, %c0_i16 : i16
    %5 = comb.icmp ne %divisor_int, %c0_i16 : i16
    %6 = comb.add %divisor_int, %c-1_i16 : i16
    %7 = comb.add %cnt, %c1_i16 : i16
    %8 = comb.xor %15, %true : i1
    %9 = comb.icmp ne %cnt, %6 : i16
    %10 = comb.and %9, %5, %8 : i1
    %11 = comb.mux %10, %7, %c0_i16 : i16
    %12 = comb.or %15, %5 : i1
    %13 = comb.mux bin %12, %11, %cnt : i16
    %cnt = seq.firreg %13 clock %3 reset async %4, %c0_i16 : i16
    %14 = comb.icmp eq %divisor_int, %c0_i16 : i16
    %15 = comb.or %14, %17, %dlf_wd : i1
    %16 = comb.xor %dly_divisor_wd, %true : i1
    %17 = comb.and %divisor_wd, %16 {sv.namehint = "divisor_wd_ed"} : i1
    %dly_divisor_wd = seq.firreg %divisor_wd clock %3 reset async %4, %false : i1
    %bclk = seq.firreg %57 clock %3 reset async %4, %false : i1
    %18:2 = llhd.process -> i1, i1 {
      cf.br ^bb1(%false, %false : i1, i1)
    ^bb1(%58: i1, %59: i1):  // 6 preds: ^bb0, ^bb2, ^bb4, ^bb5, ^bb6, ^bb6
      llhd.wait yield (%58, %59 : i1, i1), (%uart_lp_req, %15, %divisor_int, %bclk, %cnt : i1, i1, i16, i1, i16), ^bb2
    ^bb2:  // pred: ^bb1
      cf.cond_br %15, ^bb1(%false, %true : i1, i1), ^bb3
    ^bb3:  // pred: ^bb2
      %60 = comb.icmp eq %divisor_int, %c2_i16 : i16
      cf.cond_br %60, ^bb4, ^bb6
    ^bb4:  // pred: ^bb3
      cf.cond_br %uart_lp_req, ^bb1(%bclk, %true : i1, i1), ^bb5
    ^bb5:  // pred: ^bb4
      %61 = comb.xor %bclk, %true : i1
      cf.br ^bb1(%61, %true : i1, i1)
    ^bb6:  // pred: ^bb3
      %62 = comb.icmp eq %cnt, %c0_i16 : i16
      cf.cond_br %62, ^bb1(%true, %true : i1, i1), ^bb1(%false, %true : i1, i1)
    }
    llhd.drv %next_bclk, %18#0 after %0 if %18#1 : i1
    %19 = comb.icmp eq %dlf, %c0_i4 : i4
    %20 = comb.icmp eq %dlf, %c-1_i4 : i4
    %21 = comb.or %19, %15 : i1
    %22 = comb.add %f_ctr, %c-1_i4 : i4
    %23 = comb.mux %21, %c0_i4, %c-1_i4 : i4
    %24 = comb.xor %21, %true : i1
    %25 = comb.icmp ne %f_ctr, %c0_i4 : i4
    %26 = comb.and %25, %57, %24 : i1
    %27 = comb.mux %26, %22, %23 : i4
    %28 = comb.or %21, %57 : i1
    %29 = comb.mux bin %28, %27, %f_ctr : i4
    %f_ctr = seq.firreg %29 clock %3 reset async %4, %c0_i4 : i4
    %30 = comb.extract %f_ctr from 0 : (i4) -> i3
    %31 = comb.icmp eq %30, %c0_i3 {sv.namehint = "fc_low_zero"} : i3
    %32 = comb.extract %f_ctr from 3 : (i4) -> i1
    %33 = comb.xor %20, %true : i1
    %34 = comb.and %32, %33 : i1
    %35 = comb.extract %dlf from 1 : (i4) -> i3
    %36 = comb.extract %dlf from 0 : (i4) -> i1
    %37 = comb.concat %c0_i2, %36 : i2, i1
    %38 = comb.add %35, %37 : i3
    %39 = comb.xor %34, %true : i1
    %40 = comb.mux %39, %35, %38 : i3
    %41 = comb.icmp eq %40, %30 {sv.namehint = "fc_low_comp"} : i3
    %42 = comb.or %57, %15 : i1
    %43 = comb.and %42, %52 : i1
    %44 = comb.xor %43, %true : i1
    %45 = comb.or %43, %53 : i1
    %46 = comb.and %42, %44, %53 : i1
    %47 = comb.and %42, %45 : i1
    %48 = comb.mux bin %47, %46, %div_switch_flag : i1
    %div_switch_flag = seq.firreg %48 clock %3 reset async %4, %false : i1
    %49 = comb.and %32, %20 : i1
    %50 = comb.xor %49, %true : i1
    %51 = comb.and %50, %31, %57 : i1
    %52 = comb.or %51, %15 : i1
    %53 = comb.and %41, %57 : i1
    %54 = comb.or %div_switch_flag, %53 : i1
    %55 = comb.xor %52, %true : i1
    %56 = comb.and %54, %55 : i1
    %57 = llhd.prb %next_bclk : i1
    hw.output %bclk : i1
  }
  hw.module private @DW_apb_uart_bcm06(in %clk : i1, in %rst_n : i1, in %init_n : i1, in %push_req_n : i1, in %pop_req_n : i1, in %ae_level : i5, in %af_thresh : i5, out we_n : i1, out empty : i1, out almost_empty : i1, out half_full : i1, out almost_full : i1, out full : i1, out error : i1, out wr_addr : i5, out rd_addr : i5, out wrd_count : i5, out nxt_empty_n : i1, out nxt_full : i1, out nxt_error : i1) {
    %true = hw.constant true
    %c-1_i6 = hw.constant -1 : i6
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %c0_i6 = hw.constant 0 : i6
    %c0_i5 = hw.constant 0 : i5
    %c1_i6 = hw.constant 1 : i6
    %false = hw.constant false
    %c-1_i5 = hw.constant -1 : i5
    %advanced_word_count = llhd.sig %c0_i6 : i6
    %1 = comb.and %full_int, %pop_req_n : i1
    %2 = comb.or %push_req_n, %1 : i1
    %3 = comb.xor %2, %true {sv.namehint = "advance_wr_addr"} : i1
    %4 = comb.xor %pop_req_n, %true : i1
    %5 = comb.and %4, %empty_n {sv.namehint = "advance_rd_addr"} : i1
    %6 = comb.concat %wr_addr_int, %3 : i5, i1
    %7 = comb.add %6, %c1_i6 : i6
    %8 = comb.and %wr_addr_at_max, %3 : i1
    %9 = comb.extract %7 from 1 : (i6) -> i5
    %10 = comb.mux %8, %c0_i5, %9 : i5
    %11 = comb.concat %rd_addr_int, %5 : i5, i1
    %12 = comb.add %11, %c1_i6 : i6
    %13 = comb.icmp eq %45, %c-1_i5 : i5
    %14 = comb.icmp eq %10, %c-1_i5 : i5
    %15 = comb.xor %push_req_n, %true : i1
    %16 = comb.and %15, %pop_req_n : i1
    %17 = comb.xor %full_int, %true : i1
    %18 = comb.and %16, %17 : i1
    %19 = comb.xor %empty_n, %true : i1
    %20 = comb.and %15, %19 : i1
    %21 = comb.and %push_req_n, %4, %empty_n : i1
    %22:2 = llhd.process -> i6, i1 {
      cf.br ^bb1(%c0_i6, %false : i6, i1)
    ^bb1(%67: i6, %68: i1):  // 3 preds: ^bb0, ^bb3, ^bb4
      llhd.wait yield (%67, %68 : i6, i1), (%word_count, %21 : i5, i1), ^bb2
    ^bb2:  // pred: ^bb1
      cf.cond_br %21, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      %69 = comb.concat %false, %word_count : i1, i5
      %70 = comb.add %69, %c-1_i6 : i6
      cf.br ^bb1(%70, %true : i6, i1)
    ^bb4:  // pred: ^bb2
      %71 = comb.concat %false, %word_count : i1, i5
      %72 = comb.add %71, %c1_i6 : i6
      cf.br ^bb1(%72, %true : i6, i1)
    }
    llhd.drv %advanced_word_count, %22#0 after %0 if %22#1 : i6
    %23 = comb.or %18, %20, %21 : i1
    %24 = comb.xor %23, %true : i1
    %25 = llhd.prb %advanced_word_count : i6
    %26 = comb.extract %25 from 0 : (i6) -> i5
    %27 = comb.mux %24, %word_count, %26 : i5
    %28 = comb.icmp eq %word_count, %c-1_i5 : i5
    %29 = comb.and %28, %16 : i1
    %30 = comb.and %full_int, %push_req_n, %pop_req_n : i1
    %31 = comb.and %full_int, %15 : i1
    %32 = comb.or %29, %30, %31 : i1
    %33 = comb.icmp ne %27, %c0_i5 : i5
    %34 = comb.or %33, %32 : i1
    %35 = comb.extract %27 from 4 : (i5) -> i1
    %36 = comb.or %35, %32 : i1
    %37 = comb.icmp ule %27, %ae_level : i5
    %38 = comb.xor %32, %true : i1
    %39 = comb.and %37, %38 : i1
    %40 = comb.xor %39, %true : i1
    %41 = comb.icmp uge %27, %af_thresh : i5
    %42 = comb.or %41, %32 : i1
    %43 = comb.and %rd_addr_at_max, %5 : i1
    %44 = comb.extract %12 from 1 : (i6) -> i5
    %45 = comb.mux %43, %c0_i5, %44 : i5
    %46 = comb.and %4, %19 : i1
    %47 = comb.and %16, %full_int : i1
    %48 = comb.or %46, %47 : i1
    %49 = comb.xor %init_n, %true : i1
    %50 = comb.and %init_n, %34 : i1
    %51 = comb.and %init_n, %40 : i1
    %52 = comb.and %init_n, %36 : i1
    %53 = comb.and %init_n, %42 : i1
    %54 = comb.and %init_n, %32 : i1
    %55 = comb.and %init_n, %48 : i1
    %56 = comb.or %49, %8 : i1
    %57 = comb.mux %56, %c0_i5, %9 : i5
    %58 = comb.and %init_n, %13 : i1
    %59 = comb.and %init_n, %14 : i1
    %60 = comb.or %49, %43 : i1
    %61 = comb.mux %60, %c0_i5, %44 : i5
    %62 = comb.mux %49, %c0_i5, %27 : i5
    %63 = seq.to_clock %clk
    %64 = comb.xor %rst_n, %true : i1
    %empty_n = seq.firreg %50 clock %63 reset async %64, %false : i1
    %almost_empty_n = seq.firreg %51 clock %63 reset async %64, %false : i1
    %half_full_int = seq.firreg %52 clock %63 reset async %64, %false : i1
    %almost_full_int = seq.firreg %53 clock %63 reset async %64, %false : i1
    %full_int = seq.firreg %54 clock %63 reset async %64, %false : i1
    %error_int = seq.firreg %55 clock %63 reset async %64, %false : i1
    %wr_addr_int = seq.firreg %57 clock %63 reset async %64, %c0_i5 : i5
    %rd_addr_at_max = seq.firreg %58 clock %63 reset async %64, %false : i1
    %wr_addr_at_max = seq.firreg %59 clock %63 reset async %64, %false : i1
    %rd_addr_int = seq.firreg %61 clock %63 reset async %64, %c0_i5 : i5
    %word_count = seq.firreg %62 clock %63 reset async %64, %c0_i5 : i5
    %65 = comb.xor %almost_empty_n, %true : i1
    %66 = comb.or %34, %49 : i1
    hw.output %2, %19, %65, %half_full_int, %almost_full_int, %full_int, %error_int, %wr_addr_int, %rd_addr_int, %word_count, %66, %54, %55 : i1, i1, i1, i1, i1, i1, i1, i5, i5, i5, i1, i1, i1
  }
  hw.module private @DW_apb_uart_to_det(in %sclk : i1, in %s_rst_n : i1, in %bclk : i1, in %rx_pop_hld : i1, in %rx_finish : i1, out char_to : i1, out rx_pop_ack : i1, out rx_pop_ack_ed : i1, in %char_info : i4, in %to_det_cnt_ens : i3) {
    %true = hw.constant true
    %c1_i10 = hw.constant 1 : i10
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %c0_i3 = hw.constant 0 : i3
    %c448_i10 = hw.constant 448 : i10
    %c-256_i10 = hw.constant -256 : i10
    %c-1_i4 = hw.constant -1 : i4
    %c-2_i4 = hw.constant -2 : i4
    %c-3_i4 = hw.constant -3 : i4
    %c-480_i10 = hw.constant -480 : i10
    %c-4_i4 = hw.constant -4 : i4
    %c-5_i4 = hw.constant -5 : i4
    %c-6_i4 = hw.constant -6 : i4
    %c-7_i4 = hw.constant -7 : i4
    %c-8_i4 = hw.constant -8 : i4
    %c-320_i10 = hw.constant -320 : i10
    %c7_i4 = hw.constant 7 : i4
    %c6_i4 = hw.constant 6 : i4
    %c5_i4 = hw.constant 5 : i4
    %c480_i10 = hw.constant 480 : i10
    %c4_i4 = hw.constant 4 : i4
    %c-384_i10 = hw.constant -384 : i10
    %c3_i4 = hw.constant 3 : i4
    %c-448_i10 = hw.constant -448 : i10
    %c2_i4 = hw.constant 2 : i4
    %c-512_i10 = hw.constant -512 : i10
    %c1_i4 = hw.constant 1 : i4
    %false = hw.constant false
    %c0_i10 = hw.constant 0 : i10
    %internal_to_det_cnt_ens = llhd.sig %c0_i3 : i3
    %timeout_val = llhd.sig %c0_i10 : i10
    %1 = llhd.prb %timeout_val : i10
    %2 = seq.to_clock %sclk
    %3 = comb.xor %s_rst_n, %true : i1
    %dly_rx_pop_ack = seq.firreg %rx_pop_hld clock %2 reset async %3, %false : i1
    %4 = comb.xor %rx_pop_hld, %dly_rx_pop_ack : i1
    %5 = comb.xor %14, %true : i1
    %6 = comb.or %5, %4, %rx_finish, %23 : i1
    %7 = comb.add %cto_cnt, %c1_i10 : i10
    %8 = comb.xor %6, %true : i1
    %9 = comb.and %bclk, %8 : i1
    %10 = comb.mux %9, %7, %c0_i10 : i10
    %11 = comb.or %6, %bclk : i1
    %12 = comb.mux bin %11, %10, %cto_cnt : i10
    %cto_cnt = seq.firreg %12 clock %2 reset async %3, %c0_i10 : i10
    %13 = comb.xor %16, %true : i1
    %14 = comb.and %17, %13 : i1
    %15 = llhd.prb %internal_to_det_cnt_ens : i3
    %16 = comb.extract %15 from 2 : (i3) -> i1
    %17 = comb.extract %15 from 1 {sv.namehint = "fifo_en"} : (i3) -> i1
    %18:2 = llhd.process -> i3, i1 {
      cf.br ^bb1(%c0_i3, %false : i3, i1)
    ^bb1(%25: i3, %26: i1):  // 2 preds: ^bb0, ^bb2
      llhd.wait yield (%25, %26 : i3, i1), (%to_det_cnt_ens : i3), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb1(%to_det_cnt_ens, %true : i3, i1)
    }
    llhd.drv %internal_to_det_cnt_ens, %18#0 after %0 if %18#1 : i3
    %19 = comb.icmp eq %cto_cnt, %1 : i10
    %20 = comb.xor %23, %true : i1
    %21 = comb.and %19, %20 : i1
    %22 = comb.xor %21, %int_char_to : i1
    %int_char_to = seq.firreg %22 clock %2 reset async %3, %false : i1
    %dly_char_to = seq.firreg %int_char_to clock %2 reset async %3, %false : i1
    %23 = comb.xor %int_char_to, %dly_char_to : i1
    %24:2 = llhd.process -> i10, i1 {
      cf.br ^bb1(%c0_i10, %false : i10, i1)
    ^bb1(%25: i10, %26: i1):  // 17 preds: ^bb0, ^bb2, ^bb3, ^bb4, ^bb5, ^bb6, ^bb7, ^bb8, ^bb9, ^bb10, ^bb11, ^bb12, ^bb13, ^bb14, ^bb15, ^bb16, ^bb16
      llhd.wait yield (%25, %26 : i10, i1), (%char_info : i4), ^bb2
    ^bb2:  // pred: ^bb1
      %27 = comb.icmp ceq %char_info, %c1_i4 : i4
      cf.cond_br %27, ^bb1(%c-512_i10, %true : i10, i1), ^bb3
    ^bb3:  // pred: ^bb2
      %28 = comb.icmp ceq %char_info, %c2_i4 : i4
      cf.cond_br %28, ^bb1(%c-448_i10, %true : i10, i1), ^bb4
    ^bb4:  // pred: ^bb3
      %29 = comb.icmp ceq %char_info, %c3_i4 : i4
      cf.cond_br %29, ^bb1(%c-384_i10, %true : i10, i1), ^bb5
    ^bb5:  // pred: ^bb4
      %30 = comb.icmp ceq %char_info, %c4_i4 : i4
      cf.cond_br %30, ^bb1(%c480_i10, %true : i10, i1), ^bb6
    ^bb6:  // pred: ^bb5
      %31 = comb.icmp ceq %char_info, %c5_i4 : i4
      cf.cond_br %31, ^bb1(%c-448_i10, %true : i10, i1), ^bb7
    ^bb7:  // pred: ^bb6
      %32 = comb.icmp ceq %char_info, %c6_i4 : i4
      cf.cond_br %32, ^bb1(%c-384_i10, %true : i10, i1), ^bb8
    ^bb8:  // pred: ^bb7
      %33 = comb.icmp ceq %char_info, %c7_i4 : i4
      cf.cond_br %33, ^bb1(%c-320_i10, %true : i10, i1), ^bb9
    ^bb9:  // pred: ^bb8
      %34 = comb.icmp ceq %char_info, %c-8_i4 : i4
      cf.cond_br %34, ^bb1(%c-512_i10, %true : i10, i1), ^bb10
    ^bb10:  // pred: ^bb9
      %35 = comb.icmp ceq %char_info, %c-7_i4 : i4
      cf.cond_br %35, ^bb1(%c-448_i10, %true : i10, i1), ^bb11
    ^bb11:  // pred: ^bb10
      %36 = comb.icmp ceq %char_info, %c-6_i4 : i4
      cf.cond_br %36, ^bb1(%c-384_i10, %true : i10, i1), ^bb12
    ^bb12:  // pred: ^bb11
      %37 = comb.icmp ceq %char_info, %c-5_i4 : i4
      cf.cond_br %37, ^bb1(%c-320_i10, %true : i10, i1), ^bb13
    ^bb13:  // pred: ^bb12
      %38 = comb.icmp ceq %char_info, %c-4_i4 : i4
      cf.cond_br %38, ^bb1(%c-480_i10, %true : i10, i1), ^bb14
    ^bb14:  // pred: ^bb13
      %39 = comb.icmp ceq %char_info, %c-3_i4 : i4
      cf.cond_br %39, ^bb1(%c-384_i10, %true : i10, i1), ^bb15
    ^bb15:  // pred: ^bb14
      %40 = comb.icmp ceq %char_info, %c-2_i4 : i4
      cf.cond_br %40, ^bb1(%c-320_i10, %true : i10, i1), ^bb16
    ^bb16:  // pred: ^bb15
      %41 = comb.icmp ceq %char_info, %c-1_i4 : i4
      cf.cond_br %41, ^bb1(%c-256_i10, %true : i10, i1), ^bb1(%c448_i10, %true : i10, i1)
    }
    llhd.drv %timeout_val, %24#0 after %0 if %24#1 : i10
    hw.output %int_char_to, %rx_pop_hld, %4 : i1, i1, i1
  }
  hw.module private @DW_apb_uart_fifo(in %pclk : i1, in %presetn : i1, in %tx_push : i1, in %tx_pop : i1, in %rx_push : i1, in %rx_pop : i1, in %tx_fifo_rst : i1, in %rx_fifo_rst : i1, in %tx_push_data : i8, in %rx_push_data : i10, out tx_full : i1, out tx_empty : i1, out rx_full : i1, out rx_empty : i1, out rx_overflow : i1, out tx_pop_data : i8, out rx_pop_data : i10) {
    %true = hw.constant true
    %c-1_i5 = hw.constant -1 : i5
    %c0_i5 = hw.constant 0 : i5
    %U_tx_fifo.we_n, %U_tx_fifo.empty, %U_tx_fifo.almost_empty, %U_tx_fifo.half_full, %U_tx_fifo.almost_full, %U_tx_fifo.full, %U_tx_fifo.error, %U_tx_fifo.wr_addr, %U_tx_fifo.rd_addr, %U_tx_fifo.wrd_count, %U_tx_fifo.nxt_empty_n, %U_tx_fifo.nxt_full, %U_tx_fifo.nxt_error = hw.instance "U_tx_fifo" @DW_apb_uart_bcm06(clk: %pclk: i1, rst_n: %presetn: i1, init_n: %0: i1, push_req_n: %1: i1, pop_req_n: %2: i1, ae_level: %c0_i5: i5, af_thresh: %c-1_i5: i5) -> (we_n: i1, empty: i1, almost_empty: i1, half_full: i1, almost_full: i1, full: i1, error: i1, wr_addr: i5, rd_addr: i5, wrd_count: i5, nxt_empty_n: i1, nxt_full: i1, nxt_error: i1) {sv.namehint = "tx_we_n"}
    %0 = comb.xor %tx_fifo_rst, %true {sv.namehint = "tx_fifo_rst_n"} : i1
    %1 = comb.xor %tx_push, %true {sv.namehint = "tx_push_n"} : i1
    %2 = comb.xor %tx_pop, %true {sv.namehint = "tx_pop_n"} : i1
    %U_DW_tx_ram.data_out = hw.instance "U_DW_tx_ram" @DW_apb_uart_bcm57(clk: %pclk: i1, rst_n: %presetn: i1, wr_n: %U_tx_fifo.we_n: i1, data_in: %tx_push_data: i8, wr_addr: %U_tx_fifo.wr_addr: i5, rd_addr: %U_tx_fifo.rd_addr: i5) -> (data_out: i8) {sv.namehint = "tx_data_out"}
    %U_rx_fifo.we_n, %U_rx_fifo.empty, %U_rx_fifo.almost_empty, %U_rx_fifo.half_full, %U_rx_fifo.almost_full, %U_rx_fifo.full, %U_rx_fifo.error, %U_rx_fifo.wr_addr, %U_rx_fifo.rd_addr, %U_rx_fifo.wrd_count, %U_rx_fifo.nxt_empty_n, %U_rx_fifo.nxt_full, %U_rx_fifo.nxt_error = hw.instance "U_rx_fifo" @DW_apb_uart_bcm06(clk: %pclk: i1, rst_n: %presetn: i1, init_n: %5: i1, push_req_n: %6: i1, pop_req_n: %3: i1, ae_level: %c0_i5: i5, af_thresh: %c-1_i5: i5) -> (we_n: i1, empty: i1, almost_empty: i1, half_full: i1, almost_full: i1, full: i1, error: i1, wr_addr: i5, rd_addr: i5, wrd_count: i5, nxt_empty_n: i1, nxt_full: i1, nxt_error: i1) {sv.namehint = "rx_we_n"}
    %3 = comb.xor %rx_pop, %true {sv.namehint = "rx_pop_n"} : i1
    %4 = comb.and %U_rx_fifo.full, %rx_push, %3 : i1
    %5 = comb.xor %rx_fifo_rst, %true {sv.namehint = "rx_fifo_rst_n"} : i1
    %6 = comb.xor %rx_push, %true {sv.namehint = "rx_push_n"} : i1
    %U_DW_rx_ram.data_out = hw.instance "U_DW_rx_ram" @DW_apb_uart_bcm57_0(clk: %pclk: i1, rst_n: %presetn: i1, wr_n: %U_rx_fifo.we_n: i1, data_in: %rx_push_data: i10, wr_addr: %U_rx_fifo.wr_addr: i5, rd_addr: %U_rx_fifo.rd_addr: i5) -> (data_out: i10) {sv.namehint = "rx_data_out"}
    hw.output %U_tx_fifo.full, %U_tx_fifo.empty, %U_rx_fifo.full, %U_rx_fifo.empty, %4, %U_DW_tx_ram.data_out, %U_DW_rx_ram.data_out : i1, i1, i1, i1, i1, i8, i10
  }
  hw.module private @DW_apb_uart_sync(in %pclk : i1, in %presetn : i1, in %sclk : i1, in %s_rst_n : i1, in %divsr_wd : i1, in %xbreak : i1, in %lb_en : i1, in %tx_start : i1, in %tx_finish : i1, in %rx_finish : i1, in %char_to : i1, in %rx_pop_ack : i1, in %rx_pop_ack_ed : i1, in %cnt_ens_ed : i3, in %char_info_wd : i1, in %divsr : i16, in %char_info : i6, in %tx_data : i8, in %rx_data : i10, in %to_det_cnt_ens : i3, in %rx_pop_hld : i1, in %rx_pop_hld_ed : i1, in %dlf : i4, in %dlf_wd : i1, out sync_divsr_wd : i1, out sync_break : i1, out sync_lb_en : i1, out sync_tx_start : i1, out sync_tx_finish : i1, out sync_rx_finish : i1, out sync_char_to : i1, out sync_rx_pop_ack : i1, out sync_divsr : i16, out sync_char_info : i6, out sync_tx_data : i8, out sync_to_det_cnt_ens : i3, out sync_rx_pop_hld : i1, out sync_dlf : i4, out sync_dlf_wd : i1, out sync_rx_data : i10) {
    %false = hw.constant false
    %true = hw.constant true
    %U_DW_apb_uart_bcm25_p2sl_tx_data_ssyzr.empty_s, %U_DW_apb_uart_bcm25_p2sl_tx_data_ssyzr.full_s, %U_DW_apb_uart_bcm25_p2sl_tx_data_ssyzr.done_s, %U_DW_apb_uart_bcm25_p2sl_tx_data_ssyzr.data_avail_d, %U_DW_apb_uart_bcm25_p2sl_tx_data_ssyzr.data_d = hw.instance "U_DW_apb_uart_bcm25_p2sl_tx_data_ssyzr" @DW_apb_uart_bcm25(clk_s: %pclk: i1, rst_s_n: %presetn: i1, init_s_n: %true: i1, send_s: %tx_start: i1, data_s: %tx_data: i8, clk_d: %sclk: i1, rst_d_n: %s_rst_n: i1, init_d_n: %true: i1) -> (empty_s: i1, full_s: i1, done_s: i1, data_avail_d: i1, data_d: i8) {sv.namehint = "done_s_0_unconn"}
    %U_DW_apb_uart_bcm25_s2pl_0_psyzr.empty_s, %U_DW_apb_uart_bcm25_s2pl_0_psyzr.full_s, %U_DW_apb_uart_bcm25_s2pl_0_psyzr.done_s, %U_DW_apb_uart_bcm25_s2pl_0_psyzr.data_avail_d, %U_DW_apb_uart_bcm25_s2pl_0_psyzr.data_d = hw.instance "U_DW_apb_uart_bcm25_s2pl_0_psyzr" @DW_apb_uart_bcm25_1(clk_s: %sclk: i1, rst_s_n: %s_rst_n: i1, init_s_n: %true: i1, send_s: %tx_finish: i1, data_s: %false: i1, clk_d: %pclk: i1, rst_d_n: %presetn: i1, init_d_n: %true: i1) -> (empty_s: i1, full_s: i1, done_s: i1, data_avail_d: i1, data_d: i1) {sv.namehint = "done_s_1_unconn"}
    %U_DW_apb_uart_bcm25_s2pl_rx_data_psyzr.empty_s, %U_DW_apb_uart_bcm25_s2pl_rx_data_psyzr.full_s, %U_DW_apb_uart_bcm25_s2pl_rx_data_psyzr.done_s, %U_DW_apb_uart_bcm25_s2pl_rx_data_psyzr.data_avail_d, %U_DW_apb_uart_bcm25_s2pl_rx_data_psyzr.data_d = hw.instance "U_DW_apb_uart_bcm25_s2pl_rx_data_psyzr" @DW_apb_uart_bcm25_2(clk_s: %sclk: i1, rst_s_n: %s_rst_n: i1, init_s_n: %true: i1, send_s: %rx_finish: i1, data_s: %rx_data: i10, clk_d: %pclk: i1, rst_d_n: %presetn: i1, init_d_n: %true: i1) -> (empty_s: i1, full_s: i1, done_s: i1, data_avail_d: i1, data_d: i10) {sv.namehint = "done_s_2_unconn"}
    %U_DW_apb_uart_bcm25_p2sl_divsr_ssyzr.empty_s, %U_DW_apb_uart_bcm25_p2sl_divsr_ssyzr.full_s, %U_DW_apb_uart_bcm25_p2sl_divsr_ssyzr.done_s, %U_DW_apb_uart_bcm25_p2sl_divsr_ssyzr.data_avail_d, %U_DW_apb_uart_bcm25_p2sl_divsr_ssyzr.data_d = hw.instance "U_DW_apb_uart_bcm25_p2sl_divsr_ssyzr" @DW_apb_uart_bcm25_3(clk_s: %pclk: i1, rst_s_n: %presetn: i1, init_s_n: %true: i1, send_s: %divsr_wd: i1, data_s: %divsr: i16, clk_d: %sclk: i1, rst_d_n: %s_rst_n: i1, init_d_n: %true: i1) -> (empty_s: i1, full_s: i1, done_s: i1, data_avail_d: i1, data_d: i16) {sv.namehint = "sp2sl_sync_divsr"}
    %U_DW_apb_uart_bcm25_p2sl_char_info_ssyzr.empty_s, %U_DW_apb_uart_bcm25_p2sl_char_info_ssyzr.full_s, %U_DW_apb_uart_bcm25_p2sl_char_info_ssyzr.done_s, %U_DW_apb_uart_bcm25_p2sl_char_info_ssyzr.data_avail_d, %U_DW_apb_uart_bcm25_p2sl_char_info_ssyzr.data_d = hw.instance "U_DW_apb_uart_bcm25_p2sl_char_info_ssyzr" @DW_apb_uart_bcm25_4(clk_s: %pclk: i1, rst_s_n: %presetn: i1, init_s_n: %true: i1, send_s: %char_info_wd: i1, data_s: %char_info: i6, clk_d: %sclk: i1, rst_d_n: %s_rst_n: i1, init_d_n: %true: i1) -> (empty_s: i1, full_s: i1, done_s: i1, data_avail_d: i1, data_d: i6) {sv.namehint = "dr_5_unconn"}
    %0 = comb.concat %GEN_DW_APB_UART_BCM25_6_2.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.data_d, %GEN_DW_APB_UART_BCM25_6_1.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.data_d, %GEN_DW_APB_UART_BCM25_6_0.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.data_d : i1, i1, i1
    %1 = comb.extract %cnt_ens_ed from 0 : (i3) -> i1
    %2 = comb.extract %to_det_cnt_ens from 0 : (i3) -> i1
    %GEN_DW_APB_UART_BCM25_6_0.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.empty_s, %GEN_DW_APB_UART_BCM25_6_0.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.full_s, %GEN_DW_APB_UART_BCM25_6_0.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.done_s, %GEN_DW_APB_UART_BCM25_6_0.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.data_avail_d, %GEN_DW_APB_UART_BCM25_6_0.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.data_d = hw.instance "GEN_DW_APB_UART_BCM25_6_0.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr" @DW_apb_uart_bcm25_5(clk_s: %pclk: i1, rst_s_n: %presetn: i1, init_s_n: %true: i1, send_s: %1: i1, data_s: %2: i1, clk_d: %sclk: i1, rst_d_n: %s_rst_n: i1, init_d_n: %true: i1) -> (empty_s: i1, full_s: i1, done_s: i1, data_avail_d: i1, data_d: i1)
    %3 = comb.extract %cnt_ens_ed from 1 : (i3) -> i1
    %4 = comb.extract %to_det_cnt_ens from 1 : (i3) -> i1
    %GEN_DW_APB_UART_BCM25_6_1.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.empty_s, %GEN_DW_APB_UART_BCM25_6_1.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.full_s, %GEN_DW_APB_UART_BCM25_6_1.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.done_s, %GEN_DW_APB_UART_BCM25_6_1.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.data_avail_d, %GEN_DW_APB_UART_BCM25_6_1.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.data_d = hw.instance "GEN_DW_APB_UART_BCM25_6_1.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr" @DW_apb_uart_bcm25_5(clk_s: %pclk: i1, rst_s_n: %presetn: i1, init_s_n: %true: i1, send_s: %3: i1, data_s: %4: i1, clk_d: %sclk: i1, rst_d_n: %s_rst_n: i1, init_d_n: %true: i1) -> (empty_s: i1, full_s: i1, done_s: i1, data_avail_d: i1, data_d: i1)
    %5 = comb.extract %cnt_ens_ed from 2 : (i3) -> i1
    %6 = comb.extract %to_det_cnt_ens from 2 : (i3) -> i1
    %GEN_DW_APB_UART_BCM25_6_2.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.empty_s, %GEN_DW_APB_UART_BCM25_6_2.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.full_s, %GEN_DW_APB_UART_BCM25_6_2.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.done_s, %GEN_DW_APB_UART_BCM25_6_2.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.data_avail_d, %GEN_DW_APB_UART_BCM25_6_2.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr.data_d = hw.instance "GEN_DW_APB_UART_BCM25_6_2.U_DW_apb_uart_bcm25_p2sl_p2sl_to_det_cnt_ens_ssyzr" @DW_apb_uart_bcm25_5(clk_s: %pclk: i1, rst_s_n: %presetn: i1, init_s_n: %true: i1, send_s: %5: i1, data_s: %6: i1, clk_d: %sclk: i1, rst_d_n: %s_rst_n: i1, init_d_n: %true: i1) -> (empty_s: i1, full_s: i1, done_s: i1, data_avail_d: i1, data_d: i1)
    %U_DW_apb_uart_bcm21_p2sl_xbreak_ssyzr.data_d = hw.instance "U_DW_apb_uart_bcm21_p2sl_xbreak_ssyzr" @DW_apb_uart_bcm21(clk_d: %sclk: i1, rst_d_n: %s_rst_n: i1, data_s: %xbreak: i1) -> (data_d: i1) {sv.namehint = "sp2sl_sync_break"}
    %U_DW_apb_uart_bcm21_p2sl_lb_en_ssyzr.data_d = hw.instance "U_DW_apb_uart_bcm21_p2sl_lb_en_ssyzr" @DW_apb_uart_bcm21(clk_d: %sclk: i1, rst_d_n: %s_rst_n: i1, data_s: %lb_en: i1) -> (data_d: i1) {sv.namehint = "sp2sl_sync_lb_en"}
    %U_DW_apb_uart_bcm21_s2pl_char_to_psyzr.data_d = hw.instance "U_DW_apb_uart_bcm21_s2pl_char_to_psyzr" @DW_apb_uart_bcm21(clk_d: %pclk: i1, rst_d_n: %presetn: i1, data_s: %char_to: i1) -> (data_d: i1) {sv.namehint = "as2pl_sync_char_to"}
    %U_DW_apb_uart_bcm25_s2pl_rx_pop_ack_psyzr.empty_s, %U_DW_apb_uart_bcm25_s2pl_rx_pop_ack_psyzr.full_s, %U_DW_apb_uart_bcm25_s2pl_rx_pop_ack_psyzr.done_s, %U_DW_apb_uart_bcm25_s2pl_rx_pop_ack_psyzr.data_avail_d, %U_DW_apb_uart_bcm25_s2pl_rx_pop_ack_psyzr.data_d = hw.instance "U_DW_apb_uart_bcm25_s2pl_rx_pop_ack_psyzr" @DW_apb_uart_bcm25_5(clk_s: %sclk: i1, rst_s_n: %s_rst_n: i1, init_s_n: %true: i1, send_s: %rx_pop_ack_ed: i1, data_s: %rx_pop_ack: i1, clk_d: %pclk: i1, rst_d_n: %presetn: i1, init_d_n: %true: i1) -> (empty_s: i1, full_s: i1, done_s: i1, data_avail_d: i1, data_d: i1) {sv.namehint = "dr_7_unconn"}
    %U_DW_apb_uart_bcm25_p2sl_rx_pop_hld_ssyzr.empty_s, %U_DW_apb_uart_bcm25_p2sl_rx_pop_hld_ssyzr.full_s, %U_DW_apb_uart_bcm25_p2sl_rx_pop_hld_ssyzr.done_s, %U_DW_apb_uart_bcm25_p2sl_rx_pop_hld_ssyzr.data_avail_d, %U_DW_apb_uart_bcm25_p2sl_rx_pop_hld_ssyzr.data_d = hw.instance "U_DW_apb_uart_bcm25_p2sl_rx_pop_hld_ssyzr" @DW_apb_uart_bcm25_5(clk_s: %pclk: i1, rst_s_n: %presetn: i1, init_s_n: %true: i1, send_s: %rx_pop_hld_ed: i1, data_s: %rx_pop_hld: i1, clk_d: %sclk: i1, rst_d_n: %s_rst_n: i1, init_d_n: %true: i1) -> (empty_s: i1, full_s: i1, done_s: i1, data_avail_d: i1, data_d: i1) {sv.namehint = "dr_8_unconn"}
    %U_DW_apb_uart_bcm25_p2sl_dlf_ssyzr.empty_s, %U_DW_apb_uart_bcm25_p2sl_dlf_ssyzr.full_s, %U_DW_apb_uart_bcm25_p2sl_dlf_ssyzr.done_s, %U_DW_apb_uart_bcm25_p2sl_dlf_ssyzr.data_avail_d, %U_DW_apb_uart_bcm25_p2sl_dlf_ssyzr.data_d = hw.instance "U_DW_apb_uart_bcm25_p2sl_dlf_ssyzr" @DW_apb_uart_bcm25_6(clk_s: %pclk: i1, rst_s_n: %presetn: i1, init_s_n: %true: i1, send_s: %dlf_wd: i1, data_s: %dlf: i4, clk_d: %sclk: i1, rst_d_n: %s_rst_n: i1, init_d_n: %true: i1) -> (empty_s: i1, full_s: i1, done_s: i1, data_avail_d: i1, data_d: i4) {sv.namehint = "sp2sl_sync_dlf"}
    hw.output %U_DW_apb_uart_bcm25_p2sl_divsr_ssyzr.data_avail_d, %U_DW_apb_uart_bcm21_p2sl_xbreak_ssyzr.data_d, %U_DW_apb_uart_bcm21_p2sl_lb_en_ssyzr.data_d, %U_DW_apb_uart_bcm25_p2sl_tx_data_ssyzr.data_avail_d, %U_DW_apb_uart_bcm25_s2pl_0_psyzr.data_avail_d, %U_DW_apb_uart_bcm25_s2pl_rx_data_psyzr.data_avail_d, %U_DW_apb_uart_bcm21_s2pl_char_to_psyzr.data_d, %U_DW_apb_uart_bcm25_s2pl_rx_pop_ack_psyzr.data_d, %U_DW_apb_uart_bcm25_p2sl_divsr_ssyzr.data_d, %U_DW_apb_uart_bcm25_p2sl_char_info_ssyzr.data_d, %U_DW_apb_uart_bcm25_p2sl_tx_data_ssyzr.data_d, %0, %U_DW_apb_uart_bcm25_p2sl_rx_pop_hld_ssyzr.data_d, %U_DW_apb_uart_bcm25_p2sl_dlf_ssyzr.data_d, %U_DW_apb_uart_bcm25_p2sl_dlf_ssyzr.data_avail_d, %U_DW_apb_uart_bcm25_s2pl_rx_data_psyzr.data_d : i1, i1, i1, i1, i1, i1, i1, i1, i16, i6, i8, i3, i1, i4, i1, i10
  }
  hw.module @uart_top(in %UART_PCLK : i1, in %UART_PRESETn : i1, in %UART_CLK : i1, in %UART_RESETn : i1, in %PSEL : i1, in %PENABLE : i1, in %PWRITE : i1, in %PADDR : i24, in %PWDATA : i32, out PRDATA : i32, out UART0_INTR : i1, out UART1_INTR : i1, out UART0_TXD : i1, in %UART0_RXD : i1, out UART1_TXD : i1, in %UART1_RXD : i1, in %SCAN_MODE : i1) {
    %true = hw.constant true
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %c0_i32 = hw.constant 0 : i32
    %c-2_i2 = hw.constant -2 : i2
    %c1_i2 = hw.constant 1 : i2
    %false = hw.constant false
    %PRDATA = llhd.sig %c0_i32 : i32
    %1 = comb.extract %PADDR from 23 : (i24) -> i1
    %2 = comb.xor %1, %true : i1
    %3 = comb.and %2, %PSEL {sv.namehint = "psel_uart0"} : i1
    %4 = comb.and %1, %PSEL {sv.namehint = "psel_uart1"} : i1
    %5 = comb.concat %4, %3 : i1, i1
    %6:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%9: i32, %10: i1):  // 4 preds: ^bb0, ^bb2, ^bb3, ^bb3
      llhd.wait yield (%9, %10 : i32, i1), (%5, %U_UART0.prdata, %U_UART1.prdata : i2, i32, i32), ^bb2
    ^bb2:  // pred: ^bb1
      %11 = comb.icmp ceq %5, %c1_i2 : i2
      cf.cond_br %11, ^bb1(%U_UART0.prdata, %true : i32, i1), ^bb3
    ^bb3:  // pred: ^bb2
      %12 = comb.icmp ceq %5, %c-2_i2 : i2
      cf.cond_br %12, ^bb1(%U_UART1.prdata, %true : i32, i1), ^bb1(%c0_i32, %true : i32, i1)
    }
    llhd.drv %PRDATA, %6#0 after %0 if %6#1 : i32
    %7 = comb.extract %PADDR from 0 : (i24) -> i8
    %U_UART0.prdata, %U_UART0.dtr_n, %U_UART0.rts_n, %U_UART0.out2_n, %U_UART0.out1_n, %U_UART0.dma_tx_req, %U_UART0.dma_rx_req, %U_UART0.txrdy_n, %U_UART0.rxrdy_n, %U_UART0.sout, %U_UART0.intr = hw.instance "U_UART0" @DW_apb_uart(pclk: %UART_PCLK: i1, presetn: %UART_PRESETn: i1, penable: %PENABLE: i1, pwrite: %PWRITE: i1, pwdata: %PWDATA: i32, paddr: %7: i8, psel: %3: i1, sclk: %UART_CLK: i1, s_rst_n: %UART_RESETn: i1, rst_scan_mode: %SCAN_MODE: i1, cts_n: %true: i1, dsr_n: %true: i1, dcd_n: %true: i1, ri_n: %true: i1, sin: %UART0_RXD: i1) -> (prdata: i32, dtr_n: i1, rts_n: i1, out2_n: i1, out1_n: i1, dma_tx_req: i1, dma_rx_req: i1, txrdy_n: i1, rxrdy_n: i1, sout: i1, intr: i1)
    %U_UART1.prdata, %U_UART1.dtr_n, %U_UART1.rts_n, %U_UART1.out2_n, %U_UART1.out1_n, %U_UART1.dma_tx_req, %U_UART1.dma_rx_req, %U_UART1.txrdy_n, %U_UART1.rxrdy_n, %U_UART1.sout, %U_UART1.intr = hw.instance "U_UART1" @DW_apb_uart(pclk: %UART_PCLK: i1, presetn: %UART_PRESETn: i1, penable: %PENABLE: i1, pwrite: %PWRITE: i1, pwdata: %PWDATA: i32, paddr: %7: i8, psel: %4: i1, sclk: %UART_CLK: i1, s_rst_n: %UART_RESETn: i1, rst_scan_mode: %SCAN_MODE: i1, cts_n: %true: i1, dsr_n: %true: i1, dcd_n: %true: i1, ri_n: %true: i1, sin: %UART1_RXD: i1) -> (prdata: i32, dtr_n: i1, rts_n: i1, out2_n: i1, out1_n: i1, dma_tx_req: i1, dma_rx_req: i1, txrdy_n: i1, rxrdy_n: i1, sout: i1, intr: i1)
    %8 = llhd.prb %PRDATA : i32
    hw.output %8, %U_UART0.intr, %U_UART1.intr, %U_UART0.sout, %U_UART1.sout : i32, i1, i1, i1, i1
  }
  hw.module private @DW_apb_uart_tx(in %sclk : i1, in %s_rst_n : i1, in %bclk : i1, in %tx_start : i1, in %tx_data : i8, in %char_info : i6, in %xbreak : i1, in %lb_en : i1, in %sir_en : i1, out tx_finish : i1, out ser_out_lb : i1, out sout : i1) {
    %true = hw.constant true
    %c-2_i4 = hw.constant -2 : i4
    %c-1_i4 = hw.constant -1 : i4
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %c0_i8 = hw.constant 0 : i8
    %c-4_i4 = hw.constant -4 : i4
    %c-7_i4 = hw.constant -7 : i4
    %c-2_i2 = hw.constant -2 : i2
    %c-8_i4 = hw.constant -8 : i4
    %c1_i2 = hw.constant 1 : i2
    %c7_i4 = hw.constant 7 : i4
    %c-5_i4 = hw.constant -5 : i4
    %c-6_i4 = hw.constant -6 : i4
    %c0_i2 = hw.constant 0 : i2
    %c6_i4 = hw.constant 6 : i4
    %c5_i4 = hw.constant 5 : i4
    %c4_i4 = hw.constant 4 : i4
    %c3_i4 = hw.constant 3 : i4
    %c2_i4 = hw.constant 2 : i4
    %c-3_i4 = hw.constant -3 : i4
    %false = hw.constant false
    %c1_i4 = hw.constant 1 : i4
    %c0_i4 = hw.constant 0 : i4
    %ser_tx = llhd.sig %false : i1
    %1 = llhd.prb %ser_tx : i1
    %int_sir_out_n = llhd.sig %false : i1
    %n_state = llhd.sig %c0_i4 : i4
    %2 = llhd.prb %n_state : i4
    %3 = comb.icmp eq %c_state, %c0_i4 : i4
    %4 = comb.and %tx_start, %3 : i1
    %5 = comb.icmp eq %2, %c1_i4 : i4
    %6 = comb.and %22, %5 : i1
    %7 = comb.and %30, %sir_en : i1
    %8 = comb.or %4, %6, %7 : i1
    %9 = comb.add %tx_bclk_cnt, %c1_i4 : i4
    %10 = comb.xor %8, %true : i1
    %11 = comb.icmp ne %tx_bclk_cnt, %c-1_i4 : i4
    %12 = comb.and %11, %bclk, %10 : i1
    %13 = comb.mux %12, %9, %c0_i4 : i4
    %14 = seq.to_clock %sclk
    %15 = comb.xor %s_rst_n, %true : i1
    %16 = comb.or %8, %bclk : i1
    %17 = comb.mux bin %16, %13, %tx_bclk_cnt : i4
    %tx_bclk_cnt = seq.firreg %17 clock %14 reset async %15, %c0_i4 : i4
    %18 = comb.icmp eq %tx_bclk_cnt, %c-1_i4 : i4
    %19 = comb.and %18, %bclk : i1
    %20 = comb.icmp eq %tx_bclk_cnt, %c7_i4 : i4
    %21 = comb.and %20, %bclk : i1
    %22 = comb.or %tx_start, %ext_tx_start : i1
    %23 = comb.or %28, %26 : i1
    %24 = comb.mux bin %23, %28, %ext_tx_start : i1
    %ext_tx_start = seq.firreg %24 clock %14 reset async %15, %false : i1
    %25 = comb.icmp eq %c_state, %c-3_i4 : i4
    %26 = comb.mux %25, %21, %19 : i1
    %27 = comb.xor %26, %true : i1
    %28 = comb.and %tx_start, %27 : i1
    %29 = comb.xor %break_ed_reg, %true : i1
    %30 = comb.and %xbreak, %29 : i1
    %break_ed_reg = seq.firreg %xbreak clock %14 reset async %15, %false : i1
    %c_state = seq.firreg %2 clock %14 reset async %15, %c0_i4 : i4
    %31 = comb.extract %char_info from 0 : (i6) -> i4
    %32:2 = llhd.process -> i4, i1 {
      cf.br ^bb1(%c0_i4, %false : i4, i1)
    ^bb1(%105: i4, %106: i1):  // 42 preds: ^bb0, ^bb3, ^bb3, ^bb5, ^bb5, ^bb7, ^bb7, ^bb9, ^bb9, ^bb11, ^bb11, ^bb13, ^bb13, ^bb15, ^bb16, ^bb17, ^bb17, ^bb19, ^bb20, ^bb21, ^bb21, ^bb23, ^bb24, ^bb25, ^bb25, ^bb27, ^bb28, ^bb28, ^bb30, ^bb30, ^bb32, ^bb34, ^bb34, ^bb35, ^bb35, ^bb37, ^bb38, ^bb38, ^bb39, ^bb40, ^bb41, ^bb41
      llhd.wait yield (%105, %106 : i4, i1), (%c_state, %22, %19, %21, %31 : i4, i1, i1, i1, i4), ^bb2
    ^bb2:  // pred: ^bb1
      %107 = comb.icmp ceq %c_state, %c0_i4 : i4
      cf.cond_br %107, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      %108 = comb.and %22, %19 : i1
      cf.cond_br %108, ^bb1(%c1_i4, %true : i4, i1), ^bb1(%c0_i4, %true : i4, i1)
    ^bb4:  // pred: ^bb2
      %109 = comb.icmp ceq %c_state, %c1_i4 : i4
      cf.cond_br %109, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      cf.cond_br %19, ^bb1(%c2_i4, %true : i4, i1), ^bb1(%c1_i4, %true : i4, i1)
    ^bb6:  // pred: ^bb4
      %110 = comb.icmp ceq %c_state, %c2_i4 : i4
      cf.cond_br %110, ^bb7, ^bb8
    ^bb7:  // pred: ^bb6
      cf.cond_br %19, ^bb1(%c3_i4, %true : i4, i1), ^bb1(%c2_i4, %true : i4, i1)
    ^bb8:  // pred: ^bb6
      %111 = comb.icmp ceq %c_state, %c3_i4 : i4
      cf.cond_br %111, ^bb9, ^bb10
    ^bb9:  // pred: ^bb8
      cf.cond_br %19, ^bb1(%c4_i4, %true : i4, i1), ^bb1(%c3_i4, %true : i4, i1)
    ^bb10:  // pred: ^bb8
      %112 = comb.icmp ceq %c_state, %c4_i4 : i4
      cf.cond_br %112, ^bb11, ^bb12
    ^bb11:  // pred: ^bb10
      cf.cond_br %19, ^bb1(%c5_i4, %true : i4, i1), ^bb1(%c4_i4, %true : i4, i1)
    ^bb12:  // pred: ^bb10
      %113 = comb.icmp ceq %c_state, %c5_i4 : i4
      cf.cond_br %113, ^bb13, ^bb14
    ^bb13:  // pred: ^bb12
      cf.cond_br %19, ^bb1(%c6_i4, %true : i4, i1), ^bb1(%c5_i4, %true : i4, i1)
    ^bb14:  // pred: ^bb12
      %114 = comb.icmp ceq %c_state, %c6_i4 : i4
      cf.cond_br %114, ^bb15, ^bb18
    ^bb15:  // pred: ^bb14
      cf.cond_br %19, ^bb16, ^bb1(%c6_i4, %true : i4, i1)
    ^bb16:  // pred: ^bb15
      %115 = comb.extract %char_info from 0 : (i6) -> i2
      %116 = comb.icmp eq %115, %c0_i2 : i2
      cf.cond_br %116, ^bb17, ^bb1(%c7_i4, %true : i4, i1)
    ^bb17:  // pred: ^bb16
      %117 = comb.extract %char_info from 3 : (i6) -> i1
      cf.cond_br %117, ^bb1(%c-6_i4, %true : i4, i1), ^bb1(%c-5_i4, %true : i4, i1)
    ^bb18:  // pred: ^bb14
      %118 = comb.icmp ceq %c_state, %c7_i4 : i4
      cf.cond_br %118, ^bb19, ^bb22
    ^bb19:  // pred: ^bb18
      cf.cond_br %19, ^bb20, ^bb1(%c7_i4, %true : i4, i1)
    ^bb20:  // pred: ^bb19
      %119 = comb.extract %char_info from 0 : (i6) -> i2
      %120 = comb.icmp eq %119, %c1_i2 : i2
      cf.cond_br %120, ^bb21, ^bb1(%c-8_i4, %true : i4, i1)
    ^bb21:  // pred: ^bb20
      %121 = comb.extract %char_info from 3 : (i6) -> i1
      cf.cond_br %121, ^bb1(%c-6_i4, %true : i4, i1), ^bb1(%c-5_i4, %true : i4, i1)
    ^bb22:  // pred: ^bb18
      %122 = comb.icmp ceq %c_state, %c-8_i4 : i4
      cf.cond_br %122, ^bb23, ^bb26
    ^bb23:  // pred: ^bb22
      cf.cond_br %19, ^bb24, ^bb1(%c-8_i4, %true : i4, i1)
    ^bb24:  // pred: ^bb23
      %123 = comb.extract %char_info from 0 : (i6) -> i2
      %124 = comb.icmp eq %123, %c-2_i2 : i2
      cf.cond_br %124, ^bb25, ^bb1(%c-7_i4, %true : i4, i1)
    ^bb25:  // pred: ^bb24
      %125 = comb.extract %char_info from 3 : (i6) -> i1
      cf.cond_br %125, ^bb1(%c-6_i4, %true : i4, i1), ^bb1(%c-5_i4, %true : i4, i1)
    ^bb26:  // pred: ^bb22
      %126 = comb.icmp ceq %c_state, %c-7_i4 : i4
      cf.cond_br %126, ^bb27, ^bb29
    ^bb27:  // pred: ^bb26
      cf.cond_br %19, ^bb28, ^bb1(%c-7_i4, %true : i4, i1)
    ^bb28:  // pred: ^bb27
      %127 = comb.extract %char_info from 3 : (i6) -> i1
      cf.cond_br %127, ^bb1(%c-6_i4, %true : i4, i1), ^bb1(%c-5_i4, %true : i4, i1)
    ^bb29:  // pred: ^bb26
      %128 = comb.icmp ceq %c_state, %c-6_i4 : i4
      cf.cond_br %128, ^bb30, ^bb31
    ^bb30:  // pred: ^bb29
      cf.cond_br %19, ^bb1(%c-5_i4, %true : i4, i1), ^bb1(%c-6_i4, %true : i4, i1)
    ^bb31:  // pred: ^bb29
      %129 = comb.icmp ceq %c_state, %c-5_i4 : i4
      cf.cond_br %129, ^bb32, ^bb36
    ^bb32:  // pred: ^bb31
      cf.cond_br %19, ^bb33, ^bb1(%c-5_i4, %true : i4, i1)
    ^bb33:  // pred: ^bb32
      %130 = comb.extract %char_info from 2 : (i6) -> i1
      cf.cond_br %130, ^bb34, ^bb35
    ^bb34:  // pred: ^bb33
      %131 = comb.extract %char_info from 0 : (i6) -> i2
      %132 = comb.icmp eq %131, %c0_i2 : i2
      cf.cond_br %132, ^bb1(%c-3_i4, %true : i4, i1), ^bb1(%c-4_i4, %true : i4, i1)
    ^bb35:  // pred: ^bb33
      cf.cond_br %22, ^bb1(%c1_i4, %true : i4, i1), ^bb1(%c0_i4, %true : i4, i1)
    ^bb36:  // pred: ^bb31
      %133 = comb.icmp ceq %c_state, %c-4_i4 : i4
      cf.cond_br %133, ^bb37, ^bb39
    ^bb37:  // pred: ^bb36
      cf.cond_br %19, ^bb38, ^bb1(%c-4_i4, %true : i4, i1)
    ^bb38:  // pred: ^bb37
      cf.cond_br %22, ^bb1(%c1_i4, %true : i4, i1), ^bb1(%c0_i4, %true : i4, i1)
    ^bb39:  // pred: ^bb36
      %134 = comb.icmp ceq %c_state, %c-3_i4 : i4
      cf.cond_br %134, ^bb40, ^bb1(%c0_i4, %true : i4, i1)
    ^bb40:  // pred: ^bb39
      cf.cond_br %21, ^bb41, ^bb1(%c-3_i4, %true : i4, i1)
    ^bb41:  // pred: ^bb40
      cf.cond_br %22, ^bb1(%c1_i4, %true : i4, i1), ^bb1(%c0_i4, %true : i4, i1)
    }
    llhd.drv %n_state, %32#0 after %0 if %32#1 : i4
    %33 = comb.icmp eq %c_state, %c1_i4 : i4
    %34 = comb.icmp eq %c_state, %c-5_i4 : i4
    %35 = comb.icmp eq %c_state, %c-4_i4 : i4
    %36 = comb.or %3, %34, %35, %25 : i1
    %37 = comb.icmp eq %c_state, %c2_i4 : i4
    %38 = comb.and %37, %19 : i1
    %39 = comb.icmp eq %c_state, %c3_i4 : i4
    %40 = comb.and %39, %19 : i1
    %41 = comb.icmp eq %c_state, %c4_i4 : i4
    %42 = comb.and %41, %19 : i1
    %43 = comb.icmp eq %c_state, %c5_i4 : i4
    %44 = comb.and %43, %19 : i1
    %45 = comb.icmp eq %c_state, %c6_i4 : i4
    %46 = comb.extract %char_info from 0 : (i6) -> i2
    %47 = comb.icmp ne %46, %c0_i2 : i2
    %48 = comb.and %45, %19, %47 : i1
    %49 = comb.icmp eq %c_state, %c7_i4 : i4
    %50 = comb.icmp ne %46, %c1_i2 : i2
    %51 = comb.and %49, %19, %50 : i1
    %52 = comb.icmp eq %c_state, %c-8_i4 : i4
    %53 = comb.icmp ne %46, %c-2_i2 : i2
    %54 = comb.and %52, %19, %53 : i1
    %55 = comb.or %38, %40, %42, %44, %48, %51, %54 : i1
    %56 = comb.icmp eq %c_state, %c-6_i4 : i4
    %57 = comb.extract %char_info from 2 : (i6) -> i1
    %58 = comb.xor %57, %true : i1
    %59 = comb.icmp eq %tx_bclk_cnt, %c-2_i4 : i4
    %60 = comb.and %59, %bclk : i1
    %61 = comb.and %34, %58, %60 : i1
    %62 = comb.and %35, %60 : i1
    %63 = comb.icmp eq %tx_bclk_cnt, %c6_i4 : i4
    %64 = comb.and %25, %63, %bclk : i1
    %65 = comb.or %61, %62, %64 : i1
    %66 = comb.or %lb_en, %67 : i1
    %sout = seq.firreg %66 clock %14 reset async %15, %true : i1
    %67 = comb.or %sir_en, %1 : i1
    %68 = comb.extract %tx_shift_reg from 0 : (i8) -> i1
    %69:2 = llhd.process -> i1, i1 {
      cf.br ^bb1(%false, %false : i1, i1)
    ^bb1(%105: i1, %106: i1):  // 5 preds: ^bb0, ^bb2, ^bb3, ^bb4, ^bb4
      llhd.wait yield (%105, %106 : i1, i1), (%xbreak, %33, %36, %56, %parity_gen, %68, %sir_break_ext, %30 : i1, i1, i1, i1, i1, i1, i1, i1), ^bb2
    ^bb2:  // pred: ^bb1
      %107 = comb.or %xbreak, %sir_break_ext : i1
      %108 = comb.xor %30, %true : i1
      %109 = comb.and %107, %108 : i1
      %110 = comb.or %109, %33 : i1
      cf.cond_br %110, ^bb1(%false, %true : i1, i1), ^bb3
    ^bb3:  // pred: ^bb2
      cf.cond_br %36, ^bb1(%true, %true : i1, i1), ^bb4
    ^bb4:  // pred: ^bb3
      cf.cond_br %56, ^bb1(%parity_gen, %true : i1, i1), ^bb1(%68, %true : i1, i1)
    }
    llhd.drv %ser_tx, %69#0 after %0 if %69#1 : i1
    %70 = comb.and %xbreak, %80 : i1
    %71 = comb.xor %xbreak, %true : i1
    %72 = comb.xor %80, %true : i1
    %73 = comb.and %71, %72 : i1
    %74 = comb.or %70, %73 : i1
    %75 = comb.mux bin %74, %70, %sir_break_ext : i1
    %sir_break_ext = seq.firreg %75 clock %14 reset async %15, %false : i1
    %76:2 = llhd.process -> i1, i1 {
      cf.br ^bb1(%false, %false : i1, i1)
    ^bb1(%105: i1, %106: i1):  // 4 preds: ^bb0, ^bb2, ^bb3, ^bb3
      llhd.wait yield (%105, %106 : i1, i1), (%sir_en, %1, %79 : i1, i1, i1), ^bb2
    ^bb2:  // pred: ^bb1
      %107 = comb.xor %sir_en, %true : i1
      cf.cond_br %107, ^bb1(%false, %true : i1, i1), ^bb3
    ^bb3:  // pred: ^bb2
      %108 = comb.xor %1, %true : i1
      %109 = comb.and %108, %79 : i1
      cf.cond_br %109, ^bb1(%true, %true : i1, i1), ^bb1(%false, %true : i1, i1)
    }
    llhd.drv %int_sir_out_n, %76#0 after %0 if %76#1 : i1
    %77 = comb.icmp eq %tx_bclk_cnt, %c-8_i4 : i4
    %78 = comb.icmp eq %tx_bclk_cnt, %c-7_i4 : i4
    %79 = comb.or %20, %77, %78 : i1
    %80 = llhd.prb %int_sir_out_n : i1
    %81 = comb.mux %sir_en, %72, %67 : i1
    %82 = comb.extract %char_info from 5 : (i6) -> i1
    %83 = comb.extract %char_info from 4 : (i6) -> i1
    %84 = comb.xor %83, %true : i1
    %85 = comb.extract %tx_data from 0 : (i8) -> i1
    %86 = comb.xor %85, %83, %true : i1
    %87 = comb.extract %tx_shift_reg from 1 : (i8) -> i1
    %88 = comb.xor %parity_gen, %87 : i1
    %89 = comb.xor %82, %true : i1
    %90 = comb.and %tx_start, %89 : i1
    %91 = comb.mux %90, %86, %84 : i1
    %92 = comb.xor %tx_start, %true : i1
    %93 = comb.and %92, %89 : i1
    %94 = comb.and %55, %93 : i1
    %95 = comb.mux %94, %88, %91 : i1
    %96 = comb.xor %55, %true : i1
    %97 = comb.and %93, %96 : i1
    %98 = comb.mux bin %97, %parity_gen, %95 : i1
    %parity_gen = seq.firreg %98 clock %14 reset async %15, %false : i1
    %99 = comb.extract %tx_shift_reg from 1 : (i8) -> i7
    %100 = comb.concat %false, %99 : i1, i7
    %101 = comb.and %55, %92 : i1
    %102 = comb.mux %101, %100, %tx_data : i8
    %103 = comb.or %tx_start, %55 : i1
    %104 = comb.mux bin %103, %102, %tx_shift_reg : i8
    %tx_shift_reg = seq.firreg %104 clock %14 reset async %15, %c0_i8 : i8
    hw.output %65, %81, %sout : i1, i1, i1
  }
  hw.module private @DW_apb_uart(in %pclk : i1, in %presetn : i1, in %penable : i1, in %pwrite : i1, in %pwdata : i32, in %paddr : i8, in %psel : i1, in %sclk : i1, in %s_rst_n : i1, in %rst_scan_mode : i1, in %cts_n : i1, in %dsr_n : i1, in %dcd_n : i1, in %ri_n : i1, in %sin : i1, out prdata : i32, out dtr_n : i1, out rts_n : i1, out out2_n : i1, out out1_n : i1, out dma_tx_req : i1, out dma_rx_req : i1, out txrdy_n : i1, out rxrdy_n : i1, out sout : i1, out intr : i1) {
    %false = hw.constant false {sv.namehint = "int_uart_lp_req"}
    %0 = comb.extract %U_DW_apb_uart_sync.sync_char_info from 0 {sv.namehint = "int_char_info"} : (i6) -> i4
    %U_DW_apb_uart_biu.prdata, %U_DW_apb_uart_biu.wr_en, %U_DW_apb_uart_biu.wr_enx, %U_DW_apb_uart_biu.rd_en, %U_DW_apb_uart_biu.byte_en, %U_DW_apb_uart_biu.reg_addr, %U_DW_apb_uart_biu.ipwdata = hw.instance "U_DW_apb_uart_biu" @DW_apb_uart_biu(pclk: %pclk: i1, presetn: %U_DW_apb_uart_rst.new_presetn: i1, psel: %psel: i1, penable: %penable: i1, pwrite: %pwrite: i1, paddr: %paddr: i8, pwdata: %pwdata: i32, iprdata: %U_DW_apb_uart_regfile.iprdata: i32) -> (prdata: i32, wr_en: i1, wr_enx: i1, rd_en: i1, byte_en: i4, reg_addr: i6, ipwdata: i32) {sv.namehint = "ipwdata"}
    %U_DW_apb_uart_rst.new_presetn, %U_DW_apb_uart_rst.new_s_rst_n = hw.instance "U_DW_apb_uart_rst" @DW_apb_uart_rst(presetn: %presetn: i1, s_rst_n: %s_rst_n: i1, scan_mode: %rst_scan_mode: i1) -> (new_presetn: i1, new_s_rst_n: i1) {sv.namehint = "new_presetn"}
    %U_DW_apb_uart_regfile.iprdata, %U_DW_apb_uart_regfile.tx_push, %U_DW_apb_uart_regfile.tx_pop, %U_DW_apb_uart_regfile.rx_push, %U_DW_apb_uart_regfile.rx_pop, %U_DW_apb_uart_regfile.tx_fifo_rst, %U_DW_apb_uart_regfile.rx_fifo_rst, %U_DW_apb_uart_regfile.tx_push_data, %U_DW_apb_uart_regfile.rx_push_data, %U_DW_apb_uart_regfile.tx_start, %U_DW_apb_uart_regfile.tx_data, %U_DW_apb_uart_regfile.lb_en_o, %U_DW_apb_uart_regfile.xbreak_o, %U_DW_apb_uart_regfile.cnt_ens_ed, %U_DW_apb_uart_regfile.to_det_cnt_ens, %U_DW_apb_uart_regfile.rx_pop_hld, %U_DW_apb_uart_regfile.rx_pop_hld_ed, %U_DW_apb_uart_regfile.divsr, %U_DW_apb_uart_regfile.divsr_wd, %U_DW_apb_uart_regfile.char_info, %U_DW_apb_uart_regfile.dtr_n, %U_DW_apb_uart_regfile.rts_n, %U_DW_apb_uart_regfile.out1_n, %U_DW_apb_uart_regfile.out2_n, %U_DW_apb_uart_regfile.dma_tx_req, %U_DW_apb_uart_regfile.dma_rx_req, %U_DW_apb_uart_regfile.dma_tx_req_n, %U_DW_apb_uart_regfile.dma_rx_req_n, %U_DW_apb_uart_regfile.char_info_wd, %U_DW_apb_uart_regfile.dlf, %U_DW_apb_uart_regfile.dlf_wd, %U_DW_apb_uart_regfile.intr = hw.instance "U_DW_apb_uart_regfile" @DW_apb_uart_regfile(pclk: %pclk: i1, presetn: %U_DW_apb_uart_rst.new_presetn: i1, wr_en: %U_DW_apb_uart_biu.wr_en: i1, wr_enx: %U_DW_apb_uart_biu.wr_enx: i1, rd_en: %U_DW_apb_uart_biu.rd_en: i1, byte_en: %U_DW_apb_uart_biu.byte_en: i4, reg_addr: %U_DW_apb_uart_biu.reg_addr: i6, ipwdata: %1: i10, tx_full: %U_DW_apb_uart_fifo.tx_full: i1, tx_empty: %U_DW_apb_uart_fifo.tx_empty: i1, rx_full: %U_DW_apb_uart_fifo.rx_full: i1, rx_empty: %U_DW_apb_uart_fifo.rx_empty: i1, rx_overflow: %U_DW_apb_uart_fifo.rx_overflow: i1, tx_pop_data: %U_DW_apb_uart_fifo.tx_pop_data: i8, rx_pop_data: %U_DW_apb_uart_fifo.rx_pop_data: i10, tx_finish: %U_DW_apb_uart_sync.sync_tx_finish: i1, rx_finish: %U_DW_apb_uart_sync.sync_rx_finish: i1, rx_data: %U_DW_apb_uart_sync.sync_rx_data: i10, char_to: %U_DW_apb_uart_sync.sync_char_to: i1, rx_pop_ack: %U_DW_apb_uart_sync.sync_rx_pop_ack: i1, cts_n: %U_DW_apb_uart_mc_sync.sync_cts_n: i1, dsr_n: %U_DW_apb_uart_mc_sync.sync_dsr_n: i1, dcd_n: %U_DW_apb_uart_mc_sync.sync_dcd_n: i1, ri_n: %U_DW_apb_uart_mc_sync.sync_ri_n: i1) -> (iprdata: i32, tx_push: i1, tx_pop: i1, rx_push: i1, rx_pop: i1, tx_fifo_rst: i1, rx_fifo_rst: i1, tx_push_data: i8, rx_push_data: i10, tx_start: i1, tx_data: i8, lb_en_o: i1, xbreak_o: i1, cnt_ens_ed: i3, to_det_cnt_ens: i3, rx_pop_hld: i1, rx_pop_hld_ed: i1, divsr: i16, divsr_wd: i1, char_info: i6, dtr_n: i1, rts_n: i1, out1_n: i1, out2_n: i1, dma_tx_req: i1, dma_rx_req: i1, dma_tx_req_n: i1, dma_rx_req_n: i1, char_info_wd: i1, dlf: i4, dlf_wd: i1, intr: i1) {sv.namehint = "dlf"}
    %U_DW_apb_uart_fifo.tx_full, %U_DW_apb_uart_fifo.tx_empty, %U_DW_apb_uart_fifo.rx_full, %U_DW_apb_uart_fifo.rx_empty, %U_DW_apb_uart_fifo.rx_overflow, %U_DW_apb_uart_fifo.tx_pop_data, %U_DW_apb_uart_fifo.rx_pop_data = hw.instance "U_DW_apb_uart_fifo" @DW_apb_uart_fifo(pclk: %pclk: i1, presetn: %U_DW_apb_uart_rst.new_presetn: i1, tx_push: %U_DW_apb_uart_regfile.tx_push: i1, tx_pop: %U_DW_apb_uart_regfile.tx_pop: i1, rx_push: %U_DW_apb_uart_regfile.rx_push: i1, rx_pop: %U_DW_apb_uart_regfile.rx_pop: i1, tx_fifo_rst: %U_DW_apb_uart_regfile.tx_fifo_rst: i1, rx_fifo_rst: %U_DW_apb_uart_regfile.rx_fifo_rst: i1, tx_push_data: %U_DW_apb_uart_regfile.tx_push_data: i8, rx_push_data: %U_DW_apb_uart_regfile.rx_push_data: i10) -> (tx_full: i1, tx_empty: i1, rx_full: i1, rx_empty: i1, rx_overflow: i1, tx_pop_data: i8, rx_pop_data: i10) {sv.namehint = "rx_full"}
    %U_DW_apb_uart_sync.sync_divsr_wd, %U_DW_apb_uart_sync.sync_break, %U_DW_apb_uart_sync.sync_lb_en, %U_DW_apb_uart_sync.sync_tx_start, %U_DW_apb_uart_sync.sync_tx_finish, %U_DW_apb_uart_sync.sync_rx_finish, %U_DW_apb_uart_sync.sync_char_to, %U_DW_apb_uart_sync.sync_rx_pop_ack, %U_DW_apb_uart_sync.sync_divsr, %U_DW_apb_uart_sync.sync_char_info, %U_DW_apb_uart_sync.sync_tx_data, %U_DW_apb_uart_sync.sync_to_det_cnt_ens, %U_DW_apb_uart_sync.sync_rx_pop_hld, %U_DW_apb_uart_sync.sync_dlf, %U_DW_apb_uart_sync.sync_dlf_wd, %U_DW_apb_uart_sync.sync_rx_data = hw.instance "U_DW_apb_uart_sync" @DW_apb_uart_sync(pclk: %pclk: i1, presetn: %U_DW_apb_uart_rst.new_presetn: i1, sclk: %sclk: i1, s_rst_n: %U_DW_apb_uart_rst.new_s_rst_n: i1, divsr_wd: %U_DW_apb_uart_regfile.divsr_wd: i1, xbreak: %U_DW_apb_uart_regfile.xbreak_o: i1, lb_en: %U_DW_apb_uart_regfile.lb_en_o: i1, tx_start: %U_DW_apb_uart_regfile.tx_start: i1, tx_finish: %U_DW_apb_uart_tx.tx_finish: i1, rx_finish: %U_DW_apb_uart_rx.rx_finish: i1, char_to: %U_DW_apb_uart_to_det.char_to: i1, rx_pop_ack: %U_DW_apb_uart_to_det.rx_pop_ack: i1, rx_pop_ack_ed: %U_DW_apb_uart_to_det.rx_pop_ack_ed: i1, cnt_ens_ed: %U_DW_apb_uart_regfile.cnt_ens_ed: i3, char_info_wd: %U_DW_apb_uart_regfile.char_info_wd: i1, divsr: %U_DW_apb_uart_regfile.divsr: i16, char_info: %U_DW_apb_uart_regfile.char_info: i6, tx_data: %U_DW_apb_uart_regfile.tx_data: i8, rx_data: %U_DW_apb_uart_rx.rx_data: i10, to_det_cnt_ens: %U_DW_apb_uart_regfile.to_det_cnt_ens: i3, rx_pop_hld: %U_DW_apb_uart_regfile.rx_pop_hld: i1, rx_pop_hld_ed: %U_DW_apb_uart_regfile.rx_pop_hld_ed: i1, dlf: %U_DW_apb_uart_regfile.dlf: i4, dlf_wd: %U_DW_apb_uart_regfile.dlf_wd: i1) -> (sync_divsr_wd: i1, sync_break: i1, sync_lb_en: i1, sync_tx_start: i1, sync_tx_finish: i1, sync_rx_finish: i1, sync_char_to: i1, sync_rx_pop_ack: i1, sync_divsr: i16, sync_char_info: i6, sync_tx_data: i8, sync_to_det_cnt_ens: i3, sync_rx_pop_hld: i1, sync_dlf: i4, sync_dlf_wd: i1, sync_rx_data: i10) {sv.namehint = "sync_dlf"}
    %U_DW_apb_uart_mc_sync.sync_cts_n, %U_DW_apb_uart_mc_sync.sync_dsr_n, %U_DW_apb_uart_mc_sync.sync_dcd_n, %U_DW_apb_uart_mc_sync.sync_ri_n = hw.instance "U_DW_apb_uart_mc_sync" @DW_apb_uart_mc_sync(clk: %pclk: i1, resetn: %U_DW_apb_uart_rst.new_presetn: i1, cts_n: %cts_n: i1, dsr_n: %dsr_n: i1, dcd_n: %dcd_n: i1, ri_n: %ri_n: i1) -> (sync_cts_n: i1, sync_dsr_n: i1, sync_dcd_n: i1, sync_ri_n: i1) {sv.namehint = "sync_ri_n"}
    %U_DW_apb_uart_bclk_gen.bclk = hw.instance "U_DW_apb_uart_bclk_gen" @DW_apb_uart_bclk_gen(sclk: %sclk: i1, s_rst_n: %U_DW_apb_uart_rst.new_s_rst_n: i1, divisor: %U_DW_apb_uart_sync.sync_divsr: i16, divisor_wd: %U_DW_apb_uart_sync.sync_divsr_wd: i1, uart_lp_req: %false: i1, dlf: %U_DW_apb_uart_sync.sync_dlf: i4, dlf_wd: %U_DW_apb_uart_sync.sync_dlf_wd: i1) -> (bclk: i1) {sv.namehint = "bclk"}
    %U_DW_apb_uart_tx.tx_finish, %U_DW_apb_uart_tx.ser_out_lb, %U_DW_apb_uart_tx.sout = hw.instance "U_DW_apb_uart_tx" @DW_apb_uart_tx(sclk: %sclk: i1, s_rst_n: %U_DW_apb_uart_rst.new_s_rst_n: i1, bclk: %U_DW_apb_uart_bclk_gen.bclk: i1, tx_start: %U_DW_apb_uart_sync.sync_tx_start: i1, tx_data: %U_DW_apb_uart_sync.sync_tx_data: i8, char_info: %U_DW_apb_uart_sync.sync_char_info: i6, xbreak: %U_DW_apb_uart_sync.sync_break: i1, lb_en: %U_DW_apb_uart_sync.sync_lb_en: i1, sir_en: %false: i1) -> (tx_finish: i1, ser_out_lb: i1, sout: i1) {sv.namehint = "tx_finish"}
    %U_DW_apb_uart_rx.rx_finish, %U_DW_apb_uart_rx.rx_data = hw.instance "U_DW_apb_uart_rx" @DW_apb_uart_rx(sclk: %sclk: i1, s_rst_n: %U_DW_apb_uart_rst.new_s_rst_n: i1, bclk: %U_DW_apb_uart_bclk_gen.bclk: i1, sin: %sin: i1, sir_in: %false: i1, char_info: %U_DW_apb_uart_sync.sync_char_info: i6, sir_en: %false: i1, lb_en: %U_DW_apb_uart_sync.sync_lb_en: i1, ser_out_lb: %U_DW_apb_uart_tx.ser_out_lb: i1, divisor: %U_DW_apb_uart_sync.sync_divsr: i16) -> (rx_finish: i1, rx_data: i10) {sv.namehint = "rx_data"}
    %U_DW_apb_uart_to_det.char_to, %U_DW_apb_uart_to_det.rx_pop_ack, %U_DW_apb_uart_to_det.rx_pop_ack_ed = hw.instance "U_DW_apb_uart_to_det" @DW_apb_uart_to_det(sclk: %sclk: i1, s_rst_n: %U_DW_apb_uart_rst.new_s_rst_n: i1, bclk: %U_DW_apb_uart_bclk_gen.bclk: i1, rx_pop_hld: %U_DW_apb_uart_sync.sync_rx_pop_hld: i1, rx_finish: %U_DW_apb_uart_rx.rx_finish: i1, char_info: %0: i4, to_det_cnt_ens: %U_DW_apb_uart_sync.sync_to_det_cnt_ens: i3) -> (char_to: i1, rx_pop_ack: i1, rx_pop_ack_ed: i1) {sv.namehint = "char_to"}
    %1 = comb.extract %U_DW_apb_uart_biu.ipwdata from 0 {sv.namehint = "ipwdata_int"} : (i32) -> i10
    hw.output %U_DW_apb_uart_biu.prdata, %U_DW_apb_uart_regfile.dtr_n, %U_DW_apb_uart_regfile.rts_n, %U_DW_apb_uart_regfile.out2_n, %U_DW_apb_uart_regfile.out1_n, %U_DW_apb_uart_regfile.dma_tx_req, %U_DW_apb_uart_regfile.dma_rx_req, %U_DW_apb_uart_regfile.dma_tx_req_n, %U_DW_apb_uart_regfile.dma_rx_req_n, %U_DW_apb_uart_tx.sout, %U_DW_apb_uart_regfile.intr : i32, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1
  }
  hw.module private @DW_apb_uart_rx(in %sclk : i1, in %s_rst_n : i1, in %bclk : i1, in %sin : i1, in %sir_in : i1, in %char_info : i6, in %sir_en : i1, in %lb_en : i1, in %ser_out_lb : i1, in %divisor : i16, out rx_finish : i1, out rx_data : i10) {
    %c-1_i3 = hw.constant -1 : i3
    %true = hw.constant true
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %c0_i10 = hw.constant 0 : i10
    %c0_i2 = hw.constant 0 : i2
    %c0_i3 = hw.constant 0 : i3
    %c7_i4 = hw.constant 7 : i4
    %c-2_i2 = hw.constant -2 : i2
    %c6_i4 = hw.constant 6 : i4
    %c1_i2 = hw.constant 1 : i2
    %c5_i4 = hw.constant 5 : i4
    %c-1_i4 = hw.constant -1 : i4
    %c-2_i4 = hw.constant -2 : i4
    %c4_i4 = hw.constant 4 : i4
    %c3_i4 = hw.constant 3 : i4
    %c2_i4 = hw.constant 2 : i4
    %c1_i4 = hw.constant 1 : i4
    %c-6_i4 = hw.constant -6 : i4
    %c-7_i4 = hw.constant -7 : i4
    %false = hw.constant false
    %c1_i16 = hw.constant 1 : i16
    %c-1_i2 = hw.constant -1 : i2
    %c-8_i4 = hw.constant -8 : i4
    %c0_i4 = hw.constant 0 : i4
    %parity_err = llhd.sig %false : i1
    %n_state = llhd.sig %c0_i4 : i4
    %n_state_break = llhd.sig %c0_i2 : i2
    %1 = comb.icmp eq %c_state, %c-8_i4 : i4
    %2 = comb.icmp eq %c_state_break, %c-1_i2 : i2
    %3 = comb.and %sir_en, %2 : i1
    %4 = comb.xor %3, %true : i1
    %5 = comb.xor %42, %true : i1
    %6 = comb.and %1, %4, %5 : i1
    %7 = comb.icmp eq %rx_bclk_cnt, %c-1_i4 : i4
    %8 = comb.icmp eq %divisor, %c1_i16 : i16
    %9 = comb.add %rx_bclk_cnt, %c1_i4 : i4
    %10 = comb.concat %c-1_i3, %8 : i3, i1
    %11 = comb.xor %6, %true : i1
    %12 = comb.and %5, %11 : i1
    %13 = comb.and %bclk, %12 : i1
    %14 = comb.and %13, %7 : i1
    %15 = comb.mux %14, %c0_i4, %10 : i4
    %16 = comb.xor %sir_en, %true : i1
    %17 = comb.and %42, %11, %16 : i1
    %18 = comb.mux %17, %c-8_i4, %15 : i4
    %19 = comb.mux %6, %c0_i4, %18 : i4
    %20 = comb.xor %7, %true : i1
    %21 = comb.and %20, %13 : i1
    %22 = comb.mux %21, %9, %19 : i4
    %23 = comb.xor %bclk, %true : i1
    %24 = comb.and %12, %23 : i1
    %25 = seq.to_clock %sclk
    %26 = comb.xor %s_rst_n, %true : i1
    %27 = comb.mux bin %24, %rx_bclk_cnt, %22 : i4
    %rx_bclk_cnt = seq.firreg %27 clock %25 reset async %26, %c0_i4 : i4
    %28 = comb.and %7, %bclk : i1
    %dly_cnt16 = seq.firreg %28 clock %25 reset async %26, %false : i1
    %29 = comb.mux %sir_en, %sir_in, %sin {sv.namehint = "ser_in"} : i1
    %U_DW_apb_uart_bcm41_async2sl_ser_in_ssyzr.data_d = hw.instance "U_DW_apb_uart_bcm41_async2sl_ser_in_ssyzr" @DW_apb_uart_bcm41(clk_d: %sclk: i1, rst_d_n: %s_rst_n: i1, data_s: %29: i1) -> (data_d: i1) {sv.namehint = "saync2sl_sync2"}
    %30 = comb.mux %lb_en, %ser_out_lb, %U_DW_apb_uart_bcm41_async2sl_ser_in_ssyzr.data_d : i1
    %31 = comb.and %di_reg1, %di_reg2 : i1
    %32 = comb.and %di_reg2, %di_reg3 : i1
    %33 = comb.and %di_reg1, %di_reg3 : i1
    %34 = comb.or %31, %32, %33 : i1
    %35 = comb.mux bin %bclk, %30, %di_reg1 : i1
    %di_reg1 = seq.firreg %35 clock %25 reset async %26, %true : i1
    %36 = comb.mux bin %bclk, %di_reg1, %di_reg2 : i1
    %di_reg2 = seq.firreg %36 clock %25 reset async %26, %true : i1
    %37 = comb.mux bin %bclk, %di_reg2, %di_reg3 : i1
    %di_reg3 = seq.firreg %37 clock %25 reset async %26, %true : i1
    %38 = comb.mux %74, %c-8_i4, %40 : i4
    %c_state = seq.firreg %38 clock %25 reset async %26, %c-8_i4 : i4
    %39:2 = llhd.process -> i4, i1 {
      cf.br ^bb1(%c0_i4, %false : i4, i1)
    ^bb1(%104: i4, %105: i1):  // 35 preds: ^bb0, ^bb3, ^bb4, ^bb4, ^bb6, ^bb6, ^bb8, ^bb8, ^bb10, ^bb10, ^bb12, ^bb12, ^bb14, ^bb14, ^bb16, ^bb17, ^bb18, ^bb18, ^bb20, ^bb21, ^bb22, ^bb22, ^bb24, ^bb25, ^bb26, ^bb26, ^bb28, ^bb29, ^bb29, ^bb31, ^bb31, ^bb33, ^bb33, ^bb34, ^bb34
      llhd.wait yield (%104, %105 : i4, i1), (%c_state, %34, %28, %char_info, %c_state_break, %28 : i4, i1, i1, i6, i2, i1), ^bb2
    ^bb2:  // pred: ^bb1
      %106 = comb.icmp ceq %c_state, %c-7_i4 : i4
      cf.cond_br %106, ^bb3, ^bb5
    ^bb3:  // pred: ^bb2
      cf.cond_br %28, ^bb4, ^bb1(%c-7_i4, %true : i4, i1)
    ^bb4:  // pred: ^bb3
      %107 = comb.xor %34, %true : i1
      cf.cond_br %107, ^bb1(%c-6_i4, %true : i4, i1), ^bb1(%c-8_i4, %true : i4, i1)
    ^bb5:  // pred: ^bb2
      %108 = comb.icmp ceq %c_state, %c-6_i4 : i4
      cf.cond_br %108, ^bb6, ^bb7
    ^bb6:  // pred: ^bb5
      cf.cond_br %28, ^bb1(%c0_i4, %true : i4, i1), ^bb1(%c-6_i4, %true : i4, i1)
    ^bb7:  // pred: ^bb5
      %109 = comb.icmp ceq %c_state, %c0_i4 : i4
      cf.cond_br %109, ^bb8, ^bb9
    ^bb8:  // pred: ^bb7
      cf.cond_br %28, ^bb1(%c1_i4, %true : i4, i1), ^bb1(%c0_i4, %true : i4, i1)
    ^bb9:  // pred: ^bb7
      %110 = comb.icmp ceq %c_state, %c1_i4 : i4
      cf.cond_br %110, ^bb10, ^bb11
    ^bb10:  // pred: ^bb9
      cf.cond_br %28, ^bb1(%c2_i4, %true : i4, i1), ^bb1(%c1_i4, %true : i4, i1)
    ^bb11:  // pred: ^bb9
      %111 = comb.icmp ceq %c_state, %c2_i4 : i4
      cf.cond_br %111, ^bb12, ^bb13
    ^bb12:  // pred: ^bb11
      cf.cond_br %28, ^bb1(%c3_i4, %true : i4, i1), ^bb1(%c2_i4, %true : i4, i1)
    ^bb13:  // pred: ^bb11
      %112 = comb.icmp ceq %c_state, %c3_i4 : i4
      cf.cond_br %112, ^bb14, ^bb15
    ^bb14:  // pred: ^bb13
      cf.cond_br %28, ^bb1(%c4_i4, %true : i4, i1), ^bb1(%c3_i4, %true : i4, i1)
    ^bb15:  // pred: ^bb13
      %113 = comb.icmp ceq %c_state, %c4_i4 : i4
      cf.cond_br %113, ^bb16, ^bb19
    ^bb16:  // pred: ^bb15
      cf.cond_br %28, ^bb17, ^bb1(%c4_i4, %true : i4, i1)
    ^bb17:  // pred: ^bb16
      %114 = comb.extract %char_info from 0 : (i6) -> i2
      %115 = comb.icmp eq %114, %c0_i2 : i2
      cf.cond_br %115, ^bb18, ^bb1(%c5_i4, %true : i4, i1)
    ^bb18:  // pred: ^bb17
      %116 = comb.extract %char_info from 3 : (i6) -> i1
      cf.cond_br %116, ^bb1(%c-2_i4, %true : i4, i1), ^bb1(%c-1_i4, %true : i4, i1)
    ^bb19:  // pred: ^bb15
      %117 = comb.icmp ceq %c_state, %c5_i4 : i4
      cf.cond_br %117, ^bb20, ^bb23
    ^bb20:  // pred: ^bb19
      cf.cond_br %28, ^bb21, ^bb1(%c5_i4, %true : i4, i1)
    ^bb21:  // pred: ^bb20
      %118 = comb.extract %char_info from 0 : (i6) -> i2
      %119 = comb.icmp eq %118, %c1_i2 : i2
      cf.cond_br %119, ^bb22, ^bb1(%c6_i4, %true : i4, i1)
    ^bb22:  // pred: ^bb21
      %120 = comb.extract %char_info from 3 : (i6) -> i1
      cf.cond_br %120, ^bb1(%c-2_i4, %true : i4, i1), ^bb1(%c-1_i4, %true : i4, i1)
    ^bb23:  // pred: ^bb19
      %121 = comb.icmp ceq %c_state, %c6_i4 : i4
      cf.cond_br %121, ^bb24, ^bb27
    ^bb24:  // pred: ^bb23
      cf.cond_br %28, ^bb25, ^bb1(%c6_i4, %true : i4, i1)
    ^bb25:  // pred: ^bb24
      %122 = comb.extract %char_info from 0 : (i6) -> i2
      %123 = comb.icmp eq %122, %c-2_i2 : i2
      cf.cond_br %123, ^bb26, ^bb1(%c7_i4, %true : i4, i1)
    ^bb26:  // pred: ^bb25
      %124 = comb.extract %char_info from 3 : (i6) -> i1
      cf.cond_br %124, ^bb1(%c-2_i4, %true : i4, i1), ^bb1(%c-1_i4, %true : i4, i1)
    ^bb27:  // pred: ^bb23
      %125 = comb.icmp ceq %c_state, %c7_i4 : i4
      cf.cond_br %125, ^bb28, ^bb30
    ^bb28:  // pred: ^bb27
      cf.cond_br %28, ^bb29, ^bb1(%c7_i4, %true : i4, i1)
    ^bb29:  // pred: ^bb28
      %126 = comb.extract %char_info from 3 : (i6) -> i1
      cf.cond_br %126, ^bb1(%c-2_i4, %true : i4, i1), ^bb1(%c-1_i4, %true : i4, i1)
    ^bb30:  // pred: ^bb27
      %127 = comb.icmp ceq %c_state, %c-2_i4 : i4
      cf.cond_br %127, ^bb31, ^bb32
    ^bb31:  // pred: ^bb30
      cf.cond_br %28, ^bb1(%c-1_i4, %true : i4, i1), ^bb1(%c-2_i4, %true : i4, i1)
    ^bb32:  // pred: ^bb30
      %128 = comb.icmp ceq %c_state, %c-1_i4 : i4
      cf.cond_br %128, ^bb33, ^bb34
    ^bb33:  // pred: ^bb32
      %129 = comb.xor %34, %true : i1
      cf.cond_br %129, ^bb1(%c-6_i4, %true : i4, i1), ^bb1(%c-8_i4, %true : i4, i1)
    ^bb34:  // pred: ^bb32
      %130 = comb.xor %34, %true : i1
      %131 = comb.icmp ne %c_state_break, %c-1_i2 : i2
      %132 = comb.and %130, %131 : i1
      cf.cond_br %132, ^bb1(%c-7_i4, %true : i4, i1), ^bb1(%c-8_i4, %true : i4, i1)
    }
    llhd.drv %n_state, %39#0 after %0 if %39#1 : i4
    %40 = llhd.prb %n_state : i4
    %41 = comb.icmp eq %40, %c-7_i4 : i4
    %42 = comb.and %1, %41 : i1
    %43 = comb.icmp eq %c_state, %c-6_i4 : i4
    %44 = comb.icmp eq %c_state, %c0_i4 : i4
    %45 = comb.icmp eq %rx_bclk_cnt, %c0_i4 : i4
    %46 = comb.and %45, %bclk : i1
    %47 = comb.and %44, %46 : i1
    %48 = comb.icmp eq %c_state, %c1_i4 : i4
    %49 = comb.and %48, %46 : i1
    %50 = comb.icmp eq %c_state, %c2_i4 : i4
    %51 = comb.and %50, %46 : i1
    %52 = comb.icmp eq %c_state, %c3_i4 : i4
    %53 = comb.and %52, %46 : i1
    %54 = comb.icmp eq %c_state, %c4_i4 : i4
    %55 = comb.and %54, %46 : i1
    %56 = comb.icmp eq %c_state, %c5_i4 : i4
    %57 = comb.and %56, %46 : i1
    %58 = comb.icmp eq %c_state, %c6_i4 : i4
    %59 = comb.and %58, %46 : i1
    %60 = comb.icmp eq %c_state, %c7_i4 : i4
    %61 = comb.and %60, %46 : i1
    %62 = comb.or %47, %49, %51, %53, %55, %57, %59, %61 : i1
    %63 = comb.icmp eq %c_state, %c-2_i4 : i4
    %64 = comb.and %63, %45 : i1
    %65 = comb.icmp eq %c_state, %c-1_i4 : i4
    %66 = comb.xor %34, %true : i1
    %67 = comb.extract %rx_shift_reg from 9 : (i10) -> i1
    %68 = comb.mux %65, %66, %67 : i1
    %69 = comb.or %16, %28, %dly_cnt16 : i1
    %70 = comb.and %69, %34 : i1
    %c_state_break = seq.firreg %72 clock %25 reset async %26, %c0_i2 : i2
    %71:2 = llhd.process -> i2, i1 {
      cf.br ^bb1(%c0_i2, %false : i2, i1)
    ^bb1(%104: i2, %105: i1):  // 11 preds: ^bb0, ^bb3, ^bb3, ^bb6, ^bb6, ^bb7, ^bb7, ^bb9, ^bb9, ^bb10, ^bb10
      llhd.wait yield (%104, %105 : i2, i1), (%c_state_break, %43, %70, %65 : i2, i1, i1, i1), ^bb2
    ^bb2:  // pred: ^bb1
      %106 = comb.icmp ceq %c_state_break, %c0_i2 : i2
      cf.cond_br %106, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      cf.cond_br %43, ^bb1(%c1_i2, %true : i2, i1), ^bb1(%c0_i2, %true : i2, i1)
    ^bb4:  // pred: ^bb2
      %107 = comb.icmp ceq %c_state_break, %c1_i2 : i2
      cf.cond_br %107, ^bb5, ^bb8
    ^bb5:  // pred: ^bb4
      cf.cond_br %65, ^bb6, ^bb7
    ^bb6:  // pred: ^bb5
      %108 = comb.xor %70, %true : i1
      cf.cond_br %108, ^bb1(%c-1_i2, %true : i2, i1), ^bb1(%c0_i2, %true : i2, i1)
    ^bb7:  // pred: ^bb5
      %109 = comb.xor %70, %true : i1
      cf.cond_br %109, ^bb1(%c1_i2, %true : i2, i1), ^bb1(%c-2_i2, %true : i2, i1)
    ^bb8:  // pred: ^bb4
      %110 = comb.icmp ceq %c_state_break, %c-2_i2 : i2
      cf.cond_br %110, ^bb9, ^bb10
    ^bb9:  // pred: ^bb8
      cf.cond_br %65, ^bb1(%c0_i2, %true : i2, i1), ^bb1(%c-2_i2, %true : i2, i1)
    ^bb10:  // pred: ^bb8
      %111 = comb.xor %70, %true : i1
      cf.cond_br %111, ^bb1(%c-1_i2, %true : i2, i1), ^bb1(%c0_i2, %true : i2, i1)
    }
    llhd.drv %n_state_break, %71#0 after %0 if %71#1 : i2
    %72 = llhd.prb %n_state_break : i2
    %73 = comb.icmp eq %72, %c-1_i2 : i2
    %74 = comb.or %73, %2 : i1
    %75:2 = llhd.process -> i1, i1 {
      cf.br ^bb1(%false, %false : i1, i1)
    ^bb1(%104: i1, %105: i1):  // 5 preds: ^bb0, ^bb2, ^bb5, ^bb6, ^bb7
      llhd.wait yield (%104, %105 : i1, i1), (%char_info, %64, %rx_shift_reg, %34 : i6, i1, i10, i1), ^bb2
    ^bb2:  // pred: ^bb1
      %106 = comb.extract %char_info from 3 : (i6) -> i1
      cf.cond_br %106, ^bb3, ^bb1(%false, %true : i1, i1)
    ^bb3:  // pred: ^bb2
      cf.cond_br %64, ^bb4, ^bb7
    ^bb4:  // pred: ^bb3
      %107 = comb.extract %char_info from 5 : (i6) -> i1
      cf.cond_br %107, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      %108 = comb.extract %char_info from 4 : (i6) -> i1
      %109 = comb.xor %34, %108, %true : i1
      cf.br ^bb1(%109, %true : i1, i1)
    ^bb6:  // pred: ^bb4
      %110 = comb.extract %rx_shift_reg from 0 : (i10) -> i8
      %111 = comb.extract %char_info from 4 : (i6) -> i1
      %112 = comb.xor %111, %true : i1
      %113 = comb.concat %110, %112 : i8, i1
      %114 = comb.parity %113 : i9
      %115 = comb.xor %34, %114 : i1
      cf.br ^bb1(%115, %true : i1, i1)
    ^bb7:  // pred: ^bb3
      %116 = comb.extract %rx_shift_reg from 8 : (i10) -> i1
      cf.br ^bb1(%116, %true : i1, i1)
    }
    llhd.drv %parity_err, %75#0 after %0 if %75#1 : i1
    %76 = comb.extract %char_info from 0 : (i6) -> i2
    %77 = comb.extract %rx_shift_reg from 1 : (i10) -> i4
    %78 = comb.icmp ceq %76, %c1_i2 : i2
    %79 = comb.extract %rx_shift_reg from 1 : (i10) -> i5
    %80 = comb.icmp ceq %76, %c-2_i2 : i2
    %81 = comb.extract %rx_shift_reg from 1 : (i10) -> i6
    %82 = comb.extract %rx_shift_reg from 1 : (i10) -> i7
    %83 = comb.icmp cne %76, %c0_i2 : i2
    %84 = comb.and %83, %62 : i1
    %85 = comb.and %78, %84 : i1
    %86 = comb.concat %c0_i2, %34, %79 : i2, i1, i5
    %87 = comb.concat %c0_i3, %34, %77 : i3, i1, i4
    %88 = comb.mux %85, %86, %87 : i8
    %89 = comb.xor %78, %true : i1
    %90 = comb.and %89, %84 : i1
    %91 = comb.and %80, %90 : i1
    %92 = comb.concat %false, %34, %81 : i1, i1, i6
    %93 = comb.mux %91, %92, %88 : i8
    %94 = comb.xor %80, %true : i1
    %95 = comb.and %94, %90 : i1
    %96 = comb.concat %34, %82 : i1, i7
    %97 = comb.mux %95, %96, %93 : i8
    %98 = comb.xor %62, %true : i1
    %99 = comb.extract %rx_shift_reg from 0 : (i10) -> i8
    %100 = comb.mux %98, %99, %97 : i8
    %101 = comb.concat %68, %102, %100 : i1, i1, i8
    %rx_shift_reg = seq.firreg %101 clock %25 reset async %26, %c0_i10 : i10
    %102 = llhd.prb %parity_err : i1
    %103 = comb.concat %68, %102, %99 : i1, i1, i8
    hw.output %65, %103 : i1, i10
  }
  hw.module private @DW_apb_uart_regfile(in %pclk : i1, in %presetn : i1, in %wr_en : i1, in %wr_enx : i1, in %rd_en : i1, in %byte_en : i4, in %reg_addr : i6, in %ipwdata : i10, out iprdata : i32, in %tx_full : i1, in %tx_empty : i1, in %rx_full : i1, in %rx_empty : i1, in %rx_overflow : i1, in %tx_pop_data : i8, in %rx_pop_data : i10, out tx_push : i1, out tx_pop : i1, out rx_push : i1, out rx_pop : i1, out tx_fifo_rst : i1, out rx_fifo_rst : i1, out tx_push_data : i8, out rx_push_data : i10, in %tx_finish : i1, out tx_start : i1, out tx_data : i8, in %rx_finish : i1, in %rx_data : i10, out lb_en_o : i1, out xbreak_o : i1, in %char_to : i1, in %rx_pop_ack : i1, out cnt_ens_ed : i3, out to_det_cnt_ens : i3, out rx_pop_hld : i1, out rx_pop_hld_ed : i1, out divsr : i16, out divsr_wd : i1, out char_info : i6, in %cts_n : i1, in %dsr_n : i1, in %dcd_n : i1, in %ri_n : i1, out dtr_n : i1, out rts_n : i1, out out1_n : i1, out out2_n : i1, out dma_tx_req : i1, out dma_rx_req : i1, out dma_tx_req_n : i1, out dma_rx_req_n : i1, out char_info_wd : i1, out dlf : i4, out dlf_wd : i1, out intr : i1) {
    %c0_i27 = hw.constant 0 : i27
    %c0_i25 = hw.constant 0 : i25
    %c0_i28 = hw.constant 0 : i28
    %c0_i26 = hw.constant 0 : i26
    %c0_i31 = hw.constant 0 : i31
    %c0_i24 = hw.constant 0 : i24
    %true = hw.constant true
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %c0_i7 = hw.constant 0 : i7
    %c0_i10 = hw.constant 0 : i10
    %c16_i6 = hw.constant 16 : i6
    %c8_i6 = hw.constant 8 : i6
    %c1146552592_i32 = hw.constant 1146552592 : i32
    %c875574570_i32 = hw.constant 875574570 : i32
    %c0_i2 = hw.constant 0 : i2
    %c0_i8 = hw.constant 0 : i8
    %c0_i32 = hw.constant 0 : i32
    %c-2_i2 = hw.constant -2 : i2
    %c1_i2 = hw.constant 1 : i2
    %c1_i4 = hw.constant 1 : i4
    %c7_i4 = hw.constant 7 : i4
    %c0_i4 = hw.constant 0 : i4
    %c2_i4 = hw.constant 2 : i4
    %c-4_i4 = hw.constant -4 : i4
    %c4_i4 = hw.constant 4 : i4
    %c6_i4 = hw.constant 6 : i4
    %c-1_i2 = hw.constant -1 : i2
    %c0_i3 = hw.constant 0 : i3
    %c0_i16 = hw.constant 0 : i16
    %false = hw.constant false
    %c-16_i6 = hw.constant -16 : i6
    %c-1_i6 = hw.constant -1 : i6
    %c-2_i6 = hw.constant -2 : i6
    %c-3_i6 = hw.constant -3 : i6
    %c-23_i6 = hw.constant -23 : i6
    %c-31_i6 = hw.constant -31 : i6
    %c-32_i6 = hw.constant -32 : i6
    %c31_i6 = hw.constant 31 : i6
    %c30_i6 = hw.constant 30 : i6
    %c29_i6 = hw.constant 29 : i6
    %c28_i6 = hw.constant 28 : i6
    %c7_i6 = hw.constant 7 : i6
    %c6_i6 = hw.constant 6 : i6
    %c5_i6 = hw.constant 5 : i6
    %c4_i6 = hw.constant 4 : i6
    %c3_i6 = hw.constant 3 : i6
    %c2_i6 = hw.constant 2 : i6
    %c1_i6 = hw.constant 1 : i6
    %c0_i6 = hw.constant 0 : i6
    %1 = comb.extract %lcr_ir from 0 : (i8) -> i6
    %2 = comb.concat %false, %114 : i1, i7
    %iir = llhd.sig %c0_i8 : i8
    %3 = llhd.prb %iir : i8
    %4 = comb.concat %c0_i2, %180 : i2, i5
    %5 = comb.concat %270, %231, %thr_empty, %271, %272, %273, %274, %58 : i1, i1, i1, i1, i1, i1, i1, i1
    %6 = comb.concat %c0_i3, %rx_full, %306, %tx_empty, %307, %false : i3, i1, i1, i1, i1, i1
    %tfl_ir = llhd.sig %c0_i6 : i6
    %rfl_ir = llhd.sig %c0_i6 : i6
    %int_thre_intr = llhd.sig %false : i1
    %iprdata = llhd.sig %c0_i32 : i32
    %rx_fifo_trig = llhd.sig %c0_i6 : i6
    %tx_empty_trig = llhd.sig %c0_i6 : i6
    %7 = llhd.prb %tx_empty_trig : i6
    %int_tfl_cnt = llhd.sig %c0_i6 : i6
    %int_rfl_cnt = llhd.sig %c0_i6 : i6
    %8 = comb.icmp eq %reg_addr, %c0_i6 : i6
    %9 = comb.xor %173, %true : i1
    %10 = comb.and %8, %9 : i1
    %11 = comb.and %10, %rd_en : i1
    %12 = comb.and %8, %173 : i1
    %13 = comb.icmp eq %reg_addr, %c1_i6 : i6
    %14 = comb.and %13, %173 : i1
    %15 = comb.and %13, %9 : i1
    %16 = comb.icmp eq %reg_addr, %c2_i6 : i6
    %17 = comb.extract %byte_en from 0 : (i4) -> i1
    %18 = comb.and %16, %wr_en, %17 : i1
    %19 = comb.icmp eq %reg_addr, %c3_i6 : i6
    %20 = comb.icmp eq %reg_addr, %c4_i6 : i6
    %21 = comb.icmp eq %reg_addr, %c5_i6 : i6
    %22 = comb.icmp eq %reg_addr, %c6_i6 : i6
    %23 = comb.icmp eq %reg_addr, %c7_i6 : i6
    %24 = comb.icmp eq %reg_addr, %c28_i6 : i6
    %25 = comb.icmp eq %reg_addr, %c29_i6 : i6
    %26 = comb.and %25, %rd_en : i1
    %27 = comb.icmp eq %reg_addr, %c30_i6 : i6
    %28 = comb.and %27, %wr_enx : i1
    %29 = comb.icmp eq %reg_addr, %c31_i6 : i6
    %30 = comb.icmp eq %reg_addr, %c-32_i6 : i6
    %31 = comb.icmp eq %reg_addr, %c-31_i6 : i6
    %32 = comb.icmp eq %reg_addr, %c-23_i6 : i6
    %33 = comb.icmp eq %reg_addr, %c-3_i6 : i6
    %34 = comb.icmp eq %reg_addr, %c-2_i6 : i6
    %35 = comb.icmp eq %reg_addr, %c-1_i6 : i6
    %36 = comb.icmp eq %reg_addr, %c-16_i6 : i6
    %37 = comb.and %12, %wr_en, %17 : i1
    %38 = comb.and %14, %wr_en, %17 : i1
    %39 = comb.and %15, %wr_en, %17 : i1
    %40 = comb.and %19, %wr_en, %17 : i1
    %41 = comb.and %20, %wr_enx, %17 : i1
    %42 = comb.and %23, %wr_en, %17 : i1
    %43 = comb.and %24, %wr_en, %17 : i1
    %44 = comb.and %32, %wr_en, %17 : i1
    %45 = comb.and %36, %wr_en, %17 : i1
    %46 = comb.mux %far_ir, %28, %48 : i1
    %47 = comb.xor %dly_rx_finish, %true : i1
    %48 = comb.and %rx_finish, %47 : i1
    %49 = seq.to_clock %pclk
    %50 = comb.xor %presetn, %true : i1
    %dly_rx_finish = seq.firreg %rx_finish clock %49 reset async %50, %false : i1
    %51 = comb.xor %170, %true : i1
    %52 = comb.and %51, %46 : i1
    %53 = comb.mux bin %52, %54, %rbr_ir : i10
    %rbr_ir = seq.firreg %53 clock %49 reset async %50, %c0_i10 : i10
    %54 = comb.mux %far_ir, %ipwdata, %rx_data : i10
    %55 = comb.extract %rx_pop_data from 9 : (i10) -> i1
    %56 = comb.extract %rbr_ir from 9 : (i10) -> i1
    %57 = comb.mux %170, %55, %56 : i1
    %58 = comb.xor %68, %true {sv.namehint = "dr"} : i1
    %59 = comb.and %57, %58 : i1
    %60 = comb.extract %rx_pop_data from 8 : (i10) -> i1
    %61 = comb.extract %rbr_ir from 8 : (i10) -> i1
    %62 = comb.mux %170, %60, %61 : i1
    %63 = comb.and %62, %58 : i1
    %64 = comb.extract %rx_pop_data from 8 : (i10) -> i2
    %65 = comb.extract %rx_pop_data from 0 : (i10) -> i8
    %66 = comb.extract %rbr_ir from 0 : (i10) -> i8
    %67 = comb.mux %170, %65, %66 : i8
    %68 = comb.mux %170, %rx_empty, %rbr_ir_empty {sv.namehint = "w_rbr_empty"} : i1
    %69 = comb.xor %46, %true : i1
    %70 = comb.and %51, %77, %69 : i1
    %71 = comb.or %70, %171 : i1
    %72 = comb.or %71, %52 : i1
    %73 = comb.mux bin %72, %71, %rbr_ir_empty : i1
    %rbr_ir_empty = seq.firreg %73 clock %49 reset async %50, %true : i1
    %74 = comb.and %170, %46, %75 : i1
    %75 = comb.xor %rx_push_en_ed, %true : i1
    %rx_push_en_ed = seq.firreg %46 clock %49 reset async %50, %false : i1
    %76 = comb.and %170, %77 : i1
    %dly_rx_pop = seq.firreg %76 clock %49 reset async %50, %false : i1
    %77 = comb.and %11, %17 : i1
    %78 = comb.and %10, %wr_enx, %17 : i1
    %79 = comb.and %51, %78 : i1
    %80 = comb.extract %ipwdata from 0 : (i10) -> i8
    %81 = comb.xor %79, %true : i1
    %82 = comb.mux %81, %c0_i8, %80 : i8
    %83 = comb.mux bin %79, %82, %thr_ir : i8
    %thr_ir = seq.firreg %83 clock %49 reset async %50, %c0_i8 : i8
    %84 = comb.mux %170, %tx_pop_data, %thr_ir : i8
    %85 = comb.and %26, %17 : i1
    %86 = comb.mux %far_ir, %85, %93 : i1
    %87 = comb.and %170, %86 : i1
    %88 = comb.xor %tx_in_prog, %true : i1
    %89 = comb.xor %175, %true : i1
    %90 = comb.icmp ne %108, %c0_i16 : i16
    %91 = comb.xor %far_ir, %true : i1
    %92 = comb.xor %htx, %true : i1
    %93 = comb.and %92, %91, %88, %tx_data_avail, %89, %90, %9 : i1
    %94 = comb.or %93, %97 : i1
    %95 = comb.mux bin %94, %93, %tx_in_prog : i1
    %tx_in_prog = seq.firreg %95 clock %49 reset async %50, %false : i1
    %96 = comb.xor %dly_tx_finish, %true : i1
    %97 = comb.and %tx_finish, %96 : i1
    %dly_tx_finish = seq.firreg %tx_finish clock %49 reset async %50, %false : i1
    %98 = comb.xor %thr_empty, %true : i1
    %tx_data_avail = seq.firreg %98 clock %49 reset async %50, %false : i1
    %99 = comb.mux %170, %tx_empty, %thr_ir_empty : i1
    %thr_empty = seq.firreg %99 clock %49 reset async %50, %false : i1
    %100 = comb.or %93, %171 : i1
    %101 = comb.and %51, %100 : i1
    %102 = comb.or %101, %78 : i1
    %103 = comb.and %51, %102 : i1
    %104 = comb.mux bin %103, %101, %thr_ir_empty : i1
    %thr_ir_empty = seq.firreg %104 clock %49 reset async %50, %true : i1
    %105 = comb.and %170, %78 : i1
    %106 = comb.mux bin %37, %80, %dll : i8
    %dll = seq.firreg %106 clock %49 reset async %50, %c0_i8 : i8
    %107 = comb.mux bin %38, %80, %dlh : i8
    %dlh = seq.firreg %107 clock %49 reset async %50, %c0_i8 : i8
    %108 = comb.concat %dlh, %dll : i8, i8
    %109 = comb.or %37, %38 : i1
    %divsr_wd = seq.firreg %109 clock %49 reset async %50, %false : i1
    %110 = comb.extract %ipwdata from 7 : (i10) -> i1
    %111 = comb.extract %ipwdata from 0 : (i10) -> i4
    %112 = comb.concat %110, %c0_i3, %111 : i1, i3, i4
    %113 = comb.mux bin %39, %112, %ier_ir : i8
    %ier_ir = seq.firreg %113 clock %49 reset async %50, %c0_i8 : i8
    %114 = comb.extract %ier_ir from 0 : (i8) -> i7
    %115:2 = llhd.process -> i8, i1 {
      cf.br ^bb1(%c0_i8, %false : i8, i1)
    ^bb1(%382: i8, %383: i1):  // 9 preds: ^bb0, ^bb3, ^bb6, ^bb7, ^bb9, ^bb11, ^bb13, ^bb15, ^bb16
      llhd.wait yield (%382, %383 : i8, i1), (%170, %118, %dly_line_stat_intr, %dly_data_avail_intr, %dly_char_to_intr, %dly_thre_intr, %dly_modem_stat_intr, %dly_busy_det_intr : i1, i1, i1, i1, i1, i1, i1, i1), ^bb2
    ^bb2:  // pred: ^bb1
      %384 = comb.mux %170, %c-4_i4, %c0_i4 : i4
      cf.cond_br %dly_line_stat_intr, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      %385 = comb.concat %384, %c6_i4 : i4, i4
      cf.br ^bb1(%385, %true : i8, i1)
    ^bb4:  // pred: ^bb2
      cf.cond_br %dly_data_avail_intr, ^bb5, ^bb8
    ^bb5:  // pred: ^bb4
      cf.cond_br %118, ^bb6, ^bb7
    ^bb6:  // pred: ^bb5
      %386 = comb.concat %384, %c6_i4 : i4, i4
      cf.br ^bb1(%386, %true : i8, i1)
    ^bb7:  // pred: ^bb5
      %387 = comb.concat %384, %c4_i4 : i4, i4
      cf.br ^bb1(%387, %true : i8, i1)
    ^bb8:  // pred: ^bb4
      cf.cond_br %dly_char_to_intr, ^bb9, ^bb10
    ^bb9:  // pred: ^bb8
      %388 = comb.concat %384, %c-4_i4 : i4, i4
      cf.br ^bb1(%388, %true : i8, i1)
    ^bb10:  // pred: ^bb8
      cf.cond_br %dly_thre_intr, ^bb11, ^bb12
    ^bb11:  // pred: ^bb10
      %389 = comb.concat %384, %c2_i4 : i4, i4
      cf.br ^bb1(%389, %true : i8, i1)
    ^bb12:  // pred: ^bb10
      cf.cond_br %dly_modem_stat_intr, ^bb13, ^bb14
    ^bb13:  // pred: ^bb12
      %390 = comb.concat %384, %c0_i4 : i4, i4
      cf.br ^bb1(%390, %true : i8, i1)
    ^bb14:  // pred: ^bb12
      cf.cond_br %dly_busy_det_intr, ^bb15, ^bb16
    ^bb15:  // pred: ^bb14
      %391 = comb.concat %384, %c7_i4 : i4, i4
      cf.br ^bb1(%391, %true : i8, i1)
    ^bb16:  // pred: ^bb14
      %392 = comb.concat %384, %c1_i4 : i4, i4
      cf.br ^bb1(%392, %true : i8, i1)
    }
    llhd.drv %iir, %115#0 after %0 if %115#1 : i8
    %dly_line_stat_intr = seq.firreg %118 clock %49 reset async %50, %false : i1
    %116 = comb.extract %ier_ir from 2 : (i8) -> i1
    %117 = comb.or %bi, %fe, %pe, %oe : i1
    %118 = comb.and %116, %117 {sv.namehint = "intmem_line_stat_intr"} : i1
    %dly_data_avail_intr = seq.firreg %124 clock %49 reset async %50, %false : i1
    %119 = comb.extract %ier_ir from 0 : (i8) -> i1
    %120 = llhd.prb %rx_fifo_trig : i6
    %121 = comb.icmp uge %rfl_cnt, %120 : i6
    %122 = comb.xor %rbr_ir_empty, %true : i1
    %123 = comb.mux %170, %121, %122 : i1
    %124 = comb.and %119, %123 {sv.namehint = "data_avail_intr_s1"} : i1
    %dly_char_to_intr = seq.firreg %126 clock %49 reset async %50, %false : i1
    %125 = comb.xor %77, %true : i1
    %126 = comb.and %170, %119, %char_to_reg, %125, %132 : i1
    %127 = comb.xor %rx_pop_ack, %true : i1
    %128 = comb.and %127, %76 : i1
    %129 = comb.or %rx_pop_ack, %76 : i1
    %130 = comb.mux bin %129, %128, %rx_pop_hld : i1
    %rx_pop_hld = seq.firreg %130 clock %49 reset async %50, %false : i1
    %dly_rx_pop_hld = seq.firreg %rx_pop_hld clock %49 reset async %50, %false : i1
    %131 = comb.xor %rx_pop_hld, %dly_rx_pop_hld : i1
    %132 = comb.xor %rx_pop_hld, %true : i1
    %133 = comb.and %136, %58, %125, %132 : i1
    %134 = comb.or %77, %133 : i1
    %135 = comb.mux bin %134, %133, %char_to_reg : i1
    %char_to_reg = seq.firreg %135 clock %49 reset async %50, %false : i1
    %136 = comb.xor %char_to, %dly_char_to {sv.namehint = "char_to_ed"} : i1
    %dly_char_to = seq.firreg %char_to clock %49 reset async %50, %false : i1
    %dly_thre_intr = seq.firreg %139 clock %49 reset async %50, %false : i1
    %137 = comb.extract %ier_ir from 1 : (i8) -> i1
    %138 = llhd.prb %int_thre_intr : i1
    %139 = comb.and %137, %thre_not_masked, %138 : i1
    %140:2 = llhd.process -> i1, i1 {
      cf.br ^bb1(%false, %false : i1, i1)
    ^bb1(%382: i1, %383: i1):  // 2 preds: ^bb0, ^bb2
      llhd.wait yield (%382, %383 : i1, i1), (%false, %170, %tfl_cnt, %7, %thr_empty : i1, i1, i6, i6, i1), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb1(%thr_empty, %true : i1, i1)
    }
    llhd.drv %int_thre_intr, %140#0 after %0 if %140#1 : i1
    %141 = comb.extract %3 from 0 : (i8) -> i4
    %142 = comb.icmp eq %141, %c2_i4 : i4
    %143 = comb.and %16, %rd_en, %17, %142 : i1
    %144 = comb.or %147, %143 : i1
    %145 = comb.mux bin %144, %147, %thre_not_masked : i1
    %thre_not_masked = seq.firreg %145 clock %49 reset async %50, %true : i1
    %146 = comb.xor %137, %true : i1
    %147 = comb.or %98, %146 : i1
    %dly_modem_stat_intr = seq.firreg %150 clock %49 reset async %50, %false : i1
    %148 = comb.extract %ier_ir from 3 : (i8) -> i1
    %149 = comb.or %ddcd, %teri, %ddsr, %dcts : i1
    %150 = comb.and %148, %149 : i1
    %dly_busy_det_intr = seq.firreg %busy_det_intr clock %49 reset async %50, %false : i1
    %151 = comb.and %29, %rd_en : i1
    %152 = comb.xor %151, %true : i1
    %153 = comb.and %152, %busy_det_intr : i1
    %busy_det_intr = seq.firreg %153 clock %49 reset async %50, %false : i1
    %154 = comb.or %118, %124, %126, %139, %150, %busy_det_intr : i1
    %intr = seq.firreg %154 clock %49 reset async %50, %false : i1
    %155 = comb.extract %ipwdata from 3 : (i10) -> i5
    %156 = comb.extract %ipwdata from 0 : (i10) -> i1
    %157 = comb.concat %155, %c0_i2, %156 : i5, i2, i1
    %158 = comb.mux bin %18, %157, %fcr_ir : i8
    %fcr_ir = seq.firreg %158 clock %49 reset async %50, %c0_i8 : i8
    %159 = comb.extract %fcr_ir from 6 : (i8) -> i2
    %160:2 = llhd.process -> i6, i1 {
      cf.br ^bb1(%c0_i6, %false : i6, i1)
    ^bb1(%382: i6, %383: i1):  // 5 preds: ^bb0, ^bb2, ^bb3, ^bb4, ^bb4
      llhd.wait yield (%382, %383 : i6, i1), (%159 : i2), ^bb2
    ^bb2:  // pred: ^bb1
      %384 = comb.icmp ceq %159, %c1_i2 : i2
      cf.cond_br %384, ^bb1(%c8_i6, %true : i6, i1), ^bb3
    ^bb3:  // pred: ^bb2
      %385 = comb.icmp ceq %159, %c-2_i2 : i2
      cf.cond_br %385, ^bb1(%c16_i6, %true : i6, i1), ^bb4
    ^bb4:  // pred: ^bb3
      %386 = comb.icmp ceq %159, %c-1_i2 : i2
      cf.cond_br %386, ^bb1(%c30_i6, %true : i6, i1), ^bb1(%c1_i6, %true : i6, i1)
    }
    llhd.drv %rx_fifo_trig, %160#0 after %0 if %160#1 : i6
    %161:2 = llhd.process -> i6, i1 {
      cf.br ^bb1(%c0_i6, %false : i6, i1)
    ^bb1(%382: i6, %383: i1):  // 2 preds: ^bb0, ^bb2
      llhd.wait yield (%382, %383 : i6, i1), (%c0_i2 : i2), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb1(%c0_i6, %true : i6, i1)
    }
    llhd.drv %tx_empty_trig, %161#0 after %0 if %161#1 : i6
    %162 = comb.extract %fcr_ir from 3 : (i8) -> i1
    %163 = comb.and %170, %162 : i1
    %164 = comb.extract %ipwdata from 2 : (i10) -> i1
    %165 = comb.and %18, %164 : i1
    %166 = comb.or %165, %171, %304 : i1
    %int_tx_fifo_rst = seq.firreg %166 clock %49 reset async %50, %false : i1
    %167 = comb.extract %ipwdata from 1 : (i10) -> i1
    %168 = comb.and %18, %167 : i1
    %169 = comb.or %168, %171, %304 : i1
    %int_rx_fifo_rst = seq.firreg %169 clock %49 reset async %50, %false : i1
    %170 = comb.extract %fcr_ir from 0 : (i8) -> i1
    %dly_fen = seq.firreg %170 clock %49 reset async %50, %false : i1
    %171 = comb.xor %170, %dly_fen : i1
    %172 = comb.mux bin %40, %80, %lcr_ir : i8
    %lcr_ir = seq.firreg %172 clock %49 reset async %50, %c0_i8 : i8
    %173 = comb.extract %lcr_ir from 7 : (i8) -> i1
    %174 = comb.extract %lcr_ir from 6 : (i8) -> i1
    %175 = comb.and %174, %88 : i1
    %xbreak_o = seq.firreg %175 clock %49 reset async %50, %false : i1
    %char_info_wd = seq.firreg %40 clock %49 reset async %50, %false : i1
    %176 = comb.extract %ipwdata from 0 : (i10) -> i7
    %177 = comb.xor %41, %true : i1
    %178 = comb.mux %177, %c0_i7, %176 : i7
    %179 = comb.mux bin %41, %178, %mcr_ir : i7
    %mcr_ir = seq.firreg %179 clock %49 reset async %50, %c0_i7 : i7
    %180 = comb.extract %mcr_ir from 0 : (i7) -> i5
    %181 = comb.extract %mcr_ir from 4 : (i7) -> i1
    %182 = comb.or %181, %far_ir : i1
    %lb_en_o = seq.firreg %182 clock %49 reset async %50, %false : i1
    %183 = comb.extract %mcr_ir from 3 : (i7) -> i1
    %184 = comb.xor %183, %true : i1
    %185 = comb.or %184, %181 : i1
    %186 = comb.extract %mcr_ir from 2 : (i7) -> i1
    %187 = comb.xor %186, %true : i1
    %188 = comb.or %187, %181 : i1
    %189 = comb.extract %mcr_ir from 1 : (i7) -> i1
    %190 = comb.xor %189, %true : i1
    %191 = comb.or %190, %181 : i1
    %192 = comb.icmp eq %rfl_cnt, %120 : i6
    %193 = comb.or %rx_empty, %192 : i1
    %194 = comb.mux bin %193, %rx_empty, %auto_rts_ctrl : i1
    %auto_rts_ctrl = seq.firreg %194 clock %49 reset async %50, %true : i1
    %195 = comb.extract %mcr_ir from 0 : (i7) -> i1
    %196 = comb.xor %195, %true : i1
    %197 = comb.or %196, %181 : i1
    %198 = comb.xor %221, %true : i1
    %199 = comb.xor %rx_full, %true : i1
    %200 = comb.and %218, %198, %199 : i1
    %201 = comb.add %rx_fe_cnt, %c1_i6 : i6
    %202 = comb.xor %218, %true : i1
    %203 = comb.icmp ne %rx_fe_cnt, %c0_i6 : i6
    %204 = comb.and %221, %202, %203 : i1
    %205 = comb.add %rx_fe_cnt, %c-1_i6 : i6
    %206 = comb.xor %int_rx_fifo_rst, %true : i1
    %207 = comb.and %200, %206 : i1
    %208 = comb.mux %207, %201, %c0_i6 : i6
    %209 = comb.xor %200, %true : i1
    %210 = comb.and %209, %206 : i1
    %211 = comb.and %204, %210 : i1
    %212 = comb.mux %211, %205, %208 : i6
    %213 = comb.xor %204, %true : i1
    %214 = comb.and %210, %213 : i1
    %215 = comb.mux bin %214, %rx_fe_cnt, %212 : i6
    %rx_fe_cnt = seq.firreg %215 clock %49 reset async %50, %c0_i6 : i6
    %216 = comb.extract %54 from 8 : (i10) -> i2
    %217 = comb.icmp ne %216, %c0_i2 : i2
    %218 = comb.and %74, %217 : i1
    %219 = comb.icmp ne %64, %c0_i2 : i2
    %220 = comb.xor %been_seen, %true : i1
    %221 = comb.and %219, %220 : i1
    %222 = comb.or %227, %dly_rx_pop : i1
    %223 = comb.and %219, %222 : i1
    %224 = comb.and %74, %rx_empty : i1
    %225 = comb.or %223, %76, %224 : i1
    %226 = comb.mux bin %225, %223, %been_seen : i1
    %been_seen = seq.firreg %226 clock %49 reset async %50, %false : i1
    %227 = comb.and %58, %dly_rbr_empty : i1
    %dly_rbr_empty = seq.firreg %68 clock %49 reset async %50, %false : i1
    %228 = comb.and %21, %rd_en : i1
    %229 = comb.or %203, %228 : i1
    %230 = comb.mux bin %229, %203, %i_rx_fifo_err : i1
    %i_rx_fifo_err = seq.firreg %230 clock %49 reset async %50, %false : i1
    %231 = comb.and %88, %thr_empty {sv.namehint = "temt"} : i1
    %232 = comb.icmp eq %67, %c0_i8 : i8
    %233 = comb.and %232, %59 : i1
    %dly_bi_det = seq.firreg %233 clock %49 reset async %50, %false : i1
    %234 = comb.xor %dly_bi_det, %true : i1
    %235 = comb.and %233, %234 : i1
    %236 = comb.or %228, %76 : i1
    %237 = comb.xor %236, %true : i1
    %238 = comb.and %237, %235 : i1
    %239 = comb.or %236, %235 : i1
    %240 = comb.mux bin %239, %238, %bi : i1
    %bi = seq.firreg %240 clock %49 reset async %50, %false : i1
    %241 = comb.extract %rbr_ir from 8 : (i10) -> i2
    %242 = comb.icmp ne %241, %c0_i2 : i2
    %243 = comb.and %242, %51, %rx_push_en_ed : i1
    %244 = comb.or %243, %77 : i1
    %245 = comb.mux bin %244, %243, %rbr_been_seen : i1
    %rbr_been_seen = seq.firreg %245 clock %49 reset async %50, %false : i1
    %246 = comb.mux %170, %been_seen, %rbr_been_seen : i1
    %247 = comb.xor %246, %true : i1
    %248 = comb.and %59, %247 : i1
    %249 = comb.and %237, %248 : i1
    %250 = comb.or %236, %248 : i1
    %251 = comb.mux bin %250, %249, %fe : i1
    %fe = seq.firreg %251 clock %49 reset async %50, %false : i1
    %252 = comb.xor %fe, %true : i1
    %253 = comb.and %252, %59, %247 {sv.namehint = "fe_det_pulse"} : i1
    %254 = comb.and %63, %247 : i1
    %255 = comb.and %237, %254 : i1
    %256 = comb.or %236, %254 : i1
    %257 = comb.mux bin %256, %255, %pe : i1
    %pe = seq.firreg %257 clock %49 reset async %50, %false : i1
    %258 = comb.xor %pe, %true : i1
    %259 = comb.and %258, %63, %247 {sv.namehint = "pe_det_pulse"} : i1
    %260 = comb.mux %170, %rx_overflow, %rbr_overflow : i1
    %261 = comb.xor %68, %true : i1
    %262 = comb.and %51, %46, %261, %125 : i1
    %rbr_overflow = seq.firreg %262 clock %49 reset async %50, %false : i1
    %263 = comb.xor %228, %true : i1
    %264 = comb.and %263, %260 : i1
    %265 = comb.or %228, %260 : i1
    %266 = comb.mux bin %265, %264, %oe : i1
    %oe = seq.firreg %266 clock %49 reset async %50, %false : i1
    %267 = comb.xor %oe, %true : i1
    %268 = comb.and %267, %260 {sv.namehint = "oe_det_pulse"} : i1
    %269 = comb.or %i_rx_fifo_err, %203 : i1
    %270 = comb.and %170, %269 : i1
    %271 = comb.or %bi, %235 : i1
    %272 = comb.or %fe, %253 : i1
    %273 = comb.or %pe, %259 : i1
    %274 = comb.or %oe, %268 : i1
    %275 = comb.xor %dcd_n, %true : i1
    %276 = comb.mux %181, %183, %275 : i1
    %277 = comb.xor %ri_n, %true : i1
    %278 = comb.mux %181, %186, %277 : i1
    %279 = comb.xor %dsr_n, %true : i1
    %280 = comb.mux %181, %195, %279 : i1
    %281 = comb.xor %cts_n, %true : i1
    %282 = comb.mux %181, %189, %281 : i1
    %dly_dcd = seq.firreg %276 clock %49 reset async %50, %false : i1
    %283 = comb.xor %276, %dly_dcd : i1
    %msr_change_unmask1 = seq.firreg %true clock %49 reset async %50, %false : i1
    %msr_change_unmask2 = seq.firreg %msr_change_unmask1 clock %49 reset async %50, %false : i1
    %msr_change_unmask3 = seq.firreg %msr_change_unmask2 clock %49 reset async %50, %false : i1
    %msr_change_unmask4 = seq.firreg %msr_change_unmask3 clock %49 reset async %50, %false : i1
    %msr_change_unmask5 = seq.firreg %msr_change_unmask4 clock %49 reset async %50, %false : i1
    %284 = comb.xor %181, %dly_lb_mode, %true : i1
    %285 = comb.and %283, %284, %msr_change_unmask5 : i1
    %286 = comb.and %22, %rd_en : i1
    %287 = comb.or %285, %286 : i1
    %288 = comb.mux bin %287, %285, %ddcd : i1
    %ddcd = seq.firreg %288 clock %49 reset async %50, %false : i1
    %dly_lb_mode = seq.firreg %181 clock %49 reset async %50, %false : i1
    %dly_ri = seq.firreg %278 clock %49 reset async %50, %false : i1
    %289 = comb.xor %278, %true : i1
    %290 = comb.and %289, %dly_ri, %284, %msr_change_unmask5 : i1
    %291 = comb.or %290, %286 : i1
    %292 = comb.mux bin %291, %290, %teri : i1
    %teri = seq.firreg %292 clock %49 reset async %50, %false : i1
    %dly_dsr = seq.firreg %280 clock %49 reset async %50, %false : i1
    %293 = comb.xor %280, %dly_dsr : i1
    %294 = comb.and %293, %284, %msr_change_unmask5 : i1
    %295 = comb.or %294, %286 : i1
    %296 = comb.mux bin %295, %294, %ddsr : i1
    %ddsr = seq.firreg %296 clock %49 reset async %50, %false : i1
    %dly_cts = seq.firreg %282 clock %49 reset async %50, %false : i1
    %297 = comb.xor %282, %dly_cts : i1
    %298 = comb.and %297, %284, %msr_change_unmask5 : i1
    %299 = comb.or %298, %286 : i1
    %300 = comb.mux bin %299, %298, %dcts : i1
    %dcts = seq.firreg %300 clock %49 reset async %50, %false : i1
    %301 = comb.concat %276, %278, %280, %282, %ddcd, %teri, %ddsr, %dcts : i1, i1, i1, i1, i1, i1, i1, i1
    %302 = comb.mux bin %42, %80, %scr : i8
    %scr = seq.firreg %302 clock %49 reset async %50, %c0_i8 : i8
    %303 = comb.mux bin %43, %156, %far_ir : i1
    %far_ir = seq.firreg %303 clock %49 reset async %50, %false : i1
    %dly_fifo_access = seq.firreg %far_ir clock %49 reset async %50, %false : i1
    %304 = comb.xor %far_ir, %dly_fifo_access : i1
    %305 = comb.mux %far_ir, %84, %c0_i8 : i8
    %306 = comb.and %170, %58 : i1
    %307 = comb.xor %tx_full, %true : i1
    %308 = comb.xor %87, %true : i1
    %309 = comb.and %105, %308, %307 : i1
    %310 = comb.add %tfl_cnt, %c1_i6 : i6
    %311 = comb.xor %105, %true : i1
    %312 = comb.icmp ne %tfl_cnt, %c0_i6 : i6
    %313 = comb.and %311, %87, %312 : i1
    %314 = comb.add %tfl_cnt, %c-1_i6 : i6
    %315 = comb.xor %int_tx_fifo_rst, %true : i1
    %316 = comb.and %309, %315 : i1
    %317 = comb.mux %316, %310, %c0_i6 : i6
    %318 = comb.xor %309, %true : i1
    %319 = comb.and %318, %315 : i1
    %320 = comb.and %313, %319 : i1
    %321 = comb.mux %320, %314, %317 : i6
    %322 = comb.xor %313, %true : i1
    %323 = comb.and %319, %322 : i1
    %324 = comb.mux bin %323, %tfl_cnt, %321 : i6
    %tfl_cnt = seq.firreg %324 clock %49 reset async %50, %c0_i6 : i6
    %325:2 = llhd.process -> i6, i1 {
      cf.br ^bb1(%c0_i6, %false : i6, i1)
    ^bb1(%382: i6, %383: i1):  // 2 preds: ^bb0, ^bb2
      llhd.wait yield (%382, %383 : i6, i1), (%tfl_cnt : i6), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb1(%tfl_cnt, %true : i6, i1)
    }
    llhd.drv %int_tfl_cnt, %325#0 after %0 if %325#1 : i6
    %326 = llhd.prb %int_tfl_cnt : i6
    %327:2 = llhd.process -> i6, i1 {
      cf.br ^bb1(%c0_i6, %false : i6, i1)
    ^bb1(%382: i6, %383: i1):  // 2 preds: ^bb0, ^bb2
      llhd.wait yield (%382, %383 : i6, i1), (%326 : i6), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb1(%326, %true : i6, i1)
    }
    llhd.drv %tfl_ir, %327#0 after %0 if %327#1 : i6
    %328 = llhd.prb %tfl_ir : i6
    %329 = comb.xor %76, %true : i1
    %330 = comb.and %74, %329, %199 : i1
    %331 = comb.add %rfl_cnt, %c1_i6 : i6
    %332 = comb.xor %74, %true : i1
    %333 = comb.icmp ne %rfl_cnt, %c0_i6 : i6
    %334 = comb.and %332, %76, %333 : i1
    %335 = comb.add %rfl_cnt, %c-1_i6 : i6
    %336 = comb.and %330, %206 : i1
    %337 = comb.mux %336, %331, %c0_i6 : i6
    %338 = comb.xor %330, %true : i1
    %339 = comb.and %338, %206 : i1
    %340 = comb.and %334, %339 : i1
    %341 = comb.mux %340, %335, %337 : i6
    %342 = comb.xor %334, %true : i1
    %343 = comb.and %339, %342 : i1
    %344 = comb.mux bin %343, %rfl_cnt, %341 : i6
    %rfl_cnt = seq.firreg %344 clock %49 reset async %50, %c0_i6 : i6
    %345:2 = llhd.process -> i6, i1 {
      cf.br ^bb1(%c0_i6, %false : i6, i1)
    ^bb1(%382: i6, %383: i1):  // 2 preds: ^bb0, ^bb2
      llhd.wait yield (%382, %383 : i6, i1), (%rfl_cnt : i6), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb1(%rfl_cnt, %true : i6, i1)
    }
    llhd.drv %int_rfl_cnt, %345#0 after %0 if %345#1 : i6
    %346 = llhd.prb %int_rfl_cnt : i6
    %347:2 = llhd.process -> i6, i1 {
      cf.br ^bb1(%c0_i6, %false : i6, i1)
    ^bb1(%382: i6, %383: i1):  // 2 preds: ^bb0, ^bb2
      llhd.wait yield (%382, %383 : i6, i1), (%346 : i6), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb1(%346, %true : i6, i1)
    }
    llhd.drv %rfl_ir, %347#0 after %0 if %347#1 : i6
    %348 = llhd.prb %rfl_ir : i6
    %349 = comb.mux bin %44, %156, %htx_ir : i1
    %htx_ir = seq.firreg %349 clock %49 reset async %50, %false : i1
    %350 = comb.and %170, %htx_ir : i1
    %htx = seq.firreg %350 clock %49 reset async %50, %false : i1
    %351 = comb.concat %68, %170, %76 : i1, i1, i1
    %352 = comb.concat %353, %171, %354 : i1, i1, i1
    %353 = comb.xor %68, %dly_rbr_empty {sv.namehint = "rbr_empty_ed"} : i1
    %354 = comb.xor %76, %dly_rx_pop {sv.namehint = "rx_pop_ed"} : i1
    %355 = comb.xor %45, %true : i1
    %356 = comb.mux %355, %c0_i4, %111 : i4
    %357 = comb.mux bin %45, %356, %dlf_reg : i4
    %dlf_reg = seq.firreg %357 clock %49 reset async %50, %c0_i4 : i4
    %dlf_wd = seq.firreg %45 clock %49 reset async %50, %false : i1
    %358 = comb.mux bin %lcr_we_dly, %lcr_we_dly, %en_txhs_gen : i1
    %en_txhs_gen = seq.firreg %358 clock %49 reset async %50, %false : i1
    %lcr_we_dly = seq.firreg %40 clock %49 reset async %50, %false : i1
    %359 = comb.xor %dma_tx_req, %true : i1
    %360 = comb.and %thr_empty, %en_txhs_gen : i1
    %361 = comb.and %tx_empty, %en_txhs_gen : i1
    %362 = comb.and %163, %tx_full : i1
    %363 = comb.xor %362, %true : i1
    %364 = comb.and %363, %361 : i1
    %365 = comb.xor %163, %true : i1
    %366 = comb.mux %365, %360, %364 : i1
    %367 = comb.or %365, %362, %361 : i1
    %368 = comb.mux bin %367, %366, %dma_tx_req : i1
    %dma_tx_req = seq.firreg %368 clock %49 reset async %50, %false : i1
    %369 = comb.xor %dma_rx_req, %true : i1
    %370 = comb.or %121, %char_to_reg : i1
    %371 = comb.xor %rx_empty, %true : i1
    %372 = comb.and %371, %163 : i1
    %373 = comb.mux %372, %370, %261 : i1
    %374 = comb.xor %372, %true : i1
    %375 = comb.and %163, %rx_empty : i1
    %376 = comb.xor %375, %true : i1
    %377 = comb.and %376, %373 : i1
    %378 = comb.or %375, %374, %370 : i1
    %379 = comb.mux bin %378, %377, %dma_rx_req : i1
    %dma_rx_req = seq.firreg %379 clock %49 reset async %50, %false : i1
    %380:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%382: i32, %383: i1):  // 22 preds: ^bb0, ^bb3, ^bb5, ^bb7, ^bb9, ^bb11, ^bb13, ^bb15, ^bb17, ^bb19, ^bb21, ^bb23, ^bb25, ^bb27, ^bb29, ^bb31, ^bb33, ^bb34, ^bb35, ^bb36, ^bb37, ^bb38
      llhd.wait yield (%382, %383 : i32, i1), (%11, %67, %12, %dll, %14, %dlh, %15, %2, %16, %3, %19, %lcr_ir, %20, %4, %21, %5, %22, %301, %23, %scr, %24, %far_ir, %26, %305, %29, %6, %30, %328, %31, %348, %32, %htx, %36, %dlf_reg, %33, %c0_i32, %34, %c875574570_i32, %35, %c1146552592_i32 : i1, i8, i1, i8, i1, i8, i1, i8, i1, i8, i1, i8, i1, i7, i1, i8, i1, i8, i1, i8, i1, i1, i1, i8, i1, i8, i1, i6, i1, i6, i1, i1, i1, i4, i1, i32, i1, i32, i1, i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.cond_br %11, ^bb3, ^bb4
    ^bb3:  // pred: ^bb2
      %384 = comb.concat %c0_i24, %67 : i24, i8
      cf.br ^bb1(%384, %true : i32, i1)
    ^bb4:  // pred: ^bb2
      cf.cond_br %12, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      %385 = comb.concat %c0_i24, %dll : i24, i8
      cf.br ^bb1(%385, %true : i32, i1)
    ^bb6:  // pred: ^bb4
      cf.cond_br %14, ^bb7, ^bb8
    ^bb7:  // pred: ^bb6
      %386 = comb.concat %c0_i24, %dlh : i24, i8
      cf.br ^bb1(%386, %true : i32, i1)
    ^bb8:  // pred: ^bb6
      cf.cond_br %15, ^bb9, ^bb10
    ^bb9:  // pred: ^bb8
      %387 = comb.concat %c0_i25, %114 : i25, i7
      cf.br ^bb1(%387, %true : i32, i1)
    ^bb10:  // pred: ^bb8
      cf.cond_br %16, ^bb11, ^bb12
    ^bb11:  // pred: ^bb10
      %388 = comb.concat %c0_i24, %3 : i24, i8
      cf.br ^bb1(%388, %true : i32, i1)
    ^bb12:  // pred: ^bb10
      cf.cond_br %19, ^bb13, ^bb14
    ^bb13:  // pred: ^bb12
      %389 = comb.concat %c0_i24, %lcr_ir : i24, i8
      cf.br ^bb1(%389, %true : i32, i1)
    ^bb14:  // pred: ^bb12
      cf.cond_br %20, ^bb15, ^bb16
    ^bb15:  // pred: ^bb14
      %390 = comb.concat %c0_i27, %180 : i27, i5
      cf.br ^bb1(%390, %true : i32, i1)
    ^bb16:  // pred: ^bb14
      cf.cond_br %21, ^bb17, ^bb18
    ^bb17:  // pred: ^bb16
      %391 = comb.concat %c0_i24, %270, %231, %thr_empty, %271, %272, %273, %274, %58 : i24, i1, i1, i1, i1, i1, i1, i1, i1
      cf.br ^bb1(%391, %true : i32, i1)
    ^bb18:  // pred: ^bb16
      cf.cond_br %22, ^bb19, ^bb20
    ^bb19:  // pred: ^bb18
      %392 = comb.concat %c0_i24, %276, %278, %280, %282, %ddcd, %teri, %ddsr, %dcts : i24, i1, i1, i1, i1, i1, i1, i1, i1
      cf.br ^bb1(%392, %true : i32, i1)
    ^bb20:  // pred: ^bb18
      cf.cond_br %23, ^bb21, ^bb22
    ^bb21:  // pred: ^bb20
      %393 = comb.concat %c0_i24, %scr : i24, i8
      cf.br ^bb1(%393, %true : i32, i1)
    ^bb22:  // pred: ^bb20
      cf.cond_br %24, ^bb23, ^bb24
    ^bb23:  // pred: ^bb22
      %394 = comb.concat %c0_i31, %far_ir : i31, i1
      cf.br ^bb1(%394, %true : i32, i1)
    ^bb24:  // pred: ^bb22
      cf.cond_br %26, ^bb25, ^bb26
    ^bb25:  // pred: ^bb24
      %395 = comb.concat %c0_i24, %305 : i24, i8
      cf.br ^bb1(%395, %true : i32, i1)
    ^bb26:  // pred: ^bb24
      cf.cond_br %29, ^bb27, ^bb28
    ^bb27:  // pred: ^bb26
      %396 = comb.concat %c0_i27, %rx_full, %306, %tx_empty, %307, %false : i27, i1, i1, i1, i1, i1
      cf.br ^bb1(%396, %true : i32, i1)
    ^bb28:  // pred: ^bb26
      cf.cond_br %30, ^bb29, ^bb30
    ^bb29:  // pred: ^bb28
      %397 = comb.concat %c0_i26, %328 : i26, i6
      cf.br ^bb1(%397, %true : i32, i1)
    ^bb30:  // pred: ^bb28
      cf.cond_br %31, ^bb31, ^bb32
    ^bb31:  // pred: ^bb30
      %398 = comb.concat %c0_i26, %348 : i26, i6
      cf.br ^bb1(%398, %true : i32, i1)
    ^bb32:  // pred: ^bb30
      cf.cond_br %32, ^bb33, ^bb34
    ^bb33:  // pred: ^bb32
      %399 = comb.concat %c0_i31, %htx : i31, i1
      cf.br ^bb1(%399, %true : i32, i1)
    ^bb34:  // pred: ^bb32
      cf.cond_br %33, ^bb1(%c0_i32, %true : i32, i1), ^bb35
    ^bb35:  // pred: ^bb34
      cf.cond_br %34, ^bb1(%c875574570_i32, %true : i32, i1), ^bb36
    ^bb36:  // pred: ^bb35
      cf.cond_br %35, ^bb1(%c1146552592_i32, %true : i32, i1), ^bb37
    ^bb37:  // pred: ^bb36
      cf.cond_br %36, ^bb38, ^bb1(%c0_i32, %true : i32, i1)
    ^bb38:  // pred: ^bb37
      %400 = comb.concat %c0_i28, %dlf_reg : i28, i4
      cf.br ^bb1(%400, %true : i32, i1)
    }
    llhd.drv %iprdata, %380#0 after %0 if %380#1 : i32
    %381 = llhd.prb %iprdata : i32
    hw.output %381, %105, %87, %74, %76, %int_tx_fifo_rst, %int_rx_fifo_rst, %80, %54, %93, %84, %lb_en_o, %xbreak_o, %352, %351, %rx_pop_hld, %131, %108, %divsr_wd, %1, %197, %191, %188, %185, %dma_tx_req, %dma_rx_req, %359, %369, %char_info_wd, %dlf_reg, %dlf_wd, %intr : i32, i1, i1, i1, i1, i1, i1, i8, i10, i1, i8, i1, i1, i3, i3, i1, i1, i16, i1, i6, i1, i1, i1, i1, i1, i1, i1, i1, i1, i4, i1, i1
  }
}
