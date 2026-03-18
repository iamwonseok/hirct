// RUN: %hirct-gen --dump-ir --run-pass process-deseq %s 2>&1 | %FileCheck %s

// TC1: Simple sequential process with posedge clk + async active-low reset.
// Logic body has a single block with two back-edges (cond_br both → ^bb1).

module {
  hw.module @DeseqSimple(in %clk : i1, in %rst_n : i1, in %sel : i1,
                          in %a : i32, in %b : i32, out out0 : i32) {
    %c0_i32 = hw.constant 0 : i32
    %false = hw.constant false
    %true = hw.constant true

    %p:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%clk, %rst_n, %c0_i32, %false : i1, i1, i32, i1)
    ^bb1(%pc: i1, %pr: i1, %val: i32, %vld: i1):
      llhd.wait yield (%val, %vld : i32, i1), (%clk, %rst_n : i1, i1), ^bb2(%pr, %pc : i1, i1)
    ^bb2(%old_r: i1, %old_c: i1):
      %ne = comb.xor bin %old_c, %true : i1
      %pe = comb.and bin %ne, %clk : i1
      %nr = comb.xor bin %rst_n, %true : i1
      %nre = comb.and bin %old_r, %nr : i1
      %edge = comb.or bin %pe, %nre : i1
      cf.cond_br %edge, ^bb3, ^bb1(%clk, %rst_n, %val, %false : i1, i1, i32, i1)
    ^bb3:
      %is_rst = comb.xor %rst_n, %true : i1
      cf.cond_br %is_rst, ^bb1(%clk, %rst_n, %c0_i32, %true : i1, i1, i32, i1), ^bb4
    ^bb4:
      cf.cond_br %sel, ^bb1(%clk, %rst_n, %a, %true : i1, i1, i32, i1), ^bb1(%clk, %rst_n, %b, %true : i1, i1, i32, i1)
    }

    %result = comb.mux %p#1, %p#0, %c0_i32 : i32
    hw.output %result : i32
  }

// TC2: Multi-block logic body with 3 distinct back-edges to ^bb1.

  hw.module @DeseqMultiPath(in %clk : i1, in %rst_n : i1, in %c1 : i1,
                             in %c2 : i1, in %a : i32, in %b : i32,
                             in %c : i32, out out0 : i32) {
    %c0_i32 = hw.constant 0 : i32
    %false = hw.constant false
    %true = hw.constant true

    %p:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%clk, %rst_n, %c0_i32, %false : i1, i1, i32, i1)
    ^bb1(%pc: i1, %pr: i1, %val: i32, %vld: i1):
      llhd.wait yield (%val, %vld : i32, i1), (%clk, %rst_n : i1, i1), ^bb2(%pr, %pc : i1, i1)
    ^bb2(%old_r: i1, %old_c: i1):
      %ne = comb.xor bin %old_c, %true : i1
      %pe = comb.and bin %ne, %clk : i1
      %nr = comb.xor bin %rst_n, %true : i1
      %nre = comb.and bin %old_r, %nr : i1
      %edge = comb.or bin %pe, %nre : i1
      cf.cond_br %edge, ^bb3, ^bb1(%clk, %rst_n, %val, %false : i1, i1, i32, i1)
    ^bb3:
      %is_rst = comb.xor %rst_n, %true : i1
      cf.cond_br %is_rst, ^bb1(%clk, %rst_n, %c0_i32, %true : i1, i1, i32, i1), ^bb4
    ^bb4:
      cf.cond_br %c1, ^bb5, ^bb6
    ^bb5:
      cf.br ^bb1(%clk, %rst_n, %a, %true : i1, i1, i32, i1)
    ^bb6:
      cf.cond_br %c2, ^bb1(%clk, %rst_n, %b, %true : i1, i1, i32, i1), ^bb1(%clk, %rst_n, %c, %true : i1, i1, i32, i1)
    }

    %result = comb.mux %p#1, %p#0, %c0_i32 : i32
    hw.output %result : i32
  }
}

// TC1: sequential process lowered to registers
// CHECK-LABEL: hw.module @DeseqSimple
// CHECK-NOT: llhd.process
// CHECK-NOT: llhd.wait
// CHECK: seq.to_clock
// CHECK: seq.compreg
// CHECK: seq.compreg
// CHECK: comb.mux
// CHECK: hw.output

// TC2: multi-path logic body also lowered
// CHECK-LABEL: hw.module @DeseqMultiPath
// CHECK-NOT: llhd.process
// CHECK-NOT: llhd.wait
// CHECK: seq.to_clock
// CHECK: seq.compreg
// CHECK: seq.compreg
// CHECK: comb.mux
// CHECK: hw.output
