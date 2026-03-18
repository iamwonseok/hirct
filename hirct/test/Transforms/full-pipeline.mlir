// RUN: %hirct-gen --dump-ir %s 2>&1 | %FileCheck %s

// Full HIRCT pipeline test: SimCleanup + ProcessFlatten + SignalLowering + CSE + Canonicalize
// Verifies that all LLHD/CF/sim ops are removed for combinational patterns.

module {
  hw.module @FullPipelineTest(in %sel : i1, in %a : i32, in %b : i32, out out0 : i32) {
    %c0_i32 = hw.constant 0 : i32
    %false = hw.constant false
    %true = hw.constant true
    %time = llhd.constant_time <0ns, 0d, 1e>

    // sim-only process (Pass 1 target)
    llhd.process {
      llhd.halt
    }

    // combinational mux process (Pass 2 target)
    %p:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%v: i32, %valid: i1):
      llhd.wait yield (%v, %valid : i32, i1), (%sel, %a, %b : i1, i32, i32), ^bb2
    ^bb2:
      cf.cond_br %sel, ^bb1(%a, %true : i32, i1), ^bb1(%b, %true : i32, i1)
    }

    // signal with conditional drive (Pass 3 target)
    %sig = llhd.sig %c0_i32 : i32
    llhd.drv %sig, %p#0 after %time if %p#1 : i32
    %result = llhd.prb %sig : i32

    hw.output %result : i32
  }
}

// After full pipeline: no LLHD, CF, or sim ops should remain
// CHECK-LABEL: hw.module @FullPipelineTest
// CHECK-NOT: llhd.
// CHECK-NOT: cf.
// CHECK-NOT: sim.
// CHECK: comb.mux
// CHECK: hw.output
