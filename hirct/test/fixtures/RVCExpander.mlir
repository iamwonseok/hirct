module {
  hw.module @Fadu_K2_S5_RVCExpander(in %io_in : i32, out io_out_bits : i32, out io_out_rd : i5, out io_out_rs1 : i5, out io_out_rs2 : i5, out io_rvc : i1) {
    %c115_i15 = hw.constant 115 : i15
    %c31_i15 = hw.constant 31 : i15
    %c18_i8 = hw.constant 18 : i8
    %c19_i8 = hw.constant 19 : i8
    %c231_i15 = hw.constant 231 : i15
    %c103_i15 = hw.constant 103 : i15
    %c18_i10 = hw.constant 18 : i10
    %c19_i11 = hw.constant 19 : i11
    %c1_i7 = hw.constant 1 : i7
    %c111_i12 = hw.constant 111 : i12
    %c1_i8 = hw.constant 1 : i8
    %c0_i30 = hw.constant 0 : i30
    %true = hw.constant true
    %c3_i4 = hw.constant 3 : i4
    %c35_i10 = hw.constant 35 : i10
    %c35_i9 = hw.constant 35 : i9
    %c39_i10 = hw.constant 39 : i10
    %c63_i9 = hw.constant 63 : i9
    %c1_i4 = hw.constant 1 : i4
    %c65_i12 = hw.constant 65 : i12
    %c-2_i5 = hw.constant -2 : i5
    %c-3_i5 = hw.constant -3 : i5
    %c-4_i5 = hw.constant -4 : i5
    %c-5_i5 = hw.constant -5 : i5
    %c-6_i5 = hw.constant -6 : i5
    %c-7_i5 = hw.constant -7 : i5
    %c-8_i5 = hw.constant -8 : i5
    %c-9_i5 = hw.constant -9 : i5
    %c-10_i5 = hw.constant -10 : i5
    %c-11_i5 = hw.constant -11 : i5
    %c-12_i5 = hw.constant -12 : i5
    %c-13_i5 = hw.constant -13 : i5
    %c-14_i5 = hw.constant -14 : i5
    %c-15_i5 = hw.constant -15 : i5
    %c-16_i5 = hw.constant -16 : i5
    %c15_i5 = hw.constant 15 : i5
    %c14_i5 = hw.constant 14 : i5
    %c13_i5 = hw.constant 13 : i5
    %c9_i5 = hw.constant 9 : i5
    %c1_i5 = hw.constant 1 : i5
    %c0_i7 = hw.constant 0 : i7
    %c31_i7 = hw.constant 31 : i7
    %c51_i7 = hw.constant 51 : i7
    %c3_i7 = hw.constant 3 : i7
    %c1_i3 = hw.constant 1 : i3
    %c-29_i7 = hw.constant -29 : i7
    %c-1_i5 = hw.constant -1 : i5
    %c-1_i2 = hw.constant -1 : i2
    %false = hw.constant false
    %c-2_i2 = hw.constant -2 : i2
    %c1_i2 = hw.constant 1 : i2
    %c0_i6 = hw.constant 0 : i6
    %c0_i2 = hw.constant 0 : i2
    %c3_i3 = hw.constant 3 : i3
    %c-2_i3 = hw.constant -2 : i3
    %c2_i3 = hw.constant 2 : i3
    %c-1_i3 = hw.constant -1 : i3
    %c0_i4 = hw.constant 0 : i4
    %c0_i3 = hw.constant 0 : i3
    %c2_i5 = hw.constant 2 : i5
    %c0_i12 = hw.constant 0 : i12
    %c0_i5 = hw.constant 0 : i5
    %c19_i7 = hw.constant 19 : i7
    %c7_i7 = hw.constant 7 : i7
    %c0_i8 = hw.constant 0 : i8
    %0 = comb.extract %io_in from 0 {sv.namehint = "_T"} : (i32) -> i2
    %1 = comb.extract %io_in from 5 {sv.namehint = "_T_2"} : (i32) -> i8
    %2 = comb.icmp ne %1, %c0_i8 {sv.namehint = "_T_3"} : i8
    %3 = comb.mux %2, %c19_i7, %c31_i7 {sv.namehint = "_T_4"} : i7
    %4 = comb.extract %io_in from 7 {sv.namehint = "_T_5"} : (i32) -> i4
    %5 = comb.extract %io_in from 11 {sv.namehint = "_T_6"} : (i32) -> i2
    %6 = comb.extract %io_in from 5 {sv.namehint = "_T_7"} : (i32) -> i1
    %7 = comb.extract %io_in from 6 {sv.namehint = "_T_8"} : (i32) -> i1
    %8 = comb.extract %io_in from 2 {sv.namehint = "_T_13"} : (i32) -> i3
    %9 = comb.concat %c1_i2, %8 : i2, i3
    %10 = comb.extract %io_in from 5 {sv.namehint = "_T_25"} : (i32) -> i2
    %11 = comb.extract %io_in from 10 {sv.namehint = "_T_26"} : (i32) -> i3
    %12 = comb.extract %io_in from 7 {sv.namehint = "_T_29"} : (i32) -> i3
    %13 = comb.concat %c1_i2, %12 : i2, i3
    %14 = comb.extract %io_in from 12 : (i32) -> i1
    %15 = comb.extract %io_in from 10 : (i32) -> i2
    %16 = comb.extract %io_in from 12 {sv.namehint = "_T_203"} : (i32) -> i1
    %17 = comb.replicate %16 {sv.namehint = "_T_205"} : (i1) -> i7
    %18 = comb.extract %io_in from 2 : (i32) -> i5
    %19 = comb.concat %17, %18 {sv.namehint = "_T_207"} : i7, i5
    %20 = comb.extract %io_in from 7 : (i32) -> i5
    %21 = comb.concat %17, %18, %20, %c0_i3, %20, %c19_i7 : i7, i5, i5, i3, i5, i7
    %22 = comb.icmp ne %20, %c0_i5 {sv.namehint = "_T_221"} : i5
    %23 = comb.xor %22, %true : i1
    %24 = comb.concat %17, %18, %20, %c0_i3, %20, %c3_i4, %23, %c-1_i2 : i7, i5, i5, i3, i5, i4, i1, i2
    %25 = comb.concat %17, %18, %c0_i8, %20, %c19_i7 : i7, i5, i8, i5, i7
    %26 = comb.icmp ne %19, %c0_i12 {sv.namehint = "_T_260"} : i12
    %27 = comb.xor %26, %true : i1
    %28 = comb.icmp eq %20, %c0_i5 {sv.namehint = "_T_279"} : i5
    %29 = comb.icmp eq %20, %c2_i5 {sv.namehint = "_T_281"} : i5
    %30 = comb.or %28, %29 {sv.namehint = "_T_282"} : i1
    %31 = comb.mux %26, %c19_i7, %c31_i7 {sv.namehint = "_T_289"} : i7
    %32 = comb.extract %io_in from 3 {sv.namehint = "_T_293"} : (i32) -> i2
    %33 = comb.extract %io_in from 2 {sv.namehint = "_T_295"} : (i32) -> i1
    %34 = comb.replicate %16 : (i1) -> i3
    %35 = comb.concat %32, %6, %33, %7, %c0_i4, %20, %c0_i3, %20, %31 : i2, i1, i1, i1, i4, i5, i3, i5, i7
    %36 = comb.replicate %16 : (i1) -> i12
    %37 = comb.concat %36, %18, %20, %c3_i3, %27, %c-1_i3 : i12, i5, i5, i3, i1, i3
    %38 = comb.mux %30, %35, %37 : i29
    %39 = comb.concat %34, %38 : i3, i29
    %40 = comb.concat %16, %10 {sv.namehint = "_T_353"} : i1, i2
    %41 = comb.icmp eq %40, %c1_i3 {sv.namehint = "_T_354"} : i3
    %42 = comb.concat %41, %c0_i2 : i1, i2
    %43 = hw.array_create %c3_i3, %c2_i3, %c0_i3, %c0_i3, %c-1_i3, %c-2_i3, %42, %42 : i3
    %44 = hw.array_get %43[%40] {sv.namehint = "_T_367"} : !hw.array<8xi3>, i3
    %45 = comb.icmp eq %10, %c0_i2 {sv.namehint = "_T_369"} : i2
    %46 = comb.concat %45, %c0_i30 {sv.namehint = "_T_370"} : i1, i30
    %47 = comb.concat %c1_i8, %8, %c1_i2, %12, %44, %c1_i2, %12, %c3_i3, %16, %c3_i3 {sv.namehint = "_GEN_1"} : i8, i3, i2, i3, i3, i2, i3, i3, i1, i3
    %48 = comb.or %47, %46 : i31
    %49 = comb.extract %io_in from 10 {sv.namehint = "_T_384"} : (i32) -> i2
    %50 = comb.icmp eq %49, %c1_i2 {sv.namehint = "_T_385"} : i2
    %51 = comb.icmp eq %49, %c-2_i2 {sv.namehint = "_T_387"} : i2
    %52 = comb.replicate %16 : (i1) -> i7
    %53 = comb.concat %52, %18, %c1_i2, %12, %c-3_i5 : i7, i5, i2, i3, i5
    %54 = comb.concat %false, %50, %c0_i4, %16, %18, %c1_i2, %12, %c-11_i5 : i1, i1, i4, i1, i5, i2, i3, i5
    %55 = comb.mux %51, %53, %54 : i22
    %56 = comb.concat %55, %12, %c19_i7 : i22, i3, i7
    %57 = comb.icmp eq %49, %c-1_i2 {sv.namehint = "_T_389"} : i2
    %58 = comb.concat %false, %48 : i1, i31
    %59 = comb.mux %57, %58, %56 : i32
    %60 = comb.extract %io_in from 8 {sv.namehint = "_T_402"} : (i32) -> i1
    %61 = comb.extract %io_in from 9 {sv.namehint = "_T_403"} : (i32) -> i2
    %62 = comb.extract %io_in from 7 {sv.namehint = "_T_405"} : (i32) -> i1
    %63 = comb.extract %io_in from 11 {sv.namehint = "_T_407"} : (i32) -> i1
    %64 = comb.extract %io_in from 3 {sv.namehint = "_T_408"} : (i32) -> i3
    %65 = comb.replicate %16 : (i1) -> i9
    %66 = comb.concat %16, %60, %61, %7, %62, %33, %63, %64, %65, %c111_i12 : i1, i1, i2, i1, i1, i1, i1, i3, i9, i12
    %67 = comb.replicate %16 : (i1) -> i4
    %68 = comb.concat %67, %10, %33, %c1_i7, %12, %c0_i3, %49, %32, %16, %c-29_i7 : i4, i2, i1, i7, i3, i3, i2, i2, i1, i7
    %69 = comb.concat %67, %10, %33, %c1_i7, %12, %c1_i3, %49, %32, %16, %c-29_i7 : i4, i2, i1, i7, i3, i3, i2, i2, i1, i7
    %70 = comb.mux %22, %c3_i7, %c31_i7 {sv.namehint = "_T_620"} : i7
    %71 = comb.extract %io_in from 2 {sv.namehint = "_T_650"} : (i32) -> i2
    %72 = comb.extract %io_in from 4 {sv.namehint = "_T_652"} : (i32) -> i3
    %73 = comb.mux %22, %c103_i15, %c31_i15 : i15
    %74 = comb.icmp ne %18, %c0_i5 {sv.namehint = "_T_718"} : i5
    %75 = comb.concat %c0_i8, %20, %c51_i7 : i8, i5, i7
    %76 = comb.concat %20, %73 : i5, i15
    %77 = comb.mux %74, %75, %76 : i20
    %78 = comb.concat %18, %20, %c231_i15 : i5, i5, i15
    %79 = comb.or %18, %c1_i5 : i5
    %80 = comb.concat %79, %20, %c115_i15 : i5, i5, i15
    %81 = comb.mux %22, %78, %80 {sv.namehint = "_T_731"} : i25
    %82 = comb.concat %18, %20, %c0_i3, %20, %c51_i7 : i5, i5, i3, i5, i7
    %83 = comb.mux %74, %82, %81 : i25
    %84 = comb.concat %18, %77 : i5, i20
    %85 = comb.mux %16, %83, %84 : i25
    %86 = comb.concat %c0_i7, %85 : i7, i25
    %87 = comb.concat %c0_i4, %16 : i4, i1
    %88 = comb.mux %74, %20, %87 : i5
    %89 = comb.xor %74, %true : i1
    %90 = comb.or %16, %89 : i1
    %91 = comb.mux %90, %20, %c0_i5 : i5
    %92 = comb.extract %io_in from 7 {sv.namehint = "_T_761"} : (i32) -> i2
    %93 = comb.extract %io_in from 9 : (i32) -> i3
    %94 = comb.extract %io_in from 15 : (i32) -> i5
    %95 = comb.extract %io_in from 20 : (i32) -> i5
    %96 = comb.extract %io_in from 13 {sv.namehint = "_T_842"} : (i32) -> i3
    %97 = comb.concat %0, %96 {sv.namehint = "_T_843"} : i2, i3
    %98 = comb.concat %c0_i4, %10, %11, %c1_i5, %12, %c13_i5, %8, %c7_i7 : i4, i2, i3, i5, i3, i5, i3, i7
    %99 = comb.concat %c0_i2, %4, %5, %6, %7, %c65_i12, %8, %3 : i2, i4, i2, i1, i1, i12, i3, i7
    %100 = comb.concat %c0_i5, %6, %11, %7, %c1_i4, %12, %c9_i5, %8, %c3_i7 : i5, i1, i3, i1, i4, i3, i5, i3, i7
    %101 = comb.concat %c0_i4, %10, %11, %c1_i5, %12, %c13_i5, %8, %c3_i7 : i4, i2, i3, i5, i3, i5, i3, i7
    %102 = comb.concat %c0_i5, %6, %14, %c1_i2, %8, %c1_i2, %12, %c2_i3, %15, %7, %c63_i9 : i5, i1, i1, i2, i3, i2, i3, i3, i2, i1, i9
    %103 = comb.concat %c0_i4, %10, %14, %c1_i2, %8, %c1_i2, %12, %c3_i3, %15, %c39_i10 : i4, i2, i1, i2, i3, i2, i3, i3, i2, i10
    %104 = comb.concat %c0_i5, %6, %14, %c1_i2, %8, %c1_i2, %12, %c2_i3, %15, %7, %c35_i9 : i5, i1, i1, i2, i3, i2, i3, i3, i2, i1, i9
    %105 = comb.concat %c0_i4, %10, %14, %c1_i2, %8, %c1_i2, %12, %c3_i3, %15, %c35_i10 : i4, i2, i1, i2, i3, i2, i3, i3, i2, i10
    %106 = comb.icmp eq %97, %c14_i5 {sv.namehint = "_T_870"} : i5
    %107 = comb.icmp eq %97, %c15_i5 {sv.namehint = "_T_872"} : i5
    %108 = comb.or %107, %106 : i1
    %109 = comb.mux %108, %c0_i5, %9 : i5
    %110 = comb.icmp eq %97, %c-16_i5 {sv.namehint = "_T_874"} : i5
    %111 = comb.concat %c0_i6, %16, %18, %20, %c1_i3, %20, %c19_i7 : i6, i1, i5, i5, i3, i5, i7
    %112 = comb.icmp eq %97, %c-15_i5 {sv.namehint = "_T_876"} : i5
    %113 = comb.concat %c0_i3, %8, %16, %10, %c19_i11, %20, %c7_i7 : i3, i3, i1, i2, i11, i5, i7
    %114 = comb.icmp eq %97, %c-14_i5 {sv.namehint = "_T_878"} : i5
    %115 = comb.concat %c0_i4, %71, %16, %72, %c18_i10, %20, %70 : i4, i2, i1, i3, i10, i5, i7
    %116 = comb.icmp eq %97, %c-13_i5 {sv.namehint = "_T_880"} : i5
    %117 = comb.concat %c0_i3, %8, %16, %10, %c19_i11, %20, %70 : i3, i3, i1, i2, i11, i5, i7
    %118 = comb.icmp eq %97, %c-12_i5 {sv.namehint = "_T_882"} : i5
    %119 = comb.icmp eq %97, %c-11_i5 {sv.namehint = "_T_884"} : i5
    %120 = comb.concat %c0_i3, %12, %14, %18, %c19_i8, %15, %c39_i10 : i3, i3, i1, i5, i8, i2, i10
    %121 = comb.icmp eq %97, %c-10_i5 {sv.namehint = "_T_886"} : i5
    %122 = comb.concat %c0_i4, %92, %14, %18, %c18_i8, %93, %c35_i9 : i4, i2, i1, i5, i8, i3, i9
    %123 = comb.icmp eq %97, %c-9_i5 {sv.namehint = "_T_888"} : i5
    %124 = comb.concat %c0_i3, %12, %14, %18, %c19_i8, %15, %c35_i10 : i3, i3, i1, i5, i8, i2, i10
    %125 = comb.or %123, %121, %119, %118, %116, %114, %112, %110 : i1
    %126 = comb.mux %125, %18, %109 : i5
    %127 = comb.icmp eq %97, %c-8_i5 {sv.namehint = "_T_890"} : i5
    %128 = comb.icmp eq %97, %c-7_i5 {sv.namehint = "_T_892"} : i5
    %129 = comb.icmp eq %97, %c-6_i5 {sv.namehint = "_T_894"} : i5
    %130 = comb.icmp eq %97, %c-5_i5 {sv.namehint = "_T_896"} : i5
    %131 = comb.icmp eq %97, %c-4_i5 {sv.namehint = "_T_898"} : i5
    %132 = comb.icmp eq %97, %c-3_i5 {sv.namehint = "_T_900"} : i5
    %133 = comb.icmp eq %97, %c-2_i5 {sv.namehint = "_T_902"} : i5
    %134 = comb.icmp eq %97, %c-1_i5 {sv.namehint = "_T_904"} : i5
    %135 = hw.array_create %io_in, %io_in, %io_in, %io_in, %io_in, %io_in, %io_in, %io_in, %124, %122, %120, %86, %117, %115, %113, %111, %69, %68, %66, %59, %39, %25, %24, %21, %105, %104, %103, %102, %101, %100, %98, %99 : i32
    %136 = hw.array_get %135[%97] : !hw.array<32xi32>, i5
    %137 = hw.array_create %20, %20, %20, %20, %20, %20, %20, %20, %20, %20, %20, %88, %20, %20, %20, %20, %c0_i5, %13, %c0_i5, %13, %20, %20, %20, %20, %9, %9, %9, %9, %9, %9, %9, %9 : i5
    %138 = hw.array_get %137[%97] : !hw.array<32xi5>, i5
    %139 = hw.array_create %94, %94, %94, %94, %94, %94, %94, %94, %c2_i5, %c2_i5, %c2_i5, %91, %c2_i5, %c2_i5, %c2_i5, %20, %13, %13, %13, %13, %20, %c0_i5, %20, %20, %13, %13, %13, %13, %13, %13, %13, %c2_i5 : i5
    %140 = hw.array_get %139[%97] : !hw.array<32xi5>, i5
    %141 = comb.or %134, %133, %132, %131, %130, %129, %128, %127 : i1
    %142 = comb.mux %141, %95, %126 : i5
    %143 = comb.icmp ne %0, %c-1_i2 : i2
    hw.output %136, %138, %140, %142, %143 : i32, i5, i5, i5, i1
  }
}
