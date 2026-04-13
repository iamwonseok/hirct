module sub_mod (
    input  wire clk,
    input  wire rst,
    input  wire d,
    output wire q
);
  reg r;
  always @(posedge clk) begin
    if (rst)
      r <= 1'b0;
    else
      r <= d;
  end
  assign q = r;
endmodule
