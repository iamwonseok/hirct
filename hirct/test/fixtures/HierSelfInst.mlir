// HierSelfInst: a module that instantiates itself. Must be rejected.
module {
  hw.module @SelfMod(in %x : i8, out y : i8) {
    %r = hw.instance "self0" @SelfMod(x: %x: i8) -> (y: i8)
    hw.output %r : i8
  }
}
