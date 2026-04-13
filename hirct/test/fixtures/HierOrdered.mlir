// HierOrdered: Producer → Consumer unidirectional dependency.
// Consumer.data depends on Producer.out, so topo order must be: prod before cons.
// Used by M3 Batch 3 to verify topological ordering in codegen.
module {
  hw.module @Producer(in %a : i8, out out : i8) {
    %c1 = hw.constant 1 : i8
    %r = comb.add %a, %c1 : i8
    hw.output %r : i8
  }

  hw.module @Consumer(in %data : i8, out result : i8) {
    %c2 = hw.constant 2 : i8
    %r = comb.mul %data, %c2 : i8
    hw.output %r : i8
  }

  hw.module @Pipeline(in %x : i8, out out : i8) {
    %prod_out = hw.instance "prod" @Producer(a: %x: i8) -> (out: i8)
    %cons_result = hw.instance "cons" @Consumer(data: %prod_out: i8) -> (result: i8)
    hw.output %cons_result : i8
  }
}
