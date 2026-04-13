`ifdef USE_AND
module define_mod (
    input  wire clk,
    input  wire rst,
    input  wire a,
    input  wire b,
    output wire y
);
  reg r;
  always @(posedge clk) begin
    if (rst)
      r <= 1'b0;
    else
      r <= a & b;
  end
  assign y = r;
endmodule
`else
module define_mod (
    input  wire clk,
    input  wire rst,
    input  wire a,
    input  wire b,
    output wire y
);
  reg r;
  always @(posedge clk) begin
    if (rst)
      r <= 1'b0;
    else
      r <= a | b;
  end
  assign y = r;
endmodule
`endif
