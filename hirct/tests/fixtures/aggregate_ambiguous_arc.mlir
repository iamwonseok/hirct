// Ambiguous aggregate: arc body mixes patterns -- some elements come from old
// array with transform, one element is purely external (not derived from old),
// making classification neither indexed nor elementwise.
// Pattern: mixed -> AGGREGATE_UPDATE_AMBIGUOUS
module {
  arc.define @AmbigArc(%old: !hw.array<4xi8>, %ext: i8, %sel: i1) -> !hw.array<4xi8> {
    %c0 = hw.constant 0 : i2
    %c1 = hw.constant 1 : i2
    %c2 = hw.constant 2 : i2
    %c3 = hw.constant 3 : i2
    %c1_8 = hw.constant 1 : i8
    %e0 = hw.array_get %old[%c0] : !hw.array<4xi8>, i2
    %e1 = hw.array_get %old[%c1] : !hw.array<4xi8>, i2
    %e3 = hw.array_get %old[%c3] : !hw.array<4xi8>, i2
    %n0 = comb.add %e0, %c1_8 : i8
    %n1 = comb.mux %sel, %ext, %e1 : i8
    %n3 = comb.add %e3, %c1_8 : i8
    %result = hw.array_create %n3, %ext, %n1, %n0 : i8
    arc.output %result : !hw.array<4xi8>
  }
  hw.module @AmbigMod(in %clock : !seq.clock, in %ext : i8, in %sel : i1,
                      out q : !hw.array<4xi8>) {
    %q = arc.state @AmbigArc(%q, %ext, %sel) clock %clock latency 1 {names = ["ambig_reg"]} : (!hw.array<4xi8>, i8, i1) -> !hw.array<4xi8>
    hw.output %q : !hw.array<4xi8>
  }
}
