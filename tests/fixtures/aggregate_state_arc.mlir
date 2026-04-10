module {
  arc.define @AggArc(%arg0: !hw.array<4xi8>) -> !hw.array<4xi8> {
    arc.output %arg0 : !hw.array<4xi8>
  }
  hw.module @AggMod(in %clock : !seq.clock, in %d : !hw.array<4xi8>,
                     out q : !hw.array<4xi8>) {
    %q = arc.state @AggArc(%d) clock %clock latency 1 {names = ["arr_reg"]} : (!hw.array<4xi8>) -> !hw.array<4xi8>
    hw.output %q : !hw.array<4xi8>
  }
}
