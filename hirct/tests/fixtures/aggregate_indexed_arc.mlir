// Indexed-update aggregate: arc body reads the old array, updates one element
// via hw.array_get + hw.array_create pattern with index selection.
// Pattern: old aggregate -> index select -> inject at index -> indexed_update
module {
  arc.define @IdxArc(%old: !hw.array<4xi8>, %idx: i2, %val: i8) -> !hw.array<4xi8> {
    %c0 = hw.constant 0 : i2
    %c1 = hw.constant 1 : i2
    %c2 = hw.constant 2 : i2
    %c3 = hw.constant 3 : i2
    %e0 = hw.array_get %old[%c0] : !hw.array<4xi8>, i2
    %e1 = hw.array_get %old[%c1] : !hw.array<4xi8>, i2
    %e2 = hw.array_get %old[%c2] : !hw.array<4xi8>, i2
    %e3 = hw.array_get %old[%c3] : !hw.array<4xi8>, i2
    %sel0 = comb.icmp eq %idx, %c0 : i2
    %sel1 = comb.icmp eq %idx, %c1 : i2
    %sel2 = comb.icmp eq %idx, %c2 : i2
    %sel3 = comb.icmp eq %idx, %c3 : i2
    %n0 = comb.mux %sel0, %val, %e0 : i8
    %n1 = comb.mux %sel1, %val, %e1 : i8
    %n2 = comb.mux %sel2, %val, %e2 : i8
    %n3 = comb.mux %sel3, %val, %e3 : i8
    %result = hw.array_create %n3, %n2, %n1, %n0 : i8
    arc.output %result : !hw.array<4xi8>
  }
  hw.module @IdxMod(in %clock : !seq.clock, in %idx : i2, in %val : i8,
                    out q : !hw.array<4xi8>) {
    %q = arc.state @IdxArc(%q, %idx, %val) clock %clock latency 1 {names = ["idx_reg"]} : (!hw.array<4xi8>, i2, i8) -> !hw.array<4xi8>
    hw.output %q : !hw.array<4xi8>
  }
}
