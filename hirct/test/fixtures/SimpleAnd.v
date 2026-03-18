module SimpleAnd (
    input  wire clock,
    input  wire reset,
    input  wire a,
    input  wire b,
    output wire y
);
  reg r;

  always @(posedge clock) begin
    if (reset) begin
      r <= 1'b0;
    end else begin
      r <= a & b;
    end
  end

  assign y = r;
endmodule
