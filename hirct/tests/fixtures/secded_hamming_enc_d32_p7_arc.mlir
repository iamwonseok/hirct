module {
  arc.define @secded_hamming_enc_d32_p7_arc(%arg0: i32) -> i7 {
    %0 = comb.extract %arg0 from 31 : (i32) -> i1
    %1 = comb.extract %arg0 from 28 : (i32) -> i1
    %2 = comb.extract %arg0 from 26 : (i32) -> i1
    %3 = comb.extract %arg0 from 25 : (i32) -> i1
    %4 = comb.extract %arg0 from 22 : (i32) -> i1
    %5 = comb.extract %arg0 from 20 : (i32) -> i1
    %6 = comb.extract %arg0 from 19 : (i32) -> i1
    %7 = comb.extract %arg0 from 17 : (i32) -> i1
    %8 = comb.extract %arg0 from 16 : (i32) -> i1
    %9 = comb.extract %arg0 from 15 : (i32) -> i1
    %10 = comb.extract %arg0 from 10 : (i32) -> i1
    %11 = comb.extract %arg0 from 6 : (i32) -> i1
    %12 = comb.extract %arg0 from 3 : (i32) -> i1
    %13 = comb.extract %arg0 from 1 : (i32) -> i1
    %14 = comb.extract %arg0 from 0 : (i32) -> i1
    %15 = comb.xor %14, %13 : i1
    %16 = comb.xor %15, %12, %11, %10, %9, %8, %7, %6, %5, %4, %3, %2, %1, %0 : i1
    %17 = comb.extract %arg0 from 29 : (i32) -> i1
    %18 = comb.extract %arg0 from 27 : (i32) -> i1
    %19 = comb.extract %arg0 from 23 : (i32) -> i1
    %20 = comb.extract %arg0 from 21 : (i32) -> i1
    %21 = comb.extract %arg0 from 18 : (i32) -> i1
    %22 = comb.extract %arg0 from 11 : (i32) -> i1
    %23 = comb.extract %arg0 from 7 : (i32) -> i1
    %24 = comb.extract %arg0 from 4 : (i32) -> i1
    %25 = comb.extract %arg0 from 2 : (i32) -> i1
    %26 = comb.xor %14, %25, %24, %23, %22, %9, %8, %21, %6, %20, %19, %3, %18, %17 : i1
    %27 = comb.extract %arg0 from 30 : (i32) -> i1
    %28 = comb.extract %arg0 from 24 : (i32) -> i1
    %29 = comb.extract %arg0 from 12 : (i32) -> i1
    %30 = comb.extract %arg0 from 8 : (i32) -> i1
    %31 = comb.extract %arg0 from 5 : (i32) -> i1
    %32 = comb.xor %13, %25, %31, %30, %29, %9, %7, %21, %5, %20, %28, %2, %18, %27 : i1
    %33 = comb.extract %arg0 from 13 : (i32) -> i1
    %34 = comb.extract %arg0 from 9 : (i32) -> i1
    %35 = comb.xor %12, %24, %31, %34, %33, %8, %7, %21, %4, %19, %28, %1, %17, %27 : i1
    %36 = comb.extract %arg0 from 14 : (i32) -> i1
    %37 = comb.xor %11, %23, %30, %34, %36, %6, %5, %20, %4, %19, %28, %0 : i1
    %38 = comb.xor %10, %22, %29, %33, %36, %3, %2, %18, %1, %17, %27, %0 : i1
    %39 = comb.xor %15, %25, %12, %24, %31, %11, %23, %30, %34, %10, %22, %29, %33, %36 : i1
    %40 = comb.concat %39, %38, %37, %35, %32, %26, %16 : i1, i1, i1, i1, i1, i1, i1
    arc.output %40 : i7
  }
  hw.module @secded_hamming_enc_d32_p7(in %d : i32, out p : i7) {
    %0 = arc.call @secded_hamming_enc_d32_p7_arc(%d) : (i32) -> i7
    hw.output %0 : i7
  }
}

