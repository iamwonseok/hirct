// RUN: %hirct-gen --dump-ir %s 2>&1 | %FileCheck %s
// RUN: %hirct-gen --dump-ir --run-pass unroll-process-loops %s 2>&1 | %FileCheck %s --check-prefix=UNROLL

// CHECK-LABEL: hw.module @loop_array
// CHECK-NOT: cf.br
// CHECK-NOT: cf.cond_br
// CHECK-NOT: comb.icmp slt
// CHECK-NOT: llhd.process
// CHECK: hw.array_create %data_in, %data_in, %data_in, %data_in
// CHECK: hw.output
hw.module @loop_array(in %clk : i1, in %data_in : i8,
                       out arr_out : !hw.array<4xi8>) {
  %c0_i32 = hw.constant 0 : i32
  %c1_i32 = hw.constant 1 : i32
  %c4_i32 = hw.constant 4 : i32
  %false = hw.constant false
  %true = hw.constant true
  %zero_arr = hw.bitcast %c0_i32 : (i32) -> !hw.array<4xi8>
  %proc:2 = llhd.process -> !hw.array<4xi8>, i1 {
    cf.br ^wait(%zero_arr, %false : !hw.array<4xi8>, i1)
  ^wait(%arr: !hw.array<4xi8>, %changed: i1):
    llhd.wait yield (%arr, %changed : !hw.array<4xi8>, i1), (%clk : i1), ^wakeup
  ^wakeup:
    cf.br ^loop(%c0_i32, %zero_arr : i32, !hw.array<4xi8>)
  ^loop(%i: i32, %acc: !hw.array<4xi8>):
    %cmp = comb.icmp slt %i, %c4_i32 : i32
    cf.cond_br %cmp, ^body, ^done(%acc : !hw.array<4xi8>)
  ^body:
    %idx = comb.extract %i from 0 : (i32) -> i2
    %new_arr = hw.array_inject %acc[%idx], %data_in : !hw.array<4xi8>, i2
    %next_i = comb.add %i, %c1_i32 : i32
    cf.br ^loop(%next_i, %new_arr : i32, !hw.array<4xi8>)
  ^done(%result_arr: !hw.array<4xi8>):
    cf.br ^wait(%result_arr, %true : !hw.array<4xi8>, i1)
  }
  %result = comb.mux %proc#1, %proc#0, %zero_arr : !hw.array<4xi8>
  hw.output %result : !hw.array<4xi8>
}

// CHECK-LABEL: hw.module @loop_ult
// CHECK-NOT: cf.br
// CHECK-NOT: cf.cond_br
// CHECK-NOT: comb.icmp ult
// CHECK-NOT: llhd.process
// CHECK: comb.add
// CHECK: hw.output
hw.module @loop_ult(in %clk : i1, in %val : i8, out result : i8) {
  %c0_i8 = hw.constant 0 : i8
  %c0_i32 = hw.constant 0 : i32
  %c1_i32 = hw.constant 1 : i32
  %c3_i32 = hw.constant 3 : i32
  %false = hw.constant false
  %true = hw.constant true
  %proc:2 = llhd.process -> i8, i1 {
    cf.br ^wait(%c0_i8, %false : i8, i1)
  ^wait(%r: i8, %changed: i1):
    llhd.wait yield (%r, %changed : i8, i1), (%val : i8), ^wakeup
  ^wakeup:
    cf.br ^loop(%c0_i32, %c0_i8 : i32, i8)
  ^loop(%i: i32, %acc: i8):
    %cmp = comb.icmp ult %i, %c3_i32 : i32
    cf.cond_br %cmp, ^body, ^done(%acc : i8)
  ^body:
    %sum = comb.add %acc, %val : i8
    %next_i = comb.add %i, %c1_i32 : i32
    cf.br ^loop(%next_i, %sum : i32, i8)
  ^done(%final: i8):
    cf.br ^wait(%final, %true : i8, i1)
  }
  %result = comb.mux %proc#1, %proc#0, %c0_i8 : i8
  hw.output %result : i8
}

// CHECK-LABEL: hw.module @loop_bitwise
// CHECK-NOT: cf.br
// CHECK-NOT: cf.cond_br
// CHECK-NOT: comb.icmp slt
// CHECK-NOT: llhd.process
// CHECK: hw.output
hw.module @loop_bitwise(in %input : i4, out result : i4) {
  %c0_i4 = hw.constant 0 : i4
  %c0_i32 = hw.constant 0 : i32
  %c1_i32 = hw.constant 1 : i32
  %c4_i32 = hw.constant 4 : i32
  %false = hw.constant false
  %true = hw.constant true
  %proc:2 = llhd.process -> i4, i1 {
    cf.br ^wait(%c0_i4, %false : i4, i1)
  ^wait(%val: i4, %changed: i1):
    llhd.wait yield (%val, %changed : i4, i1), (%input : i4), ^wakeup
  ^wakeup:
    cf.br ^loop(%c0_i32, %c0_i4 : i32, i4)
  ^loop(%i: i32, %acc: i4):
    %cmp = comb.icmp slt %i, %c4_i32 : i32
    cf.cond_br %cmp, ^body, ^done(%acc : i4)
  ^body:
    %i_i4 = comb.extract %i from 0 : (i32) -> i4
    %shifted = comb.shru %input, %i_i4 : i4
    %bit = comb.extract %shifted from 0 : (i4) -> i1
    %mask_shift = comb.shl %c1_i32, %i : i32
    %mask_i4 = comb.extract %mask_shift from 0 : (i32) -> i4
    %inv_mask = comb.xor bin %mask_i4, %c0_i4 : i4
    %cleared = comb.and %acc, %inv_mask : i4
    %bit_ext = comb.concat %false, %false, %false, %bit : i1, i1, i1, i1
    %bit_shifted = comb.shl %bit_ext, %i_i4 : i4
    %combined = comb.or %cleared, %bit_shifted : i4
    %next_i = comb.add %i, %c1_i32 : i32
    cf.br ^loop(%next_i, %combined : i32, i4)
  ^done(%final: i4):
    cf.br ^wait(%final, %true : i4, i1)
  }
  %result = comb.mux %proc#1, %proc#0, %c0_i4 : i4
  hw.output %result : i4
}

// UNROLL-LABEL: hw.module @loop_array
// UNROLL:       llhd.process
// UNROLL:       llhd.wait
// UNROLL-NOT:   cf.cond_br
// UNROLL:       hw.array_inject
// UNROLL:       hw.array_inject
// UNROLL:       hw.array_inject
// UNROLL:       hw.array_inject
// UNROLL:       hw.output

// UNROLL-LABEL: hw.module @loop_ult
// UNROLL:       llhd.process
// UNROLL:       llhd.wait
// UNROLL-NOT:   cf.cond_br
// UNROLL:       comb.add {{.+}} : i8
// UNROLL:       comb.add {{.+}} : i8
// UNROLL:       comb.add {{.+}} : i8
// UNROLL:       hw.output

// UNROLL-LABEL: hw.module @loop_bitwise
// UNROLL:       llhd.process
// UNROLL:       llhd.wait
// UNROLL-NOT:   cf.cond_br
// UNROLL:       comb.shru
// UNROLL:       comb.shru
// UNROLL:       comb.shru
// UNROLL:       comb.shru
// UNROLL:       hw.output

// CHECK-LABEL: hw.module @loop_reverse_sgt
// CHECK-NOT: cf.br
// CHECK-NOT: cf.cond_br
// CHECK-NOT: comb.icmp sgt
// CHECK: hw.output

// UNROLL-LABEL: hw.module @loop_reverse_sgt
// UNROLL-NOT:   cf.cond_br
// UNROLL:       comb.add {{.+}} : i32
// UNROLL:       comb.add {{.+}} : i32
// UNROLL:       comb.add {{.+}} : i32
// UNROLL:       comb.add {{.+}} : i32
// UNROLL:       hw.output

// Reverse loop: begin=3, end=-1, step=-1 (sgt condition), trip_count=4
// Reproduces i2cm pattern: for(i=3; i>-1; i--)
hw.module @loop_reverse_sgt(in %input : i4, out result : i4) {
  %c3_i32 = hw.constant 3 : i32
  %c-1_i32 = hw.constant -1 : i32
  %c0_i4 = hw.constant 0 : i4
  %0 = llhd.combinational -> i4 {
    cf.br ^bb1(%c3_i32, %c0_i4 : i32, i4)
  ^bb1(%1: i32, %2: i4):
    %3 = comb.icmp sgt %1, %c-1_i32 : i32
    cf.cond_br %3, ^bb2(%2 : i4), ^bb3(%2 : i4)
  ^bb2(%4: i4):
    %5 = comb.add %1, %c-1_i32 : i32
    cf.br ^bb1(%5, %4 : i32, i4)
  ^bb3(%6: i4):
    llhd.yield %6 : i4
  }
  hw.output %0 : i4
}
