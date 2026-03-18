// STUB — S5MC_EICG_wrapper placeholder for HIRCT parsing
// Original is defined alongside S5MC_EICG in rtl/plat/src/s5mc/design/S5MC_EICG.v,
// but -y resolution requires <module_name>.v file naming.
`timescale 1ns/10ps
module S5MC_EICG_wrapper(output out, input en, in);
  reg en_latched;
  always @(*) begin  // verilog_lint: waive always-comb
    if (!in)
      en_latched = en;
  end
  assign out = in & en_latched;
endmodule
