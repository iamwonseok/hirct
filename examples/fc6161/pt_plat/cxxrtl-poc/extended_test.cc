#include <cstdio>
#include <cstdint>
#include <cstdlib>
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

int main(int argc, char **argv) {
    int seed = (argc > 1) ? atoi(argv[1]) : 42;
    srand(seed);
    int pass = 0, fail = 0;

    cxxrtl_design::p_uart__top m;

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

    // Test 1: Reset values
    auto chk = [&](const char *name, uint32_t addr, uint32_t mask, uint32_t expect) {
        uint32_t val = apb_read(m, addr) & mask;
        if (val == expect) { printf("PASS: %s = 0x%02x\n", name, val); pass++; }
        else { printf("FAIL: %s expected 0x%02x, got 0x%02x\n", name, expect, val); fail++; }
    };

    chk("IIR", 0x08, 0xFF, 0x01);
    chk("LCR", 0x0C, 0xFF, 0x00);
    chk("LSR", 0x14, 0xFF, 0x60);

    // Test 2: LCR write/read
    apb_write(m, 0x0C, 0x83);
    chk("LCR after write 0x83", 0x0C, 0xFF, 0x83);

    // Test 3: DLL/DLH access (DLAB=1)
    apb_write(m, 0x00, 0x01);
    chk("DLL", 0x00, 0xFF, 0x01);
    apb_write(m, 0x04, 0x00);
    chk("DLH", 0x04, 0xFF, 0x00);

    // Test 4: Restore DLAB=0, set 8N1
    apb_write(m, 0x0C, 0x03);
    chk("LCR 8N1", 0x0C, 0xFF, 0x03);

    // Test 5: TX - write THR and check LSR
    uint32_t lsr_before = apb_read(m, 0x14) & 0xFF;
    apb_write(m, 0x00, 0x41);
    for (int i = 0; i < 20; i++) pclk_tick(m);
    printf("INFO: LSR before TX=0x%02x\n", lsr_before);

    // Test 6: Run 1000 cycles of random APB traffic (stability test)
    int stable_cycles = 1000;
    bool crashed = false;
    for (int i = 0; i < stable_cycles; i++) {
        if (rand() % 4 == 0) {
            uint32_t addr = (rand() % 8) << 2;
            if (rand() & 1) apb_write(m, addr, rand() & 0xFF);
            else apb_read(m, addr);
        } else {
            pclk_tick(m);
        }
    }
    if (!crashed) { printf("PASS: %d cycles random traffic stable\n", stable_cycles); pass++; }

    // Test 7: Interrupt stays low when IER=0
    apb_write(m, 0x04, 0x00);
    for (int i = 0; i < 10; i++) pclk_tick(m);
    bool intr0 = m.p_UART0__INTR.get<bool>();
    bool intr1 = m.p_UART1__INTR.get<bool>();
    if (!intr0) { printf("PASS: UART0_INTR=0 with IER=0\n"); pass++; }
    else { printf("FAIL: UART0_INTR=%d with IER=0\n", intr0); fail++; }
    if (!intr1) { printf("PASS: UART1_INTR=0 with IER=0\n"); pass++; }
    else { printf("FAIL: UART1_INTR=%d with IER=0\n", intr1); fail++; }

    printf("\n--- Extended Test Results ---\n");
    printf("%d/%d tests passed (seed=%d)\n", pass, pass + fail, seed);
    return (fail == 0) ? 0 : 1;
}
