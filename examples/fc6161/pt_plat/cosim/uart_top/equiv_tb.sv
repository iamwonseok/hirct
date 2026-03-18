// Lock-step RTL vs CModel equivalence testbench
`timescale 1ns / 10ps

module equiv_tb;

  reg         UART_PCLK;
  reg         UART_PRESETn;
  reg         UART_CLK;
  reg         UART_RESETn;
  reg         PSEL;
  reg         PENABLE;
  reg         PWRITE;
  reg  [23:0] PADDR;
  reg  [31:0] PWDATA;
  reg         UART0_RXD;
  reg         UART1_RXD;
  reg         SCAN_MODE;

  wire [31:0] rtl_PRDATA;
  wire        rtl_UART0_INTR, rtl_UART1_INTR;
  wire        rtl_UART0_TXD,  rtl_UART1_TXD;

  wire [31:0] dpi_PRDATA;
  wire        dpi_UART0_INTR, dpi_UART1_INTR;
  wire        dpi_UART0_TXD,  dpi_UART1_TXD;

  uart_top u_rtl (
    .UART_PCLK   (UART_PCLK),
    .UART_PRESETn (UART_PRESETn),
    .UART_CLK     (UART_CLK),
    .UART_RESETn  (UART_RESETn),
    .PSEL         (PSEL),
    .PENABLE      (PENABLE),
    .PWRITE       (PWRITE),
    .PADDR        (PADDR),
    .PWDATA       (PWDATA),
    .PRDATA       (rtl_PRDATA),
    .UART0_INTR   (rtl_UART0_INTR),
    .UART1_INTR   (rtl_UART1_INTR),
    .UART0_TXD    (rtl_UART0_TXD),
    .UART0_RXD    (UART0_RXD),
    .UART1_TXD    (rtl_UART1_TXD),
    .UART1_RXD    (UART1_RXD),
    .SCAN_MODE    (SCAN_MODE)
  );

  uart_top_dpi_wrapper u_dpi (
    .UART_PCLK   (UART_PCLK),
    .UART_PRESETn (UART_PRESETn),
    .UART_CLK     (UART_CLK),
    .UART_RESETn  (UART_RESETn),
    .PSEL         (PSEL),
    .PENABLE      (PENABLE),
    .PWRITE       (PWRITE),
    .PADDR        (PADDR),
    .PWDATA       (PWDATA),
    .PRDATA       (dpi_PRDATA),
    .UART0_INTR   (dpi_UART0_INTR),
    .UART1_INTR   (dpi_UART1_INTR),
    .UART0_TXD    (dpi_UART0_TXD),
    .UART0_RXD    (UART0_RXD),
    .UART1_TXD    (dpi_UART1_TXD),
    .UART1_RXD    (UART1_RXD),
    .SCAN_MODE    (SCAN_MODE)
  );

  initial begin
    UART_CLK = 0;
    forever #5 UART_CLK = ~UART_CLK;
  end

  initial begin
    UART_PCLK = 0;
    forever #5 UART_PCLK = ~UART_PCLK;
  end

  string  fsdb_dummy;
  integer mismatch_cnt = 0;
  integer check_cnt    = 0;

  task automatic compare(input string label);
    check_cnt = check_cnt + 1;
    if (rtl_PRDATA !== dpi_PRDATA) begin
      $display("[MISMATCH] %s PRDATA: RTL=0x%08h CModel=0x%08h",
               label, rtl_PRDATA, dpi_PRDATA);
      mismatch_cnt = mismatch_cnt + 1;
    end
    if (rtl_UART0_INTR !== dpi_UART0_INTR) begin
      $display("[MISMATCH] %s UART0_INTR: RTL=%0b CModel=%0b",
               label, rtl_UART0_INTR, dpi_UART0_INTR);
      mismatch_cnt = mismatch_cnt + 1;
    end
    if (rtl_UART1_INTR !== dpi_UART1_INTR) begin
      $display("[MISMATCH] %s UART1_INTR: RTL=%0b CModel=%0b",
               label, rtl_UART1_INTR, dpi_UART1_INTR);
      mismatch_cnt = mismatch_cnt + 1;
    end
    if (rtl_UART0_TXD !== dpi_UART0_TXD) begin
      $display("[MISMATCH] %s UART0_TXD: RTL=%0b CModel=%0b",
               label, rtl_UART0_TXD, dpi_UART0_TXD);
      mismatch_cnt = mismatch_cnt + 1;
    end
    if (rtl_UART1_TXD !== dpi_UART1_TXD) begin
      $display("[MISMATCH] %s UART1_TXD: RTL=%0b CModel=%0b",
               label, rtl_UART1_TXD, dpi_UART1_TXD);
      mismatch_cnt = mismatch_cnt + 1;
    end
  endtask

  task automatic apb_write(input logic [5:0] addr, input logic [31:0] data);
    @(posedge UART_CLK);
    PADDR   <= addr;
    PWDATA  <= data;
    PWRITE  <= 1'b1;
    PSEL    <= 1'b1;
    @(posedge UART_CLK);
    PENABLE <= 1'b1;
    @(posedge UART_CLK);
    PSEL    <= 1'b0;
    PENABLE <= 1'b0;
  endtask

  task automatic apb_read_compare(input logic [5:0] addr, input string label);
    @(posedge UART_CLK);
    PADDR   <= addr;
    PWRITE  <= 1'b0;
    PSEL    <= 1'b1;
    @(posedge UART_CLK);
    PENABLE <= 1'b1;
    @(posedge UART_CLK);
    compare(label);
    PSEL    <= 1'b0;
    PENABLE <= 1'b0;
  endtask

  task automatic reset_dut();
    PSEL       = 0;
    PENABLE    = 0;
    PWRITE     = 0;
    PADDR      = 0;
    PWDATA     = 0;
    UART0_RXD  = 1;
    UART1_RXD  = 1;
    SCAN_MODE  = 0;
    UART_PRESETn = 0;
    UART_RESETn  = 0;
    repeat (10) @(posedge UART_CLK);
    UART_PRESETn = 1;
    UART_RESETn  = 1;
    repeat (10) @(posedge UART_CLK);
  endtask

  initial begin
`ifdef VCS
    if ($value$plusargs("fsdbfile=%s", fsdb_dummy)) begin
      $fsdbDumpfile("equiv_uart_top.fsdb");
      $fsdbDumpvars(0, equiv_tb);
    end
`endif
    $display("=== uart_top RTL vs CModel Equivalence Test ===");
    reset_dut();

    $display("[TEST] Post-reset read");
    apb_read_compare(6'h00, "reset_rd_0x00");
    apb_read_compare(6'h01, "reset_rd_0x01");
    apb_read_compare(6'h02, "reset_rd_0x02");
    apb_read_compare(6'h05, "reset_rd_0x05");

    $display("[TEST] Write-then-read");
    apb_write(6'h00, 32'hDEADBEEF);
    apb_read_compare(6'h00, "wr_rd_0x00");
    apb_write(6'h01, 32'h12345678);
    apb_read_compare(6'h01, "wr_rd_0x01");
    apb_write(6'h04, 32'hCAFEBABE);
    apb_read_compare(6'h04, "wr_rd_0x04");

    $display("[TEST] Sequential APB burst");
    apb_write(6'h00, 32'hA5A5A5A5);
    apb_write(6'h01, 32'h5A5A5A5A);
    apb_read_compare(6'h00, "burst_rd_0x00");
    apb_read_compare(6'h01, "burst_rd_0x01");

    $display("[TEST] LCR write-read (word addr 0x0C = reg 3)");
    apb_write(6'h0C, 32'h00000003);
    apb_read_compare(6'h0C, "wr_rd_LCR");

    $display("[TEST] MCR write-read (word addr 0x10 = reg 4)");
    apb_write(6'h10, 32'h00000013);
    apb_read_compare(6'h10, "wr_rd_MCR");

    $display("[TEST] SCR write-read (word addr 0x1C = reg 7)");
    apb_write(6'h1C, 32'hABCDEF01);
    apb_read_compare(6'h1C, "wr_rd_SCR");

    $display("[TEST] DLL/DLH via DLAB (LCR[7]=1)");
    apb_write(6'h0C, 32'h00000083);
    apb_write(6'h00, 32'h00000036);
    apb_write(6'h04, 32'h00000000);
    apb_read_compare(6'h00, "wr_rd_DLL");
    apb_read_compare(6'h04, "wr_rd_DLH");
    apb_write(6'h0C, 32'h00000003);

    $display("[TEST] TX send byte — baud=54 (DLL=0x36), 8N1");
    apb_write(6'h08, 32'h00000007);
    apb_write(6'h00, 32'h00000055);
    repeat (200) @(posedge UART_CLK);
    compare("tx_mid_200");
    repeat (200) @(posedge UART_CLK);
    compare("tx_mid_400");
    repeat (600) @(posedge UART_CLK);
    compare("tx_done_1000");
    apb_read_compare(6'h14, "LSR_after_tx");

    $display("[TEST] Sustained TXD/RXD idle");
    repeat (100) @(posedge UART_CLK);
    compare("idle_final");

    // ----------------------------------------------------------------
    // Phase A: TX Independent — RTL TXD vs CModel TXD with same APB
    // ----------------------------------------------------------------
    $display("[TEST] Phase A: TX independent comparison");
    apb_write(6'h0C, 32'h00000083);  // LCR: DLAB=1
    apb_write(6'h00, 32'h00000001);  // DLL=1 (fastest baud)
    apb_write(6'h04, 32'h00000000);  // DLH=0
    apb_write(6'h0C, 32'h00000003);  // LCR: 8N1, DLAB=0
    apb_write(6'h08, 32'h00000000);  // FCR: FIFO disabled
    repeat (50) @(posedge UART_CLK);
    apb_write(6'h00, 32'h00000048);  // THR = 'H'
    repeat (200) @(posedge UART_CLK);
    compare("phase_a_tx_200");
    apb_read_compare(6'h14, "phase_a_LSR");

    // ----------------------------------------------------------------
    // Phase B: RX Independent — inject serial frame to shared RXD
    // ----------------------------------------------------------------
    $display("[TEST] Phase B: RX independent comparison");
    apb_write(6'h0C, 32'h00000083);
    apb_write(6'h00, 32'h00000001);
    apb_write(6'h04, 32'h00000000);
    apb_write(6'h0C, 32'h00000003);
    apb_write(6'h08, 32'h00000000);
    repeat (50) @(posedge UART_CLK);
    // 0x41 = 8'b01000001, LSB-first: 1,0,0,0,0,0,1,0
    UART0_RXD = 1; repeat (16) @(posedge UART_CLK);  // idle
    UART0_RXD = 0; repeat (16) @(posedge UART_CLK);  // start bit
    UART0_RXD = 1; repeat (16) @(posedge UART_CLK);  // bit 0
    UART0_RXD = 0; repeat (16) @(posedge UART_CLK);  // bit 1
    UART0_RXD = 0; repeat (16) @(posedge UART_CLK);  // bit 2
    UART0_RXD = 0; repeat (16) @(posedge UART_CLK);  // bit 3
    UART0_RXD = 0; repeat (16) @(posedge UART_CLK);  // bit 4
    UART0_RXD = 0; repeat (16) @(posedge UART_CLK);  // bit 5
    UART0_RXD = 1; repeat (16) @(posedge UART_CLK);  // bit 6
    UART0_RXD = 0; repeat (16) @(posedge UART_CLK);  // bit 7
    UART0_RXD = 1; repeat (16) @(posedge UART_CLK);  // stop bit
    repeat (100) @(posedge UART_CLK);
    apb_read_compare(6'h14, "phase_b_LSR");
    apb_read_compare(6'h00, "phase_b_RBR");

    // ----------------------------------------------------------------
    // Phase C: Pin Loopback — RTL TXD → both instances' RXD
    // ----------------------------------------------------------------
    $display("[TEST] Phase C: Pin loopback comparison");
    apb_write(6'h0C, 32'h00000083);
    apb_write(6'h00, 32'h00000001);
    apb_write(6'h04, 32'h00000000);
    apb_write(6'h0C, 32'h00000003);
    apb_write(6'h08, 32'h00000000);
    repeat (50) @(posedge UART_CLK);
    apb_write(6'h00, 32'h00000048);  // THR = 'H'
    repeat (500) @(posedge UART_CLK) begin
      UART0_RXD = rtl_UART0_TXD;
    end
    compare("phase_c_500");
    repeat (100) @(posedge UART_CLK);
    apb_read_compare(6'h14, "phase_c_LSR");
    apb_read_compare(6'h00, "phase_c_RBR");

    $display("=== Results: %0d checks, %0d mismatches ===", check_cnt, mismatch_cnt);
    if (mismatch_cnt > 0)
      $display("FAIL");
    else
      $display("PASS");
    $finish;
  end

endmodule
