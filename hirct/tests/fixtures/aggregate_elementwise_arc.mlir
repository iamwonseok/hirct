// Elementwise-update aggregate: arc body extracts every element from old array,
// applies uniform transform to each, then creates new array.
// Pattern: old[i] -> transform(old[i]) for all i -> elementwise_update
module {
  arc.define @EwArc(%old: !hw.array<4xi8>) -> !hw.array<4xi8> {
    %c0 = hw.constant 0 : i2
    %c1 = hw.constant 1 : i2
    %c2 = hw.constant 2 : i2
    %c3 = hw.constant 3 : i2
    %c1_8 = hw.constant 1 : i8
    %e0 = hw.array_get %old[%c0] : !hw.array<4xi8>, i2
    %e1 = hw.array_get %old[%c1] : !hw.array<4xi8>, i2
    %e2 = hw.array_get %old[%c2] : !hw.array<4xi8>, i2
    %e3 = hw.array_get %old[%c3] : !hw.array<4xi8>, i2
    %n0 = comb.add %e0, %c1_8 : i8
    %n1 = comb.add %e1, %c1_8 : i8
    %n2 = comb.add %e2, %c1_8 : i8
    %n3 = comb.add %e3, %c1_8 : i8
    %result = hw.array_create %n3, %n2, %n1, %n0 : i8
    arc.output %result : !hw.array<4xi8>
  }
  hw.module @EwMod(in %clock : !seq.clock,
                   out q : !hw.array<4xi8>) {
    %q = arc.state @EwArc(%q) clock %clock latency 1 {names = ["ew_reg"]} : (!hw.array<4xi8>) -> !hw.array<4xi8>
    hw.output %q : !hw.array<4xi8>
  }
}
