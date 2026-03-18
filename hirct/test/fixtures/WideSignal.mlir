module {
  hw.module @WideSignal(in %data_a : i64, in %data_b : i64, in %mask : i64, in %sel : i1, in %mode : i2, in %ch0 : i8, in %ch1 : i8, in %ch2 : i8, in %ch3 : i8, in %ch4 : i8, in %ch5 : i8, in %ch6 : i8, in %ch7 : i8, in %ch8 : i8, in %ch9 : i8, in %ch10 : i8, in %ch11 : i8, in %ch12 : i8, in %ch13 : i8, in %ch14 : i8, in %ch15 : i8, in %en : i16, in %cfg_a : i32, in %cfg_b : i32, in %addr : i8, in %threshold : i32, in %limit : i32, in %offset : i16, in %strobe : i1, in %flush : i1, in %bypass : i1, in %debug : i2, out data_out : i64, out status : i16, out valid : i1) {
    %masked = comb.and %data_a, %mask : i64
    %result = comb.mux %sel, %data_a, %masked : i64
    %upper = comb.extract %result from 32 : (i64) -> i32
    %lower = comb.extract %result from 0 : (i64) -> i32
    %sum = comb.add %upper, %lower : i32
    %over = comb.icmp ugt %sum, %threshold : i32
    %active = comb.and %en, %en : i16
    hw.output %result, %active, %over : i64, i16, i1
  }
}
