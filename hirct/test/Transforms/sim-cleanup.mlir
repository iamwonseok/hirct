// RUN: %hirct-gen --dump-ir %s 2>&1 | %FileCheck %s

// Verify that HirctSimCleanup removes no-result processes with llhd.halt
// while preserving result-producing processes.

module {
  hw.module @SimCleanupTest(in %clk : i1, in %data : i32) {
    // TC1: no-result process with halt — should be removed
    llhd.process {
      llhd.halt
    }

    // TC2: result-producing process with halt — should be preserved
    %res = llhd.process -> i32 {
      llhd.halt %data : i32
    }
  }
}

// CHECK-LABEL: hw.module @SimCleanupTest
// CHECK-NOT:     llhd.process {
// CHECK:         llhd.process -> i32
// CHECK:         llhd.halt %data : i32
