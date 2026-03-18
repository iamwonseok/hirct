#include <cstdio>
#include <cstdint>
#include <cstdlib>

#include "../output/cmodel/uart_top.h"

#include <cxxrtl/cxxrtl.h>
namespace cxxrtl_uart {
#include "uart_cxxrtl.cc"
}

// GenModel step()은 PCLK phase 이후 CLK phase를 순차 적용하므로,
// CXXRTL도 동일하게 posedge를 분리해 재현해야 위상 일치한다.
static void cx_tick(cxxrtl_uart::cxxrtl_design::p_uart__top &m) {
    m.p_UART__PCLK.set<bool>(true);
    m.p_UART__CLK.set<bool>(false);
    m.step();
    m.p_UART__PCLK.set<bool>(false);
    m.p_UART__CLK.set<bool>(false);
    m.step();
    m.p_UART__PCLK.set<bool>(false);
    m.p_UART__CLK.set<bool>(true);
    m.step();
    m.p_UART__PCLK.set<bool>(false);
    m.p_UART__CLK.set<bool>(false);
    m.step();
}

static const int RESET_CYCLES = 10;
static const int SETTLE_CYCLES = 10;

int main(int argc, char **argv) {
    int cycles = (argc > 1) ? atoi(argv[1]) : 100;
    int seed = (argc > 2) ? atoi(argv[2]) : 42;
    srand(seed);

    // --- CXXRTL 리셋 ---
    cxxrtl_uart::cxxrtl_design::p_uart__top cx;
    cx.p_UART__PRESETn.set<bool>(false);
    cx.p_UART__RESETn.set<bool>(false);
    cx.p_UART__CLK.set<bool>(false);
    cx.p_SCAN__MODE.set<bool>(false);
    cx.p_PSEL.set<bool>(false);
    cx.p_PENABLE.set<bool>(false);
    cx.p_PWRITE.set<bool>(false);
    cx.p_PADDR.set<uint32_t>(0);
    cx.p_PWDATA.set<uint32_t>(0);
    cx.p_UART0__RXD.set<bool>(true);
    cx.p_UART1__RXD.set<bool>(true);
    for (int i = 0; i < RESET_CYCLES; i++) cx_tick(cx);
    cx.p_UART__PRESETn.set<bool>(true);
    cx.p_UART__RESETn.set<bool>(true);
    for (int i = 0; i < SETTLE_CYCLES; i++) cx_tick(cx);

    // --- GenModel 리셋 ---
    uart_top gm;
    gm.do_reset();
    gm.UART0_RXD = true;
    gm.UART1_RXD = true;
    for (int i = 0; i < RESET_CYCLES; i++) gm.step();
    gm.UART_PRESETn = true;
    gm.UART_RESETn = true;
    for (int i = 0; i < SETTLE_CYCLES; i++) gm.step();

    int prdata_mismatches = 0;
    int intr_mismatches = 0;
    int txd_mismatches = 0;
    int read_compared = 0;
    for (int cyc = 0; cyc < cycles; cyc++) {
        bool psel = (rand() % 4) == 0;
        bool penable = psel && (rand() & 1);
        bool pwrite = rand() & 1;
        uint32_t paddr = (rand() % 8) << 2;
        uint32_t pwdata = rand() & 0xFF;

        gm.PSEL = psel;
        gm.PENABLE = penable;
        gm.PWRITE = pwrite;
        gm.PADDR = paddr;
        gm.PWDATA = pwdata;
        gm.step();

        cx.p_PSEL.set<bool>(psel);
        cx.p_PENABLE.set<bool>(penable);
        cx.p_PWRITE.set<bool>(pwrite);
        cx.p_PADDR.set<uint32_t>(paddr);
        cx.p_PWDATA.set<uint32_t>(pwdata);
        cx_tick(cx);

        uint32_t gm_prdata = gm.PRDATA;
        uint32_t cx_prdata = cx.p_PRDATA.get<uint32_t>();
        bool gm_intr = gm.UART0_INTR;
        bool cx_intr = cx.p_UART0__INTR.get<bool>();
        bool gm_txd = gm.UART0_TXD;
        bool cx_txd = cx.p_UART0__TXD.get<bool>();

        if (psel && penable && !pwrite) {
            read_compared++;
            if (gm_prdata != cx_prdata) {
                if (prdata_mismatches < 10)
                    printf("PRDATA MISMATCH cyc=%d addr=0x%02x: GM=0x%08x CX=0x%08x\n",
                           cyc, paddr, gm_prdata, cx_prdata);
                prdata_mismatches++;
            }
        }
        if (gm_intr != cx_intr) {
            if (intr_mismatches < 5)
                printf("INTR MISMATCH cyc=%d: GM=%d CX=%d\n", cyc, gm_intr, cx_intr);
            intr_mismatches++;
        }
        if (gm_txd != cx_txd) {
            if (txd_mismatches < 5)
                printf("TXD MISMATCH cyc=%d: GM=%d CX=%d\n", cyc, gm_txd, cx_txd);
            txd_mismatches++;
        }
    }

    printf("\n--- Results ---\n");
    printf("Cycles: %d, Seed: %d\n", cycles, seed);
    printf("PRDATA: %d reads, %d mismatches  %s\n",
           read_compared, prdata_mismatches,
           prdata_mismatches == 0 ? "PASS" : "FAIL");
    printf("INTR:   %d mismatches  %s\n",
           intr_mismatches,
           intr_mismatches == 0 ? "PASS" : "FAIL");
    printf("TXD:    %d mismatches  %s\n",
           txd_mismatches,
           txd_mismatches == 0 ? "PASS" : "FAIL");

    int total = prdata_mismatches + intr_mismatches + txd_mismatches;
    if (total == 0)
        printf("\nCOMPARE PASS\n");
    else
        printf("\nCOMPARE FAIL\n");

    if (txd_mismatches > 0)
        printf("NOTE: TXD mismatch may reflect GenModel step() vs CXXRTL "
               "rising-edge timing difference (known simulation model gap)\n");

    return (total == 0) ? 0 : 1;
}
