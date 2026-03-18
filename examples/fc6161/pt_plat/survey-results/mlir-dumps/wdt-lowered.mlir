module {
  hw.module private @DW_apb_wdt_biu(in %pclk : i1, in %presetn : i1, in %psel : i1, in %penable : i1, in %pwrite : i1, in %paddr : i8, in %pwdata : i32, out prdata : i32, out wr_en : i1, out rd_en : i1, out byte_en : i1, out reg_addr : i6, out ipwdata : i32, out penable_int : i1, in %iprdata : i32) {
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %false = hw.constant false
    %true = hw.constant true
    %c0_i32 = hw.constant 0 : i32
    %ipwdata = llhd.sig %c0_i32 : i32
    %1 = comb.and %psel, %pwrite : i1
    %2 = comb.xor %pwrite, %true : i1
    %3 = comb.and %psel, %2 : i1
    %4 = comb.extract %paddr from 2 : (i8) -> i6
    %5:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%12: i32, %13: i1):  // 2 preds: ^bb0, ^bb2
      llhd.wait yield (%12, %13 : i32, i1), (%pwdata : i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb1(%pwdata, %true : i32, i1)
    }
    llhd.drv %ipwdata, %5#0 after %0 if %5#1 : i32
    %6 = comb.xor %penable, %true : i1
    %7 = comb.and %3, %6 : i1
    %8 = seq.to_clock %pclk
    %9 = comb.xor %presetn, %true : i1
    %10 = comb.mux bin %7, %iprdata, %prdata : i32
    %prdata = seq.firreg %10 clock %8 reset async %9, %c0_i32 : i32
    %11 = llhd.prb %ipwdata : i32
    hw.output %prdata, %1, %3, %true, %4, %11, %penable : i32, i1, i1, i1, i6, i32, i1
  }
  hw.module private @DW_apb_wdt_isrg(in %clk : i1, in %rst_n : i1, in %eoi_en : i1, in %restart : i1, in %rst_pulse_len : i8, in %resp_mod : i1, in %zero_cnt : i1, out irq_pc : i1, out sys_rst_pc : i1) {
    %true = hw.constant true
    %c1_i8 = hw.constant 1 : i8
    %c0_i8 = hw.constant 0 : i8
    %false = hw.constant false
    %0 = comb.or %restart, %eoi_en {sv.namehint = "clr_intr"} : i1
    %1 = comb.add %rst_cnt, %c1_i8 : i8
    %2 = comb.mux %sys_rst_pc, %1, %c0_i8 : i8
    %3 = seq.to_clock %clk
    %4 = comb.xor %rst_n, %true : i1
    %rst_cnt = seq.firreg %2 clock %3 reset async %4, %c0_i8 : i8
    %5 = comb.and %resp_mod, %0 : i1
    %6 = comb.xor %5, %true : i1
    %7 = comb.xor %resp_mod, %true : i1
    %8 = comb.and %resp_mod, %6, %zero_cnt : i1
    %9 = comb.or %7, %5, %zero_cnt : i1
    %10 = comb.mux bin %9, %8, %int_intr : i1
    %int_intr = seq.firreg %10 clock %3 reset async %4, %false : i1
    %11 = comb.xor %0, %true : i1
    %12 = comb.and %zero_cnt, %11 : i1
    %13 = comb.or %7, %int_intr : i1
    %14 = comb.mux bin %12, %13, %sys_rst_pc : i1
    %sys_rst_pc = seq.firreg %14 clock %3 reset async %4, %false : i1
    hw.output %int_intr, %sys_rst_pc : i1, i1
  }
  hw.module @DW_apb_wdt_bcm36_nhs(in %clk_d : i1, in %rst_d_n : i1, in %data_s : i1, out data_d : i1) {
    hw.output %data_s : i1
  }
  hw.module @DW_apb_wdt_bcm00_ck_inv(in %clk_in : i1, out clk_out : i1) {
    %true = hw.constant true
    %0 = comb.xor %clk_in, %true : i1
    hw.output %0 : i1
  }
  hw.module @wdt_top(in %APB_CLK : i1, in %WDT0_PRESETn : i1, in %WDT1_PRESETn : i1, in %WDT2_PRESETn : i1, in %WDT3_PRESETn : i1, in %WDT4_PRESETn : i1, in %APB1_PSEL : i1, in %APB1_PENABLE : i1, in %APB1_PWRITE : i1, in %APB1_PADDR : i8, in %APB1_PWDATA : i32, out APB1_PRDATA : i32, in %APB2_PSEL : i1, in %APB2_PENABLE : i1, in %APB2_PWRITE : i1, in %APB2_PADDR : i24, in %APB2_PWDATA : i32, out APB2_PRDATA : i32, out WDT0_INTR : i1, out WDT1_INTR : i1, out WDT2_INTR : i1, out WDT3_INTR : i1, out WDT4_INTR : i1, out WDT0_RST_REQ : i1, out WDT1_RST_REQ : i1, out WDT2_RST_REQ : i1, out WDT3_RST_REQ : i1, out WDT4_RST_REQ : i1, in %SCAN_MODE : i1) {
    %true = hw.constant true
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %c0_i32 = hw.constant 0 : i32
    %c-8_i4 = hw.constant -8 : i4
    %c4_i4 = hw.constant 4 : i4
    %c2_i4 = hw.constant 2 : i4
    %c1_i4 = hw.constant 1 : i4
    %c-1_i2 = hw.constant -1 : i2
    %c-2_i2 = hw.constant -2 : i2
    %c1_i2 = hw.constant 1 : i2
    %false = hw.constant false
    %c0_i2 = hw.constant 0 : i2
    %APB2_PRDATA = llhd.sig %c0_i32 : i32
    %1 = comb.extract %APB2_PADDR from 22 : (i24) -> i2
    %2 = comb.icmp eq %1, %c0_i2 : i2
    %3 = comb.and %2, %APB2_PSEL {sv.namehint = "psel_wdt0"} : i1
    %4 = comb.icmp eq %1, %c1_i2 : i2
    %5 = comb.and %4, %APB2_PSEL {sv.namehint = "psel_wdt1"} : i1
    %6 = comb.icmp eq %1, %c-2_i2 : i2
    %7 = comb.and %6, %APB2_PSEL {sv.namehint = "psel_wdt2"} : i1
    %8 = comb.icmp eq %1, %c-1_i2 : i2
    %9 = comb.and %8, %APB2_PSEL {sv.namehint = "psel_wdt3"} : i1
    %10 = comb.concat %9, %7, %5, %3 : i1, i1, i1, i1
    %11:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%14: i32, %15: i1):  // 6 preds: ^bb0, ^bb2, ^bb3, ^bb4, ^bb5, ^bb5
      llhd.wait yield (%14, %15 : i32, i1), (%10, %U_WDT0.prdata, %U_WDT1.prdata, %U_WDT2.prdata, %U_WDT3.prdata : i4, i32, i32, i32, i32), ^bb2
    ^bb2:  // pred: ^bb1
      %16 = comb.icmp ceq %10, %c1_i4 : i4
      cf.cond_br %16, ^bb1(%U_WDT0.prdata, %true : i32, i1), ^bb3
    ^bb3:  // pred: ^bb2
      %17 = comb.icmp ceq %10, %c2_i4 : i4
      cf.cond_br %17, ^bb1(%U_WDT1.prdata, %true : i32, i1), ^bb4
    ^bb4:  // pred: ^bb3
      %18 = comb.icmp ceq %10, %c4_i4 : i4
      cf.cond_br %18, ^bb1(%U_WDT2.prdata, %true : i32, i1), ^bb5
    ^bb5:  // pred: ^bb4
      %19 = comb.icmp ceq %10, %c-8_i4 : i4
      cf.cond_br %19, ^bb1(%U_WDT3.prdata, %true : i32, i1), ^bb1(%c0_i32, %true : i32, i1)
    }
    llhd.drv %APB2_PRDATA, %11#0 after %0 if %11#1 : i32
    %12 = comb.extract %APB2_PADDR from 0 : (i24) -> i8
    %U_WDT0.wdt_intr, %U_WDT0.wdt_sys_rst, %U_WDT0.prdata = hw.instance "U_WDT0" @DW_apb_wdt(pclk: %APB_CLK: i1, presetn: %WDT0_PRESETn: i1, penable: %APB2_PENABLE: i1, pwrite: %APB2_PWRITE: i1, pwdata: %APB2_PWDATA: i32, paddr: %12: i8, psel: %3: i1, speed_up: %false: i1, scan_mode: %SCAN_MODE: i1) -> (wdt_intr: i1, wdt_sys_rst: i1, prdata: i32)
    %U_WDT1.wdt_intr, %U_WDT1.wdt_sys_rst, %U_WDT1.prdata = hw.instance "U_WDT1" @DW_apb_wdt(pclk: %APB_CLK: i1, presetn: %WDT1_PRESETn: i1, penable: %APB2_PENABLE: i1, pwrite: %APB2_PWRITE: i1, pwdata: %APB2_PWDATA: i32, paddr: %12: i8, psel: %5: i1, speed_up: %false: i1, scan_mode: %SCAN_MODE: i1) -> (wdt_intr: i1, wdt_sys_rst: i1, prdata: i32)
    %U_WDT2.wdt_intr, %U_WDT2.wdt_sys_rst, %U_WDT2.prdata = hw.instance "U_WDT2" @DW_apb_wdt(pclk: %APB_CLK: i1, presetn: %WDT2_PRESETn: i1, penable: %APB2_PENABLE: i1, pwrite: %APB2_PWRITE: i1, pwdata: %APB2_PWDATA: i32, paddr: %12: i8, psel: %7: i1, speed_up: %false: i1, scan_mode: %SCAN_MODE: i1) -> (wdt_intr: i1, wdt_sys_rst: i1, prdata: i32)
    %U_WDT3.wdt_intr, %U_WDT3.wdt_sys_rst, %U_WDT3.prdata = hw.instance "U_WDT3" @DW_apb_wdt(pclk: %APB_CLK: i1, presetn: %WDT3_PRESETn: i1, penable: %APB2_PENABLE: i1, pwrite: %APB2_PWRITE: i1, pwdata: %APB2_PWDATA: i32, paddr: %12: i8, psel: %9: i1, speed_up: %false: i1, scan_mode: %SCAN_MODE: i1) -> (wdt_intr: i1, wdt_sys_rst: i1, prdata: i32)
    %U_WDT4.wdt_intr, %U_WDT4.wdt_sys_rst, %U_WDT4.prdata = hw.instance "U_WDT4" @DW_apb_wdt(pclk: %APB_CLK: i1, presetn: %WDT4_PRESETn: i1, penable: %APB1_PENABLE: i1, pwrite: %APB1_PWRITE: i1, pwdata: %APB1_PWDATA: i32, paddr: %APB1_PADDR: i8, psel: %APB1_PSEL: i1, speed_up: %false: i1, scan_mode: %SCAN_MODE: i1) -> (wdt_intr: i1, wdt_sys_rst: i1, prdata: i32)
    %13 = llhd.prb %APB2_PRDATA : i32
    hw.output %U_WDT4.prdata, %13, %U_WDT0.wdt_intr, %U_WDT1.wdt_intr, %U_WDT2.wdt_intr, %U_WDT3.wdt_intr, %U_WDT4.wdt_intr, %U_WDT0.wdt_sys_rst, %U_WDT1.wdt_sys_rst, %U_WDT2.wdt_sys_rst, %U_WDT3.wdt_sys_rst, %U_WDT4.wdt_sys_rst : i32, i32, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1
  }
  hw.module private @DW_apb_wdt_isrc(in %pclk : i1, in %presetn : i1, in %top : i32, in %restart : i1, in %wdt_en : i1, in %speed_up : i1, in %scan_mode : i1, out zero_cnt : i1, out cnt : i32) {
    %c0_i32 = hw.constant 0 : i32
    %U_DW_apb_wdt_cnt.cnt = hw.instance "U_DW_apb_wdt_cnt" @DW_apb_wdt_cnt(clk: %pclk: i1, rst_n: %presetn: i1, start_val: %top: i32, restart: %restart: i1, cnt_en: %wdt_en: i1, speed_up: %speed_up: i1, scan_mode: %scan_mode: i1) -> (cnt: i32) {sv.namehint = "int_cnt"}
    %0 = comb.icmp eq %U_DW_apb_wdt_cnt.cnt, %c0_i32 : i32
    hw.output %0, %U_DW_apb_wdt_cnt.cnt : i1, i32
  }
  hw.module private @DW_apb_wdt_core(in %pclk : i1, in %presetn : i1, in %psel : i1, in %penable : i1, in %pwrite : i1, in %paddr : i8, in %pwdata : i32, out prdata : i32, in %cnt : i32, out restart : i1, out wdt_en : i1, out rmod : i1, in %zero_cnt : i1, in %rmod_isrg : i1, out top_out : i32, in %intr : i1, out irq_pc : i1, out sys_rst_pc : i1) {
    %U_DW_apb_wdt_biu.prdata, %U_DW_apb_wdt_biu.wr_en, %U_DW_apb_wdt_biu.rd_en, %U_DW_apb_wdt_biu.byte_en, %U_DW_apb_wdt_biu.reg_addr, %U_DW_apb_wdt_biu.ipwdata, %U_DW_apb_wdt_biu.penable_int = hw.instance "U_DW_apb_wdt_biu" @DW_apb_wdt_biu(pclk: %pclk: i1, presetn: %presetn: i1, psel: %psel: i1, penable: %penable: i1, pwrite: %pwrite: i1, paddr: %paddr: i8, pwdata: %pwdata: i32, iprdata: %U_DW_apb_wdt_regfile.iprdata: i32) -> (prdata: i32, wr_en: i1, rd_en: i1, byte_en: i1, reg_addr: i6, ipwdata: i32, penable_int: i1) {sv.namehint = "ipwdata"}
    %U_DW_apb_wdt_regfile.iprdata, %U_DW_apb_wdt_regfile.top, %U_DW_apb_wdt_regfile.restart, %U_DW_apb_wdt_regfile.wdt_en, %U_DW_apb_wdt_regfile.eoi_en, %U_DW_apb_wdt_regfile.rpl, %U_DW_apb_wdt_regfile.rmod = hw.instance "U_DW_apb_wdt_regfile" @DW_apb_wdt_regfile(pclk: %pclk: i1, presetn: %presetn: i1, wr_en: %U_DW_apb_wdt_biu.wr_en: i1, rd_en: %U_DW_apb_wdt_biu.rd_en: i1, byte_en: %U_DW_apb_wdt_biu.byte_en: i1, reg_addr: %U_DW_apb_wdt_biu.reg_addr: i6, ipwdata: %U_DW_apb_wdt_biu.ipwdata: i32, penable_int: %U_DW_apb_wdt_biu.penable_int: i1, intr: %intr: i1, cnt: %cnt: i32) -> (iprdata: i32, top: i32, restart: i1, wdt_en: i1, eoi_en: i1, rpl: i8, rmod: i1) {sv.namehint = "rpl"}
    %U_DW_apb_wdt_isrg.irq_pc, %U_DW_apb_wdt_isrg.sys_rst_pc = hw.instance "U_DW_apb_wdt_isrg" @DW_apb_wdt_isrg(clk: %pclk: i1, rst_n: %presetn: i1, eoi_en: %U_DW_apb_wdt_regfile.eoi_en: i1, restart: %U_DW_apb_wdt_regfile.restart: i1, rst_pulse_len: %U_DW_apb_wdt_regfile.rpl: i8, resp_mod: %rmod_isrg: i1, zero_cnt: %zero_cnt: i1) -> (irq_pc: i1, sys_rst_pc: i1)
    hw.output %U_DW_apb_wdt_biu.prdata, %U_DW_apb_wdt_regfile.restart, %U_DW_apb_wdt_regfile.wdt_en, %U_DW_apb_wdt_regfile.rmod, %U_DW_apb_wdt_regfile.top, %U_DW_apb_wdt_isrg.irq_pc, %U_DW_apb_wdt_isrg.sys_rst_pc : i32, i1, i1, i1, i32, i1, i1
  }
  hw.module private @DW_apb_wdt_cnt(in %clk : i1, in %rst_n : i1, in %start_val : i32, in %restart : i1, in %cnt_en : i1, in %speed_up : i1, in %scan_mode : i1, out cnt : i32) {
    %true = hw.constant true
    %c0_i24 = hw.constant 0 : i24
    %c-1_i32 = hw.constant -1 : i32
    %c0_i32 = hw.constant 0 : i32
    %c-1_i8 = hw.constant -1 : i8
    %c65535_i32 = hw.constant 65535 : i32
    %0 = comb.concat %12, %14 : i24, i8
    %1 = comb.icmp eq %cnt, %c0_i32 : i32
    %2 = comb.or %restart, %1 : i1
    %3 = comb.add %cnt, %c-1_i32 : i32
    %4 = comb.xor %2, %true : i1
    %5 = comb.and %cnt_en, %4 : i1
    %6 = comb.mux %5, %3, %0 : i32
    %7 = seq.to_clock %clk
    %8 = comb.xor %rst_n, %true : i1
    %9 = comb.or %2, %cnt_en : i1
    %10 = comb.mux bin %9, %6, %cnt : i32
    %cnt = seq.firreg %10 clock %7 reset async %8, %c65535_i32 : i32
    %11 = comb.extract %start_val from 8 : (i32) -> i24
    %12 = comb.mux %16, %c0_i24, %11 : i24
    %13 = comb.extract %start_val from 0 : (i32) -> i8
    %14 = comb.mux %16, %c-1_i8, %13 : i8
    %15 = comb.extract %cnt from 0 : (i32) -> i1
    %16 = comb.mux %scan_mode, %15, %speed_up {sv.namehint = "int_speed_up"} : i1
    hw.output %cnt : i32
  }
  hw.module private @DW_apb_wdt(in %pclk : i1, in %presetn : i1, in %penable : i1, in %pwrite : i1, in %pwdata : i32, in %paddr : i8, in %psel : i1, in %speed_up : i1, in %scan_mode : i1, out wdt_intr : i1, out wdt_sys_rst : i1, out prdata : i32) {
    %U_DW_apb_wdt_core.prdata, %U_DW_apb_wdt_core.restart, %U_DW_apb_wdt_core.wdt_en, %U_DW_apb_wdt_core.rmod, %U_DW_apb_wdt_core.top_out, %U_DW_apb_wdt_core.irq_pc, %U_DW_apb_wdt_core.sys_rst_pc = hw.instance "U_DW_apb_wdt_core" @DW_apb_wdt_core(pclk: %pclk: i1, presetn: %presetn: i1, psel: %psel: i1, penable: %penable: i1, pwrite: %pwrite: i1, paddr: %paddr: i8, pwdata: %pwdata: i32, cnt: %U_DW_apb_wdt_isrc.cnt: i32, zero_cnt: %U_DW_apb_wdt_isrc.zero_cnt: i1, rmod_isrg: %U_DW_apb_wdt_core.rmod: i1, intr: %U_DW_apb_wdt_core.irq_pc: i1) -> (prdata: i32, restart: i1, wdt_en: i1, rmod: i1, top_out: i32, irq_pc: i1, sys_rst_pc: i1) {sv.namehint = "top"}
    %U_DW_apb_wdt_isrc.zero_cnt, %U_DW_apb_wdt_isrc.cnt = hw.instance "U_DW_apb_wdt_isrc" @DW_apb_wdt_isrc(pclk: %pclk: i1, presetn: %presetn: i1, top: %U_DW_apb_wdt_core.top_out: i32, restart: %U_DW_apb_wdt_core.restart: i1, wdt_en: %U_DW_apb_wdt_core.wdt_en: i1, speed_up: %speed_up: i1, scan_mode: %scan_mode: i1) -> (zero_cnt: i1, cnt: i32) {sv.namehint = "cnt"}
    hw.output %U_DW_apb_wdt_core.irq_pc, %U_DW_apb_wdt_core.sys_rst_pc, %U_DW_apb_wdt_core.prdata : i1, i1, i32
  }
  hw.module private @DW_apb_wdt_regfile(in %pclk : i1, in %presetn : i1, in %wr_en : i1, in %rd_en : i1, in %byte_en : i1, in %reg_addr : i6, in %ipwdata : i32, out iprdata : i32, in %penable_int : i1, out top : i32, out restart : i1, out wdt_en : i1, out eoi_en : i1, out rpl : i8, out rmod : i1, in %intr : i1, in %cnt : i32) {
    %c0_i26 = hw.constant 0 : i26
    %true = hw.constant true
    %c268436032_i32 = hw.constant 268436032 : i32
    %c-7_i6 = hw.constant -7 : i6
    %c-6_i6 = hw.constant -6 : i6
    %c-5_i6 = hw.constant -5 : i6
    %c-4_i6 = hw.constant -4 : i6
    %c-3_i6 = hw.constant -3 : i6
    %c-1_i6 = hw.constant -1 : i6
    %c-2_i6 = hw.constant -2 : i6
    %c5_i6 = hw.constant 5 : i6
    %c4_i6 = hw.constant 4 : i6
    %c3_i6 = hw.constant 3 : i6
    %c2_i6 = hw.constant 2 : i6
    %c1_i6 = hw.constant 1 : i6
    %c0_i29 = hw.constant 0 : i29
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %c0_i8 = hw.constant 0 : i8
    %c0_i6 = hw.constant 0 : i6
    %c1_i8 = hw.constant 1 : i8
    %c-1_i8 = hw.constant -1 : i8
    %c127_i8 = hw.constant 127 : i8
    %c63_i8 = hw.constant 63 : i8
    %c31_i8 = hw.constant 31 : i8
    %c15_i8 = hw.constant 15 : i8
    %c7_i8 = hw.constant 7 : i8
    %c3_i8 = hw.constant 3 : i8
    %c65535_i32 = hw.constant 65535 : i32
    %c1146552608_i32 = hw.constant 1146552608 : i32
    %c825308202_i32 = hw.constant 825308202 : i32
    %c118_i8 = hw.constant 118 : i8
    %false = hw.constant false
    %c7_i32 = hw.constant 7 : i32
    %c6_i32 = hw.constant 6 : i32
    %c5_i32 = hw.constant 5 : i32
    %c4_i32 = hw.constant 4 : i32
    %c3_i32 = hw.constant 3 : i32
    %c2_i32 = hw.constant 2 : i32
    %c1_i32 = hw.constant 1 : i32
    %c0_i32 = hw.constant 0 : i32
    %wdt_ccvr = llhd.sig %c0_i32 : i32
    %int_cnt = llhd.sig %c0_i32 : i32
    %rpl = llhd.sig %c0_i8 : i8
    %1 = comb.and %penable_int, %wr_en {sv.namehint = "wr_en_int"} : i1
    %2 = comb.xor %penable_int, %true : i1
    %3 = comb.icmp eq %reg_addr, %c0_i6 : i6
    %4 = comb.icmp eq %reg_addr, %c1_i6 : i6
    %5 = comb.icmp eq %reg_addr, %c2_i6 : i6
    %6 = comb.icmp eq %reg_addr, %c3_i6 : i6
    %7 = comb.icmp eq %reg_addr, %c5_i6 : i6
    %8 = comb.icmp eq %reg_addr, %c-2_i6 : i6
    %9 = comb.icmp eq %reg_addr, %c-1_i6 : i6
    %10 = comb.icmp eq %reg_addr, %c-3_i6 : i6
    %11 = comb.icmp eq %reg_addr, %c-4_i6 : i6
    %12 = comb.icmp eq %reg_addr, %c-5_i6 : i6
    %13 = comb.icmp eq %reg_addr, %c-6_i6 : i6
    %14 = comb.icmp eq %reg_addr, %c-7_i6 : i6
    %15 = comb.and %3, %1 : i1
    %16 = comb.extract %ipwdata from 1 : (i32) -> i5
    %17 = comb.extract %wdt_cr_ir from 0 : (i6) -> i1
    %18 = comb.concat %16, %17 : i5, i1
    %19 = comb.extract %ipwdata from 0 : (i32) -> i6
    %20 = comb.and %byte_en, %15, %17 : i1
    %21 = comb.mux %20, %18, %19 : i6
    %22 = comb.xor %byte_en, %true : i1
    %23 = comb.and %15, %22 : i1
    %24 = comb.xor %23, %true : i1
    %25 = comb.xor %15, %true : i1
    %26 = comb.and %15, %24 : i1
    %27 = seq.to_clock %pclk
    %28 = comb.xor %presetn, %true : i1
    %29 = comb.xor %26, %true : i1
    %30 = comb.or %29, %25, %23 : i1
    %31 = comb.mux bin %30, %wdt_cr_ir, %21 : i6
    %wdt_cr_ir = seq.firreg %31 clock %27 reset async %28, %c0_i6 : i6
    %32 = comb.and %4, %1, %byte_en : i1
    %33 = comb.mux bin %32, %ipwdata, %wdt_torr_ir : i32
    %wdt_torr_ir = seq.firreg %33 clock %27 reset async %28, %c0_i32 : i32
    %34 = comb.extract %ipwdata from 0 : (i32) -> i8
    %35 = comb.icmp eq %34, %c118_i8 : i8
    %36 = comb.and %6, %1, %byte_en, %35 : i1
    %37:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%63: i32, %64: i1):  // 2 preds: ^bb0, ^bb2
      llhd.wait yield (%63, %64 : i32, i1), (%cnt : i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb1(%cnt, %true : i32, i1)
    }
    llhd.drv %int_cnt, %37#0 after %0 if %37#1 : i32
    %38 = llhd.prb %int_cnt : i32
    %39:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%63: i32, %64: i1):  // 2 preds: ^bb0, ^bb2
      llhd.wait yield (%63, %64 : i32, i1), (%38 : i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb1(%38, %true : i32, i1)
    }
    llhd.drv %wdt_ccvr, %39#0 after %0 if %39#1 : i32
    %40 = comb.xor %prev_wdt_eoi_rd, %true : i1
    %41 = comb.and %42, %40 : i1
    %prev_wdt_eoi_rd = seq.firreg %42 clock %27 reset async %28, %false : i1
    %42 = comb.and %7, %2, %rd_en : i1
    %43 = llhd.prb %wdt_ccvr : i32
    %44 = comb.concat %c0_i26, %wdt_cr_ir : i26, i6
    %45 = comb.xor %3, %true : i1
    %46 = comb.mux %45, %c0_i32, %44 : i32
    %47 = comb.mux %4, %wdt_torr_ir, %46 : i32
    %48 = comb.mux %5, %43, %47 : i32
    %49 = comb.extract %48 from 1 : (i32) -> i31
    %50 = comb.concat %49, %intr : i31, i1
    %51 = comb.icmp ne %reg_addr, %c4_i6 : i6
    %52 = comb.mux %51, %48, %50 : i32
    %53 = comb.mux %8, %c825308202_i32, %52 : i32
    %54 = comb.mux %9, %c1146552608_i32, %53 : i32
    %55 = comb.mux %10, %c268436032_i32, %54 : i32
    %56 = comb.mux %11, %c65535_i32, %55 : i32
    %57 = comb.or %14, %13, %12 : i1
    %58 = comb.mux %57, %c0_i32, %56 : i32
    %59 = comb.extract %wdt_cr_ir from 2 : (i6) -> i3
    %60:2 = llhd.process -> i8, i1 {
      cf.br ^bb1(%c0_i8, %false : i8, i1)
    ^bb1(%63: i8, %64: i1):  // 9 preds: ^bb0, ^bb2, ^bb3, ^bb4, ^bb5, ^bb6, ^bb7, ^bb8, ^bb8
      llhd.wait yield (%63, %64 : i8, i1), (%59 : i3), ^bb2
    ^bb2:  // pred: ^bb1
      %65 = comb.concat %c0_i29, %59 : i29, i3
      %66 = comb.icmp ceq %65, %c1_i32 : i32
      cf.cond_br %66, ^bb1(%c3_i8, %true : i8, i1), ^bb3
    ^bb3:  // pred: ^bb2
      %67 = comb.icmp ceq %65, %c2_i32 : i32
      cf.cond_br %67, ^bb1(%c7_i8, %true : i8, i1), ^bb4
    ^bb4:  // pred: ^bb3
      %68 = comb.icmp ceq %65, %c3_i32 : i32
      cf.cond_br %68, ^bb1(%c15_i8, %true : i8, i1), ^bb5
    ^bb5:  // pred: ^bb4
      %69 = comb.icmp ceq %65, %c4_i32 : i32
      cf.cond_br %69, ^bb1(%c31_i8, %true : i8, i1), ^bb6
    ^bb6:  // pred: ^bb5
      %70 = comb.icmp ceq %65, %c5_i32 : i32
      cf.cond_br %70, ^bb1(%c63_i8, %true : i8, i1), ^bb7
    ^bb7:  // pred: ^bb6
      %71 = comb.icmp ceq %65, %c6_i32 : i32
      cf.cond_br %71, ^bb1(%c127_i8, %true : i8, i1), ^bb8
    ^bb8:  // pred: ^bb7
      %72 = comb.icmp ceq %65, %c7_i32 : i32
      cf.cond_br %72, ^bb1(%c-1_i8, %true : i8, i1), ^bb1(%c1_i8, %true : i8, i1)
    }
    llhd.drv %rpl, %60#0 after %0 if %60#1 : i8
    %61 = comb.extract %wdt_cr_ir from 1 : (i6) -> i1
    %62 = llhd.prb %rpl : i8
    hw.output %58, %wdt_torr_ir, %36, %17, %41, %62, %61 : i32, i32, i1, i1, i1, i8, i1
  }
  hw.module @DW_apb_wdt_bcm00_maj(in %a : i1, in %b : i1, in %c : i1, out z : i1) {
    %0 = comb.and %a, %b : i1
    %1 = comb.and %a, %c : i1
    %2 = comb.and %b, %c : i1
    %3 = comb.or %0, %1, %2 : i1
    hw.output %3 : i1
  }
  hw.module @DW_apb_wdt_bcm99(in %clk_d : i1, in %rst_d_n : i1, in %data_s : i1, out data_d : i1) {
    %true = hw.constant true
    %false = hw.constant false
    %0 = seq.to_clock %clk_d
    %1 = comb.xor %rst_d_n, %true : i1
    %sample_meta = seq.firreg %data_s clock %0 reset async %1, %false : i1
    %sample_syncl = seq.firreg %sample_meta clock %0 reset async %1, %false : i1
    hw.output %sample_syncl : i1
  }
}
