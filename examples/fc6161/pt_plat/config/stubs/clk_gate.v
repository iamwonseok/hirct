// STUB — foundry ICG cell placeholder for HIRCT parsing
// NOTE: sim_lib/clk_gate.v has different ports (CLK_I, CKE_G, CLK_O, CLK_G_O).
//       This stub matches the foundry cell port signature used by clk_rst/ and EICG modules.
`timescale 1ns/10ps
module clk_gate(output oclk, input iclk, en, te);
  reg en_latched;
  always @(*) begin  // verilog_lint: waive always-comb
    if (!iclk)
      en_latched = en | te;
  end
  assign oclk = iclk & en_latched;
endmodule
