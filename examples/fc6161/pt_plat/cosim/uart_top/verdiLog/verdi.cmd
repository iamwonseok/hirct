verdiSetActWin -dock widgetDock_<Decl._Tree>
simSetSimulator "-vcssv" -exec \
           "/user/wonseok/project-iamwonseok/llvm-cpp-model/examples/fc6161/pt_plat/cosim/uart_top/simv" \
           -args
debImport "-dbdir" \
          "/user/wonseok/project-iamwonseok/llvm-cpp-model/examples/fc6161/pt_plat/cosim/uart_top/simv.daidir"
debLoadSimResult \
           /user/wonseok/project-iamwonseok/llvm-cpp-model/examples/fc6161/pt_plat/cosim/uart_top/equiv_uart_top.fsdb
wvCreateWindow
verdiWindowResize -win $_Verdi_1 "2007" "25" "1916" "917"
verdiSetActWin -dock widgetDock_MTB_SOURCE_TAB_1
verdiSetActWin -win $_nWave2
wvGetSignalOpen -win $_nWave2
wvGetSignalSetScope -win $_nWave2 "/equiv_tb"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_rtl"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_rtl/PRDATA_MUX"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_dpi"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_rtl/U_UART0"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_rtl"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_dpi"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_rtl"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_dpi"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_rtl"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_dpi"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_rtl"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_dpi"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_rtl"
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_dpi"
wvSetPosition -win $_nWave2 {("G1" 20)}
wvSetPosition -win $_nWave2 {("G1" 20)}
wvAddSignal -win $_nWave2 -clear
wvAddSignal -win $_nWave2 -group {"G1" \
{/equiv_tb/u_dpi/PADDR\[23:0\]} \
{/equiv_tb/u_dpi/PENABLE} \
{/equiv_tb/u_dpi/PRDATA\[31:0\]} \
{/equiv_tb/u_dpi/PSEL} \
{/equiv_tb/u_dpi/PWDATA\[31:0\]} \
{/equiv_tb/u_dpi/PWRITE} \
{/equiv_tb/u_dpi/SCAN_MODE} \
{/equiv_tb/u_dpi/UART0_INTR} \
{/equiv_tb/u_dpi/UART0_RXD} \
{/equiv_tb/u_dpi/UART0_TXD} \
{/equiv_tb/u_dpi/UART1_INTR} \
{/equiv_tb/u_dpi/UART1_RXD} \
{/equiv_tb/u_dpi/UART1_TXD} \
{/equiv_tb/u_dpi/UART_CLK} \
{/equiv_tb/u_dpi/UART_PCLK} \
{/equiv_tb/u_dpi/UART_PRESETn} \
{/equiv_tb/u_dpi/UART_RESETn} \
{/LOGIC_LOW} \
{/LOGIC_HIGH} \
{/BLANK} \
}
wvAddSignal -win $_nWave2 -group {"G2" \
}
wvSelectSignal -win $_nWave2 {( "G1" 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 \
           18 19 20 )} 
wvSetPosition -win $_nWave2 {("G1" 20)}
wvGetSignalSetScope -win $_nWave2 "/equiv_tb/u_rtl"
wvSetPosition -win $_nWave2 {("G1" 44)}
wvSetPosition -win $_nWave2 {("G1" 44)}
wvAddSignal -win $_nWave2 -clear
wvAddSignal -win $_nWave2 -group {"G1" \
{/equiv_tb/u_dpi/PADDR\[23:0\]} \
{/equiv_tb/u_dpi/PENABLE} \
{/equiv_tb/u_dpi/PRDATA\[31:0\]} \
{/equiv_tb/u_dpi/PSEL} \
{/equiv_tb/u_dpi/PWDATA\[31:0\]} \
{/equiv_tb/u_dpi/PWRITE} \
{/equiv_tb/u_dpi/SCAN_MODE} \
{/equiv_tb/u_dpi/UART0_INTR} \
{/equiv_tb/u_dpi/UART0_RXD} \
{/equiv_tb/u_dpi/UART0_TXD} \
{/equiv_tb/u_dpi/UART1_INTR} \
{/equiv_tb/u_dpi/UART1_RXD} \
{/equiv_tb/u_dpi/UART1_TXD} \
{/equiv_tb/u_dpi/UART_CLK} \
{/equiv_tb/u_dpi/UART_PCLK} \
{/equiv_tb/u_dpi/UART_PRESETn} \
{/equiv_tb/u_dpi/UART_RESETn} \
{/LOGIC_LOW} \
{/LOGIC_HIGH} \
{/BLANK} \
{/equiv_tb/u_rtl/PADDR\[23:0\]} \
{/equiv_tb/u_rtl/PENABLE} \
{/equiv_tb/u_rtl/PRDATA\[31:0\]} \
{/equiv_tb/u_rtl/PSEL} \
{/equiv_tb/u_rtl/PWDATA\[31:0\]} \
{/equiv_tb/u_rtl/PWRITE} \
{/equiv_tb/u_rtl/SCAN_MODE} \
{/equiv_tb/u_rtl/UART0_INTR} \
{/equiv_tb/u_rtl/UART0_RXD} \
{/equiv_tb/u_rtl/UART0_TXD} \
{/equiv_tb/u_rtl/UART1_INTR} \
{/equiv_tb/u_rtl/UART1_RXD} \
{/equiv_tb/u_rtl/UART1_TXD} \
{/equiv_tb/u_rtl/UART_CLK} \
{/equiv_tb/u_rtl/UART_PCLK} \
{/equiv_tb/u_rtl/UART_PRESETn} \
{/equiv_tb/u_rtl/UART_RESETn} \
{/equiv_tb/u_rtl/prdata_uart0\[31:0\]} \
{/equiv_tb/u_rtl/prdata_uart1\[31:0\]} \
{/equiv_tb/u_rtl/psel_uart0} \
{/equiv_tb/u_rtl/psel_uart1} \
{/equiv_tb/u_rtl/uart_sel\[1:0\]} \
{/LOGIC_LOW} \
{/LOGIC_HIGH} \
}
wvAddSignal -win $_nWave2 -group {"G2" \
}
wvSelectSignal -win $_nWave2 {( "G1" 21 22 23 24 25 26 27 28 29 30 31 32 33 34 \
           35 36 37 38 39 40 41 42 43 44 )} 
wvSetPosition -win $_nWave2 {("G1" 44)}
wvSetPosition -win $_nWave2 {("G1" 44)}
wvSetPosition -win $_nWave2 {("G1" 44)}
wvAddSignal -win $_nWave2 -clear
wvAddSignal -win $_nWave2 -group {"G1" \
{/equiv_tb/u_dpi/PADDR\[23:0\]} \
{/equiv_tb/u_dpi/PENABLE} \
{/equiv_tb/u_dpi/PRDATA\[31:0\]} \
{/equiv_tb/u_dpi/PSEL} \
{/equiv_tb/u_dpi/PWDATA\[31:0\]} \
{/equiv_tb/u_dpi/PWRITE} \
{/equiv_tb/u_dpi/SCAN_MODE} \
{/equiv_tb/u_dpi/UART0_INTR} \
{/equiv_tb/u_dpi/UART0_RXD} \
{/equiv_tb/u_dpi/UART0_TXD} \
{/equiv_tb/u_dpi/UART1_INTR} \
{/equiv_tb/u_dpi/UART1_RXD} \
{/equiv_tb/u_dpi/UART1_TXD} \
{/equiv_tb/u_dpi/UART_CLK} \
{/equiv_tb/u_dpi/UART_PCLK} \
{/equiv_tb/u_dpi/UART_PRESETn} \
{/equiv_tb/u_dpi/UART_RESETn} \
{/LOGIC_LOW} \
{/LOGIC_HIGH} \
{/BLANK} \
{/equiv_tb/u_rtl/PADDR\[23:0\]} \
{/equiv_tb/u_rtl/PENABLE} \
{/equiv_tb/u_rtl/PRDATA\[31:0\]} \
{/equiv_tb/u_rtl/PSEL} \
{/equiv_tb/u_rtl/PWDATA\[31:0\]} \
{/equiv_tb/u_rtl/PWRITE} \
{/equiv_tb/u_rtl/SCAN_MODE} \
{/equiv_tb/u_rtl/UART0_INTR} \
{/equiv_tb/u_rtl/UART0_RXD} \
{/equiv_tb/u_rtl/UART0_TXD} \
{/equiv_tb/u_rtl/UART1_INTR} \
{/equiv_tb/u_rtl/UART1_RXD} \
{/equiv_tb/u_rtl/UART1_TXD} \
{/equiv_tb/u_rtl/UART_CLK} \
{/equiv_tb/u_rtl/UART_PCLK} \
{/equiv_tb/u_rtl/UART_PRESETn} \
{/equiv_tb/u_rtl/UART_RESETn} \
{/equiv_tb/u_rtl/prdata_uart0\[31:0\]} \
{/equiv_tb/u_rtl/prdata_uart1\[31:0\]} \
{/equiv_tb/u_rtl/psel_uart0} \
{/equiv_tb/u_rtl/psel_uart1} \
{/equiv_tb/u_rtl/uart_sel\[1:0\]} \
{/LOGIC_LOW} \
{/LOGIC_HIGH} \
}
wvAddSignal -win $_nWave2 -group {"G2" \
}
wvSelectSignal -win $_nWave2 {( "G1" 21 22 23 24 25 26 27 28 29 30 31 32 33 34 \
           35 36 37 38 39 40 41 42 43 44 )} 
wvSetPosition -win $_nWave2 {("G1" 44)}
wvGetSignalClose -win $_nWave2
wvScrollUp -win $_nWave2 11
wvScrollDown -win $_nWave2 4
wvScrollDown -win $_nWave2 2
wvScrollDown -win $_nWave2 3
wvScrollDown -win $_nWave2 2
wvScrollDown -win $_nWave2 0
wvZoomAll -win $_nWave2
wvZoomAll -win $_nWave2
wvSelectSignal -win $_nWave2 {( "G1" 38 )} 
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollDown -win $_nWave2 0
wvScrollDown -win $_nWave2 0
wvScrollDown -win $_nWave2 0
verdiWindowResize -win $_Verdi_1 "2052" "0" "2159" "1039"
wvSelectSignal -win $_nWave2 {( "G1" 1 )} 
wvSelectSignal -win $_nWave2 {( "G1" 21 )} 
wvSelectSignal -win $_nWave2 {( "G1" 21 )} 
wvSelectSignal -win $_nWave2 {( "G1" 2 )} 
wvSelectSignal -win $_nWave2 {( "G1" 1 )} 
wvScrollDown -win $_nWave2 1
wvScrollDown -win $_nWave2 1
wvScrollDown -win $_nWave2 1
wvSelectSignal -win $_nWave2 {( "G1" 3 )} 
wvSelectSignal -win $_nWave2 {( "G1" 23 )} 
wvSetCursor -win $_nWave2 9426.954416 -snap {("G1" 14)}
wvScrollDown -win $_nWave2 1
wvScrollDown -win $_nWave2 1
wvScrollDown -win $_nWave2 1
wvScrollDown -win $_nWave2 1
wvScrollDown -win $_nWave2 1
wvScrollDown -win $_nWave2 1
wvScrollDown -win $_nWave2 1
wvScrollDown -win $_nWave2 1
wvScrollDown -win $_nWave2 1
wvScrollDown -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvSelectSignal -win $_nWave2 {( "G1" 13 )} 
wvSelectSignal -win $_nWave2 {( "G1" 25 )} 
wvScrollUp -win $_nWave2 1
wvSetCursor -win $_nWave2 36353.669516 -snap {("G1" 25)}
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollUp -win $_nWave2 1
wvScrollDown -win $_nWave2 0
wvSetCursor -win $_nWave2 69634.464387 -snap {("G1" 23)}
wvSetCursor -win $_nWave2 69634.464387 -snap {("G1" 21)}
wvSetCursor -win $_nWave2 69374.051282 -snap {("G1" 22)}
debExit
