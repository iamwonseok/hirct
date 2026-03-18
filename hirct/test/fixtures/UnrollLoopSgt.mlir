module {
  // Reverse loop: for (i = 3; i > -1; i--) — sgt condition, trip_count = 4
  // Constants defined inside the process body so try_const can resolve them
  // directly via hw.ConstantOp::getDefiningOp without going through tmpvar.
  hw.module @UnrollLoopSgt(in %clk : i1, in %data : i8, out result : i8) {
    %c0_i8 = hw.constant 0 : i8
    %false  = hw.constant false
    %true   = hw.constant true
    %proc:2 = llhd.process -> i8, i1 {
      cf.br ^wait(%c0_i8, %false : i8, i1)
    ^wait(%r : i8, %changed : i1):
      llhd.wait yield (%r, %changed : i8, i1), (%data : i8), ^wakeup
    ^wakeup:
      %c3_i32  = hw.constant 3  : i32
      %c0_i8_2 = hw.constant 0  : i8
      cf.br ^loop(%c3_i32, %c0_i8_2 : i32, i8)
    ^loop(%i : i32, %acc : i8):
      %c-1_i32 = hw.constant -1 : i32
      %cmp = comb.icmp sgt %i, %c-1_i32 : i32
      cf.cond_br %cmp, ^body(%acc : i8), ^done(%acc : i8)
    ^body(%acc2 : i8):
      %c-1_i32_b = hw.constant -1 : i32
      %sum       = comb.add %acc2, %data : i8
      %next_i    = comb.add %i, %c-1_i32_b : i32
      cf.br ^loop(%next_i, %sum : i32, i8)
    ^done(%final : i8):
      cf.br ^wait(%final, %true : i8, i1)
    }
    %result = comb.mux %proc#1, %proc#0, %c0_i8 : i8
    hw.output %result : i8
  }
}
