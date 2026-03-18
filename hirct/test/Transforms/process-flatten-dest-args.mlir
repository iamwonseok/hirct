// RUN: %hirct-gen --dump-ir %s 2>&1 | %FileCheck %s
// RUN: %hirct-gen --dump-ir --run-pass process-flatten %s 2>&1 | %FileCheck %s --check-prefix=FLAT

// Verify that a COMBINATIONAL process with dest operands on wait
// (^wakeup with block arguments) is correctly flattened. This exercises
// the code path enabled by removing the dest-operands guard in Task 1.
// No edge-detection (xor+and) → classified as COMBINATIONAL, not SEQUENTIAL.

module {
  hw.module @CombWithDestArgs(in %a : i32, in %b : i32, in %sel : i1,
                               out result : i32) {
    %c0_i32 = hw.constant 0 : i32
    %false = hw.constant false
    %true = hw.constant true
    %time = llhd.constant_time <0ns, 0d, 1e>

    // Combinational process with dest operands:
    // wait passes %a to wakeup block as block arg (no edge detect).
    // True branch uses prev_val (from dest operand), false branch uses %b,
    // producing a non-trivial mux.
    %p:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%v: i32, %valid: i1):
      llhd.wait yield (%v, %valid : i32, i1), (%a, %b, %sel : i32, i32, i1), ^wakeup(%a : i32)
    ^wakeup(%prev_val: i32):
      cf.cond_br %sel, ^bb1(%prev_val, %true : i32, i1), ^bb1(%b, %true : i32, i1)
    }

    %sig = llhd.sig %c0_i32 : i32
    llhd.drv %sig, %p#0 after %time if %p#1 : i32
    %r = llhd.prb %sig : i32
    hw.output %r : i32
  }
}

// CHECK-LABEL: hw.module @CombWithDestArgs
// CHECK-NOT: llhd.process
// CHECK-NOT: llhd.wait
// CHECK-NOT: cf.br
// CHECK-NOT: cf.cond_br
// CHECK-NOT: llhd.sig
// CHECK-NOT: llhd.drv
// CHECK-NOT: llhd.prb
// CHECK: hw.output

// FLAT-LABEL: hw.module @CombWithDestArgs
// FLAT-NOT: llhd.process
// FLAT-NOT: llhd.wait
// FLAT-NOT: cf.br
// FLAT-NOT: cf.cond_br
// FLAT: hw.output
