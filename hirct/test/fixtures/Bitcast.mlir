module {
  hw.module @Bitcast(in %a : i32, out y : i32) {
    %0 = hw.bitcast %a : (i32) -> i32
    hw.output %0 : i32
  }
}
