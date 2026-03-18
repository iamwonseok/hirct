`ifdef SMOKE_DEFINE
module define_mod(input wire clk, input wire [7:0] a, output reg [7:0] y);
  always @(posedge clk) y <= a;
endmodule
`endif
