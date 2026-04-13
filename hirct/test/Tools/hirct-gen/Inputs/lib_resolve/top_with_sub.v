module top_with_sub (
    input  wire clk,
    input  wire rst,
    input  wire d,
    output wire q
);
  sub_mod u_sub (
    .clk(clk),
    .rst(rst),
    .d(d),
    .q(q)
  );
endmodule
