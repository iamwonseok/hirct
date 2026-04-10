module {
  arc.define @simple_mux_arc(%arg0: i4, %arg1: i4, %arg2: i512) -> i32 {
    %0 = comb.sub %arg0, %arg1 : i4
    %1 = comb.extract %arg2 from 480 : (i512) -> i32
    %2 = comb.extract %arg2 from 448 : (i512) -> i32
    %3 = comb.extract %arg2 from 416 : (i512) -> i32
    %4 = comb.extract %arg2 from 384 : (i512) -> i32
    %5 = comb.extract %arg2 from 352 : (i512) -> i32
    %6 = comb.extract %arg2 from 320 : (i512) -> i32
    %7 = comb.extract %arg2 from 288 : (i512) -> i32
    %8 = comb.extract %arg2 from 256 : (i512) -> i32
    %9 = comb.extract %arg2 from 224 : (i512) -> i32
    %10 = comb.extract %arg2 from 192 : (i512) -> i32
    %11 = comb.extract %arg2 from 160 : (i512) -> i32
    %12 = comb.extract %arg2 from 128 : (i512) -> i32
    %13 = comb.extract %arg2 from 96 : (i512) -> i32
    %14 = comb.extract %arg2 from 64 : (i512) -> i32
    %15 = comb.extract %arg2 from 32 : (i512) -> i32
    %16 = comb.extract %arg2 from 0 : (i512) -> i32
    %17 = hw.array_create %16, %15, %14, %13, %12, %11, %10, %9, %8, %7, %6, %5, %4, %3, %2, %1 : i32
    %18 = hw.array_get %17[%0] : !hw.array<16xi32>, i4
    arc.output %18 : i32
  }
  hw.module @simple_mux(in %SEL_I : i4, in %DIN_I : i512, out DOUT_O : i32) {
    %c-1_i4 = hw.constant -1 : i4
    %0 = arc.call @simple_mux_arc(%c-1_i4, %SEL_I, %DIN_I) : (i4, i4, i512) -> i32
    hw.output %0 : i32
  }
}

