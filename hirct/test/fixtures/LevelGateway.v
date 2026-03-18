// Generated from LevelGateway.mlir via circt-opt --lower-seq-to-sv --export-verilog
// Cleaned: FIRRTL RANDOMIZE_REG_INIT blocks removed
// verilog_lint: waive-start module-filename
module Fadu_K2_S5_LevelGateway(
  input  preset_flops,
         clock,
         reset,
         io_interrupt,
  output io_plic_valid,
  input  io_plic_ready,
         io_plic_complete
);

  reg  inFlight;
  wire _GEN   = io_interrupt & io_plic_ready;
  wire _GEN_0 = ~reset;
  wire _GEN_1 = _GEN_0 & io_plic_complete;
  wire _GEN_2 = _GEN_0 & ~_GEN_1 & _GEN;
  wire _GEN_3 = reset | _GEN_1 | _GEN;

  always @(posedge clock or posedge preset_flops) begin
    if (preset_flops)
      inFlight <= 1'h0;
    else if (_GEN_3)
      inFlight <= _GEN_2;
  end

  assign io_plic_valid = io_interrupt & ~inFlight;
endmodule
