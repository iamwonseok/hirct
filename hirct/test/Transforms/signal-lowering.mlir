// RUN: %hirct-gen --dump-ir %s 2>&1 | %FileCheck %s

// TC1: conditional drive → comb.mux
// TC2: unconditional drive → direct forwarding
// TC3: sig+drv+prb 3-tuple complete removal
// TC4: constant_time removed by DCE

module {
  hw.module @SignalLoweringTest(in %data : i32, in %enable : i1, out out0 : i32, out out1 : i32) {
    %c0_i32 = hw.constant 0 : i32
    %time = llhd.constant_time <0ns, 0d, 1e>

    // TC1: conditional drive — should become comb.mux
    %sig0 = llhd.sig %c0_i32 : i32
    llhd.drv %sig0, %data after %time if %enable : i32
    %prb0 = llhd.prb %sig0 : i32

    // TC2: unconditional drive — should forward %data directly
    %sig1 = llhd.sig %c0_i32 : i32
    llhd.drv %sig1, %data after %time : i32
    %prb1 = llhd.prb %sig1 : i32

    hw.output %prb0, %prb1 : i32, i32
  }
}

// CHECK-LABEL: hw.module @SignalLoweringTest
// CHECK-NOT: llhd.sig
// CHECK-NOT: llhd.drv
// CHECK-NOT: llhd.prb
// CHECK-NOT: llhd.constant_time
// CHECK: comb.mux
// CHECK: hw.output
