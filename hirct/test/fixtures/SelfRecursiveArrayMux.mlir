module {
  hw.module @SelfRecursiveArrayMux(in %clk : i1, in %rst : i1, in %enable : i1,
                                   in %index : i2, in %data : i8, out out : i8) {
    %false = hw.constant false
    %zero = hw.aggregate_constant [0 : i8, 0 : i8, 0 : i8, 0 : i8] : !hw.array<4xi8>
    %clock = seq.to_clock %clk
    %reg = seq.compreg %state, %clock reset %rst, %zero : !hw.array<4xi8>
    %en_reg = seq.compreg %enable, %clock reset %rst, %false : i1
    %next = hw.array_inject %reg[%index], %data : !hw.array<4xi8>, i2
    %state = comb.mux %en_reg, %next, %state : !hw.array<4xi8>
    %elt = hw.array_get %state[%index] : !hw.array<4xi8>, i2
    hw.output %elt : i8
  }
}
