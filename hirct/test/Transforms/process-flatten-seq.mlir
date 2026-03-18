// RUN: %hirct-gen --dump-ir %s 2>&1 | %FileCheck %s

// Verify that sequential processes (edge-detection pattern: xor+and) are
// classified as SEQUENTIAL and preserved by HirctProcessFlatten.

module {
  hw.module @SeqProcessPreserved(in %clk : i1, in %rst_n : i1, in %data : i32, out out0 : i32) {
    %c0_i32 = hw.constant 0 : i32
    %false = hw.constant false
    %true = hw.constant true
    %time = llhd.constant_time <0ns, 0d, 1e>

    // Sequential process with edge detection (Pattern B) — should be PRESERVED
    %p:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%clk, %rst_n, %c0_i32, %false : i1, i1, i32, i1)
    ^bb1(%prev_clk: i1, %prev_rst: i1, %val: i32, %valid: i1):
      llhd.wait yield (%val, %valid : i32, i1), (%clk, %rst_n : i1, i1), ^bb2(%prev_rst, %prev_clk : i1, i1)
    ^bb2(%old_rst: i1, %old_clk: i1):
      %posedge = comb.xor bin %old_clk, %true : i1
      %edge = comb.and bin %posedge, %clk : i1
      cf.cond_br %edge, ^bb3, ^bb1(%clk, %rst_n, %val, %false : i1, i1, i32, i1)
    ^bb3:
      cf.br ^bb1(%clk, %rst_n, %data, %true : i1, i1, i32, i1)
    }

    // Signal lowering still handles the sig/drv/prb even with process preserved
    %sig = llhd.sig %c0_i32 : i32
    llhd.drv %sig, %p#0 after %time if %p#1 : i32
    %result = llhd.prb %sig : i32

    hw.output %result : i32
  }
}

// Sequential process should remain (not flattened)
// CHECK-LABEL: hw.module @SeqProcessPreserved
// CHECK: llhd.process -> i32, i1
// CHECK: llhd.wait
// Signal lowering should still work
// CHECK-NOT: llhd.sig
// CHECK-NOT: llhd.drv
// CHECK-NOT: llhd.prb
