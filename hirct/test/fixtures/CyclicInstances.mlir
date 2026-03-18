module {
  hw.module @Producer(in %clk : !seq.clock, in %feedback : i8,
                      out comb_out : i8, out seq_out : i8) {
    %c1 = hw.constant 1 : i8
    %sum = comb.add %feedback, %c1 : i8
    %reg = seq.compreg %sum, %clk : i8
    hw.output %sum, %reg : i8, i8
  }

  hw.module @Consumer(in %clk : !seq.clock, in %data : i8,
                      out result : i8) {
    %reg = seq.compreg %data, %clk : i8
    hw.output %reg : i8
  }

  hw.module @CyclicTop(in %clk : !seq.clock, in %in : i8, out out : i8) {
    %comb_out, %seq_out = hw.instance "prod" @Producer(
        clk: %clk: !seq.clock, feedback: %cons_result: i8)
        -> (comb_out: i8, seq_out: i8)
    %cons_result = hw.instance "cons" @Consumer(
        clk: %clk: !seq.clock, data: %comb_out: i8)
        -> (result: i8)
    hw.output %seq_out : i8
  }
}
