// Fixture: two-level forward reference chain.
// %0 uses %3, %1 uses %0, %3 is defined later.
// Requires two deferred-processing iterations to resolve.
module {
  hw.module @ForwardRefChain(in %clk : i1, in %rst_n : i1, in %a : i1, in %b : i1, out y : i1) {
    %true = hw.constant true
    %false = hw.constant false
    // First forward ref: %0 uses %3 (defined much later)
    %0 = comb.and %a, %3 : i1
    // %1 depends on %0 (also deferred)
    %1 = comb.or %0, %b : i1
    %clk_s = seq.to_clock %clk
    %rst_inv = comb.xor %rst_n, %true : i1
    %reg = seq.firreg %1 clock %clk_s reset async %rst_inv, %false : i1
    // %2 uses %reg (already in val from pre-pass)
    %2 = comb.xor %reg, %a : i1
    // %3 is defined here — after %0 and %1 used it
    %3 = comb.and %b, %2 : i1
    hw.output %reg : i1
  }
}
