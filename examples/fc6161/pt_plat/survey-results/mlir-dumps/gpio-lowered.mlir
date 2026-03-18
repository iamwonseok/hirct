module {
  hw.module private @DW_apb_gpio_bcm21(in %clk_d : i1, in %rst_d_n : i1, in %data_s : i32, out data_d : i32) {
    %true = hw.constant true
    %0 = sim.fmt.literal "Information: *** Instance <module> module is using the <Double Register Synchronizer (1)> Clock Domain Crossing Method ***\0A"
    %c0_i32 = hw.constant 0 : i32
    llhd.process {
      sim.proc.print %0
      llhd.halt
    }
    %1 = seq.to_clock %clk_d
    %2 = comb.xor %rst_d_n, %true : i1
    %GEN_FST2.sample_meta = seq.firreg %data_s clock %1 reset async %2, %c0_i32 : i32
    %GEN_FST2.sample_syncl = seq.firreg %GEN_FST2.sample_meta clock %1 reset async %2, %c0_i32 : i32
    hw.output %GEN_FST2.sample_syncl : i32
  }
  hw.module private @DW_apb_gpio_bcm21_0(in %clk_d : i1, in %rst_d_n : i1, in %data_s : i32, out data_d : i32) {
    %true = hw.constant true
    %0 = sim.fmt.literal "Information: *** Instance <module> module is using the <Double Register Synchronizer (1)> Clock Domain Crossing Method ***\0A"
    %c0_i32 = hw.constant 0 : i32
    llhd.process {
      sim.proc.print %0
      llhd.halt
    }
    %1 = seq.to_clock %clk_d
    %2 = comb.xor %rst_d_n, %true : i1
    %GEN_FST2.sample_meta = seq.firreg %data_s clock %1 reset async %2, %c0_i32 : i32
    %GEN_FST2.sample_syncl = seq.firreg %GEN_FST2.sample_meta clock %1 reset async %2, %c0_i32 : i32
    hw.output %GEN_FST2.sample_syncl : i32
  }
  hw.module private @DW_apb_gpio_debounce(in %dbclk : i1, in %dbclk_res : i1, in %scan_mode : i1, in %gpio_ext_porta : i32, in %gpio_int_polarity : i32, out debounce_both_edge_out : i32, out gpio_ext_porta_int : i32, out debounce_d2_out : i32) {
    %0 = llhd.constant_time <0ns, 1d, 0e>
    %c0_i31 = hw.constant 0 : i31
    %c-1_i32 = hw.constant -1 : i32
    %true = hw.constant true
    %c-1_i5 = hw.constant -1 : i5
    %c0_i27 = hw.constant 0 : i27
    %1 = llhd.constant_time <0ns, 0d, 1e>
    %c1_i32 = hw.constant 1 : i32
    %false = hw.constant false
    %c32_i32 = hw.constant 32 : i32
    %c0_i32 = hw.constant 0 : i32
    %gpio_ext_porta_int = llhd.sig %c0_i32 : i32
    %debounce_d1 = llhd.sig %c0_i32 : i32
    %2 = llhd.prb %debounce_d1 : i32
    %debounce_both_edge = llhd.sig %c0_i32 : i32
    %3:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%263: i32, %264: i1):  // 2 preds: ^bb0, ^bb3
      llhd.wait yield (%263, %264 : i32, i1), (%gpio_ext_porta, %gpio_int_polarity : i32, i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb3(%c0_i32, %c0_i32 : i32, i32)
    ^bb3(%265: i32, %266: i32):  // 2 preds: ^bb2, ^bb7
      %267 = comb.icmp slt %265, %c32_i32 : i32
      cf.cond_br %267, ^bb4, ^bb1(%266, %true : i32, i1)
    ^bb4:  // pred: ^bb3
      %268 = comb.shru %gpio_int_polarity, %265 : i32
      %269 = comb.extract %268 from 0 : (i32) -> i1
      %270 = comb.xor %269, %true : i1
      cf.cond_br %270, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      %271 = comb.extract %265 from 5 : (i32) -> i27
      %272 = comb.icmp eq %271, %c0_i27 : i27
      %273 = comb.extract %265 from 0 : (i32) -> i5
      %274 = comb.mux %272, %273, %c-1_i5 : i5
      %275 = comb.shru %gpio_ext_porta, %265 : i32
      %276 = comb.extract %275 from 0 : (i32) -> i1
      %277 = comb.xor %276, %true : i1
      %278 = comb.concat %c0_i27, %274 : i27, i5
      %279 = comb.shl %c1_i32, %278 : i32
      %280 = comb.xor bin %279, %c-1_i32 : i32
      %281 = comb.and %266, %280 : i32
      %282 = comb.concat %c0_i31, %277 : i31, i1
      %283 = comb.shl %282, %278 : i32
      %284 = comb.or %281, %283 : i32
      cf.br ^bb7(%284 : i32)
    ^bb6:  // pred: ^bb4
      %285 = comb.extract %265 from 5 : (i32) -> i27
      %286 = comb.icmp eq %285, %c0_i27 : i27
      %287 = comb.extract %265 from 0 : (i32) -> i5
      %288 = comb.mux %286, %287, %c-1_i5 : i5
      %289 = comb.shru %gpio_ext_porta, %265 : i32
      %290 = comb.extract %289 from 0 : (i32) -> i1
      %291 = comb.concat %c0_i27, %288 : i27, i5
      %292 = comb.shl %c1_i32, %291 : i32
      %293 = comb.xor bin %292, %c-1_i32 : i32
      %294 = comb.and %266, %293 : i32
      %295 = comb.concat %c0_i31, %290 : i31, i1
      %296 = comb.shl %295, %291 : i32
      %297 = comb.or %294, %296 : i32
      cf.br ^bb7(%297 : i32)
    ^bb7(%298: i32):  // 2 preds: ^bb5, ^bb6
      %299 = comb.add %265, %c1_i32 : i32
      cf.br ^bb3(%299, %298 : i32, i32)
    }
    llhd.drv %gpio_ext_porta_int, %3#0 after %1 if %3#1 : i32
    %4 = llhd.prb %gpio_ext_porta_int : i32
    %5 = comb.extract %4 from 0 : (i32) -> i1
    %6 = comb.or %5, %scan_mode : i1
    %7 = comb.and %6, %dbclk_res : i1
    %8 = comb.extract %4 from 1 : (i32) -> i1
    %9 = comb.or %8, %scan_mode : i1
    %10 = comb.and %9, %dbclk_res : i1
    %11 = comb.extract %4 from 2 : (i32) -> i1
    %12 = comb.or %11, %scan_mode : i1
    %13 = comb.and %12, %dbclk_res : i1
    %14 = comb.extract %4 from 3 : (i32) -> i1
    %15 = comb.or %14, %scan_mode : i1
    %16 = comb.and %15, %dbclk_res : i1
    %17 = comb.extract %4 from 4 : (i32) -> i1
    %18 = comb.or %17, %scan_mode : i1
    %19 = comb.and %18, %dbclk_res : i1
    %20 = comb.extract %4 from 5 : (i32) -> i1
    %21 = comb.or %20, %scan_mode : i1
    %22 = comb.and %21, %dbclk_res : i1
    %23 = comb.extract %4 from 6 : (i32) -> i1
    %24 = comb.or %23, %scan_mode : i1
    %25 = comb.and %24, %dbclk_res : i1
    %26 = comb.extract %4 from 7 : (i32) -> i1
    %27 = comb.or %26, %scan_mode : i1
    %28 = comb.and %27, %dbclk_res : i1
    %29 = comb.extract %4 from 8 : (i32) -> i1
    %30 = comb.or %29, %scan_mode : i1
    %31 = comb.and %30, %dbclk_res : i1
    %32 = comb.extract %4 from 9 : (i32) -> i1
    %33 = comb.or %32, %scan_mode : i1
    %34 = comb.and %33, %dbclk_res : i1
    %35 = comb.extract %4 from 10 : (i32) -> i1
    %36 = comb.or %35, %scan_mode : i1
    %37 = comb.and %36, %dbclk_res : i1
    %38 = comb.extract %4 from 11 : (i32) -> i1
    %39 = comb.or %38, %scan_mode : i1
    %40 = comb.and %39, %dbclk_res : i1
    %41 = comb.extract %4 from 12 : (i32) -> i1
    %42 = comb.or %41, %scan_mode : i1
    %43 = comb.and %42, %dbclk_res : i1
    %44 = comb.extract %4 from 13 : (i32) -> i1
    %45 = comb.or %44, %scan_mode : i1
    %46 = comb.and %45, %dbclk_res : i1
    %47 = comb.extract %4 from 14 : (i32) -> i1
    %48 = comb.or %47, %scan_mode : i1
    %49 = comb.and %48, %dbclk_res : i1
    %50 = comb.extract %4 from 15 : (i32) -> i1
    %51 = comb.or %50, %scan_mode : i1
    %52 = comb.and %51, %dbclk_res : i1
    %53 = comb.extract %4 from 16 : (i32) -> i1
    %54 = comb.or %53, %scan_mode : i1
    %55 = comb.and %54, %dbclk_res : i1
    %56 = comb.extract %4 from 17 : (i32) -> i1
    %57 = comb.or %56, %scan_mode : i1
    %58 = comb.and %57, %dbclk_res : i1
    %59 = comb.extract %4 from 18 : (i32) -> i1
    %60 = comb.or %59, %scan_mode : i1
    %61 = comb.and %60, %dbclk_res : i1
    %62 = comb.extract %4 from 19 : (i32) -> i1
    %63 = comb.or %62, %scan_mode : i1
    %64 = comb.and %63, %dbclk_res : i1
    %65 = comb.extract %4 from 20 : (i32) -> i1
    %66 = comb.or %65, %scan_mode : i1
    %67 = comb.and %66, %dbclk_res : i1
    %68 = comb.extract %4 from 21 : (i32) -> i1
    %69 = comb.or %68, %scan_mode : i1
    %70 = comb.and %69, %dbclk_res : i1
    %71 = comb.extract %4 from 22 : (i32) -> i1
    %72 = comb.or %71, %scan_mode : i1
    %73 = comb.and %72, %dbclk_res : i1
    %74 = comb.extract %4 from 23 : (i32) -> i1
    %75 = comb.or %74, %scan_mode : i1
    %76 = comb.and %75, %dbclk_res : i1
    %77 = comb.extract %4 from 24 : (i32) -> i1
    %78 = comb.or %77, %scan_mode : i1
    %79 = comb.and %78, %dbclk_res : i1
    %80 = comb.extract %4 from 25 : (i32) -> i1
    %81 = comb.or %80, %scan_mode : i1
    %82 = comb.and %81, %dbclk_res : i1
    %83 = comb.extract %4 from 26 : (i32) -> i1
    %84 = comb.or %83, %scan_mode : i1
    %85 = comb.and %84, %dbclk_res : i1
    %86 = comb.extract %4 from 27 : (i32) -> i1
    %87 = comb.or %86, %scan_mode : i1
    %88 = comb.and %87, %dbclk_res : i1
    %89 = comb.extract %4 from 28 : (i32) -> i1
    %90 = comb.or %89, %scan_mode : i1
    %91 = comb.and %90, %dbclk_res : i1
    %92 = comb.extract %4 from 29 : (i32) -> i1
    %93 = comb.or %92, %scan_mode : i1
    %94 = comb.and %93, %dbclk_res : i1
    %95 = comb.extract %4 from 30 : (i32) -> i1
    %96 = comb.or %95, %scan_mode : i1
    %97 = comb.and %96, %dbclk_res : i1
    %98 = comb.extract %4 from 31 : (i32) -> i1
    %99 = comb.or %98, %scan_mode : i1
    %100 = comb.and %99, %dbclk_res : i1
    %101 = comb.extract %2 from 1 : (i32) -> i31
    %102 = comb.concat %101, %false : i31, i1
    %103 = comb.concat %101, %5 : i31, i1
    %104 = seq.to_clock %dbclk
    %105 = comb.xor %7, %true : i1
    %debounce_d1_0 = seq.firreg %103 clock %104 reset async %105, %102 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_0 after %1 : i32
    %106 = comb.extract %2 from 2 : (i32) -> i30
    %107 = comb.extract %2 from 0 : (i32) -> i1
    %108 = comb.concat %106, %false, %107 : i30, i1, i1
    %109 = comb.concat %106, %8, %107 : i30, i1, i1
    %110 = comb.xor %10, %true : i1
    %debounce_d1_1 = seq.firreg %109 clock %104 reset async %110, %108 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_1 after %1 : i32
    %111 = comb.extract %2 from 3 : (i32) -> i29
    %112 = comb.extract %2 from 0 : (i32) -> i2
    %113 = comb.concat %111, %false, %112 : i29, i1, i2
    %114 = comb.concat %111, %11, %112 : i29, i1, i2
    %115 = comb.xor %13, %true : i1
    %debounce_d1_2 = seq.firreg %114 clock %104 reset async %115, %113 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_2 after %1 : i32
    %116 = comb.extract %2 from 4 : (i32) -> i28
    %117 = comb.extract %2 from 0 : (i32) -> i3
    %118 = comb.concat %116, %false, %117 : i28, i1, i3
    %119 = comb.concat %116, %14, %117 : i28, i1, i3
    %120 = comb.xor %16, %true : i1
    %debounce_d1_3 = seq.firreg %119 clock %104 reset async %120, %118 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_3 after %1 : i32
    %121 = comb.extract %2 from 5 : (i32) -> i27
    %122 = comb.extract %2 from 0 : (i32) -> i4
    %123 = comb.concat %121, %false, %122 : i27, i1, i4
    %124 = comb.concat %121, %17, %122 : i27, i1, i4
    %125 = comb.xor %19, %true : i1
    %debounce_d1_4 = seq.firreg %124 clock %104 reset async %125, %123 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_4 after %1 : i32
    %126 = comb.extract %2 from 6 : (i32) -> i26
    %127 = comb.extract %2 from 0 : (i32) -> i5
    %128 = comb.concat %126, %false, %127 : i26, i1, i5
    %129 = comb.concat %126, %20, %127 : i26, i1, i5
    %130 = comb.xor %22, %true : i1
    %debounce_d1_5 = seq.firreg %129 clock %104 reset async %130, %128 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_5 after %1 : i32
    %131 = comb.extract %2 from 7 : (i32) -> i25
    %132 = comb.extract %2 from 0 : (i32) -> i6
    %133 = comb.concat %131, %false, %132 : i25, i1, i6
    %134 = comb.concat %131, %23, %132 : i25, i1, i6
    %135 = comb.xor %25, %true : i1
    %debounce_d1_6 = seq.firreg %134 clock %104 reset async %135, %133 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_6 after %1 : i32
    %136 = comb.extract %2 from 8 : (i32) -> i24
    %137 = comb.extract %2 from 0 : (i32) -> i7
    %138 = comb.concat %136, %false, %137 : i24, i1, i7
    %139 = comb.concat %136, %26, %137 : i24, i1, i7
    %140 = comb.xor %28, %true : i1
    %debounce_d1_7 = seq.firreg %139 clock %104 reset async %140, %138 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_7 after %1 : i32
    %141 = comb.extract %2 from 9 : (i32) -> i23
    %142 = comb.extract %2 from 0 : (i32) -> i8
    %143 = comb.concat %141, %false, %142 : i23, i1, i8
    %144 = comb.concat %141, %29, %142 : i23, i1, i8
    %145 = comb.xor %31, %true : i1
    %debounce_d1_8 = seq.firreg %144 clock %104 reset async %145, %143 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_8 after %1 : i32
    %146 = comb.extract %2 from 10 : (i32) -> i22
    %147 = comb.extract %2 from 0 : (i32) -> i9
    %148 = comb.concat %146, %false, %147 : i22, i1, i9
    %149 = comb.concat %146, %32, %147 : i22, i1, i9
    %150 = comb.xor %34, %true : i1
    %debounce_d1_9 = seq.firreg %149 clock %104 reset async %150, %148 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_9 after %1 : i32
    %151 = comb.extract %2 from 11 : (i32) -> i21
    %152 = comb.extract %2 from 0 : (i32) -> i10
    %153 = comb.concat %151, %false, %152 : i21, i1, i10
    %154 = comb.concat %151, %35, %152 : i21, i1, i10
    %155 = comb.xor %37, %true : i1
    %debounce_d1_10 = seq.firreg %154 clock %104 reset async %155, %153 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_10 after %1 : i32
    %156 = comb.extract %2 from 12 : (i32) -> i20
    %157 = comb.extract %2 from 0 : (i32) -> i11
    %158 = comb.concat %156, %false, %157 : i20, i1, i11
    %159 = comb.concat %156, %38, %157 : i20, i1, i11
    %160 = comb.xor %40, %true : i1
    %debounce_d1_11 = seq.firreg %159 clock %104 reset async %160, %158 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_11 after %1 : i32
    %161 = comb.extract %2 from 13 : (i32) -> i19
    %162 = comb.extract %2 from 0 : (i32) -> i12
    %163 = comb.concat %161, %false, %162 : i19, i1, i12
    %164 = comb.concat %161, %41, %162 : i19, i1, i12
    %165 = comb.xor %43, %true : i1
    %debounce_d1_12 = seq.firreg %164 clock %104 reset async %165, %163 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_12 after %1 : i32
    %166 = comb.extract %2 from 14 : (i32) -> i18
    %167 = comb.extract %2 from 0 : (i32) -> i13
    %168 = comb.concat %166, %false, %167 : i18, i1, i13
    %169 = comb.concat %166, %44, %167 : i18, i1, i13
    %170 = comb.xor %46, %true : i1
    %debounce_d1_13 = seq.firreg %169 clock %104 reset async %170, %168 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_13 after %1 : i32
    %171 = comb.extract %2 from 15 : (i32) -> i17
    %172 = comb.extract %2 from 0 : (i32) -> i14
    %173 = comb.concat %171, %false, %172 : i17, i1, i14
    %174 = comb.concat %171, %47, %172 : i17, i1, i14
    %175 = comb.xor %49, %true : i1
    %debounce_d1_14 = seq.firreg %174 clock %104 reset async %175, %173 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_14 after %1 : i32
    %176 = comb.extract %2 from 16 : (i32) -> i16
    %177 = comb.extract %2 from 0 : (i32) -> i15
    %178 = comb.concat %176, %false, %177 : i16, i1, i15
    %179 = comb.concat %176, %50, %177 : i16, i1, i15
    %180 = comb.xor %52, %true : i1
    %debounce_d1_15 = seq.firreg %179 clock %104 reset async %180, %178 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_15 after %1 : i32
    %181 = comb.extract %2 from 17 : (i32) -> i15
    %182 = comb.extract %2 from 0 : (i32) -> i16
    %183 = comb.concat %181, %false, %182 : i15, i1, i16
    %184 = comb.concat %181, %53, %182 : i15, i1, i16
    %185 = comb.xor %55, %true : i1
    %debounce_d1_16 = seq.firreg %184 clock %104 reset async %185, %183 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_16 after %1 : i32
    %186 = comb.extract %2 from 18 : (i32) -> i14
    %187 = comb.extract %2 from 0 : (i32) -> i17
    %188 = comb.concat %186, %false, %187 : i14, i1, i17
    %189 = comb.concat %186, %56, %187 : i14, i1, i17
    %190 = comb.xor %58, %true : i1
    %debounce_d1_17 = seq.firreg %189 clock %104 reset async %190, %188 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_17 after %1 : i32
    %191 = comb.extract %2 from 19 : (i32) -> i13
    %192 = comb.extract %2 from 0 : (i32) -> i18
    %193 = comb.concat %191, %false, %192 : i13, i1, i18
    %194 = comb.concat %191, %59, %192 : i13, i1, i18
    %195 = comb.xor %61, %true : i1
    %debounce_d1_18 = seq.firreg %194 clock %104 reset async %195, %193 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_18 after %1 : i32
    %196 = comb.extract %2 from 20 : (i32) -> i12
    %197 = comb.extract %2 from 0 : (i32) -> i19
    %198 = comb.concat %196, %false, %197 : i12, i1, i19
    %199 = comb.concat %196, %62, %197 : i12, i1, i19
    %200 = comb.xor %64, %true : i1
    %debounce_d1_19 = seq.firreg %199 clock %104 reset async %200, %198 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_19 after %1 : i32
    %201 = comb.extract %2 from 21 : (i32) -> i11
    %202 = comb.extract %2 from 0 : (i32) -> i20
    %203 = comb.concat %201, %false, %202 : i11, i1, i20
    %204 = comb.concat %201, %65, %202 : i11, i1, i20
    %205 = comb.xor %67, %true : i1
    %debounce_d1_20 = seq.firreg %204 clock %104 reset async %205, %203 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_20 after %1 : i32
    %206 = comb.extract %2 from 22 : (i32) -> i10
    %207 = comb.extract %2 from 0 : (i32) -> i21
    %208 = comb.concat %206, %false, %207 : i10, i1, i21
    %209 = comb.concat %206, %68, %207 : i10, i1, i21
    %210 = comb.xor %70, %true : i1
    %debounce_d1_21 = seq.firreg %209 clock %104 reset async %210, %208 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_21 after %1 : i32
    %211 = comb.extract %2 from 23 : (i32) -> i9
    %212 = comb.extract %2 from 0 : (i32) -> i22
    %213 = comb.concat %211, %false, %212 : i9, i1, i22
    %214 = comb.concat %211, %71, %212 : i9, i1, i22
    %215 = comb.xor %73, %true : i1
    %debounce_d1_22 = seq.firreg %214 clock %104 reset async %215, %213 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_22 after %1 : i32
    %216 = comb.extract %2 from 24 : (i32) -> i8
    %217 = comb.extract %2 from 0 : (i32) -> i23
    %218 = comb.concat %216, %false, %217 : i8, i1, i23
    %219 = comb.concat %216, %74, %217 : i8, i1, i23
    %220 = comb.xor %76, %true : i1
    %debounce_d1_23 = seq.firreg %219 clock %104 reset async %220, %218 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_23 after %1 : i32
    %221 = comb.extract %2 from 25 : (i32) -> i7
    %222 = comb.extract %2 from 0 : (i32) -> i24
    %223 = comb.concat %221, %false, %222 : i7, i1, i24
    %224 = comb.concat %221, %77, %222 : i7, i1, i24
    %225 = comb.xor %79, %true : i1
    %debounce_d1_24 = seq.firreg %224 clock %104 reset async %225, %223 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_24 after %1 : i32
    %226 = comb.extract %2 from 26 : (i32) -> i6
    %227 = comb.extract %2 from 0 : (i32) -> i25
    %228 = comb.concat %226, %false, %227 : i6, i1, i25
    %229 = comb.concat %226, %80, %227 : i6, i1, i25
    %230 = comb.xor %82, %true : i1
    %debounce_d1_25 = seq.firreg %229 clock %104 reset async %230, %228 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_25 after %1 : i32
    %231 = comb.extract %2 from 27 : (i32) -> i5
    %232 = comb.extract %2 from 0 : (i32) -> i26
    %233 = comb.concat %231, %false, %232 : i5, i1, i26
    %234 = comb.concat %231, %83, %232 : i5, i1, i26
    %235 = comb.xor %85, %true : i1
    %debounce_d1_26 = seq.firreg %234 clock %104 reset async %235, %233 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_26 after %1 : i32
    %236 = comb.extract %2 from 28 : (i32) -> i4
    %237 = comb.extract %2 from 0 : (i32) -> i27
    %238 = comb.concat %236, %false, %237 : i4, i1, i27
    %239 = comb.concat %236, %86, %237 : i4, i1, i27
    %240 = comb.xor %88, %true : i1
    %debounce_d1_27 = seq.firreg %239 clock %104 reset async %240, %238 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_27 after %1 : i32
    %241 = comb.extract %2 from 29 : (i32) -> i3
    %242 = comb.extract %2 from 0 : (i32) -> i28
    %243 = comb.concat %241, %false, %242 : i3, i1, i28
    %244 = comb.concat %241, %89, %242 : i3, i1, i28
    %245 = comb.xor %91, %true : i1
    %debounce_d1_28 = seq.firreg %244 clock %104 reset async %245, %243 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_28 after %1 : i32
    %246 = comb.extract %2 from 30 : (i32) -> i2
    %247 = comb.extract %2 from 0 : (i32) -> i29
    %248 = comb.concat %246, %false, %247 : i2, i1, i29
    %249 = comb.concat %246, %92, %247 : i2, i1, i29
    %250 = comb.xor %94, %true : i1
    %debounce_d1_29 = seq.firreg %249 clock %104 reset async %250, %248 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_29 after %1 : i32
    %251 = comb.extract %2 from 31 : (i32) -> i1
    %252 = comb.extract %2 from 0 : (i32) -> i30
    %253 = comb.concat %251, %false, %252 : i1, i1, i30
    %254 = comb.concat %251, %95, %252 : i1, i1, i30
    %255 = comb.xor %97, %true : i1
    %debounce_d1_30 = seq.firreg %254 clock %104 reset async %255, %253 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_30 after %1 : i32
    %256 = comb.extract %2 from 0 : (i32) -> i31
    %257 = comb.concat %false, %256 : i1, i31
    %258 = comb.concat %98, %256 : i1, i31
    %259 = comb.xor %100, %true : i1
    %debounce_d1_31 = seq.firreg %258 clock %104 reset async %259, %257 {name = "debounce_d1"} : i32
    llhd.drv %debounce_d1, %debounce_d1_31 after %1 : i32
    %260 = comb.xor %dbclk_res, %true : i1
    %debounce_d2 = seq.firreg %2 clock %104 reset async %260, %c0_i32 : i32
    %gpio_ext_porta_int_1d = seq.firreg %4 clock %104 reset async %260, %c0_i32 : i32
    %gpio_ext_porta_int_d1 = seq.firreg %gpio_ext_porta_int_1d clock %104 reset async %260, %c0_i32 : i32
    %261:4 = llhd.process -> i32, i1, i32, i1 {
      cf.br ^bb1(%dbclk, %dbclk_res, %c0_i32, %false, %c0_i32, %false : i1, i1, i32, i1, i32, i1)
    ^bb1(%263: i1, %264: i1, %265: i32, %266: i1, %267: i32, %268: i1):  // 4 preds: ^bb0, ^bb2, ^bb3, ^bb4
      llhd.wait yield (%267, %268, %265, %266 : i32, i1, i32, i1), (%dbclk, %dbclk_res : i1, i1), ^bb2(%264, %263 : i1, i1)
    ^bb2(%269: i1, %270: i1):  // pred: ^bb1
      %271 = comb.xor bin %270, %true : i1
      %272 = comb.and bin %271, %dbclk : i1
      %273 = comb.xor bin %dbclk_res, %true : i1
      %274 = comb.and bin %269, %273 : i1
      %275 = comb.or bin %272, %274 : i1
      cf.cond_br %275, ^bb3, ^bb1(%dbclk, %dbclk_res, %262, %false, %c0_i32, %false : i1, i1, i32, i1, i32, i1)
    ^bb3:  // pred: ^bb2
      cf.cond_br %260, ^bb1(%dbclk, %dbclk_res, %c0_i32, %true, %c0_i32, %false : i1, i1, i32, i1, i32, i1), ^bb4(%262, %false, %c0_i32 : i32, i1, i32)
    ^bb4(%276: i32, %277: i1, %278: i32):  // 2 preds: ^bb3, ^bb5
      %279 = comb.icmp slt %278, %c32_i32 : i32
      cf.cond_br %279, ^bb5, ^bb1(%dbclk, %dbclk_res, %276, %277, %278, %true : i1, i1, i32, i1, i32, i1)
    ^bb5:  // pred: ^bb4
      %280 = comb.extract %278 from 5 : (i32) -> i27
      %281 = comb.icmp eq %280, %c0_i27 : i27
      %282 = comb.extract %278 from 0 : (i32) -> i5
      %283 = comb.mux %281, %282, %c-1_i5 : i5
      %284 = comb.shru %gpio_ext_porta_int_1d, %278 : i32
      %285 = comb.extract %284 from 0 : (i32) -> i1
      %286 = comb.shru %gpio_ext_porta_int_d1, %278 : i32
      %287 = comb.extract %286 from 0 : (i32) -> i1
      %288 = comb.icmp eq %285, %287 : i1
      %289 = comb.shru %262, %278 : i32
      %290 = comb.extract %289 from 0 : (i32) -> i1
      %291 = comb.mux %288, %285, %290 : i1
      %292 = comb.concat %c0_i27, %283 : i27, i5
      %293 = comb.shl %c1_i32, %292 : i32
      %294 = comb.xor bin %293, %c-1_i32 : i32
      %295 = comb.and %276, %294 : i32
      %296 = comb.concat %c0_i31, %291 : i31, i1
      %297 = comb.shl %296, %292 : i32
      %298 = comb.or %295, %297 : i32
      %299 = comb.add %278, %c1_i32 : i32
      cf.br ^bb4(%298, %true, %299 : i32, i1, i32)
    }
    llhd.drv %debounce_both_edge, %261#2 after %0 if %261#3 : i32
    %262 = llhd.prb %debounce_both_edge : i32
    hw.output %262, %4, %debounce_d2 : i32, i32, i32
  }
  hw.module @DW_apb_gpio_bcm99(in %clk_d : i1, in %rst_d_n : i1, in %data_s : i1, out data_d : i1) {
    %true = hw.constant true
    %false = hw.constant false
    %0 = seq.to_clock %clk_d
    %1 = comb.xor %rst_d_n, %true : i1
    %sample_meta = seq.firreg %data_s clock %0 reset async %1, %false : i1
    %sample_syncl = seq.firreg %sample_meta clock %0 reset async %1, %false : i1
    hw.output %sample_syncl : i1
  }
  hw.module private @DW_apb_gpio_apbif(in %pclk : i1, in %presetn : i1, in %penable : i1, in %pwrite : i1, in %pwdata : i32, in %paddr : i5, in %psel : i1, out gpio_swporta_dr : i32, out gpio_swporta_ddr : i32, in %gpio_ext_porta_rb : i32, out gpio_inten : i32, out gpio_intmask : i32, out gpio_inttype_level : i32, in %gpio_intstatus : i32, in %gpio_raw_intstatus : i32, out gpio_porta_eoi : i32, out gpio_ls_sync : i1, out gpio_int_polarity : i32, out gpio_debounce : i32, out gpio_int_bothedge : i32, out prdata : i32) {
    %c4157682_i32 = hw.constant 4157682 : i32
    %c236799_i32 = hw.constant 236799 : i32
    %c0_i24 = hw.constant 0 : i24
    %true = hw.constant true
    %c0_i31 = hw.constant 0 : i31
    %0 = llhd.constant_time <0ns, 0d, 1e>
    %c0_i4 = hw.constant 0 : i4
    %c-3_i5 = hw.constant -3 : i5
    %c-4_i5 = hw.constant -4 : i5
    %c-5_i5 = hw.constant -5 : i5
    %c-15_i5 = hw.constant -15 : i5
    %c-16_i5 = hw.constant -16 : i5
    %c-12_i5 = hw.constant -12 : i5
    %c0_i32 = hw.constant 0 : i32
    %c-13_i5 = hw.constant -13 : i5
    %c-8_i5 = hw.constant -8 : i5
    %c-6_i5 = hw.constant -6 : i5
    %c-14_i5 = hw.constant -14 : i5
    %c15_i5 = hw.constant 15 : i5
    %c14_i5 = hw.constant 14 : i5
    %c13_i5 = hw.constant 13 : i5
    %c12_i5 = hw.constant 12 : i5
    %c1_i5 = hw.constant 1 : i5
    %c0_i5 = hw.constant 0 : i5
    %false = hw.constant false
    %c842085930_i32 = hw.constant 842085930 : i32
    %prdata = llhd.sig %c0_i32 : i32
    %pwdata_int = llhd.sig %c0_i32 : i32
    %1:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%332: i32, %333: i1):  // 2 preds: ^bb0, ^bb2
      llhd.wait yield (%332, %333 : i32, i1), (%pwdata : i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb1(%pwdata, %true : i32, i1)
    }
    llhd.drv %pwdata_int, %1#0 after %0 if %1#1 : i32
    %2 = comb.and %psel, %penable, %pwrite : i1
    %3 = comb.icmp eq %paddr, %c0_i5 : i5
    %4 = comb.replicate %3 : (i1) -> i4
    %5 = comb.xor %2, %true : i1
    %6 = comb.mux %5, %c0_i4, %4 : i4
    %7 = comb.icmp eq %paddr, %c1_i5 : i5
    %8 = comb.replicate %7 : (i1) -> i4
    %9 = comb.mux %5, %c0_i4, %8 : i4
    %10 = comb.icmp eq %paddr, %c12_i5 : i5
    %11 = comb.replicate %10 : (i1) -> i4
    %12 = comb.mux %5, %c0_i4, %11 : i4
    %13 = comb.icmp eq %paddr, %c13_i5 : i5
    %14 = comb.replicate %13 : (i1) -> i4
    %15 = comb.mux %5, %c0_i4, %14 : i4
    %16 = comb.icmp eq %paddr, %c14_i5 : i5
    %17 = comb.replicate %16 : (i1) -> i4
    %18 = comb.mux %5, %c0_i4, %17 : i4
    %19 = comb.icmp eq %paddr, %c15_i5 : i5
    %20 = comb.replicate %19 : (i1) -> i4
    %21 = comb.mux %5, %c0_i4, %20 : i4
    %22 = comb.icmp eq %paddr, %c-14_i5 : i5
    %23 = comb.replicate %22 : (i1) -> i4
    %24 = comb.mux %5, %c0_i4, %23 : i4
    %25 = comb.icmp eq %paddr, %c-6_i5 : i5
    %26 = comb.replicate %25 : (i1) -> i4
    %27 = comb.mux %5, %c0_i4, %26 : i4
    %28 = comb.icmp eq %paddr, %c-8_i5 : i5
    %29 = comb.and %2, %28 : i1
    %30 = comb.icmp eq %paddr, %c-13_i5 : i5
    %31 = comb.replicate %30 : (i1) -> i4
    %32 = comb.mux %5, %c0_i4, %31 : i4
    %33 = comb.extract %6 from 0 : (i4) -> i1
    %34 = comb.extract %233 from 0 : (i32) -> i8
    %35 = comb.extract %gpio_swporta_dr from 8 : (i32) -> i24
    %36 = comb.concat %35, %34 : i24, i8
    %37 = comb.xor %33, %true : i1
    %38 = comb.mux %37, %gpio_swporta_dr, %36 : i32
    %39 = comb.extract %6 from 1 : (i4) -> i1
    %40 = comb.extract %233 from 8 : (i32) -> i8
    %41 = comb.extract %38 from 16 : (i32) -> i16
    %42 = comb.extract %38 from 0 : (i32) -> i8
    %43 = comb.concat %41, %40, %42 : i16, i8, i8
    %44 = comb.xor %39, %true : i1
    %45 = comb.mux %44, %38, %43 : i32
    %46 = comb.extract %6 from 2 : (i4) -> i1
    %47 = comb.extract %233 from 16 : (i32) -> i8
    %48 = comb.extract %45 from 24 : (i32) -> i8
    %49 = comb.extract %45 from 0 : (i32) -> i16
    %50 = comb.concat %48, %47, %49 : i8, i8, i16
    %51 = comb.xor %46, %true : i1
    %52 = comb.mux %51, %45, %50 : i32
    %53 = comb.extract %6 from 3 : (i4) -> i1
    %54 = comb.extract %233 from 24 : (i32) -> i8
    %55 = comb.extract %52 from 0 : (i32) -> i24
    %56 = comb.concat %54, %55 : i8, i24
    %57 = comb.xor %53, %true : i1
    %58 = comb.mux %57, %52, %56 : i32
    %59 = comb.icmp ne %6, %c0_i4 : i4
    %60 = seq.to_clock %pclk
    %61 = comb.xor %presetn, %true : i1
    %62 = comb.mux bin %59, %58, %gpio_swporta_dr : i32
    %gpio_swporta_dr = seq.firreg %62 clock %60 reset async %61, %c0_i32 : i32
    %63 = comb.extract %9 from 0 : (i4) -> i1
    %64 = comb.extract %gpio_swporta_ddr from 8 : (i32) -> i24
    %65 = comb.concat %64, %34 : i24, i8
    %66 = comb.xor %63, %true : i1
    %67 = comb.mux %66, %gpio_swporta_ddr, %65 : i32
    %68 = comb.extract %9 from 1 : (i4) -> i1
    %69 = comb.extract %67 from 16 : (i32) -> i16
    %70 = comb.extract %67 from 0 : (i32) -> i8
    %71 = comb.concat %69, %40, %70 : i16, i8, i8
    %72 = comb.xor %68, %true : i1
    %73 = comb.mux %72, %67, %71 : i32
    %74 = comb.extract %9 from 2 : (i4) -> i1
    %75 = comb.extract %73 from 24 : (i32) -> i8
    %76 = comb.extract %73 from 0 : (i32) -> i16
    %77 = comb.concat %75, %47, %76 : i8, i8, i16
    %78 = comb.xor %74, %true : i1
    %79 = comb.mux %78, %73, %77 : i32
    %80 = comb.extract %9 from 3 : (i4) -> i1
    %81 = comb.extract %79 from 0 : (i32) -> i24
    %82 = comb.concat %54, %81 : i8, i24
    %83 = comb.xor %80, %true : i1
    %84 = comb.mux %83, %79, %82 : i32
    %85 = comb.icmp ne %9, %c0_i4 : i4
    %86 = comb.mux bin %85, %84, %gpio_swporta_ddr : i32
    %gpio_swporta_ddr = seq.firreg %86 clock %60 reset async %61, %c0_i32 : i32
    %87 = comb.extract %12 from 0 : (i4) -> i1
    %88 = comb.extract %gpio_inten from 8 : (i32) -> i24
    %89 = comb.concat %88, %34 : i24, i8
    %90 = comb.xor %87, %true : i1
    %91 = comb.mux %90, %gpio_inten, %89 : i32
    %92 = comb.extract %12 from 1 : (i4) -> i1
    %93 = comb.extract %91 from 16 : (i32) -> i16
    %94 = comb.extract %91 from 0 : (i32) -> i8
    %95 = comb.concat %93, %40, %94 : i16, i8, i8
    %96 = comb.xor %92, %true : i1
    %97 = comb.mux %96, %91, %95 : i32
    %98 = comb.extract %12 from 2 : (i4) -> i1
    %99 = comb.extract %97 from 24 : (i32) -> i8
    %100 = comb.extract %97 from 0 : (i32) -> i16
    %101 = comb.concat %99, %47, %100 : i8, i8, i16
    %102 = comb.xor %98, %true : i1
    %103 = comb.mux %102, %97, %101 : i32
    %104 = comb.extract %12 from 3 : (i4) -> i1
    %105 = comb.extract %103 from 0 : (i32) -> i24
    %106 = comb.concat %54, %105 : i8, i24
    %107 = comb.xor %104, %true : i1
    %108 = comb.mux %107, %103, %106 : i32
    %109 = comb.icmp ne %12, %c0_i4 : i4
    %110 = comb.mux bin %109, %108, %gpio_inten : i32
    %gpio_inten = seq.firreg %110 clock %60 reset async %61, %c0_i32 : i32
    %111 = comb.extract %15 from 0 : (i4) -> i1
    %112 = comb.extract %gpio_intmask from 8 : (i32) -> i24
    %113 = comb.concat %112, %34 : i24, i8
    %114 = comb.xor %111, %true : i1
    %115 = comb.mux %114, %gpio_intmask, %113 : i32
    %116 = comb.extract %15 from 1 : (i4) -> i1
    %117 = comb.extract %115 from 16 : (i32) -> i16
    %118 = comb.extract %115 from 0 : (i32) -> i8
    %119 = comb.concat %117, %40, %118 : i16, i8, i8
    %120 = comb.xor %116, %true : i1
    %121 = comb.mux %120, %115, %119 : i32
    %122 = comb.extract %15 from 2 : (i4) -> i1
    %123 = comb.extract %121 from 24 : (i32) -> i8
    %124 = comb.extract %121 from 0 : (i32) -> i16
    %125 = comb.concat %123, %47, %124 : i8, i8, i16
    %126 = comb.xor %122, %true : i1
    %127 = comb.mux %126, %121, %125 : i32
    %128 = comb.extract %15 from 3 : (i4) -> i1
    %129 = comb.extract %127 from 0 : (i32) -> i24
    %130 = comb.concat %54, %129 : i8, i24
    %131 = comb.xor %128, %true : i1
    %132 = comb.mux %131, %127, %130 : i32
    %133 = comb.icmp ne %15, %c0_i4 : i4
    %134 = comb.mux bin %133, %132, %gpio_intmask : i32
    %gpio_intmask = seq.firreg %134 clock %60 reset async %61, %c0_i32 : i32
    %135 = comb.extract %18 from 0 : (i4) -> i1
    %136 = comb.extract %gpio_inttype_level from 8 : (i32) -> i24
    %137 = comb.concat %136, %34 : i24, i8
    %138 = comb.xor %135, %true : i1
    %139 = comb.mux %138, %gpio_inttype_level, %137 : i32
    %140 = comb.extract %18 from 1 : (i4) -> i1
    %141 = comb.extract %139 from 16 : (i32) -> i16
    %142 = comb.extract %139 from 0 : (i32) -> i8
    %143 = comb.concat %141, %40, %142 : i16, i8, i8
    %144 = comb.xor %140, %true : i1
    %145 = comb.mux %144, %139, %143 : i32
    %146 = comb.extract %18 from 2 : (i4) -> i1
    %147 = comb.extract %145 from 24 : (i32) -> i8
    %148 = comb.extract %145 from 0 : (i32) -> i16
    %149 = comb.concat %147, %47, %148 : i8, i8, i16
    %150 = comb.xor %146, %true : i1
    %151 = comb.mux %150, %145, %149 : i32
    %152 = comb.extract %18 from 3 : (i4) -> i1
    %153 = comb.extract %151 from 0 : (i32) -> i24
    %154 = comb.concat %54, %153 : i8, i24
    %155 = comb.xor %152, %true : i1
    %156 = comb.mux %155, %151, %154 : i32
    %157 = comb.icmp ne %18, %c0_i4 : i4
    %158 = comb.mux bin %157, %156, %gpio_inttype_level : i32
    %gpio_inttype_level = seq.firreg %158 clock %60 reset async %61, %c0_i32 : i32
    %159 = comb.extract %21 from 0 : (i4) -> i1
    %160 = comb.extract %gpio_int_polarity from 8 : (i32) -> i24
    %161 = comb.concat %160, %34 : i24, i8
    %162 = comb.xor %159, %true : i1
    %163 = comb.mux %162, %gpio_int_polarity, %161 : i32
    %164 = comb.extract %21 from 1 : (i4) -> i1
    %165 = comb.extract %163 from 16 : (i32) -> i16
    %166 = comb.extract %163 from 0 : (i32) -> i8
    %167 = comb.concat %165, %40, %166 : i16, i8, i8
    %168 = comb.xor %164, %true : i1
    %169 = comb.mux %168, %163, %167 : i32
    %170 = comb.extract %21 from 2 : (i4) -> i1
    %171 = comb.extract %169 from 24 : (i32) -> i8
    %172 = comb.extract %169 from 0 : (i32) -> i16
    %173 = comb.concat %171, %47, %172 : i8, i8, i16
    %174 = comb.xor %170, %true : i1
    %175 = comb.mux %174, %169, %173 : i32
    %176 = comb.extract %21 from 3 : (i4) -> i1
    %177 = comb.extract %175 from 0 : (i32) -> i24
    %178 = comb.concat %54, %177 : i8, i24
    %179 = comb.xor %176, %true : i1
    %180 = comb.mux %179, %175, %178 : i32
    %181 = comb.icmp ne %21, %c0_i4 : i4
    %182 = comb.mux bin %181, %180, %gpio_int_polarity : i32
    %gpio_int_polarity = seq.firreg %182 clock %60 reset async %61, %c0_i32 : i32
    %183 = comb.extract %24 from 0 : (i4) -> i1
    %184 = comb.extract %gpio_debounce from 8 : (i32) -> i24
    %185 = comb.concat %184, %34 : i24, i8
    %186 = comb.xor %183, %true : i1
    %187 = comb.mux %186, %gpio_debounce, %185 : i32
    %188 = comb.extract %24 from 1 : (i4) -> i1
    %189 = comb.extract %187 from 16 : (i32) -> i16
    %190 = comb.extract %187 from 0 : (i32) -> i8
    %191 = comb.concat %189, %40, %190 : i16, i8, i8
    %192 = comb.xor %188, %true : i1
    %193 = comb.mux %192, %187, %191 : i32
    %194 = comb.extract %24 from 2 : (i4) -> i1
    %195 = comb.extract %193 from 24 : (i32) -> i8
    %196 = comb.extract %193 from 0 : (i32) -> i16
    %197 = comb.concat %195, %47, %196 : i8, i8, i16
    %198 = comb.xor %194, %true : i1
    %199 = comb.mux %198, %193, %197 : i32
    %200 = comb.extract %24 from 3 : (i4) -> i1
    %201 = comb.extract %199 from 0 : (i32) -> i24
    %202 = comb.concat %54, %201 : i8, i24
    %203 = comb.xor %200, %true : i1
    %204 = comb.mux %203, %199, %202 : i32
    %205 = comb.icmp ne %24, %c0_i4 : i4
    %206 = comb.mux bin %205, %204, %gpio_debounce : i32
    %gpio_debounce = seq.firreg %206 clock %60 reset async %61, %c0_i32 : i32
    %207 = comb.extract %27 from 0 : (i4) -> i1
    %208 = comb.extract %gpio_int_bothedge from 8 : (i32) -> i24
    %209 = comb.concat %208, %34 : i24, i8
    %210 = comb.xor %207, %true : i1
    %211 = comb.mux %210, %gpio_int_bothedge, %209 : i32
    %212 = comb.extract %27 from 1 : (i4) -> i1
    %213 = comb.extract %211 from 16 : (i32) -> i16
    %214 = comb.extract %211 from 0 : (i32) -> i8
    %215 = comb.concat %213, %40, %214 : i16, i8, i8
    %216 = comb.xor %212, %true : i1
    %217 = comb.mux %216, %211, %215 : i32
    %218 = comb.extract %27 from 2 : (i4) -> i1
    %219 = comb.extract %217 from 24 : (i32) -> i8
    %220 = comb.extract %217 from 0 : (i32) -> i16
    %221 = comb.concat %219, %47, %220 : i8, i8, i16
    %222 = comb.xor %218, %true : i1
    %223 = comb.mux %222, %217, %221 : i32
    %224 = comb.extract %27 from 3 : (i4) -> i1
    %225 = comb.extract %223 from 0 : (i32) -> i24
    %226 = comb.concat %54, %225 : i8, i24
    %227 = comb.xor %224, %true : i1
    %228 = comb.mux %227, %223, %226 : i32
    %229 = comb.icmp ne %27, %c0_i4 : i4
    %230 = comb.mux bin %229, %228, %gpio_int_bothedge : i32
    %gpio_int_bothedge = seq.firreg %230 clock %60 reset async %61, %c0_i32 : i32
    %231 = comb.extract %233 from 0 : (i32) -> i1
    %232 = comb.mux bin %29, %231, %gpio_ls_sync : i1
    %gpio_ls_sync = seq.firreg %232 clock %60 reset async %61, %false : i1
    %233 = llhd.prb %pwdata_int : i32
    %234 = comb.extract %32 from 0 : (i4) -> i1
    %235 = comb.concat %c0_i24, %34 : i24, i8
    %236 = comb.xor %234, %true : i1
    %237 = comb.mux %236, %c0_i32, %235 : i32
    %238 = comb.extract %32 from 1 : (i4) -> i1
    %239 = comb.extract %237 from 16 : (i32) -> i16
    %240 = comb.extract %237 from 0 : (i32) -> i8
    %241 = comb.concat %239, %40, %240 : i16, i8, i8
    %242 = comb.xor %238, %true : i1
    %243 = comb.mux %242, %237, %241 : i32
    %244 = comb.extract %32 from 2 : (i4) -> i1
    %245 = comb.extract %243 from 24 : (i32) -> i8
    %246 = comb.extract %243 from 0 : (i32) -> i16
    %247 = comb.concat %245, %47, %246 : i8, i8, i16
    %248 = comb.xor %244, %true : i1
    %249 = comb.mux %248, %243, %247 : i32
    %250 = comb.extract %32 from 3 : (i4) -> i1
    %251 = comb.extract %249 from 0 : (i32) -> i24
    %252 = comb.concat %54, %251 : i8, i24
    %253 = comb.mux %250, %252, %249 : i32
    %254 = comb.xor %pwrite, %true : i1
    %255 = comb.xor %penable, %true : i1
    %256 = comb.and %254, %psel, %255 : i1
    %257 = comb.icmp ceq %paddr, %c-12_i5 : i5
    %258 = comb.icmp ceq %paddr, %c0_i5 : i5
    %259 = comb.icmp ceq %paddr, %c1_i5 : i5
    %260 = comb.icmp ceq %paddr, %c12_i5 : i5
    %261 = comb.icmp ceq %paddr, %c13_i5 : i5
    %262 = comb.icmp ceq %paddr, %c14_i5 : i5
    %263 = comb.icmp ceq %paddr, %c-16_i5 : i5
    %264 = comb.icmp ceq %paddr, %c-15_i5 : i5
    %265 = comb.icmp ceq %paddr, %c15_i5 : i5
    %266 = comb.icmp ceq %paddr, %c-14_i5 : i5
    %267 = comb.icmp ceq %paddr, %c-6_i5 : i5
    %268 = comb.icmp ceq %paddr, %c-8_i5 : i5
    %269 = comb.icmp ceq %paddr, %c-5_i5 : i5
    %270 = comb.icmp ceq %paddr, %c-4_i5 : i5
    %271 = comb.icmp ceq %paddr, %c-3_i5 : i5
    %272 = comb.concat %c0_i31, %gpio_ls_sync : i31, i1
    %273 = comb.mux %271, %c4157682_i32, %c0_i32 : i32
    %274 = comb.xor %257, %true : i1
    %275 = comb.and %274, %256 : i1
    %276 = comb.xor %258, %true : i1
    %277 = comb.and %276, %275 : i1
    %278 = comb.xor %259, %true : i1
    %279 = comb.and %278, %277 : i1
    %280 = comb.xor %260, %true : i1
    %281 = comb.and %280, %279 : i1
    %282 = comb.xor %261, %true : i1
    %283 = comb.and %282, %281 : i1
    %284 = comb.xor %262, %true : i1
    %285 = comb.and %284, %283 : i1
    %286 = comb.xor %263, %true : i1
    %287 = comb.and %286, %285 : i1
    %288 = comb.xor %264, %true : i1
    %289 = comb.and %288, %287 : i1
    %290 = comb.xor %265, %true : i1
    %291 = comb.and %290, %289 : i1
    %292 = comb.xor %266, %true : i1
    %293 = comb.and %292, %291 : i1
    %294 = comb.xor %267, %true : i1
    %295 = comb.and %294, %293 : i1
    %296 = comb.xor %268, %true : i1
    %297 = comb.and %296, %295 : i1
    %298 = comb.xor %269, %true : i1
    %299 = comb.and %298, %297, %270 : i1
    %300 = comb.mux %299, %c236799_i32, %273 : i32
    %301 = comb.and %297, %269 : i1
    %302 = comb.mux %301, %c842085930_i32, %300 : i32
    %303 = comb.and %293, %267 : i1
    %304 = comb.mux %303, %gpio_int_bothedge, %302 : i32
    %305 = comb.and %291, %266 : i1
    %306 = comb.mux %305, %gpio_debounce, %304 : i32
    %307 = comb.and %289, %265 : i1
    %308 = comb.mux %307, %gpio_int_polarity, %306 : i32
    %309 = comb.and %287, %264 : i1
    %310 = comb.mux %309, %gpio_raw_intstatus, %308 : i32
    %311 = comb.and %285, %263 : i1
    %312 = comb.mux %311, %gpio_intstatus, %310 : i32
    %313 = comb.and %283, %262 : i1
    %314 = comb.mux %313, %gpio_inttype_level, %312 : i32
    %315 = comb.and %281, %261 : i1
    %316 = comb.mux %315, %gpio_intmask, %314 : i32
    %317 = comb.and %279, %260 : i1
    %318 = comb.mux %317, %gpio_inten, %316 : i32
    %319 = comb.and %277, %259 : i1
    %320 = comb.mux %319, %gpio_swporta_ddr, %318 : i32
    %321 = comb.and %275, %258 : i1
    %322 = comb.mux %321, %gpio_swporta_dr, %320 : i32
    %323 = comb.and %256, %257 : i1
    %324 = comb.mux %323, %gpio_ext_porta_rb, %322 : i32
    %325 = comb.and %268, %295 : i1
    %326 = comb.mux %325, %272, %324 : i32
    %327 = comb.xor %256, %true : i1
    %328 = comb.mux %327, %c0_i32, %326 : i32
    %329 = comb.mux bin %256, %328, %iprdata : i32
    %iprdata = seq.firreg %329 clock %60 reset async %61, %c0_i32 : i32
    %330:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%332: i32, %333: i1):  // 2 preds: ^bb0, ^bb2
      llhd.wait yield (%332, %333 : i32, i1), (%iprdata : i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb1(%iprdata, %true : i32, i1)
    }
    llhd.drv %prdata, %330#0 after %0 if %330#1 : i32
    %331 = llhd.prb %prdata : i32
    hw.output %gpio_swporta_dr, %gpio_swporta_ddr, %gpio_inten, %gpio_intmask, %gpio_inttype_level, %253, %gpio_ls_sync, %gpio_int_polarity, %gpio_debounce, %gpio_int_bothedge, %331 : i32, i32, i32, i32, i32, i32, i1, i32, i32, i32, i32
  }
  hw.module @DW_apb_gpio_bcm36_nhs(in %clk_d : i1, in %rst_d_n : i1, in %data_s : i1, out data_d : i1) {
    hw.output %data_s : i1
  }
  hw.module @DW_apb_gpio(in %pclk : i1, in %pclk_intr : i1, in %presetn : i1, in %penable : i1, in %pwrite : i1, in %pwdata : i32, in %paddr : i7, in %psel : i1, in %dbclk : i1, in %dbclk_res_n : i1, in %scan_mode : i1, in %gpio_ext_porta : i32, out gpio_porta_dr : i32, out gpio_porta_ddr : i32, out gpio_intr_flag : i1, out gpio_intrclk_en : i1, out prdata : i32) {
    %0 = comb.extract %paddr from 2 : (i7) -> i5
    %U_apb_gpio_apbif.gpio_swporta_dr, %U_apb_gpio_apbif.gpio_swporta_ddr, %U_apb_gpio_apbif.gpio_inten, %U_apb_gpio_apbif.gpio_intmask, %U_apb_gpio_apbif.gpio_inttype_level, %U_apb_gpio_apbif.gpio_porta_eoi, %U_apb_gpio_apbif.gpio_ls_sync, %U_apb_gpio_apbif.gpio_int_polarity, %U_apb_gpio_apbif.gpio_debounce, %U_apb_gpio_apbif.gpio_int_bothedge, %U_apb_gpio_apbif.prdata = hw.instance "U_apb_gpio_apbif" @DW_apb_gpio_apbif(pclk: %pclk: i1, presetn: %presetn: i1, penable: %penable: i1, pwrite: %pwrite: i1, pwdata: %pwdata: i32, paddr: %0: i5, psel: %psel: i1, gpio_ext_porta_rb: %U_apb_gpio_ctrl.gpio_ext_porta_rb: i32, gpio_intstatus: %U_apb_gpio_ctrl.gpio_intr_int: i32, gpio_raw_intstatus: %U_apb_gpio_ctrl.gpio_raw_intstatus: i32) -> (gpio_swporta_dr: i32, gpio_swporta_ddr: i32, gpio_inten: i32, gpio_intmask: i32, gpio_inttype_level: i32, gpio_porta_eoi: i32, gpio_ls_sync: i1, gpio_int_polarity: i32, gpio_debounce: i32, gpio_int_bothedge: i32, prdata: i32) {sv.namehint = "gpio_inten"}
    %U_apb_gpio_ctrl.gpio_porta_dr, %U_apb_gpio_ctrl.gpio_porta_ddr, %U_apb_gpio_ctrl.gpio_ext_porta_rb, %U_apb_gpio_ctrl.gpio_intr_flag, %U_apb_gpio_ctrl.gpio_intr_int, %U_apb_gpio_ctrl.gpio_raw_intstatus, %U_apb_gpio_ctrl.gpio_intrclk_en, %U_apb_gpio_ctrl.int_sy_in_unsync = hw.instance "U_apb_gpio_ctrl" @DW_apb_gpio_ctrl(gpio_swporta_dr: %U_apb_gpio_apbif.gpio_swporta_dr: i32, gpio_swporta_ddr: %U_apb_gpio_apbif.gpio_swporta_ddr: i32, pclk: %pclk: i1, presetn: %presetn: i1, pclk_intr: %pclk_intr: i1, gpio_inten: %U_apb_gpio_apbif.gpio_inten: i32, gpio_intmask: %U_apb_gpio_apbif.gpio_intmask: i32, gpio_inttype_level: %U_apb_gpio_apbif.gpio_inttype_level: i32, gpio_porta_eoi: %U_apb_gpio_apbif.gpio_porta_eoi: i32, gpio_ls_sync: %U_apb_gpio_apbif.gpio_ls_sync: i1, gpio_debounce: %U_apb_gpio_apbif.gpio_debounce: i32, gpio_int_bothedge: %U_apb_gpio_apbif.gpio_int_bothedge: i32, gpio_ext_porta_int: %U_apb_gpio_debounce.gpio_ext_porta_int: i32, debounce_d2: %U_apb_gpio_debounce.debounce_d2_out: i32, debounce_both_edge: %U_apb_gpio_debounce.debounce_both_edge_out: i32, gpio_ext_porta_sync: %U_apb_gpio_sync.gpio_ext_porta_sync: i32, int_pre_in: %U_apb_gpio_sync.int_pre_in: i32) -> (gpio_porta_dr: i32, gpio_porta_ddr: i32, gpio_ext_porta_rb: i32, gpio_intr_flag: i1, gpio_intr_int: i32, gpio_raw_intstatus: i32, gpio_intrclk_en: i1, int_sy_in_unsync: i32) {sv.namehint = "gpio_intstatus"}
    %U_apb_gpio_debounce.debounce_both_edge_out, %U_apb_gpio_debounce.gpio_ext_porta_int, %U_apb_gpio_debounce.debounce_d2_out = hw.instance "U_apb_gpio_debounce" @DW_apb_gpio_debounce(dbclk: %dbclk: i1, dbclk_res: %dbclk_res_n: i1, scan_mode: %scan_mode: i1, gpio_ext_porta: %gpio_ext_porta: i32, gpio_int_polarity: %U_apb_gpio_apbif.gpio_int_polarity: i32) -> (debounce_both_edge_out: i32, gpio_ext_porta_int: i32, debounce_d2_out: i32) {sv.namehint = "debounce_d2_out"}
    %U_apb_gpio_sync.int_pre_in, %U_apb_gpio_sync.gpio_ext_porta_sync = hw.instance "U_apb_gpio_sync" @DW_apb_gpio_sync(presetn: %presetn: i1, pclk_intr: %pclk_intr: i1, int_sy_in: %U_apb_gpio_ctrl.int_sy_in_unsync: i32, pclk: %pclk: i1, gpio_ext_porta: %gpio_ext_porta: i32) -> (int_pre_in: i32, gpio_ext_porta_sync: i32) {sv.namehint = "int_pre_in"}
    hw.output %U_apb_gpio_ctrl.gpio_porta_dr, %U_apb_gpio_ctrl.gpio_porta_ddr, %U_apb_gpio_ctrl.gpio_intr_flag, %U_apb_gpio_ctrl.gpio_intrclk_en, %U_apb_gpio_apbif.prdata : i32, i32, i1, i1, i32
  }
  hw.module private @DW_apb_gpio_sync(in %presetn : i1, in %pclk_intr : i1, in %int_sy_in : i32, out int_pre_in : i32, in %pclk : i1, in %gpio_ext_porta : i32, out gpio_ext_porta_sync : i32) {
    %U_DW_apb_gpio_bcm21_db2pil_int_sy_in_pisyzr.data_d = hw.instance "U_DW_apb_gpio_bcm21_db2pil_int_sy_in_pisyzr" @DW_apb_gpio_bcm21(clk_d: %pclk_intr: i1, rst_d_n: %presetn: i1, data_s: %int_sy_in: i32) -> (data_d: i32) {sv.namehint = "sdb2pil_int_pre_in"}
    %U_DW_apb_gpio_bcm21_async2pl_gpio_ext_porta_psyzr.data_d = hw.instance "U_DW_apb_gpio_bcm21_async2pl_gpio_ext_porta_psyzr" @DW_apb_gpio_bcm21_0(clk_d: %pclk: i1, rst_d_n: %presetn: i1, data_s: %gpio_ext_porta: i32) -> (data_d: i32) {sv.namehint = "sasync2pl_gpio_ext_porta_s"}
    hw.output %U_DW_apb_gpio_bcm21_db2pil_int_sy_in_pisyzr.data_d, %U_DW_apb_gpio_bcm21_async2pl_gpio_ext_porta_psyzr.data_d : i32, i32
  }
  hw.module private @DW_apb_gpio_ctrl(in %gpio_swporta_dr : i32, in %gpio_swporta_ddr : i32, out gpio_porta_dr : i32, out gpio_porta_ddr : i32, out gpio_ext_porta_rb : i32, in %pclk : i1, in %presetn : i1, in %pclk_intr : i1, in %gpio_inten : i32, in %gpio_intmask : i32, in %gpio_inttype_level : i32, in %gpio_porta_eoi : i32, in %gpio_ls_sync : i1, in %gpio_debounce : i32, in %gpio_int_bothedge : i32, out gpio_intr_flag : i1, out gpio_intr_int : i32, out gpio_raw_intstatus : i32, out gpio_intrclk_en : i1, in %gpio_ext_porta_int : i32, in %debounce_d2 : i32, in %debounce_both_edge : i32, in %gpio_ext_porta_sync : i32, in %int_pre_in : i32, out int_sy_in_unsync : i32) {
    %0 = llhd.constant_time <0ns, 1d, 0e>
    %c0_i31 = hw.constant 0 : i31
    %true = hw.constant true
    %c-1_i32 = hw.constant -1 : i32
    %c-1_i5 = hw.constant -1 : i5
    %c0_i27 = hw.constant 0 : i27
    %1 = llhd.constant_time <0ns, 0d, 1e>
    %false = hw.constant false
    %c1_i32 = hw.constant 1 : i32
    %c32_i32 = hw.constant 32 : i32
    %c0_i32 = hw.constant 0 : i32
    %int_sy_in = llhd.sig %c0_i32 : i32
    %2 = llhd.prb %int_sy_in : i32
    %ls_int_in = llhd.sig %c0_i32 : i32
    %3 = llhd.prb %ls_int_in : i32
    %gpio_intr_ed_pm = llhd.sig %c0_i32 : i32
    %4 = llhd.prb %gpio_intr_ed_pm : i32
    %int_gpio_raw_intstatus = llhd.sig %c0_i32 : i32
    %intrclk_en = llhd.sig %c0_i32 : i32
    %ed_out_n = llhd.sig %c0_i32 : i32
    %5 = llhd.prb %ed_out_n : i32
    %ed_out = llhd.sig %c0_i32 : i32
    %6 = llhd.prb %ed_out : i32
    %int_gpio_ext_porta_rb = llhd.sig %c0_i32 : i32
    %7:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%28: i32, %29: i1):  // 2 preds: ^bb0, ^bb3
      llhd.wait yield (%28, %29 : i32, i1), (%gpio_ext_porta_int, %debounce_d2, %debounce_both_edge, %gpio_int_bothedge, %gpio_debounce : i32, i32, i32, i32, i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb3(%c0_i32, %c0_i32 : i32, i32)
    ^bb3(%30: i32, %31: i32):  // 2 preds: ^bb2, ^bb9
      %32 = comb.icmp slt %30, %c32_i32 : i32
      cf.cond_br %32, ^bb4, ^bb1(%31, %true : i32, i1)
    ^bb4:  // pred: ^bb3
      %33 = comb.shru %gpio_debounce, %30 : i32
      %34 = comb.extract %33 from 0 : (i32) -> i1
      cf.cond_br %34, ^bb5, ^bb8
    ^bb5:  // pred: ^bb4
      %35 = comb.shru %gpio_int_bothedge, %30 : i32
      %36 = comb.extract %35 from 0 : (i32) -> i1
      cf.cond_br %36, ^bb6, ^bb7
    ^bb6:  // pred: ^bb5
      %37 = comb.extract %30 from 5 : (i32) -> i27
      %38 = comb.icmp eq %37, %c0_i27 : i27
      %39 = comb.extract %30 from 0 : (i32) -> i5
      %40 = comb.mux %38, %39, %c-1_i5 : i5
      %41 = comb.shru %debounce_both_edge, %30 : i32
      %42 = comb.extract %41 from 0 : (i32) -> i1
      %43 = comb.concat %c0_i27, %40 : i27, i5
      %44 = comb.shl %c1_i32, %43 : i32
      %45 = comb.xor bin %44, %c-1_i32 : i32
      %46 = comb.and %31, %45 : i32
      %47 = comb.concat %c0_i31, %42 : i31, i1
      %48 = comb.shl %47, %43 : i32
      %49 = comb.or %46, %48 : i32
      cf.br ^bb9(%49 : i32)
    ^bb7:  // pred: ^bb5
      %50 = comb.extract %30 from 5 : (i32) -> i27
      %51 = comb.icmp eq %50, %c0_i27 : i27
      %52 = comb.extract %30 from 0 : (i32) -> i5
      %53 = comb.mux %51, %52, %c-1_i5 : i5
      %54 = comb.shru %debounce_d2, %30 : i32
      %55 = comb.extract %54 from 0 : (i32) -> i1
      %56 = comb.concat %c0_i27, %53 : i27, i5
      %57 = comb.shl %c1_i32, %56 : i32
      %58 = comb.xor bin %57, %c-1_i32 : i32
      %59 = comb.and %31, %58 : i32
      %60 = comb.concat %c0_i31, %55 : i31, i1
      %61 = comb.shl %60, %56 : i32
      %62 = comb.or %59, %61 : i32
      cf.br ^bb9(%62 : i32)
    ^bb8:  // pred: ^bb4
      %63 = comb.extract %30 from 5 : (i32) -> i27
      %64 = comb.icmp eq %63, %c0_i27 : i27
      %65 = comb.extract %30 from 0 : (i32) -> i5
      %66 = comb.mux %64, %65, %c-1_i5 : i5
      %67 = comb.shru %gpio_ext_porta_int, %30 : i32
      %68 = comb.extract %67 from 0 : (i32) -> i1
      %69 = comb.concat %c0_i27, %66 : i27, i5
      %70 = comb.shl %c1_i32, %69 : i32
      %71 = comb.xor bin %70, %c-1_i32 : i32
      %72 = comb.and %31, %71 : i32
      %73 = comb.concat %c0_i31, %68 : i31, i1
      %74 = comb.shl %73, %69 : i32
      %75 = comb.or %72, %74 : i32
      cf.br ^bb9(%75 : i32)
    ^bb9(%76: i32):  // 3 preds: ^bb6, ^bb7, ^bb8
      %77 = comb.add %30, %c1_i32 : i32
      cf.br ^bb3(%77, %76 : i32, i32)
    }
    llhd.drv %int_sy_in, %7#0 after %1 if %7#1 : i32
    %8 = seq.to_clock %pclk_intr
    %9 = comb.xor %presetn, %true : i1
    %ed_int_d1 = seq.firreg %int_pre_in clock %8 reset async %9, %c0_i32 : i32
    %10 = comb.xor %int_pre_in, %ed_int_d1 : i32
    %11:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%28: i32, %29: i1):  // 2 preds: ^bb0, ^bb3
      llhd.wait yield (%28, %29 : i32, i1), (%10, %int_pre_in : i32, i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb3(%c0_i32, %c0_i32 : i32, i32)
    ^bb3(%30: i32, %31: i32):  // 2 preds: ^bb2, ^bb4
      %32 = comb.icmp slt %30, %c32_i32 : i32
      cf.cond_br %32, ^bb4, ^bb1(%31, %true : i32, i1)
    ^bb4:  // pred: ^bb3
      %33 = comb.extract %30 from 5 : (i32) -> i27
      %34 = comb.icmp eq %33, %c0_i27 : i27
      %35 = comb.extract %30 from 0 : (i32) -> i5
      %36 = comb.mux %34, %35, %c-1_i5 : i5
      %37 = comb.shru %10, %30 : i32
      %38 = comb.extract %37 from 0 : (i32) -> i1
      %39 = comb.shru %int_pre_in, %30 : i32
      %40 = comb.extract %39 from 0 : (i32) -> i1
      %41 = comb.and %38, %40 : i1
      %42 = comb.concat %c0_i27, %36 : i27, i5
      %43 = comb.shl %c1_i32, %42 : i32
      %44 = comb.xor bin %43, %c-1_i32 : i32
      %45 = comb.and %31, %44 : i32
      %46 = comb.concat %c0_i31, %41 : i31, i1
      %47 = comb.shl %46, %42 : i32
      %48 = comb.or %45, %47 : i32
      %49 = comb.add %30, %c1_i32 : i32
      cf.br ^bb3(%49, %48 : i32, i32)
    }
    llhd.drv %ed_out, %11#0 after %1 if %11#1 : i32
    %12 = comb.xor %int_pre_in, %c-1_i32 : i32
    %ed_int_n_d1 = seq.firreg %12 clock %8 reset async %9, %c0_i32 : i32
    %13 = comb.xor %12, %ed_int_n_d1 : i32
    %14:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%28: i32, %29: i1):  // 2 preds: ^bb0, ^bb3
      llhd.wait yield (%28, %29 : i32, i1), (%13, %12 : i32, i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb3(%c0_i32, %c0_i32 : i32, i32)
    ^bb3(%30: i32, %31: i32):  // 2 preds: ^bb2, ^bb4
      %32 = comb.icmp slt %30, %c32_i32 : i32
      cf.cond_br %32, ^bb4, ^bb1(%31, %true : i32, i1)
    ^bb4:  // pred: ^bb3
      %33 = comb.extract %30 from 5 : (i32) -> i27
      %34 = comb.icmp eq %33, %c0_i27 : i27
      %35 = comb.extract %30 from 0 : (i32) -> i5
      %36 = comb.mux %34, %35, %c-1_i5 : i5
      %37 = comb.shru %13, %30 : i32
      %38 = comb.extract %37 from 0 : (i32) -> i1
      %39 = comb.shru %12, %30 : i32
      %40 = comb.extract %39 from 0 : (i32) -> i1
      %41 = comb.and %38, %40 : i1
      %42 = comb.concat %c0_i27, %36 : i27, i5
      %43 = comb.shl %c1_i32, %42 : i32
      %44 = comb.xor bin %43, %c-1_i32 : i32
      %45 = comb.and %31, %44 : i32
      %46 = comb.concat %c0_i31, %41 : i31, i1
      %47 = comb.shl %46, %42 : i32
      %48 = comb.or %45, %47 : i32
      %49 = comb.add %30, %c1_i32 : i32
      cf.br ^bb3(%49, %48 : i32, i32)
    }
    llhd.drv %ed_out_n, %14#0 after %1 if %14#1 : i32
    %15:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%28: i32, %29: i1):  // 2 preds: ^bb0, ^bb3
      llhd.wait yield (%28, %29 : i32, i1), (%gpio_inttype_level, %gpio_ls_sync, %gpio_inten, %gpio_int_bothedge : i32, i1, i32, i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb3(%c0_i32, %16, %false : i32, i32, i1)
    ^bb3(%30: i32, %31: i32, %32: i1):  // 2 preds: ^bb2, ^bb9
      %33 = comb.icmp slt %30, %c32_i32 : i32
      cf.cond_br %33, ^bb4, ^bb1(%31, %32 : i32, i1)
    ^bb4:  // pred: ^bb3
      %34 = comb.shru %gpio_inten, %30 : i32
      %35 = comb.extract %34 from 0 : (i32) -> i1
      cf.cond_br %35, ^bb5, ^bb8
    ^bb5:  // pred: ^bb4
      %36 = comb.shru %gpio_inttype_level, %30 : i32
      %37 = comb.extract %36 from 0 : (i32) -> i1
      %38 = comb.shru %gpio_int_bothedge, %30 : i32
      %39 = comb.extract %38 from 0 : (i32) -> i1
      %40 = comb.or %37, %39 : i1
      cf.cond_br %40, ^bb6, ^bb7
    ^bb6:  // pred: ^bb5
      %41 = comb.extract %30 from 5 : (i32) -> i27
      %42 = comb.icmp eq %41, %c0_i27 : i27
      %43 = comb.extract %30 from 0 : (i32) -> i5
      %44 = comb.mux %42, %43, %c-1_i5 : i5
      %45 = comb.concat %c0_i27, %44 : i27, i5
      %46 = comb.shl %c1_i32, %45 : i32
      %47 = comb.xor bin %46, %c-1_i32 : i32
      %48 = comb.and %31, %47 : i32
      %49 = comb.or %48, %46 : i32
      cf.br ^bb9(%49 : i32)
    ^bb7:  // pred: ^bb5
      %50 = comb.extract %30 from 5 : (i32) -> i27
      %51 = comb.icmp eq %50, %c0_i27 : i27
      %52 = comb.extract %30 from 0 : (i32) -> i5
      %53 = comb.mux %51, %52, %c-1_i5 : i5
      %54 = comb.concat %c0_i27, %53 : i27, i5
      %55 = comb.shl %c1_i32, %54 : i32
      %56 = comb.xor bin %55, %c-1_i32 : i32
      %57 = comb.and %31, %56 : i32
      %58 = comb.concat %c0_i31, %gpio_ls_sync : i31, i1
      %59 = comb.shl %58, %54 : i32
      %60 = comb.or %57, %59 : i32
      cf.br ^bb9(%60 : i32)
    ^bb8:  // pred: ^bb4
      %61 = comb.extract %30 from 5 : (i32) -> i27
      %62 = comb.icmp eq %61, %c0_i27 : i27
      %63 = comb.extract %30 from 0 : (i32) -> i5
      %64 = comb.mux %62, %63, %c-1_i5 : i5
      %65 = comb.concat %c0_i27, %64 : i27, i5
      %66 = comb.shl %c1_i32, %65 : i32
      %67 = comb.xor bin %66, %c-1_i32 : i32
      %68 = comb.and %31, %67 : i32
      %69 = comb.shl %c0_i32, %65 : i32
      %70 = comb.or %68, %69 : i32
      cf.br ^bb9(%70 : i32)
    ^bb9(%71: i32):  // 3 preds: ^bb6, ^bb7, ^bb8
      %72 = comb.add %30, %c1_i32 : i32
      cf.br ^bb3(%72, %71, %true : i32, i32, i1)
    }
    llhd.drv %intrclk_en, %15#0 after %1 if %15#1 : i32
    %16 = llhd.prb %intrclk_en : i32
    %17 = comb.icmp ne %16, %c0_i32 : i32
    %18 = seq.to_clock %pclk
    %gpio_intrclk_en = seq.firreg %17 clock %18 reset async %9, %false : i1
    %19:4 = llhd.process -> i32, i1, i32, i1 {
      cf.br ^bb1(%pclk_intr, %presetn, %c0_i32, %false, %c0_i32, %false : i1, i1, i32, i1, i32, i1)
    ^bb1(%28: i1, %29: i1, %30: i32, %31: i1, %32: i32, %33: i1):  // 4 preds: ^bb0, ^bb2, ^bb3, ^bb4
      llhd.wait yield (%32, %33, %30, %31 : i32, i1, i32, i1), (%pclk_intr, %presetn : i1, i1), ^bb2(%29, %28 : i1, i1)
    ^bb2(%34: i1, %35: i1):  // pred: ^bb1
      %36 = comb.xor bin %35, %true : i1
      %37 = comb.and bin %36, %pclk_intr : i1
      %38 = comb.xor bin %presetn, %true : i1
      %39 = comb.and bin %34, %38 : i1
      %40 = comb.or bin %37, %39 : i1
      cf.cond_br %40, ^bb3, ^bb1(%pclk_intr, %presetn, %4, %false, %c0_i32, %false : i1, i1, i32, i1, i32, i1)
    ^bb3:  // pred: ^bb2
      cf.cond_br %9, ^bb1(%pclk_intr, %presetn, %c0_i32, %true, %c0_i32, %false : i1, i1, i32, i1, i32, i1), ^bb4(%4, %false, %c0_i32 : i32, i1, i32)
    ^bb4(%41: i32, %42: i1, %43: i32):  // 2 preds: ^bb3, ^bb13
      %44 = comb.icmp slt %43, %c32_i32 : i32
      cf.cond_br %44, ^bb5, ^bb1(%pclk_intr, %presetn, %41, %42, %43, %true : i1, i1, i32, i1, i32, i1)
    ^bb5:  // pred: ^bb4
      %45 = comb.shru %gpio_inten, %43 : i32
      %46 = comb.extract %45 from 0 : (i32) -> i1
      %47 = comb.xor %46, %true : i1
      cf.cond_br %47, ^bb6, ^bb7
    ^bb6:  // pred: ^bb5
      %48 = comb.extract %43 from 5 : (i32) -> i27
      %49 = comb.icmp eq %48, %c0_i27 : i27
      %50 = comb.extract %43 from 0 : (i32) -> i5
      %51 = comb.mux %49, %50, %c-1_i5 : i5
      %52 = comb.concat %c0_i27, %51 : i27, i5
      %53 = comb.shl %c1_i32, %52 : i32
      %54 = comb.xor bin %53, %c-1_i32 : i32
      %55 = comb.and %41, %54 : i32
      %56 = comb.shl %c0_i32, %52 : i32
      %57 = comb.or %55, %56 : i32
      cf.br ^bb13(%57, %true : i32, i1)
    ^bb7:  // pred: ^bb5
      %58 = comb.shru %gpio_int_bothedge, %43 : i32
      %59 = comb.extract %58 from 0 : (i32) -> i1
      %60 = comb.shru %6, %43 : i32
      %61 = comb.extract %60 from 0 : (i32) -> i1
      %62 = comb.shru %5, %43 : i32
      %63 = comb.extract %62 from 0 : (i32) -> i1
      %64 = comb.or %61, %63 : i1
      %65 = comb.shru %gpio_swporta_ddr, %43 : i32
      %66 = comb.extract %65 from 0 : (i32) -> i1
      %67 = comb.xor %66, %true : i1
      %68 = comb.and %59, %64, %46, %67 : i1
      cf.cond_br %68, ^bb8, ^bb9
    ^bb8:  // pred: ^bb7
      %69 = comb.extract %43 from 5 : (i32) -> i27
      %70 = comb.icmp eq %69, %c0_i27 : i27
      %71 = comb.extract %43 from 0 : (i32) -> i5
      %72 = comb.mux %70, %71, %c-1_i5 : i5
      %73 = comb.concat %c0_i27, %72 : i27, i5
      %74 = comb.shl %c1_i32, %73 : i32
      %75 = comb.xor bin %74, %c-1_i32 : i32
      %76 = comb.and %41, %75 : i32
      %77 = comb.or %76, %74 : i32
      cf.br ^bb13(%77, %true : i32, i1)
    ^bb9:  // pred: ^bb7
      %78 = comb.xor %59, %true : i1
      %79 = comb.and %78, %61, %46, %67 : i1
      cf.cond_br %79, ^bb10, ^bb11
    ^bb10:  // pred: ^bb9
      %80 = comb.extract %43 from 5 : (i32) -> i27
      %81 = comb.icmp eq %80, %c0_i27 : i27
      %82 = comb.extract %43 from 0 : (i32) -> i5
      %83 = comb.mux %81, %82, %c-1_i5 : i5
      %84 = comb.concat %c0_i27, %83 : i27, i5
      %85 = comb.shl %c1_i32, %84 : i32
      %86 = comb.xor bin %85, %c-1_i32 : i32
      %87 = comb.and %41, %86 : i32
      %88 = comb.or %87, %85 : i32
      cf.br ^bb13(%88, %true : i32, i1)
    ^bb11:  // pred: ^bb9
      %89 = comb.shru %gpio_porta_eoi, %43 : i32
      %90 = comb.extract %89 from 0 : (i32) -> i1
      cf.cond_br %90, ^bb12, ^bb13(%41, %42 : i32, i1)
    ^bb12:  // pred: ^bb11
      %91 = comb.extract %43 from 5 : (i32) -> i27
      %92 = comb.icmp eq %91, %c0_i27 : i27
      %93 = comb.extract %43 from 0 : (i32) -> i5
      %94 = comb.mux %92, %93, %c-1_i5 : i5
      %95 = comb.concat %c0_i27, %94 : i27, i5
      %96 = comb.shl %c1_i32, %95 : i32
      %97 = comb.xor bin %96, %c-1_i32 : i32
      %98 = comb.and %41, %97 : i32
      %99 = comb.shl %c0_i32, %95 : i32
      %100 = comb.or %98, %99 : i32
      cf.br ^bb13(%100, %true : i32, i1)
    ^bb13(%101: i32, %102: i1):  // 5 preds: ^bb6, ^bb8, ^bb10, ^bb11, ^bb12
      %103 = comb.add %43, %c1_i32 : i32
      cf.br ^bb4(%101, %102, %103 : i32, i1, i32)
    }
    llhd.drv %gpio_intr_ed_pm, %19#2 after %0 if %19#3 : i32
    %20:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%28: i32, %29: i1):  // 2 preds: ^bb0, ^bb3
      llhd.wait yield (%28, %29 : i32, i1), (%int_pre_in, %2, %gpio_ls_sync, %gpio_swporta_ddr : i32, i32, i1, i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb3(%c0_i32, %c0_i32 : i32, i32)
    ^bb3(%30: i32, %31: i32):  // 2 preds: ^bb2, ^bb9
      %32 = comb.icmp slt %30, %c32_i32 : i32
      cf.cond_br %32, ^bb4, ^bb1(%31, %true : i32, i1)
    ^bb4:  // pred: ^bb3
      %33 = comb.shru %gpio_swporta_ddr, %30 : i32
      %34 = comb.extract %33 from 0 : (i32) -> i1
      cf.cond_br %34, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      %35 = comb.extract %30 from 5 : (i32) -> i27
      %36 = comb.icmp eq %35, %c0_i27 : i27
      %37 = comb.extract %30 from 0 : (i32) -> i5
      %38 = comb.mux %36, %37, %c-1_i5 : i5
      %39 = comb.concat %c0_i27, %38 : i27, i5
      %40 = comb.shl %c1_i32, %39 : i32
      %41 = comb.xor bin %40, %c-1_i32 : i32
      %42 = comb.and %31, %41 : i32
      %43 = comb.shl %c0_i32, %39 : i32
      %44 = comb.or %42, %43 : i32
      cf.br ^bb9(%44 : i32)
    ^bb6:  // pred: ^bb4
      cf.cond_br %gpio_ls_sync, ^bb7, ^bb8
    ^bb7:  // pred: ^bb6
      %45 = comb.extract %30 from 5 : (i32) -> i27
      %46 = comb.icmp eq %45, %c0_i27 : i27
      %47 = comb.extract %30 from 0 : (i32) -> i5
      %48 = comb.mux %46, %47, %c-1_i5 : i5
      %49 = comb.shru %int_pre_in, %30 : i32
      %50 = comb.extract %49 from 0 : (i32) -> i1
      %51 = comb.concat %c0_i27, %48 : i27, i5
      %52 = comb.shl %c1_i32, %51 : i32
      %53 = comb.xor bin %52, %c-1_i32 : i32
      %54 = comb.and %31, %53 : i32
      %55 = comb.concat %c0_i31, %50 : i31, i1
      %56 = comb.shl %55, %51 : i32
      %57 = comb.or %54, %56 : i32
      cf.br ^bb9(%57 : i32)
    ^bb8:  // pred: ^bb6
      %58 = comb.extract %30 from 5 : (i32) -> i27
      %59 = comb.icmp eq %58, %c0_i27 : i27
      %60 = comb.extract %30 from 0 : (i32) -> i5
      %61 = comb.mux %59, %60, %c-1_i5 : i5
      %62 = comb.shru %2, %30 : i32
      %63 = comb.extract %62 from 0 : (i32) -> i1
      %64 = comb.concat %c0_i27, %61 : i27, i5
      %65 = comb.shl %c1_i32, %64 : i32
      %66 = comb.xor bin %65, %c-1_i32 : i32
      %67 = comb.and %31, %66 : i32
      %68 = comb.concat %c0_i31, %63 : i31, i1
      %69 = comb.shl %68, %64 : i32
      %70 = comb.or %67, %69 : i32
      cf.br ^bb9(%70 : i32)
    ^bb9(%71: i32):  // 3 preds: ^bb5, ^bb7, ^bb8
      %72 = comb.add %30, %c1_i32 : i32
      cf.br ^bb3(%72, %71 : i32, i32)
    }
    llhd.drv %ls_int_in, %20#0 after %1 if %20#1 : i32
    %21:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%28: i32, %29: i1):  // 2 preds: ^bb0, ^bb3
      llhd.wait yield (%28, %29 : i32, i1), (%gpio_inttype_level, %3, %4, %gpio_int_bothedge, %gpio_inten : i32, i32, i32, i32, i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb3(%c0_i32, %c0_i32 : i32, i32)
    ^bb3(%30: i32, %31: i32):  // 2 preds: ^bb2, ^bb11
      %32 = comb.icmp slt %30, %c32_i32 : i32
      cf.cond_br %32, ^bb4, ^bb1(%31, %true : i32, i1)
    ^bb4:  // pred: ^bb3
      %33 = comb.shru %gpio_inten, %30 : i32
      %34 = comb.extract %33 from 0 : (i32) -> i1
      %35 = comb.xor %34, %true : i1
      cf.cond_br %35, ^bb5, ^bb6
    ^bb5:  // pred: ^bb4
      %36 = comb.extract %30 from 5 : (i32) -> i27
      %37 = comb.icmp eq %36, %c0_i27 : i27
      %38 = comb.extract %30 from 0 : (i32) -> i5
      %39 = comb.mux %37, %38, %c-1_i5 : i5
      %40 = comb.concat %c0_i27, %39 : i27, i5
      %41 = comb.shl %c1_i32, %40 : i32
      %42 = comb.xor bin %41, %c-1_i32 : i32
      %43 = comb.and %31, %42 : i32
      %44 = comb.shl %c0_i32, %40 : i32
      %45 = comb.or %43, %44 : i32
      cf.br ^bb11(%45 : i32)
    ^bb6:  // pred: ^bb4
      %46 = comb.shru %gpio_int_bothedge, %30 : i32
      %47 = comb.extract %46 from 0 : (i32) -> i1
      cf.cond_br %47, ^bb7, ^bb8
    ^bb7:  // pred: ^bb6
      %48 = comb.extract %30 from 5 : (i32) -> i27
      %49 = comb.icmp eq %48, %c0_i27 : i27
      %50 = comb.extract %30 from 0 : (i32) -> i5
      %51 = comb.mux %49, %50, %c-1_i5 : i5
      %52 = comb.shru %4, %30 : i32
      %53 = comb.extract %52 from 0 : (i32) -> i1
      %54 = comb.concat %c0_i27, %51 : i27, i5
      %55 = comb.shl %c1_i32, %54 : i32
      %56 = comb.xor bin %55, %c-1_i32 : i32
      %57 = comb.and %31, %56 : i32
      %58 = comb.concat %c0_i31, %53 : i31, i1
      %59 = comb.shl %58, %54 : i32
      %60 = comb.or %57, %59 : i32
      cf.br ^bb11(%60 : i32)
    ^bb8:  // pred: ^bb6
      %61 = comb.shru %gpio_inttype_level, %30 : i32
      %62 = comb.extract %61 from 0 : (i32) -> i1
      cf.cond_br %62, ^bb9, ^bb10
    ^bb9:  // pred: ^bb8
      %63 = comb.extract %30 from 5 : (i32) -> i27
      %64 = comb.icmp eq %63, %c0_i27 : i27
      %65 = comb.extract %30 from 0 : (i32) -> i5
      %66 = comb.mux %64, %65, %c-1_i5 : i5
      %67 = comb.shru %4, %30 : i32
      %68 = comb.extract %67 from 0 : (i32) -> i1
      %69 = comb.concat %c0_i27, %66 : i27, i5
      %70 = comb.shl %c1_i32, %69 : i32
      %71 = comb.xor bin %70, %c-1_i32 : i32
      %72 = comb.and %31, %71 : i32
      %73 = comb.concat %c0_i31, %68 : i31, i1
      %74 = comb.shl %73, %69 : i32
      %75 = comb.or %72, %74 : i32
      cf.br ^bb11(%75 : i32)
    ^bb10:  // pred: ^bb8
      %76 = comb.extract %30 from 5 : (i32) -> i27
      %77 = comb.icmp eq %76, %c0_i27 : i27
      %78 = comb.extract %30 from 0 : (i32) -> i5
      %79 = comb.mux %77, %78, %c-1_i5 : i5
      %80 = comb.shru %3, %30 : i32
      %81 = comb.extract %80 from 0 : (i32) -> i1
      %82 = comb.concat %c0_i27, %79 : i27, i5
      %83 = comb.shl %c1_i32, %82 : i32
      %84 = comb.xor bin %83, %c-1_i32 : i32
      %85 = comb.and %31, %84 : i32
      %86 = comb.concat %c0_i31, %81 : i31, i1
      %87 = comb.shl %86, %82 : i32
      %88 = comb.or %85, %87 : i32
      cf.br ^bb11(%88 : i32)
    ^bb11(%89: i32):  // 4 preds: ^bb5, ^bb7, ^bb9, ^bb10
      %90 = comb.add %30, %c1_i32 : i32
      cf.br ^bb3(%90, %89 : i32, i32)
    }
    llhd.drv %int_gpio_raw_intstatus, %21#0 after %1 if %21#1 : i32
    %22 = llhd.prb %int_gpio_raw_intstatus : i32
    %23 = comb.xor %gpio_intmask, %c-1_i32 : i32
    %24 = comb.and %22, %23 : i32
    %25 = comb.icmp ne %24, %c0_i32 : i32
    %26:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%28: i32, %29: i1):  // 2 preds: ^bb0, ^bb3
      llhd.wait yield (%28, %29 : i32, i1), (%gpio_ext_porta_sync : i32), ^bb2
    ^bb2:  // pred: ^bb1
      cf.br ^bb3(%c0_i32, %c0_i32 : i32, i32)
    ^bb3(%30: i32, %31: i32):  // 2 preds: ^bb2, ^bb4
      %32 = comb.icmp slt %30, %c32_i32 : i32
      cf.cond_br %32, ^bb4, ^bb1(%31, %true : i32, i1)
    ^bb4:  // pred: ^bb3
      %33 = comb.extract %30 from 5 : (i32) -> i27
      %34 = comb.icmp eq %33, %c0_i27 : i27
      %35 = comb.extract %30 from 0 : (i32) -> i5
      %36 = comb.mux %34, %35, %c-1_i5 : i5
      %37 = comb.shru %gpio_ext_porta_sync, %30 : i32
      %38 = comb.extract %37 from 0 : (i32) -> i1
      %39 = comb.concat %c0_i27, %36 : i27, i5
      %40 = comb.shl %c1_i32, %39 : i32
      %41 = comb.xor bin %40, %c-1_i32 : i32
      %42 = comb.and %31, %41 : i32
      %43 = comb.concat %c0_i31, %38 : i31, i1
      %44 = comb.shl %43, %39 : i32
      %45 = comb.or %42, %44 : i32
      %46 = comb.add %30, %c1_i32 : i32
      cf.br ^bb3(%46, %45 : i32, i32)
    }
    llhd.drv %int_gpio_ext_porta_rb, %26#0 after %1 if %26#1 : i32
    %27 = llhd.prb %int_gpio_ext_porta_rb : i32
    hw.output %gpio_swporta_dr, %gpio_swporta_ddr, %27, %25, %24, %22, %gpio_intrclk_en, %2 : i32, i32, i32, i1, i32, i32, i1, i32
  }
  hw.module @DW_apb_gpio_bcm00_ck_inv(in %clk_in : i1, out clk_out : i1) {
    %true = hw.constant true
    %0 = comb.xor %clk_in, %true : i1
    hw.output %0 : i1
  }
}
