// RUN: %hirct-gen --dump-ir %s 2>&1 | %FileCheck %s

// This regression covers array-valued signals that preserve prior contents
// through a probe-fed array_inject update. Falling back to the init value on
// a disabled drive drops the held contents and breaks RAM-like behavior.

module {
  hw.module @ArraySignalHold(in %enable : i1, in %index : i2, in %data : i8,
                             out out : i8) {
    %time = llhd.constant_time <0ns, 0d, 1e>
    %zero = hw.aggregate_constant [0 : i8, 0 : i8, 0 : i8, 0 : i8] : !hw.array<4xi8>
    %sig = llhd.sig %zero : !hw.array<4xi8>
    %cur = llhd.prb %sig : !hw.array<4xi8>
    %next = hw.array_inject %cur[%index], %data : !hw.array<4xi8>, i2
    llhd.drv %sig, %next after %time if %enable : !hw.array<4xi8>
    %elt = hw.array_get %cur[%index] : !hw.array<4xi8>, i2
    hw.output %elt : i8
  }
}

// CHECK-LABEL: hw.module @ArraySignalHold
// CHECK: %[[NEXT:.+]] = hw.array_inject %[[STATE:.+]]{{\[}}%index{{\]}}, %data : !hw.array<4xi8>, i2
// CHECK: %[[STATE]] = comb.mux %enable, %[[NEXT]], %[[STATE]] : !hw.array<4xi8>
// CHECK: hw.output
