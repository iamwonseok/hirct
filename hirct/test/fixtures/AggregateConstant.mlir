module {
  hw.module @AggregateConstant(in %idx : i2, out y : i8) {
    %0 = hw.aggregate_constant [10 : i8, 20 : i8, 30 : i8, 40 : i8] : !hw.array<4xi8>
    %1 = hw.array_get %0[%idx] : !hw.array<4xi8>, i2
    hw.output %1 : i8
  }
}
