// HierCyclic: cyclic module instantiation — module A instantiates B, B instantiates A.
// Must be rejected by M3 cycle detection with diagnostic + non-zero exit.
module {
  hw.module @CycA(in %x : i8, out y : i8) {
    %r = hw.instance "b0" @CycB(x: %x: i8) -> (y: i8)
    hw.output %r : i8
  }
  hw.module @CycB(in %x : i8, out y : i8) {
    %r = hw.instance "a0" @CycA(x: %x: i8) -> (y: i8)
    hw.output %r : i8
  }
}
