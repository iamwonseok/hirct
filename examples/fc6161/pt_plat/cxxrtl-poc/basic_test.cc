#include <cstdio>
#include <cstdint>
#include <cassert>
#include "uart_cxxrtl.cc"

static void pclk_tick(cxxrtl_design::p_uart__top &m) {
    m.p_UART__PCLK.set<bool>(false); m.step();
    m.p_UART__PCLK.set<bool>(true);  m.step();
}

static uint32_t apb_read(cxxrtl_design::p_uart__top &m, uint32_t addr) {
    m.p_PSEL.set<bool>(true);
    m.p_PENABLE.set<bool>(false);
    m.p_PWRITE.set<bool>(false);
    m.p_PADDR.set<uint32_t>(addr);
    pclk_tick(m);
    m.p_PENABLE.set<bool>(true);
    pclk_tick(m);
    uint32_t val = m.p_PRDATA.get<uint32_t>();
    m.p_PSEL.set<bool>(false);
    m.p_PENABLE.set<bool>(false);
    return val;
}

static void apb_write(cxxrtl_design::p_uart__top &m, uint32_t addr, uint32_t data) {
    m.p_PSEL.set<bool>(true);
    m.p_PENABLE.set<bool>(false);
    m.p_PWRITE.set<bool>(true);
    m.p_PADDR.set<uint32_t>(addr);
    m.p_PWDATA.set<uint32_t>(data);
    pclk_tick(m);
    m.p_PENABLE.set<bool>(true);
    pclk_tick(m);
    m.p_PSEL.set<bool>(false);
    m.p_PENABLE.set<bool>(false);
    m.p_PWRITE.set<bool>(false);
}

int main() {
    cxxrtl_design::p_uart__top m;
    int pass = 0, fail = 0;

    m.p_UART__PRESETn.set<bool>(false);
    m.p_UART__RESETn.set<bool>(false);
    m.p_UART__CLK.set<bool>(false);
    m.p_SCAN__MODE.set<bool>(false);
    m.p_PSEL.set<bool>(false);
    m.p_PENABLE.set<bool>(false);
    m.p_PWRITE.set<bool>(false);
    m.p_UART0__RXD.set<bool>(true);
    m.p_UART1__RXD.set<bool>(true);

    for (int i = 0; i < 10; i++) pclk_tick(m);

    m.p_UART__PRESETn.set<bool>(true);
    m.p_UART__RESETn.set<bool>(true);
    for (int i = 0; i < 10; i++) pclk_tick(m);

    // Test 1: IIR read (offset 0x08) — reset default = 0x01 (no pending interrupt)
    uint32_t iir = apb_read(m, 0x08) & 0xFF;
    if (iir == 0x01) { printf("PASS: IIR reset value = 0x%02x\n", iir); pass++; }
    else { printf("FAIL: IIR expected 0x01, got 0x%02x\n", iir); fail++; }

    // Test 2: LCR write/read (offset 0x0C)
    apb_write(m, 0x0C, 0x03);
    uint32_t lcr = apb_read(m, 0x0C) & 0xFF;
    if (lcr == 0x03) { printf("PASS: LCR write/read = 0x%02x\n", lcr); pass++; }
    else { printf("FAIL: LCR expected 0x03, got 0x%02x\n", lcr); fail++; }

    // Test 3: LSR read (offset 0x14) — reset default has THRE+TEMT bits (0x60)
    uint32_t lsr = apb_read(m, 0x14) & 0xFF;
    if ((lsr & 0x60) == 0x60) { printf("PASS: LSR THRE+TEMT set = 0x%02x\n", lsr); pass++; }
    else { printf("FAIL: LSR expected THRE+TEMT, got 0x%02x\n", lsr); fail++; }

    // Test 4: interrupt output should be low after reset
    bool intr = m.p_UART0__INTR.get<bool>();
    if (!intr) { printf("PASS: UART0_INTR = 0 after reset\n"); pass++; }
    else { printf("FAIL: UART0_INTR expected 0, got %d\n", intr); fail++; }

    printf("\n%d/%d tests passed\n", pass, pass + fail);
    return (fail == 0) ? 0 : 1;
}
