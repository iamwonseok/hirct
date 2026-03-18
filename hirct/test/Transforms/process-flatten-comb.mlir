// RUN: %hirct-gen --dump-ir %s 2>&1 | %FileCheck %s
// RUN: %hirct-gen --dump-ir --run-pass process-flatten %s 2>&1 | %FileCheck %s --check-prefix=FLAT

// TC1: simple passthrough process → direct forwarding
// TC2: if-else (cond_br) process → comb.mux

module {
  hw.module @ProcessFlattenCombTest(in %sel : i1, in %a : i32, in %b : i32, in %data : i32, out out0 : i32, out out1 : i32) {
    %c0_i32 = hw.constant 0 : i32
    %false = hw.constant false
    %true = hw.constant true
    %time = llhd.constant_time <0ns, 0d, 1e>

    // TC1: simple passthrough — should just forward %data
    %p0:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%v0: i32, %valid0: i1):
      llhd.wait yield (%v0, %valid0 : i32, i1), (%data : i32), ^bb2
    ^bb2:
      cf.br ^bb1(%data, %true : i32, i1)
    }
    %sig0 = llhd.sig %c0_i32 : i32
    llhd.drv %sig0, %p0#0 after %time if %p0#1 : i32
    %r0 = llhd.prb %sig0 : i32

    // TC2: if-else mux — should become comb.mux
    %p1:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%v1: i32, %valid1: i1):
      llhd.wait yield (%v1, %valid1 : i32, i1), (%sel, %a, %b : i1, i32, i32), ^bb2
    ^bb2:
      cf.cond_br %sel, ^bb1(%a, %true : i32, i1), ^bb1(%b, %true : i32, i1)
    }
    %sig1 = llhd.sig %c0_i32 : i32
    llhd.drv %sig1, %p1#0 after %time if %p1#1 : i32
    %r1 = llhd.prb %sig1 : i32

    hw.output %r0, %r1 : i32, i32
  }
}

// CHECK-LABEL: hw.module @ProcessFlattenCombTest
// CHECK-NOT: llhd.process
// CHECK-NOT: llhd.wait
// CHECK-NOT: cf.br
// CHECK-NOT: cf.cond_br
// CHECK-NOT: llhd.sig
// CHECK-NOT: llhd.drv
// CHECK-NOT: llhd.prb
// CHECK: comb.mux
// CHECK: hw.output

// FLAT-LABEL: hw.module @ProcessFlattenCombTest
// FLAT-NOT: llhd.process
// FLAT-NOT: llhd.wait
// FLAT-NOT: cf.br
// FLAT-NOT: cf.cond_br
// FLAT: comb.mux
// FLAT: hw.output
