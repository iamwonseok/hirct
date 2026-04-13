module timescale_mod (
    input  wire clk,
    input  wire rst,
    input  wire a,
    output wire y
);
  reg r;
  always @(posedge clk) begin
    if (rst)
      r <= 1'b0;
    else
      r <= a;
  end
  assign y = r;
endmodule
