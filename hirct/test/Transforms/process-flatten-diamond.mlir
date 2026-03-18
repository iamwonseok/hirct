// RUN: %hirct-gen --dump-ir %s 2>&1 | %FileCheck %s
// RUN: %hirct-gen --dump-ir --run-pass process-flatten %s 2>&1 | %FileCheck %s --check-prefix=FLAT

// Verify that nested diamond CFG patterns (multiple levels of cond_br that
// reconverge) are flattened by the topological-order algorithm without
// exponential blowup.

module {
  hw.module @DiamondNested(in %sel0 : i1, in %sel1 : i1, in %a : i32,
                            in %b : i32, in %c : i32, in %d : i32,
                            out result : i32) {
    %c0_i32 = hw.constant 0 : i32
    %false = hw.constant false
    %true = hw.constant true
    %time = llhd.constant_time <0ns, 0d, 1e>

    // Nested diamond: outer sel0 → inner sel1 on each branch → 4 leaf values
    //   wakeup → cond_br sel0
    //     true  → cond_br sel1 → [%a, %b] → merge_true
    //     false → cond_br sel1 → [%c, %d] → merge_false
    //   merge_true, merge_false → final_merge → wait_block
    %p:2 = llhd.process -> i32, i1 {
      cf.br ^bb1(%c0_i32, %false : i32, i1)
    ^bb1(%v: i32, %valid: i1):
      llhd.wait yield (%v, %valid : i32, i1), (%sel0, %sel1, %a, %b, %c, %d : i1, i1, i32, i32, i32, i32), ^wakeup
    ^wakeup:
      cf.cond_br %sel0, ^true_path, ^false_path
    ^true_path:
      cf.cond_br %sel1, ^leaf_a, ^leaf_b
    ^leaf_a:
      cf.br ^bb1(%a, %true : i32, i1)
    ^leaf_b:
      cf.br ^bb1(%b, %true : i32, i1)
    ^false_path:
      cf.cond_br %sel1, ^leaf_c, ^leaf_d
    ^leaf_c:
      cf.br ^bb1(%c, %true : i32, i1)
    ^leaf_d:
      cf.br ^bb1(%d, %true : i32, i1)
    }

    %sig = llhd.sig %c0_i32 : i32
    llhd.drv %sig, %p#0 after %time if %p#1 : i32
    %r = llhd.prb %sig : i32
    hw.output %r : i32
  }
}

// CHECK-LABEL: hw.module @DiamondNested
// CHECK-NOT: llhd.process
// CHECK-NOT: llhd.wait
// CHECK-NOT: cf.br
// CHECK-NOT: cf.cond_br
// CHECK-NOT: llhd.sig
// CHECK-NOT: llhd.drv
// CHECK-NOT: llhd.prb
// CHECK: comb.mux
// CHECK: comb.mux
// CHECK: hw.output

// FLAT-LABEL: hw.module @DiamondNested
// FLAT-NOT: llhd.process
// FLAT-NOT: llhd.wait
// FLAT-NOT: cf.br
// FLAT-NOT: cf.cond_br
// FLAT: comb.mux
// FLAT: comb.mux
// FLAT: hw.output
