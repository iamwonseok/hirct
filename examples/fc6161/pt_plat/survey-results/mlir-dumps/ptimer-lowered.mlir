module {
  hw.module private @ptimer_apbif(in %PCLK : i1, in %PRESETn : i1, in %PSEL : i1, in %PENABLE : i1, in %PWRITE : i1, in %PADDR : i3, in %PWDATA : i32, out PRDATA : i32, in %nTIMER_CLR_SYNC : i1, in %TDAT_LDV_UPDATE_SYNC : i1, in %TCNT_UPDATE_SYNC : i1, in %TDUR_UPDATE_SYNC : i1, in %TDAT_LDV : i32, in %TCNT_LDV : i32, in %TDUR_LDV : i32, in %INT_TMC_SYNC : i1, in %INT_TOF_SYNC : i1, out TDAT_UPDATE : i1, out TOMS_UPDATE : i1, out TPWM_UPDATE : i1, out TLCV_UPDATE : i1, out TOVF_UPDATE : i1, out CNT_TDAT : i32, out CNT_TEN : i1, out CNT_OMS : i3, out CNT_IVT : i1, out CNT_CLR : i1, out CNT_TPWM : i32, out CNT_TLCV : i32, out CNT_TOVF : i32, out TMC_INTR_CLR : i1, out TOF_INTR_CLR : i1, in %TMC_INTR_CLR_ACK_SYNC : i1, in %TOF_INTR_CLR_ACK_SYNC : i1) {
    %true = hw.constant true
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %c0_i30 = hw.constant 0 : i30
    %c-128_i8 = hw.constant -128 : i8
    %c64_i8 = hw.constant 64 : i8
    %c32_i8 = hw.constant 32 : i8
    %c16_i8 = hw.constant 16 : i8
    %c8_i8 = hw.constant 8 : i8
    %c4_i8 = hw.constant 4 : i8
    %c0_i26 = hw.constant 0 : i26
    %c2_i8 = hw.constant 2 : i8
    %c1_i8 = hw.constant 1 : i8
    %c0_i6 = hw.constant 0 : i6
    %c-4_i3 = hw.constant -4 : i3
    %c2_i3 = hw.constant 2 : i3
    %c-1_i3 = hw.constant -1 : i3
    %c-2_i3 = hw.constant -2 : i3
    %c-3_i3 = hw.constant -3 : i3
    %c3_i3 = hw.constant 3 : i3
    %c1_i3 = hw.constant 1 : i3
    %false = hw.constant false
    %c0_i32 = hw.constant 0 : i32
    %c0_i3 = hw.constant 0 : i3
    %1 = comb.concat %53, %56 : i1, i5
    %PRDATA = llhd.sig %c0_i32 : i32
    %2 = comb.and %PENABLE, %PSEL : i1
    %3 = comb.and %2, %PWRITE {sv.namehint = "iwen"} : i1
    %4 = comb.xor %PWRITE, %true : i1
    %5 = comb.and %2, %4 {sv.namehint = "iren"} : i1
    %6 = comb.mux %PSEL, %PADDR, %c0_i3 {sv.namehint = "paddr_f"} : i3
    %7 = comb.and %PSEL, %PWRITE : i1
    %8 = comb.mux %7, %PWDATA, %c0_i32 : i32
    %9 = comb.icmp eq %6, %c0_i3 : i3
    %10 = comb.and %3, %9 : i1
    %11 = comb.icmp eq %6, %c1_i3 : i3
    %12 = comb.and %3, %11 : i1
    %13 = comb.icmp eq %6, %c3_i3 : i3
    %14 = comb.and %3, %13 : i1
    %15 = comb.icmp eq %6, %c-3_i3 : i3
    %16 = comb.and %3, %15 : i1
    %17 = comb.icmp eq %6, %c-2_i3 : i3
    %18 = comb.and %3, %17 : i1
    %19 = comb.icmp eq %6, %c-1_i3 : i3
    %20 = comb.and %3, %19 : i1
    %21 = seq.to_clock %PCLK
    %22 = comb.xor %PRESETn, %true : i1
    %tdat_ldv_update_sync_del = seq.firreg %TDAT_LDV_UPDATE_SYNC clock %21 reset async %22, %false : i1
    %tcnt_update_sync_del = seq.firreg %TCNT_UPDATE_SYNC clock %21 reset async %22, %false : i1
    %tdur_update_sync_del = seq.firreg %TDUR_UPDATE_SYNC clock %21 reset async %22, %false : i1
    %23 = comb.xor %tdat_ldv_update_sync_del, %TDAT_LDV_UPDATE_SYNC {sv.namehint = "itdat_load"} : i1
    %24 = comb.xor %tcnt_update_sync_del, %TCNT_UPDATE_SYNC {sv.namehint = "itcnt_load"} : i1
    %25 = comb.xor %tdur_update_sync_del, %TDUR_UPDATE_SYNC {sv.namehint = "itdur_load"} : i1
    %26 = comb.xor %10, %curr_tdat_update : i1
    %27 = comb.extract %itcon from 1 : (i6) -> i3
    %28 = comb.extract %8 from 1 : (i32) -> i3
    %29 = comb.icmp ne %27, %28 : i3
    %30 = comb.and %12, %29 : i1
    %31 = comb.xor %30, %curr_toms_update : i1
    %32 = comb.xor %14, %curr_tpwm_update : i1
    %33 = comb.xor %16, %curr_tlcv_update : i1
    %34 = comb.xor %18, %curr_tovf_update : i1
    %curr_tdat_update = seq.firreg %26 clock %21 reset async %22, %false : i1
    %curr_toms_update = seq.firreg %31 clock %21 reset async %22, %false : i1
    %curr_tpwm_update = seq.firreg %32 clock %21 reset async %22, %false : i1
    %curr_tlcv_update = seq.firreg %33 clock %21 reset async %22, %false : i1
    %curr_tovf_update = seq.firreg %34 clock %21 reset async %22, %false : i1
    %35 = comb.and %5, %9 : i1
    %36 = comb.and %5, %11 : i1
    %37 = comb.icmp eq %6, %c2_i3 : i3
    %38 = comb.and %5, %37 : i1
    %39 = comb.and %5, %13 : i1
    %40 = comb.icmp eq %6, %c-4_i3 : i3
    %41 = comb.and %5, %40 : i1
    %42 = comb.and %5, %15 : i1
    %43 = comb.and %5, %17 : i1
    %44 = comb.and %5, %19 : i1
    %45 = comb.mux %10, %8, %itdat : i32
    %46 = comb.mux %23, %TDAT_LDV, %45 : i32
    %47 = comb.extract %itcon from 5 : (i6) -> i1
    %48 = comb.xor %nTIMER_CLR_SYNC, %true : i1
    %49 = comb.and %47, %48 : i1
    %50 = comb.extract %8 from 5 : (i32) -> i1
    %51 = comb.mux %12, %50, %47 : i1
    %52 = comb.xor %49, %true : i1
    %53 = comb.and %52, %51 : i1
    %54 = comb.extract %8 from 0 : (i32) -> i5
    %55 = comb.extract %itcon from 0 : (i6) -> i5
    %56 = comb.mux %12, %54, %55 : i5
    %57 = comb.mux %24, %TCNT_LDV, %itcnt : i32
    %58 = comb.mux %14, %8, %itpwm : i32
    %59 = comb.mux %25, %TDUR_LDV, %itdur : i32
    %60 = comb.mux %16, %8, %itlcv : i32
    %61 = comb.mux %18, %8, %itovf : i32
    %itdat = seq.firreg %46 clock %21 reset async %22, %c0_i32 : i32
    %itcon = seq.firreg %1 clock %21 reset async %22, %c0_i6 : i6
    %itcnt = seq.firreg %57 clock %21 reset async %22, %c0_i32 : i32
    %itpwm = seq.firreg %58 clock %21 reset async %22, %c0_i32 : i32
    %itdur = seq.firreg %59 clock %21 reset async %22, %c0_i32 : i32
    %itlcv = seq.firreg %60 clock %21 reset async %22, %c0_i32 : i32
    %itovf = seq.firreg %61 clock %21 reset async %22, %c0_i32 : i32
    %62 = comb.concat %44, %43, %42, %41, %39, %38, %36, %35 : i1, i1, i1, i1, i1, i1, i1, i1
    %63:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%77: i32, %78: i1):  // 10 preds: ^bb0, ^bb2, ^bb4, ^bb5, ^bb6, ^bb7, ^bb8, ^bb9, ^bb10, ^bb11
      llhd.wait yield (%77, %78 : i32, i1), (%62, %itdat, %itcon, %itcnt, %itpwm, %itdur, %itlcv, %itovf, %INT_TOF_SYNC, %INT_TMC_SYNC : i8, i32, i6, i32, i32, i32, i32, i32, i1, i1), ^bb2
    ^bb2:  // pred: ^bb1
      %79 = comb.icmp ceq %62, %c1_i8 : i8
      cf.cond_br %79, ^bb1(%itdat, %true : i32, i1), ^bb3
    ^bb3:  // pred: ^bb2
      %80 = comb.icmp ceq %62, %c2_i8 : i8
      cf.cond_br %80, ^bb4, ^bb5
    ^bb4:  // pred: ^bb3
      %81 = comb.concat %c0_i26, %itcon : i26, i6
      cf.br ^bb1(%81, %true : i32, i1)
    ^bb5:  // pred: ^bb3
      %82 = comb.icmp ceq %62, %c4_i8 : i8
      cf.cond_br %82, ^bb1(%itcnt, %true : i32, i1), ^bb6
    ^bb6:  // pred: ^bb5
      %83 = comb.icmp ceq %62, %c8_i8 : i8
      cf.cond_br %83, ^bb1(%itpwm, %true : i32, i1), ^bb7
    ^bb7:  // pred: ^bb6
      %84 = comb.icmp ceq %62, %c16_i8 : i8
      cf.cond_br %84, ^bb1(%itdur, %true : i32, i1), ^bb8
    ^bb8:  // pred: ^bb7
      %85 = comb.icmp ceq %62, %c32_i8 : i8
      cf.cond_br %85, ^bb1(%itlcv, %true : i32, i1), ^bb9
    ^bb9:  // pred: ^bb8
      %86 = comb.icmp ceq %62, %c64_i8 : i8
      cf.cond_br %86, ^bb1(%itovf, %true : i32, i1), ^bb10
    ^bb10:  // pred: ^bb9
      %87 = comb.icmp ceq %62, %c-128_i8 : i8
      cf.cond_br %87, ^bb11, ^bb1(%c0_i32, %true : i32, i1)
    ^bb11:  // pred: ^bb10
      %88 = comb.concat %c0_i30, %INT_TOF_SYNC, %INT_TMC_SYNC : i30, i1, i1
      cf.br ^bb1(%88, %true : i32, i1)
    }
    llhd.drv %PRDATA, %63#0 after %0 if %63#1 : i32
    %64 = comb.extract %itcon from 0 : (i6) -> i1
    %65 = comb.extract %itcon from 4 : (i6) -> i1
    %66 = comb.and %curr_tmc_intr_clr, %TMC_INTR_CLR_ACK_SYNC : i1
    %67 = comb.extract %8 from 0 : (i32) -> i1
    %68 = comb.mux %20, %67, %curr_tmc_intr_clr : i1
    %69 = comb.xor %66, %true : i1
    %70 = comb.and %69, %68 : i1
    %71 = comb.and %curr_tof_intr_clr, %TOF_INTR_CLR_ACK_SYNC : i1
    %72 = comb.extract %8 from 1 : (i32) -> i1
    %73 = comb.mux %20, %72, %curr_tof_intr_clr : i1
    %74 = comb.xor %71, %true : i1
    %75 = comb.and %74, %73 : i1
    %curr_tmc_intr_clr = seq.firreg %70 clock %21 reset async %22, %false : i1
    %curr_tof_intr_clr = seq.firreg %75 clock %21 reset async %22, %false : i1
    %76 = llhd.prb %PRDATA : i32
    hw.output %76, %curr_tdat_update, %curr_toms_update, %curr_tpwm_update, %curr_tlcv_update, %curr_tovf_update, %itdat, %64, %27, %65, %47, %itpwm, %itlcv, %itovf, %curr_tmc_intr_clr, %curr_tof_intr_clr : i32, i1, i1, i1, i1, i1, i32, i1, i3, i1, i1, i32, i32, i32, i1, i1
  }
  hw.module private @ptimer_cnt(in %TIMER_CLK : i1, in %nTIMER_RST : i1, in %nTIMER_CLR : i1, in %TDAT_UPDATE_SYNC : i1, in %TOMS_UPDATE_SYNC : i1, in %TPWM_UPDATE_SYNC : i1, in %TLCV_UPDATE_SYNC : i1, in %TOVF_UPDATE_SYNC : i1, in %CNT_TDAT : i32, in %CNT_TEN_SYNC : i1, in %CNT_OMS : i3, in %CNT_IVT_SYNC : i1, in %CNT_TPWM : i32, in %CNT_TLCV : i32, in %CNT_TOVF : i32, out TDAT_LDV_UPDATE : i1, out TCNT_UPDATE : i1, out TDUR_UPDATE : i1, out TDAT_LDV : i32, out TCNT_LDV : i32, out TDUR_LDV : i32, in %TCAP : i1, out TOUT : i1, in %TMC_INTR_CLR_SYNC : i1, in %TOF_INTR_CLR_SYNC : i1, out INT_TMC : i1, out INT_TOF : i1, out INT_TIMER : i1, out TMC_INTR_CLR_ACK : i1, out TOF_INTR_CLR_ACK : i1) {
    %true = hw.constant true
    %c2_i3 = hw.constant 2 : i3
    %c1_i3 = hw.constant 1 : i3
    %c1_i32 = hw.constant 1 : i32
    %c-3_i3 = hw.constant -3 : i3
    %c-4_i3 = hw.constant -4 : i3
    %c3_i3 = hw.constant 3 : i3
    %c-1_i3 = hw.constant -1 : i3
    %c-2_i3 = hw.constant -2 : i3
    %c-1_i32 = hw.constant -1 : i32
    %c0_i3 = hw.constant 0 : i3
    %c0_i32 = hw.constant 0 : i32
    %false = hw.constant false
    %0 = seq.to_clock %TIMER_CLK
    %1 = comb.xor %nTIMER_RST, %true : i1
    %tdat_update_sync_del = seq.firreg %TDAT_UPDATE_SYNC clock %0 reset async %1, %false : i1
    %toms_update_sync_del = seq.firreg %TOMS_UPDATE_SYNC clock %0 reset async %1, %false : i1
    %tpwm_update_sync_del = seq.firreg %TPWM_UPDATE_SYNC clock %0 reset async %1, %false : i1
    %tlcv_update_sync_del = seq.firreg %TLCV_UPDATE_SYNC clock %0 reset async %1, %false : i1
    %tovf_update_sync_del = seq.firreg %TOVF_UPDATE_SYNC clock %0 reset async %1, %false : i1
    %2 = comb.xor %tdat_update_sync_del, %TDAT_UPDATE_SYNC {sv.namehint = "itdat_load"} : i1
    %3 = comb.xor %toms_update_sync_del, %TOMS_UPDATE_SYNC {sv.namehint = "itoms_load"} : i1
    %4 = comb.xor %tpwm_update_sync_del, %TPWM_UPDATE_SYNC {sv.namehint = "itpwm_load"} : i1
    %5 = comb.xor %tlcv_update_sync_del, %TLCV_UPDATE_SYNC : i1
    %6 = comb.xor %tovf_update_sync_del, %TOVF_UPDATE_SYNC {sv.namehint = "itovf_load"} : i1
    %7 = comb.mux %2, %CNT_TDAT, %curr_cnt_tdat : i32
    %8 = comb.mux %3, %CNT_OMS, %curr_cnt_oms : i3
    %9 = comb.mux %4, %CNT_TPWM, %curr_cnt_tpwm : i32
    %10 = comb.mux %6, %CNT_TOVF, %curr_cnt_tovf : i32
    %curr_cnt_tdat = seq.firreg %7 clock %0 reset async %1, %c0_i32 : i32
    %curr_cnt_oms = seq.firreg %8 clock %0 reset async %1, %c0_i3 : i3
    %curr_cnt_tpwm = seq.firreg %9 clock %0 reset async %1, %c0_i32 : i32
    %curr_cnt_tovf = seq.firreg %10 clock %0 reset async %1, %c-1_i32 : i32
    %11 = comb.icmp eq %curr_cnt_oms, %c-2_i3 : i3
    %12 = comb.and %tcap_sync, %i_tof : i1
    %13 = comb.or %32, %12 : i1
    %14 = comb.and %11, %13 : i1
    %15 = comb.icmp eq %curr_cnt_oms, %c-1_i3 : i3
    %16 = comb.xor %tcap_sync, %true : i1
    %17 = comb.and %16, %i_tof : i1
    %18 = comb.or %31, %17 : i1
    %19 = comb.and %15, %18 : i1
    %20 = comb.or %14, %19 : i1
    %21 = comb.and %11, %16, %i_tof : i1
    %22 = comb.and %15, %tcap_sync, %i_tof : i1
    %23 = comb.or %21, %22 : i1
    %24 = comb.mux %23, %c0_i32, %curr_mdur : i32
    %25 = comb.mux %20, %curr_tcnt, %24 : i32
    %curr_mdur = seq.firreg %25 clock %0 reset async %1, %c0_i32 : i32
    %26 = comb.icmp eq %curr_cnt_oms, %c3_i3 : i3
    %27 = comb.icmp eq %curr_cnt_oms, %c-4_i3 : i3
    %28 = comb.icmp eq %curr_cnt_oms, %c-3_i3 : i3
    %29 = comb.or %26, %27, %28, %11, %15 : i1
    %tcap_f = seq.firreg %TCAP clock %0 reset async %1, %false : i1
    %tcap_sync = seq.firreg %tcap_f clock %0 reset async %1, %false : i1
    %tcap_sync_del = seq.firreg %tcap_sync clock %0 reset async %1, %false : i1
    %30 = comb.xor %tcap_sync_del, %true : i1
    %31 = comb.and %tcap_sync, %30 : i1
    %32 = comb.and %16, %tcap_sync_del : i1
    %33 = comb.or %31, %32 : i1
    %34 = comb.or %26, %15 : i1
    %35 = comb.or %27, %11 : i1
    %36 = comb.and %35, %31 : i1
    %37 = comb.mux %34, %32, %36 : i1
    %38 = comb.mux %28, %33, %37 : i1
    %cnt_ten_sync_del = seq.firreg %CNT_TEN_SYNC clock %0 reset async %1, %false : i1
    %39 = comb.xor %cnt_ten_sync_del, %true : i1
    %40 = comb.and %CNT_TEN_SYNC, %39 : i1
    %41 = comb.icmp ceq %curr_cnt_oms, %c0_i3 : i3
    %42 = comb.icmp eq %curr_tcnt, %curr_fdat : i32
    %43 = comb.or %40, %42 : i1
    %44 = comb.mux %43, %curr_cnt_tdat, %curr_fdat : i32
    %45 = comb.add %curr_fdat, %c-1_i32 : i32
    %46 = comb.icmp eq %curr_tcnt, %45 : i32
    %47 = comb.icmp ceq %curr_cnt_oms, %c1_i3 : i3
    %48 = comb.add %curr_cnt_tdat, %c-1_i32 : i32
    %49 = comb.icmp eq %curr_tcnt, %48 : i32
    %50 = comb.add %curr_cnt_tovf, %c-1_i32 : i32
    %51 = comb.icmp eq %curr_tcnt, %50 : i32
    %52 = comb.icmp eq %curr_tcnt, %curr_cnt_tovf : i32
    %53 = comb.icmp ceq %curr_cnt_oms, %c2_i3 : i3
    %54 = comb.icmp eq %curr_tcnt, %curr_fpwm : i32
    %55 = comb.or %40, %54 : i1
    %56 = comb.mux %55, %curr_cnt_tdat, %curr_fdat : i32
    %57 = comb.mux %55, %curr_cnt_tpwm, %curr_fpwm : i32
    %58 = comb.add %curr_tcnt, %c1_i32 : i32
    %59 = comb.add %curr_fpwm, %c-1_i32 : i32
    %60 = comb.icmp eq %curr_tcnt, %59 : i32
    %61 = comb.xor %5, %true : i1
    %62 = comb.and %CNT_TEN_SYNC, %61 : i1
    %63 = comb.xor %41, %true : i1
    %64 = comb.and %63, %62 : i1
    %65 = comb.and %47, %64 : i1
    %66 = comb.xor %49, %true : i1
    %67 = comb.and %66, %65 : i1
    %68 = comb.xor %51, %true : i1
    %69 = comb.and %68, %67 : i1
    %70 = comb.and %69, %52 : i1
    %71 = comb.xor %70, %true : i1
    %72 = comb.xor %43, %true : i1
    %73 = comb.or %70, %72 : i1
    %74 = comb.mux %73, %curr_fdat, %curr_cnt_tdat : i32
    %75 = comb.and %71, %43 : i1
    %76 = comb.xor %47, %true : i1
    %77 = comb.and %76, %64 : i1
    %78 = comb.and %53, %77 : i1
    %79 = comb.and %78, %54 : i1
    %80 = comb.or %79, %71 : i1
    %81 = comb.mux %79, %56, %74 : i32
    %82 = comb.mux %79, %55, %75 : i1
    %83 = comb.xor %53, %true : i1
    %84 = comb.and %83, %77 : i1
    %85 = comb.xor %38, %true : i1
    %86 = comb.and %85, %84 : i1
    %87 = comb.and %86, %52 : i1
    %88 = comb.xor %87, %true : i1
    %89 = comb.and %88, %80 : i1
    %90 = comb.xor %79, %true : i1
    %91 = comb.and %84, %38 : i1
    %92 = comb.or %91, %87 : i1
    %93 = comb.mux %92, %curr_fdat, %81 : i32
    %94 = comb.xor %91, %true : i1
    %95 = comb.and %94, %88, %82 : i1
    %96 = comb.xor %55, %true : i1
    %97 = comb.or %91, %87, %90, %96 : i1
    %98 = comb.and %94, %88, %79, %55 : i1
    %99 = comb.xor %CNT_TEN_SYNC, %true : i1
    %100 = comb.and %61, %99 : i1
    %101 = comb.xor %100, %true : i1
    %102 = comb.or %100, %91, %89 : i1
    %103 = comb.or %100, %91, %87, %79, %70 : i1
    %104 = comb.mux %100, %curr_tcnt, %c0_i32 : i32
    %105 = comb.mux %100, %curr_cnt_tdat, %93 : i32
    %106 = comb.or %100, %95 : i1
    %107 = comb.xor %97, %true : i1
    %108 = comb.or %100, %107 : i1
    %109 = comb.or %100, %98 : i1
    %110 = comb.and %61, %101, %91 : i1
    %111 = comb.and %61, %102 : i1
    %112 = comb.mux %5, %CNT_TLCV, %104 : i32
    %113 = comb.mux %5, %curr_fdat, %105 : i32
    %114 = comb.and %61, %106 : i1
    %115 = comb.and %41, %62 : i1
    %116 = comb.and %46, %115 : i1
    %117 = comb.or %116, %110 : i1
    %118 = comb.xor %116, %true : i1
    %119 = comb.xor %46, %true : i1
    %120 = comb.xor %42, %true : i1
    %121 = comb.and %120, %119, %115 : i1
    %122 = comb.xor %121, %true : i1
    %123 = comb.and %122, %117 : i1
    %124 = comb.or %121, %116 : i1
    %125 = comb.mux %124, %44, %113 : i32
    %126 = comb.mux %124, %43, %114 : i1
    %127 = comb.and %49, %65 : i1
    %128 = comb.or %127, %123 : i1
    %129 = comb.or %127, %121, %116, %111 : i1
    %130 = comb.xor %127, %true : i1
    %131 = comb.and %130, %122, %118, %61, %103 : i1
    %132 = comb.and %51, %67 : i1
    %133 = comb.xor %132, %true : i1
    %134 = comb.and %133, %129 : i1
    %135 = comb.or %132, %131 : i1
    %136 = comb.xor %52, %true : i1
    %137 = comb.and %136, %69 : i1
    %138 = comb.xor %137, %true : i1
    %139 = comb.and %138, %133, %128 : i1
    %140 = comb.or %137, %132, %127 : i1
    %141 = comb.mux %140, %curr_fdat, %125 : i32
    %142 = comb.and %138, %133, %130, %126 : i1
    %143 = comb.xor %108, %true : i1
    %144 = comb.or %137, %132, %127, %121, %116, %5, %143 : i1
    %145 = comb.mux %144, %curr_fpwm, %curr_cnt_tpwm : i32
    %146 = comb.and %138, %133, %130, %122, %118, %61, %109 : i1
    %147 = comb.xor %54, %true : i1
    %148 = comb.and %147, %78 : i1
    %149 = comb.and %46, %148 : i1
    %150 = comb.or %149, %139 : i1
    %151 = comb.or %149, %137, %134 : i1
    %152 = comb.xor %149, %true : i1
    %153 = comb.and %152, %138, %132 : i1
    %154 = comb.and %152, %138, %135 : i1
    %155 = comb.and %119, %148 : i1
    %156 = comb.and %60, %155 : i1
    %157 = comb.xor %156, %true : i1
    %158 = comb.and %157, %151 : i1
    %159 = comb.or %156, %153 : i1
    %160 = comb.xor %60, %true : i1
    %161 = comb.and %160, %155 : i1
    %162 = comb.xor %161, %true : i1
    %163 = comb.or %161, %158 : i1
    %164 = comb.and %162, %159 : i1
    %165 = comb.or %161, %156, %149 : i1
    %166 = comb.mux %165, %56, %141 : i32
    %167 = comb.mux %165, %55, %142 : i1
    %168 = comb.mux %165, %57, %145 : i32
    %169 = comb.mux %165, %55, %146 : i1
    %170 = comb.and %136, %86 : i1
    %171 = comb.and %51, %170 : i1
    %172 = comb.xor %171, %true : i1
    %173 = comb.and %172, %163 : i1
    %174 = comb.or %171, %164 : i1
    %175 = comb.and %68, %170 : i1
    %176 = comb.xor %175, %true : i1
    %177 = comb.and %176, %172, %162, %157, %150 : i1
    %178 = comb.or %175, %173 : i1
    %179 = comb.and %176, %174 : i1
    %180 = comb.or %175, %171, %161, %156, %154 : i1
    %181 = comb.and %176, %172, %162, %157, %152, %138, %133, %130, %122, %116 : i1
    %182 = comb.or %175, %171, %161, %156, %149, %137, %132, %127, %121, %116 : i1
    %183 = comb.mux %182, %58, %112 : i32
    %184 = comb.or %175, %171, %161, %156, %149, %137, %132, %127, %121, %116, %5, %101 : i1
    %185 = comb.or %175, %171 : i1
    %186 = comb.and %176, %172, %167 : i1
    %187 = comb.and %176, %172, %169 : i1
    %188 = comb.xor %nTIMER_CLR, %true : i1
    %189 = comb.mux bin %178, %177, %i_tmc : i1
    %i_tmc = seq.firreg %189 clock %0 reset async %188, %false : i1
    %190 = comb.mux bin %180, %179, %i_tof : i1
    %i_tof = seq.firreg %190 clock %0 reset async %188, %false : i1
    %191 = comb.xor %181, %itout : i1
    %itout = seq.firreg %191 clock %0 reset async %188, %false : i1
    %192 = comb.mux bin %184, %183, %curr_tcnt : i32
    %curr_tcnt = seq.firreg %192 clock %0 reset async %188, %c0_i32 : i32
    %193 = comb.xor %186, %true : i1
    %194 = comb.or %193, %185 : i1
    %195 = comb.mux bin %194, %curr_fdat, %166 : i32
    %curr_fdat = seq.firreg %195 clock %0 reset async %188, %c0_i32 : i32
    %196 = comb.xor %187, %true : i1
    %197 = comb.or %196, %185 : i1
    %198 = comb.mux bin %197, %curr_fpwm, %168 : i32
    %curr_fpwm = seq.firreg %198 clock %0 reset async %188, %c0_i32 : i32
    %199 = comb.or %38, %i_tof : i1
    %200 = comb.and %29, %199 {sv.namehint = "itdat_ld"} : i1
    %201 = comb.and %11, %18 : i1
    %202 = comb.and %15, %13 : i1
    %203 = comb.or %201, %202 : i1
    %204 = comb.and %11, %i_tof, %tcap_sync : i1
    %205 = comb.and %15, %i_tof, %16 : i1
    %206 = comb.or %204, %205 : i1
    %207 = comb.xor %200, %curr_tdat_ldv_update : i1
    %208 = comb.or %203, %206 : i1
    %209 = comb.xor %208, %curr_tdur_update : i1
    %210 = comb.xor %curr_tcnt_update, %true : i1
    %curr_tdat_ldv_update = seq.firreg %207 clock %0 reset async %1, %false : i1
    %curr_tdur_update = seq.firreg %209 clock %0 reset async %1, %false : i1
    %curr_tcnt_update = seq.firreg %210 clock %0 reset async %1, %false : i1
    %211 = comb.mux %200, %curr_tcnt, %curr_tdat_ldv : i32
    %212 = comb.mux %206, %curr_tcnt, %curr_tdur_ldv : i32
    %213 = comb.mux %203, %curr_mdur, %212 : i32
    %curr_tdat_ldv = seq.firreg %211 clock %0 reset async %1, %c-1_i32 : i32
    %curr_tdur_ldv = seq.firreg %213 clock %0 reset async %1, %c-1_i32 : i32
    %214 = comb.icmp eq %curr_cnt_oms, %c2_i3 : i3
    %215 = comb.icmp ult %curr_tcnt, %curr_fdat : i32
    %216 = comb.and %CNT_TEN_SYNC, %214, %215 : i1
    %217 = comb.xor %CNT_IVT_SYNC, %216 : i1
    %218 = comb.xor %CNT_IVT_SYNC, %itout : i1
    %219 = comb.mux %214, %217, %218 : i1
    %TOUT = seq.firreg %219 clock %0 reset async %1, %false : i1
    %220 = comb.or %i_tmc, %curr_tmc_intr {sv.namehint = "inext_tmc_intr"} : i1
    %221 = comb.or %i_tof, %curr_tof_intr {sv.namehint = "inext_tof_intr"} : i1
    %222 = comb.xor %TMC_INTR_CLR_SYNC, %true : i1
    %223 = comb.and %220, %222, %nTIMER_CLR : i1
    %224 = comb.xor %TOF_INTR_CLR_SYNC, %true : i1
    %225 = comb.and %221, %224, %nTIMER_CLR : i1
    %curr_tmc_intr = seq.firreg %223 clock %0 reset async %1, %false : i1
    %curr_tof_intr = seq.firreg %225 clock %0 reset async %1, %false : i1
    %curr_tmc_intr_clr_ack = seq.firreg %TMC_INTR_CLR_SYNC clock %0 reset async %1, %false : i1
    %curr_tof_intr_clr_ack = seq.firreg %TOF_INTR_CLR_SYNC clock %0 reset async %1, %false : i1
    %226 = comb.or %curr_tmc_intr, %curr_tof_intr : i1
    hw.output %curr_tdat_ldv_update, %curr_tcnt_update, %curr_tdur_update, %curr_tdat_ldv, %curr_tcnt, %curr_tdur_ldv, %TOUT, %curr_tmc_intr, %curr_tof_intr, %226, %curr_tmc_intr_clr_ack, %curr_tof_intr_clr_ack : i1, i1, i1, i32, i32, i32, i1, i1, i1, i1, i1, i1
  }
  hw.module private @syncto_pclk(in %PCLK : i1, in %PRESETn : i1, in %nTIMER_CLR : i1, in %TDAT_LDV_UPDATE : i1, in %TCNT_UPDATE : i1, in %TDUR_UPDATE : i1, in %INT_TMC : i1, in %INT_TOF : i1, in %TMC_INTR_CLR_ACK : i1, in %TOF_INTR_CLR_ACK : i1, out nTIMER_CLR_SYNC : i1, out TDAT_UPDATE_LDV_SYNC : i1, out TCNT_UPDATE_SYNC : i1, out TDUR_UPDATE_SYNC : i1, out INT_TMC_SYNC : i1, out INT_TOF_SYNC : i1, out TMC_INTR_CLR_ACK_SYNC : i1, out TOF_INTR_CLR_ACK_SYNC : i1) {
    %false = hw.constant false
    %true = hw.constant true
    %0 = seq.to_clock %PCLK
    %1 = comb.xor %PRESETn, %true : i1
    %ntimer_clr_f = seq.firreg %nTIMER_CLR clock %0 reset async %1, %true : i1
    %ntimer_clr_ff = seq.firreg %ntimer_clr_f clock %0 reset async %1, %true : i1
    %tdat_update_ldv_f = seq.firreg %TDAT_LDV_UPDATE clock %0 reset async %1, %false : i1
    %tdat_update_ldv_ff = seq.firreg %tdat_update_ldv_f clock %0 reset async %1, %false : i1
    %tcnt_update_f = seq.firreg %TCNT_UPDATE clock %0 reset async %1, %false : i1
    %tcnt_update_ff = seq.firreg %tcnt_update_f clock %0 reset async %1, %false : i1
    %tdur_update_f = seq.firreg %TDUR_UPDATE clock %0 reset async %1, %false : i1
    %tdur_update_ff = seq.firreg %tdur_update_f clock %0 reset async %1, %false : i1
    %int_tmc_f = seq.firreg %INT_TMC clock %0 reset async %1, %false : i1
    %int_tmc_ff = seq.firreg %int_tmc_f clock %0 reset async %1, %false : i1
    %int_tof_f = seq.firreg %INT_TOF clock %0 reset async %1, %false : i1
    %int_tof_ff = seq.firreg %int_tof_f clock %0 reset async %1, %false : i1
    %tmc_intr_clr_ack_f = seq.firreg %TMC_INTR_CLR_ACK clock %0 reset async %1, %false : i1
    %tmc_intr_clr_ack_ff = seq.firreg %tmc_intr_clr_ack_f clock %0 reset async %1, %false : i1
    %tof_intr_clr_ack_f = seq.firreg %TOF_INTR_CLR_ACK clock %0 reset async %1, %false : i1
    %tof_intr_clr_ack_ff = seq.firreg %tof_intr_clr_ack_f clock %0 reset async %1, %false : i1
    hw.output %ntimer_clr_ff, %tdat_update_ldv_ff, %tcnt_update_ff, %tdur_update_ff, %int_tmc_ff, %int_tof_ff, %tmc_intr_clr_ack_ff, %tof_intr_clr_ack_ff : i1, i1, i1, i1, i1, i1, i1, i1
  }
  hw.module private @ptimer(in %PCLK : i1, in %PRESETn : i1, in %TIMER_CLK : i1, in %nTIMER_RST : i1, in %nTIMER_CLR : i1, out TIMER_CLR_REQ : i1, in %PENABLE : i1, in %PSEL : i1, in %PADDR : i3, in %PWRITE : i1, in %PWDATA : i32, out PRDATA : i32, in %TCAP : i1, out TOUT : i1, out INT_TOF : i1, out INT_TMC : i1, out INT_TIMER : i1) {
    %U_TIMER_APBIF.PRDATA, %U_TIMER_APBIF.TDAT_UPDATE, %U_TIMER_APBIF.TOMS_UPDATE, %U_TIMER_APBIF.TPWM_UPDATE, %U_TIMER_APBIF.TLCV_UPDATE, %U_TIMER_APBIF.TOVF_UPDATE, %U_TIMER_APBIF.CNT_TDAT, %U_TIMER_APBIF.CNT_TEN, %U_TIMER_APBIF.CNT_OMS, %U_TIMER_APBIF.CNT_IVT, %U_TIMER_APBIF.CNT_CLR, %U_TIMER_APBIF.CNT_TPWM, %U_TIMER_APBIF.CNT_TLCV, %U_TIMER_APBIF.CNT_TOVF, %U_TIMER_APBIF.TMC_INTR_CLR, %U_TIMER_APBIF.TOF_INTR_CLR = hw.instance "U_TIMER_APBIF" @ptimer_apbif(PCLK: %PCLK: i1, PRESETn: %PRESETn: i1, PSEL: %PSEL: i1, PENABLE: %PENABLE: i1, PWRITE: %PWRITE: i1, PADDR: %PADDR: i3, PWDATA: %PWDATA: i32, nTIMER_CLR_SYNC: %U_SYNCTO_PCLK.nTIMER_CLR_SYNC: i1, TDAT_LDV_UPDATE_SYNC: %U_SYNCTO_PCLK.TDAT_UPDATE_LDV_SYNC: i1, TCNT_UPDATE_SYNC: %U_SYNCTO_PCLK.TCNT_UPDATE_SYNC: i1, TDUR_UPDATE_SYNC: %U_SYNCTO_PCLK.TDUR_UPDATE_SYNC: i1, TDAT_LDV: %U_TIMER_CNT.TDAT_LDV: i32, TCNT_LDV: %U_TIMER_CNT.TCNT_LDV: i32, TDUR_LDV: %U_TIMER_CNT.TDUR_LDV: i32, INT_TMC_SYNC: %U_SYNCTO_PCLK.INT_TMC_SYNC: i1, INT_TOF_SYNC: %U_SYNCTO_PCLK.INT_TOF_SYNC: i1, TMC_INTR_CLR_ACK_SYNC: %U_SYNCTO_PCLK.TMC_INTR_CLR_ACK_SYNC: i1, TOF_INTR_CLR_ACK_SYNC: %U_SYNCTO_PCLK.TOF_INTR_CLR_ACK_SYNC: i1) -> (PRDATA: i32, TDAT_UPDATE: i1, TOMS_UPDATE: i1, TPWM_UPDATE: i1, TLCV_UPDATE: i1, TOVF_UPDATE: i1, CNT_TDAT: i32, CNT_TEN: i1, CNT_OMS: i3, CNT_IVT: i1, CNT_CLR: i1, CNT_TPWM: i32, CNT_TLCV: i32, CNT_TOVF: i32, TMC_INTR_CLR: i1, TOF_INTR_CLR: i1) {sv.namehint = "cnt_clr"}
    %U_SYNCTO_TIMERCLK.TDAT_UPDATE_SYNC, %U_SYNCTO_TIMERCLK.TOMS_UPDATE_SYNC, %U_SYNCTO_TIMERCLK.TPWM_UPDATE_SYNC, %U_SYNCTO_TIMERCLK.TLCV_UPDATE_SYNC, %U_SYNCTO_TIMERCLK.TOVF_UPDATE_SYNC, %U_SYNCTO_TIMERCLK.CNT_TEN_SYNC, %U_SYNCTO_TIMERCLK.CNT_IVT_SYNC, %U_SYNCTO_TIMERCLK.CNT_CLR_SYNC, %U_SYNCTO_TIMERCLK.TMC_INTR_CLR_SYNC, %U_SYNCTO_TIMERCLK.TOF_INTR_CLR_SYNC = hw.instance "U_SYNCTO_TIMERCLK" @syncto_timerclk(TIMER_CLK: %TIMER_CLK: i1, nTIMER_RST: %nTIMER_RST: i1, TDAT_UPDATE: %U_TIMER_APBIF.TDAT_UPDATE: i1, TOMS_UPDATE: %U_TIMER_APBIF.TOMS_UPDATE: i1, TPWM_UPDATE: %U_TIMER_APBIF.TPWM_UPDATE: i1, TLCV_UPDATE: %U_TIMER_APBIF.TLCV_UPDATE: i1, TOVF_UPDATE: %U_TIMER_APBIF.TOVF_UPDATE: i1, CNT_TEN: %U_TIMER_APBIF.CNT_TEN: i1, CNT_IVT: %U_TIMER_APBIF.CNT_IVT: i1, CNT_CLR: %U_TIMER_APBIF.CNT_CLR: i1, TMC_INTR_CLR: %U_TIMER_APBIF.TMC_INTR_CLR: i1, TOF_INTR_CLR: %U_TIMER_APBIF.TOF_INTR_CLR: i1) -> (TDAT_UPDATE_SYNC: i1, TOMS_UPDATE_SYNC: i1, TPWM_UPDATE_SYNC: i1, TLCV_UPDATE_SYNC: i1, TOVF_UPDATE_SYNC: i1, CNT_TEN_SYNC: i1, CNT_IVT_SYNC: i1, CNT_CLR_SYNC: i1, TMC_INTR_CLR_SYNC: i1, TOF_INTR_CLR_SYNC: i1) {sv.namehint = "cnt_ivt_sync"}
    %U_SYNCTO_PCLK.nTIMER_CLR_SYNC, %U_SYNCTO_PCLK.TDAT_UPDATE_LDV_SYNC, %U_SYNCTO_PCLK.TCNT_UPDATE_SYNC, %U_SYNCTO_PCLK.TDUR_UPDATE_SYNC, %U_SYNCTO_PCLK.INT_TMC_SYNC, %U_SYNCTO_PCLK.INT_TOF_SYNC, %U_SYNCTO_PCLK.TMC_INTR_CLR_ACK_SYNC, %U_SYNCTO_PCLK.TOF_INTR_CLR_ACK_SYNC = hw.instance "U_SYNCTO_PCLK" @syncto_pclk(PCLK: %PCLK: i1, PRESETn: %PRESETn: i1, nTIMER_CLR: %nTIMER_CLR: i1, TDAT_LDV_UPDATE: %U_TIMER_CNT.TDAT_LDV_UPDATE: i1, TCNT_UPDATE: %U_TIMER_CNT.TCNT_UPDATE: i1, TDUR_UPDATE: %U_TIMER_CNT.TDUR_UPDATE: i1, INT_TMC: %U_TIMER_CNT.INT_TMC: i1, INT_TOF: %U_TIMER_CNT.INT_TOF: i1, TMC_INTR_CLR_ACK: %U_TIMER_CNT.TMC_INTR_CLR_ACK: i1, TOF_INTR_CLR_ACK: %U_TIMER_CNT.TOF_INTR_CLR_ACK: i1) -> (nTIMER_CLR_SYNC: i1, TDAT_UPDATE_LDV_SYNC: i1, TCNT_UPDATE_SYNC: i1, TDUR_UPDATE_SYNC: i1, INT_TMC_SYNC: i1, INT_TOF_SYNC: i1, TMC_INTR_CLR_ACK_SYNC: i1, TOF_INTR_CLR_ACK_SYNC: i1) {sv.namehint = "int_tof_sync"}
    %U_TIMER_CNT.TDAT_LDV_UPDATE, %U_TIMER_CNT.TCNT_UPDATE, %U_TIMER_CNT.TDUR_UPDATE, %U_TIMER_CNT.TDAT_LDV, %U_TIMER_CNT.TCNT_LDV, %U_TIMER_CNT.TDUR_LDV, %U_TIMER_CNT.TOUT, %U_TIMER_CNT.INT_TMC, %U_TIMER_CNT.INT_TOF, %U_TIMER_CNT.INT_TIMER, %U_TIMER_CNT.TMC_INTR_CLR_ACK, %U_TIMER_CNT.TOF_INTR_CLR_ACK = hw.instance "U_TIMER_CNT" @ptimer_cnt(TIMER_CLK: %TIMER_CLK: i1, nTIMER_RST: %nTIMER_RST: i1, nTIMER_CLR: %nTIMER_CLR: i1, TDAT_UPDATE_SYNC: %U_SYNCTO_TIMERCLK.TDAT_UPDATE_SYNC: i1, TOMS_UPDATE_SYNC: %U_SYNCTO_TIMERCLK.TOMS_UPDATE_SYNC: i1, TPWM_UPDATE_SYNC: %U_SYNCTO_TIMERCLK.TPWM_UPDATE_SYNC: i1, TLCV_UPDATE_SYNC: %U_SYNCTO_TIMERCLK.TLCV_UPDATE_SYNC: i1, TOVF_UPDATE_SYNC: %U_SYNCTO_TIMERCLK.TOVF_UPDATE_SYNC: i1, CNT_TDAT: %U_TIMER_APBIF.CNT_TDAT: i32, CNT_TEN_SYNC: %U_SYNCTO_TIMERCLK.CNT_TEN_SYNC: i1, CNT_OMS: %U_TIMER_APBIF.CNT_OMS: i3, CNT_IVT_SYNC: %U_SYNCTO_TIMERCLK.CNT_IVT_SYNC: i1, CNT_TPWM: %U_TIMER_APBIF.CNT_TPWM: i32, CNT_TLCV: %U_TIMER_APBIF.CNT_TLCV: i32, CNT_TOVF: %U_TIMER_APBIF.CNT_TOVF: i32, TCAP: %TCAP: i1, TMC_INTR_CLR_SYNC: %U_SYNCTO_TIMERCLK.TMC_INTR_CLR_SYNC: i1, TOF_INTR_CLR_SYNC: %U_SYNCTO_TIMERCLK.TOF_INTR_CLR_SYNC: i1) -> (TDAT_LDV_UPDATE: i1, TCNT_UPDATE: i1, TDUR_UPDATE: i1, TDAT_LDV: i32, TCNT_LDV: i32, TDUR_LDV: i32, TOUT: i1, INT_TMC: i1, INT_TOF: i1, INT_TIMER: i1, TMC_INTR_CLR_ACK: i1, TOF_INTR_CLR_ACK: i1) {sv.namehint = "iINT_TOF"}
    hw.output %U_SYNCTO_TIMERCLK.CNT_CLR_SYNC, %U_TIMER_APBIF.PRDATA, %U_TIMER_CNT.TOUT, %U_TIMER_CNT.INT_TOF, %U_TIMER_CNT.INT_TMC, %U_TIMER_CNT.INT_TIMER : i1, i32, i1, i1, i1, i1
  }
  hw.module private @syncto_timerclk(in %TIMER_CLK : i1, in %nTIMER_RST : i1, in %TDAT_UPDATE : i1, in %TOMS_UPDATE : i1, in %TPWM_UPDATE : i1, in %TLCV_UPDATE : i1, in %TOVF_UPDATE : i1, in %CNT_TEN : i1, in %CNT_IVT : i1, in %CNT_CLR : i1, in %TMC_INTR_CLR : i1, in %TOF_INTR_CLR : i1, out TDAT_UPDATE_SYNC : i1, out TOMS_UPDATE_SYNC : i1, out TPWM_UPDATE_SYNC : i1, out TLCV_UPDATE_SYNC : i1, out TOVF_UPDATE_SYNC : i1, out CNT_TEN_SYNC : i1, out CNT_IVT_SYNC : i1, out CNT_CLR_SYNC : i1, out TMC_INTR_CLR_SYNC : i1, out TOF_INTR_CLR_SYNC : i1) {
    %true = hw.constant true
    %false = hw.constant false
    %0 = seq.to_clock %TIMER_CLK
    %1 = comb.xor %nTIMER_RST, %true : i1
    %tdat_update_f = seq.firreg %TDAT_UPDATE clock %0 reset async %1, %false : i1
    %tdat_update_ff = seq.firreg %tdat_update_f clock %0 reset async %1, %false : i1
    %toms_update_f = seq.firreg %TOMS_UPDATE clock %0 reset async %1, %false : i1
    %toms_update_ff = seq.firreg %toms_update_f clock %0 reset async %1, %false : i1
    %tpwm_update_f = seq.firreg %TPWM_UPDATE clock %0 reset async %1, %false : i1
    %tpwm_update_ff = seq.firreg %tpwm_update_f clock %0 reset async %1, %false : i1
    %tlcv_update_f = seq.firreg %TLCV_UPDATE clock %0 reset async %1, %false : i1
    %tlcv_update_ff = seq.firreg %tlcv_update_f clock %0 reset async %1, %false : i1
    %tovf_update_f = seq.firreg %TOVF_UPDATE clock %0 reset async %1, %false : i1
    %tovf_update_ff = seq.firreg %tovf_update_f clock %0 reset async %1, %false : i1
    %cnt_ten_f = seq.firreg %CNT_TEN clock %0 reset async %1, %false : i1
    %cnt_ten_ff = seq.firreg %cnt_ten_f clock %0 reset async %1, %false : i1
    %cnt_ivt_f = seq.firreg %CNT_IVT clock %0 reset async %1, %false : i1
    %cnt_ivt_ff = seq.firreg %cnt_ivt_f clock %0 reset async %1, %false : i1
    %cnt_clr_f = seq.firreg %CNT_CLR clock %0 reset async %1, %false : i1
    %cnt_clr_ff = seq.firreg %cnt_clr_f clock %0 reset async %1, %false : i1
    %tmc_intr_clr_f = seq.firreg %TMC_INTR_CLR clock %0 reset async %1, %false : i1
    %tmc_intr_clr_ff = seq.firreg %tmc_intr_clr_f clock %0 reset async %1, %false : i1
    %tof_intr_clr_f = seq.firreg %TOF_INTR_CLR clock %0 reset async %1, %false : i1
    %tof_intr_clr_ff = seq.firreg %tof_intr_clr_f clock %0 reset async %1, %false : i1
    hw.output %tdat_update_ff, %toms_update_ff, %tpwm_update_ff, %tlcv_update_ff, %tovf_update_ff, %cnt_ten_ff, %cnt_ivt_ff, %cnt_clr_ff, %tmc_intr_clr_ff, %tof_intr_clr_ff : i1, i1, i1, i1, i1, i1, i1, i1, i1, i1
  }
  hw.module @ptimer_top(in %TIMER_PCLK : i1, in %TIMER_PRESETn : i1, in %TIMER0_CLK : i1, in %TIMER1_CLK : i1, in %TIMER2_CLK : i1, in %TIMER3_CLK : i1, in %TIMER4_CLK : i1, in %TIMER5_CLK : i1, in %TIMER6_CLK : i1, in %TIMER7_CLK : i1, in %TIMER8_CLK : i1, in %TIMER9_CLK : i1, in %TIMER_RESETn : i1, in %PSEL : i1, in %PENABLE : i1, in %PWRITE : i1, in %PADDR : i22, in %PWDATA : i32, out PRDATA : i32, out TIMER_INTR : i10, out TIMER0_CLR_REQ : i1, out TIMER1_CLR_REQ : i1, out TIMER2_CLR_REQ : i1, out TIMER3_CLR_REQ : i1, out TIMER4_CLR_REQ : i1, out TIMER5_CLR_REQ : i1, out TIMER6_CLR_REQ : i1, out TIMER7_CLR_REQ : i1, out TIMER8_CLR_REQ : i1, out TIMER9_CLR_REQ : i1, in %TIMER0_nCLR : i1, in %TIMER1_nCLR : i1, in %TIMER2_nCLR : i1, in %TIMER3_nCLR : i1, in %TIMER4_nCLR : i1, in %TIMER5_nCLR : i1, in %TIMER6_nCLR : i1, in %TIMER7_nCLR : i1, in %TIMER8_nCLR : i1, in %TIMER9_nCLR : i1, in %TCAP : i4, out TOUT : i4) {
    %true = hw.constant true
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %c0_i32 = hw.constant 0 : i32
    %c-512_i10 = hw.constant -512 : i10
    %c256_i10 = hw.constant 256 : i10
    %c128_i10 = hw.constant 128 : i10
    %c64_i10 = hw.constant 64 : i10
    %c32_i10 = hw.constant 32 : i10
    %c16_i10 = hw.constant 16 : i10
    %c8_i10 = hw.constant 8 : i10
    %c4_i10 = hw.constant 4 : i10
    %c2_i10 = hw.constant 2 : i10
    %c1_i10 = hw.constant 1 : i10
    %c-7_i4 = hw.constant -7 : i4
    %c-8_i4 = hw.constant -8 : i4
    %c7_i4 = hw.constant 7 : i4
    %c6_i4 = hw.constant 6 : i4
    %c5_i4 = hw.constant 5 : i4
    %c4_i4 = hw.constant 4 : i4
    %c3_i4 = hw.constant 3 : i4
    %c2_i4 = hw.constant 2 : i4
    %c1_i4 = hw.constant 1 : i4
    %false = hw.constant false
    %c0_i4 = hw.constant 0 : i4
    %1 = comb.concat %U_PTIMER9.INT_TIMER, %U_PTIMER8.INT_TIMER, %U_PTIMER7.INT_TIMER, %U_PTIMER6.INT_TIMER, %U_PTIMER5.INT_TIMER, %U_PTIMER4.INT_TIMER, %U_PTIMER3.INT_TIMER, %U_PTIMER2.INT_TIMER, %U_PTIMER1.INT_TIMER, %U_PTIMER0.INT_TIMER : i1, i1, i1, i1, i1, i1, i1, i1, i1, i1
    %2 = comb.concat %U_PTIMER3.TOUT, %U_PTIMER2.TOUT, %U_PTIMER1.TOUT, %U_PTIMER0.TOUT : i1, i1, i1, i1
    %PRDATA = llhd.sig %c0_i32 : i32
    %3 = comb.extract %PADDR from 18 : (i22) -> i4
    %4 = comb.icmp eq %3, %c0_i4 : i4
    %5 = comb.and %4, %PSEL {sv.namehint = "psel_timer0"} : i1
    %6 = comb.icmp eq %3, %c1_i4 : i4
    %7 = comb.and %6, %PSEL {sv.namehint = "psel_timer1"} : i1
    %8 = comb.icmp eq %3, %c2_i4 : i4
    %9 = comb.and %8, %PSEL {sv.namehint = "psel_timer2"} : i1
    %10 = comb.icmp eq %3, %c3_i4 : i4
    %11 = comb.and %10, %PSEL {sv.namehint = "psel_timer3"} : i1
    %12 = comb.icmp eq %3, %c4_i4 : i4
    %13 = comb.and %12, %PSEL {sv.namehint = "psel_timer4"} : i1
    %14 = comb.icmp eq %3, %c5_i4 : i4
    %15 = comb.and %14, %PSEL {sv.namehint = "psel_timer5"} : i1
    %16 = comb.icmp eq %3, %c6_i4 : i4
    %17 = comb.and %16, %PSEL {sv.namehint = "psel_timer6"} : i1
    %18 = comb.icmp eq %3, %c7_i4 : i4
    %19 = comb.and %18, %PSEL {sv.namehint = "psel_timer7"} : i1
    %20 = comb.icmp eq %3, %c-8_i4 : i4
    %21 = comb.and %20, %PSEL {sv.namehint = "psel_timer8"} : i1
    %22 = comb.icmp eq %3, %c-7_i4 : i4
    %23 = comb.and %22, %PSEL {sv.namehint = "psel_timer9"} : i1
    %24 = comb.concat %23, %21, %19, %17, %15, %13, %11, %9, %7, %5 : i1, i1, i1, i1, i1, i1, i1, i1, i1, i1
    %25:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%32: i32, %33: i1):  // 12 preds: ^bb0, ^bb2, ^bb3, ^bb4, ^bb5, ^bb6, ^bb7, ^bb8, ^bb9, ^bb10, ^bb11, ^bb11
      llhd.wait yield (%32, %33 : i32, i1), (%24, %U_PTIMER0.PRDATA, %U_PTIMER1.PRDATA, %U_PTIMER2.PRDATA, %U_PTIMER3.PRDATA, %U_PTIMER4.PRDATA, %U_PTIMER5.PRDATA, %U_PTIMER6.PRDATA, %U_PTIMER7.PRDATA, %U_PTIMER8.PRDATA, %U_PTIMER9.PRDATA : i10, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32), ^bb2
    ^bb2:  // pred: ^bb1
      %34 = comb.icmp ceq %24, %c1_i10 : i10
      cf.cond_br %34, ^bb1(%U_PTIMER0.PRDATA, %true : i32, i1), ^bb3
    ^bb3:  // pred: ^bb2
      %35 = comb.icmp ceq %24, %c2_i10 : i10
      cf.cond_br %35, ^bb1(%U_PTIMER1.PRDATA, %true : i32, i1), ^bb4
    ^bb4:  // pred: ^bb3
      %36 = comb.icmp ceq %24, %c4_i10 : i10
      cf.cond_br %36, ^bb1(%U_PTIMER2.PRDATA, %true : i32, i1), ^bb5
    ^bb5:  // pred: ^bb4
      %37 = comb.icmp ceq %24, %c8_i10 : i10
      cf.cond_br %37, ^bb1(%U_PTIMER3.PRDATA, %true : i32, i1), ^bb6
    ^bb6:  // pred: ^bb5
      %38 = comb.icmp ceq %24, %c16_i10 : i10
      cf.cond_br %38, ^bb1(%U_PTIMER4.PRDATA, %true : i32, i1), ^bb7
    ^bb7:  // pred: ^bb6
      %39 = comb.icmp ceq %24, %c32_i10 : i10
      cf.cond_br %39, ^bb1(%U_PTIMER5.PRDATA, %true : i32, i1), ^bb8
    ^bb8:  // pred: ^bb7
      %40 = comb.icmp ceq %24, %c64_i10 : i10
      cf.cond_br %40, ^bb1(%U_PTIMER6.PRDATA, %true : i32, i1), ^bb9
    ^bb9:  // pred: ^bb8
      %41 = comb.icmp ceq %24, %c128_i10 : i10
      cf.cond_br %41, ^bb1(%U_PTIMER7.PRDATA, %true : i32, i1), ^bb10
    ^bb10:  // pred: ^bb9
      %42 = comb.icmp ceq %24, %c256_i10 : i10
      cf.cond_br %42, ^bb1(%U_PTIMER8.PRDATA, %true : i32, i1), ^bb11
    ^bb11:  // pred: ^bb10
      %43 = comb.icmp ceq %24, %c-512_i10 : i10
      cf.cond_br %43, ^bb1(%U_PTIMER9.PRDATA, %true : i32, i1), ^bb1(%c0_i32, %true : i32, i1)
    }
    llhd.drv %PRDATA, %25#0 after %0 if %25#1 : i32
    %26 = comb.extract %PADDR from 0 : (i22) -> i3
    %27 = comb.extract %TCAP from 0 : (i4) -> i1
    %U_PTIMER0.TIMER_CLR_REQ, %U_PTIMER0.PRDATA, %U_PTIMER0.TOUT, %U_PTIMER0.INT_TOF, %U_PTIMER0.INT_TMC, %U_PTIMER0.INT_TIMER = hw.instance "U_PTIMER0" @ptimer(PCLK: %TIMER_PCLK: i1, PRESETn: %TIMER_PRESETn: i1, TIMER_CLK: %TIMER0_CLK: i1, nTIMER_RST: %TIMER_RESETn: i1, nTIMER_CLR: %TIMER0_nCLR: i1, PENABLE: %PENABLE: i1, PSEL: %5: i1, PADDR: %26: i3, PWRITE: %PWRITE: i1, PWDATA: %PWDATA: i32, TCAP: %27: i1) -> (TIMER_CLR_REQ: i1, PRDATA: i32, TOUT: i1, INT_TOF: i1, INT_TMC: i1, INT_TIMER: i1)
    %28 = comb.extract %TCAP from 1 : (i4) -> i1
    %U_PTIMER1.TIMER_CLR_REQ, %U_PTIMER1.PRDATA, %U_PTIMER1.TOUT, %U_PTIMER1.INT_TOF, %U_PTIMER1.INT_TMC, %U_PTIMER1.INT_TIMER = hw.instance "U_PTIMER1" @ptimer(PCLK: %TIMER_PCLK: i1, PRESETn: %TIMER_PRESETn: i1, TIMER_CLK: %TIMER1_CLK: i1, nTIMER_RST: %TIMER_RESETn: i1, nTIMER_CLR: %TIMER1_nCLR: i1, PENABLE: %PENABLE: i1, PSEL: %7: i1, PADDR: %26: i3, PWRITE: %PWRITE: i1, PWDATA: %PWDATA: i32, TCAP: %28: i1) -> (TIMER_CLR_REQ: i1, PRDATA: i32, TOUT: i1, INT_TOF: i1, INT_TMC: i1, INT_TIMER: i1)
    %29 = comb.extract %TCAP from 2 : (i4) -> i1
    %U_PTIMER2.TIMER_CLR_REQ, %U_PTIMER2.PRDATA, %U_PTIMER2.TOUT, %U_PTIMER2.INT_TOF, %U_PTIMER2.INT_TMC, %U_PTIMER2.INT_TIMER = hw.instance "U_PTIMER2" @ptimer(PCLK: %TIMER_PCLK: i1, PRESETn: %TIMER_PRESETn: i1, TIMER_CLK: %TIMER2_CLK: i1, nTIMER_RST: %TIMER_RESETn: i1, nTIMER_CLR: %TIMER2_nCLR: i1, PENABLE: %PENABLE: i1, PSEL: %9: i1, PADDR: %26: i3, PWRITE: %PWRITE: i1, PWDATA: %PWDATA: i32, TCAP: %29: i1) -> (TIMER_CLR_REQ: i1, PRDATA: i32, TOUT: i1, INT_TOF: i1, INT_TMC: i1, INT_TIMER: i1)
    %30 = comb.extract %TCAP from 3 : (i4) -> i1
    %U_PTIMER3.TIMER_CLR_REQ, %U_PTIMER3.PRDATA, %U_PTIMER3.TOUT, %U_PTIMER3.INT_TOF, %U_PTIMER3.INT_TMC, %U_PTIMER3.INT_TIMER = hw.instance "U_PTIMER3" @ptimer(PCLK: %TIMER_PCLK: i1, PRESETn: %TIMER_PRESETn: i1, TIMER_CLK: %TIMER3_CLK: i1, nTIMER_RST: %TIMER_RESETn: i1, nTIMER_CLR: %TIMER3_nCLR: i1, PENABLE: %PENABLE: i1, PSEL: %11: i1, PADDR: %26: i3, PWRITE: %PWRITE: i1, PWDATA: %PWDATA: i32, TCAP: %30: i1) -> (TIMER_CLR_REQ: i1, PRDATA: i32, TOUT: i1, INT_TOF: i1, INT_TMC: i1, INT_TIMER: i1)
    %U_PTIMER4.TIMER_CLR_REQ, %U_PTIMER4.PRDATA, %U_PTIMER4.TOUT, %U_PTIMER4.INT_TOF, %U_PTIMER4.INT_TMC, %U_PTIMER4.INT_TIMER = hw.instance "U_PTIMER4" @ptimer(PCLK: %TIMER_PCLK: i1, PRESETn: %TIMER_PRESETn: i1, TIMER_CLK: %TIMER4_CLK: i1, nTIMER_RST: %TIMER_RESETn: i1, nTIMER_CLR: %TIMER4_nCLR: i1, PENABLE: %PENABLE: i1, PSEL: %13: i1, PADDR: %26: i3, PWRITE: %PWRITE: i1, PWDATA: %PWDATA: i32, TCAP: %false: i1) -> (TIMER_CLR_REQ: i1, PRDATA: i32, TOUT: i1, INT_TOF: i1, INT_TMC: i1, INT_TIMER: i1)
    %U_PTIMER5.TIMER_CLR_REQ, %U_PTIMER5.PRDATA, %U_PTIMER5.TOUT, %U_PTIMER5.INT_TOF, %U_PTIMER5.INT_TMC, %U_PTIMER5.INT_TIMER = hw.instance "U_PTIMER5" @ptimer(PCLK: %TIMER_PCLK: i1, PRESETn: %TIMER_PRESETn: i1, TIMER_CLK: %TIMER5_CLK: i1, nTIMER_RST: %TIMER_RESETn: i1, nTIMER_CLR: %TIMER5_nCLR: i1, PENABLE: %PENABLE: i1, PSEL: %15: i1, PADDR: %26: i3, PWRITE: %PWRITE: i1, PWDATA: %PWDATA: i32, TCAP: %false: i1) -> (TIMER_CLR_REQ: i1, PRDATA: i32, TOUT: i1, INT_TOF: i1, INT_TMC: i1, INT_TIMER: i1)
    %U_PTIMER6.TIMER_CLR_REQ, %U_PTIMER6.PRDATA, %U_PTIMER6.TOUT, %U_PTIMER6.INT_TOF, %U_PTIMER6.INT_TMC, %U_PTIMER6.INT_TIMER = hw.instance "U_PTIMER6" @ptimer(PCLK: %TIMER_PCLK: i1, PRESETn: %TIMER_PRESETn: i1, TIMER_CLK: %TIMER6_CLK: i1, nTIMER_RST: %TIMER_RESETn: i1, nTIMER_CLR: %TIMER6_nCLR: i1, PENABLE: %PENABLE: i1, PSEL: %17: i1, PADDR: %26: i3, PWRITE: %PWRITE: i1, PWDATA: %PWDATA: i32, TCAP: %false: i1) -> (TIMER_CLR_REQ: i1, PRDATA: i32, TOUT: i1, INT_TOF: i1, INT_TMC: i1, INT_TIMER: i1)
    %U_PTIMER7.TIMER_CLR_REQ, %U_PTIMER7.PRDATA, %U_PTIMER7.TOUT, %U_PTIMER7.INT_TOF, %U_PTIMER7.INT_TMC, %U_PTIMER7.INT_TIMER = hw.instance "U_PTIMER7" @ptimer(PCLK: %TIMER_PCLK: i1, PRESETn: %TIMER_PRESETn: i1, TIMER_CLK: %TIMER7_CLK: i1, nTIMER_RST: %TIMER_RESETn: i1, nTIMER_CLR: %TIMER7_nCLR: i1, PENABLE: %PENABLE: i1, PSEL: %19: i1, PADDR: %26: i3, PWRITE: %PWRITE: i1, PWDATA: %PWDATA: i32, TCAP: %false: i1) -> (TIMER_CLR_REQ: i1, PRDATA: i32, TOUT: i1, INT_TOF: i1, INT_TMC: i1, INT_TIMER: i1)
    %U_PTIMER8.TIMER_CLR_REQ, %U_PTIMER8.PRDATA, %U_PTIMER8.TOUT, %U_PTIMER8.INT_TOF, %U_PTIMER8.INT_TMC, %U_PTIMER8.INT_TIMER = hw.instance "U_PTIMER8" @ptimer(PCLK: %TIMER_PCLK: i1, PRESETn: %TIMER_PRESETn: i1, TIMER_CLK: %TIMER8_CLK: i1, nTIMER_RST: %TIMER_RESETn: i1, nTIMER_CLR: %TIMER8_nCLR: i1, PENABLE: %PENABLE: i1, PSEL: %21: i1, PADDR: %26: i3, PWRITE: %PWRITE: i1, PWDATA: %PWDATA: i32, TCAP: %false: i1) -> (TIMER_CLR_REQ: i1, PRDATA: i32, TOUT: i1, INT_TOF: i1, INT_TMC: i1, INT_TIMER: i1)
    %U_PTIMER9.TIMER_CLR_REQ, %U_PTIMER9.PRDATA, %U_PTIMER9.TOUT, %U_PTIMER9.INT_TOF, %U_PTIMER9.INT_TMC, %U_PTIMER9.INT_TIMER = hw.instance "U_PTIMER9" @ptimer(PCLK: %TIMER_PCLK: i1, PRESETn: %TIMER_PRESETn: i1, TIMER_CLK: %TIMER9_CLK: i1, nTIMER_RST: %TIMER_RESETn: i1, nTIMER_CLR: %TIMER9_nCLR: i1, PENABLE: %PENABLE: i1, PSEL: %23: i1, PADDR: %26: i3, PWRITE: %PWRITE: i1, PWDATA: %PWDATA: i32, TCAP: %false: i1) -> (TIMER_CLR_REQ: i1, PRDATA: i32, TOUT: i1, INT_TOF: i1, INT_TMC: i1, INT_TIMER: i1)
    %31 = llhd.prb %PRDATA : i32
    hw.output %31, %1, %U_PTIMER0.TIMER_CLR_REQ, %U_PTIMER1.TIMER_CLR_REQ, %U_PTIMER2.TIMER_CLR_REQ, %U_PTIMER3.TIMER_CLR_REQ, %U_PTIMER4.TIMER_CLR_REQ, %U_PTIMER5.TIMER_CLR_REQ, %U_PTIMER6.TIMER_CLR_REQ, %U_PTIMER7.TIMER_CLR_REQ, %U_PTIMER8.TIMER_CLR_REQ, %U_PTIMER9.TIMER_CLR_REQ, %2 : i32, i10, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i4
  }
}
