#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include "../../output/uart_top/cmodel/uart_top.h"

constexpr uint32_t REG_THR_RBR_DLL = 0x00;
constexpr uint32_t REG_IER_DLH     = 0x04;
constexpr uint32_t REG_FCR         = 0x08;
constexpr uint32_t REG_LCR         = 0x0C;
constexpr uint32_t REG_LSR         = 0x14;
constexpr uint32_t REG_SCR         = 0x1C;

constexpr int BIT_PERIOD = 16;

static uart_top dut;
static int measured_bp = 0;

static void clock_step() {
    dut.UART_PCLK = true;
    dut.UART_CLK  = true;
    dut.step();
    dut.UART_PCLK = false;
    dut.UART_CLK  = false;
    dut.eval_comb();  // falling edge propagation for BCM57 edge detection
}

static void apb_write(uint32_t addr, uint32_t data) {
    dut.PADDR   = addr;
    dut.PWDATA  = data;
    dut.PWRITE  = true;
    dut.PSEL    = true;
    dut.PENABLE = false;
    clock_step();
    dut.PENABLE = true;
    clock_step();
    dut.PSEL    = false;
    dut.PENABLE = false;
    dut.PWRITE  = false;
    clock_step();
}

static uint32_t apb_read(uint32_t addr) {
    dut.PADDR   = addr;
    dut.PWRITE  = false;
    dut.PSEL    = true;
    dut.PENABLE = false;
    clock_step();
    dut.PENABLE = true;
    clock_step();
    uint32_t result = dut.PRDATA;
    dut.PSEL    = false;
    dut.PENABLE = false;
    clock_step();
    return result;
}

static bool check(const char *name, uint32_t expected, uint32_t actual) {
    bool pass = (expected == actual);
    std::cout << (pass ? "[PASS] " : "[FAIL] ") << name
              << " - expected: 0x" << std::hex << std::setw(8) << std::setfill('0') << expected
              << ", got: 0x" << std::setw(8) << std::setfill('0') << actual
              << std::dec << "\n";
    return pass;
}

static void uart_init_8n1() {
    apb_write(REG_LCR, 0x83);
    apb_write(REG_THR_RBR_DLL, 0x01);
    apb_write(REG_IER_DLH, 0x00);
    apb_write(REG_LCR, 0x03);
    apb_write(REG_FCR, 0x00);
    for (int i = 0; i < 50; ++i) clock_step();
}

static int tx_decode_char() {
    constexpr int LIMIT = 2000;
    int bp;

    for (int i = 0; i < LIMIT; ++i) {
        if (dut.UART0_TXD) break;
        clock_step();
        if (i == LIMIT - 1) return -1;
    }
    for (int i = 0; i < LIMIT; ++i) {
        if (!dut.UART0_TXD) break;
        clock_step();
        if (i == LIMIT - 1) return -1;
    }

    if (measured_bp == 0) {
        bp = 0;
        for (int i = 0; i < LIMIT; ++i) {
            if (dut.UART0_TXD) break;
            clock_step();
            ++bp;
            if (i == LIMIT - 1) return -1;
        }
        measured_bp = bp;
        std::cout << "[INFO] Measured bit period: " << bp << " sclk cycles\n";
        for (int i = 0; i < bp / 2; ++i) clock_step();
    } else {
        bp = measured_bp;
        for (int i = 0; i < bp + bp / 2; ++i) clock_step();
    }

    uint8_t byte_val = 0;
    if (dut.UART0_TXD) byte_val |= 1;
    for (int bit = 1; bit < 8; ++bit) {
        for (int i = 0; i < bp; ++i) clock_step();
        if (dut.UART0_TXD) byte_val |= (1 << bit);
    }
    for (int i = 0; i < bp; ++i) clock_step();
    return byte_val;
}

static void rx_inject_char(uint8_t ch) {
    int bp = (measured_bp > 0) ? measured_bp : BIT_PERIOD;
    dut.UART0_RXD = true;
    for (int i = 0; i < bp; ++i) clock_step();
    dut.UART0_RXD = false;
    for (int i = 0; i < bp; ++i) clock_step();
    for (int bit = 0; bit < 8; ++bit) {
        dut.UART0_RXD = (ch >> bit) & 1;
        for (int i = 0; i < bp; ++i) clock_step();
    }
    dut.UART0_RXD = true;
    for (int i = 0; i < bp; ++i) clock_step();
}

int main() {
    int passed = 0;
    constexpr int total = 10;

    dut.do_reset();
    dut.UART0_RXD = true;
    dut.UART1_RXD = true;
    for (int i = 0; i < 4; ++i) clock_step();
    dut.UART_PRESETn = true;
    dut.UART_RESETn  = true;
    for (int i = 0; i < 4; ++i) clock_step();

    // Test 1: LCR
    apb_write(REG_LCR, 0x03);
    if (check("LCR readback", 0x00000003, apb_read(REG_LCR))) ++passed;

    // Test 2: SCR
    apb_write(REG_SCR, 0xA5);
    if (check("SCR readback", 0x000000A5, apb_read(REG_SCR))) ++passed;

    // Test 3: DLL
    apb_write(REG_LCR, 0x83);
    apb_write(REG_THR_RBR_DLL, 0x36);
    if (check("DLL readback", 0x00000036, apb_read(REG_THR_RBR_DLL))) ++passed;
    apb_write(REG_LCR, 0x03);

    // Phase A: TX decode
    std::cout << "\n--- Phase A: TX decode (\"Hello\") ---\n";
    measured_bp = 0;
    uart_init_8n1();
    apb_write(REG_THR_RBR_DLL, 0xFF);
    tx_decode_char();
    const char *tx_msg = "Hello";
    for (int ci = 0; tx_msg[ci]; ++ci) {
        uint8_t exp = static_cast<uint8_t>(tx_msg[ci]);
        apb_write(REG_THR_RBR_DLL, exp);
        int dec = tx_decode_char();
        bool ok = (dec == exp);
        std::cout << "TX '" << tx_msg[ci] << "' (0x" << std::hex << std::setw(2)
                  << std::setfill('0') << (int)exp << "): decoded=0x"
                  << std::setw(2) << std::setfill('0') << (dec < 0 ? 0 : dec)
                  << (ok ? " PASS" : " FAIL") << std::dec << "\n";
        if (ok) ++passed;
    }

    // Phase B: RX inject
    std::cout << "\n--- Phase B: RX inject (0x00~0x7F) ---\n";
    uart_init_8n1();
    int rx_p = 0, rx_f = 0;
    for (int ch = 0; ch <= 0x7F; ++ch) {
        rx_inject_char(static_cast<uint8_t>(ch));
        for (int i = 0; i < 100; ++i) clock_step();
        bool dr = false;
        for (int i = 0; i < 500; ++i) {
            if (apb_read(REG_LSR) & 1) { dr = true; break; }
            clock_step();
        }
        if (!dr) {
            std::cout << "[FAIL] RX 0x" << std::hex << std::setw(2)
                      << std::setfill('0') << ch << " timeout" << std::dec << "\n";
            ++rx_f;
            continue;
        }
        uint32_t rbr = apb_read(REG_THR_RBR_DLL);
        if ((rbr & 0xFF) == static_cast<uint32_t>(ch)) {
            ++rx_p;
        } else {
            std::cout << "[FAIL] RX 0x" << std::hex << std::setw(2) << std::setfill('0') << ch
                      << ": expected=0x" << std::setw(2) << std::setfill('0') << ch
                      << ", got=0x" << std::setw(2) << std::setfill('0') << (rbr & 0xFF)
                      << std::dec << "\n";
            ++rx_f;
        }
    }
    std::cout << "RX " << rx_p << "/128 PASS, " << rx_f << " FAIL\n";
    if (rx_f == 0) ++passed;

    // Phase C: Pin loopback — one character at a time (FIFO disabled)
    std::cout << "\n--- Phase C: Pin loopback (\"Hello\") ---\n";
    uart_init_8n1();
    const char *lb = "Hello";
    int ll = static_cast<int>(std::strlen(lb));
    int lp = 0;
    for (int ci = 0; ci < ll; ++ci) {
        apb_write(REG_THR_RBR_DLL, static_cast<uint8_t>(lb[ci]));
        for (int i = 0; i < 800; ++i) {
            dut.UART0_RXD = dut.UART0_TXD;
            clock_step();
        }
        dut.UART0_RXD = true;
        bool dr = false;
        for (int i = 0; i < 500; ++i) {
            if (apb_read(REG_LSR) & 1) { dr = true; break; }
            clock_step();
        }
        if (!dr) {
            std::cout << "[FAIL] Loopback char " << ci << " '" << lb[ci]
                      << "': no Data Ready\n";
            continue;
        }
        uint32_t r = apb_read(REG_THR_RBR_DLL);
        if ((r & 0xFF) == static_cast<uint8_t>(lb[ci])) {
            ++lp;
        } else {
            std::cout << "[FAIL] Loopback char " << ci << " '" << lb[ci]
                      << "': expected=0x" << std::hex << std::setw(2)
                      << std::setfill('0') << (int)(uint8_t)lb[ci]
                      << ", got=0x" << std::setw(2) << std::setfill('0') << (r & 0xFF)
                      << std::dec << "\n";
        }
    }
    std::cout << "Pin loopback: " << lp << "/" << ll << " PASS\n";
    if (lp == ll) ++passed;

    // Phase D: FIFO mode TX
    std::cout << "\n--- Phase D: FIFO mode TX ---\n";
    uart_init_8n1();
    apb_write(REG_FCR, 0x07);  // FIFO enable + TX/RX FIFO reset
    for (int i = 0; i < 50; ++i) clock_step();
    apb_write(REG_THR_RBR_DLL, 0x48);  // 'H'
    measured_bp = 0;
    int fifo_dec = tx_decode_char();
    bool fifo_ok = (fifo_dec == 0x48);
    std::cout << "FIFO TX 'H' (0x48): decoded=0x"
              << std::hex << std::setw(2) << std::setfill('0')
              << (fifo_dec < 0 ? 0 : fifo_dec)
              << (fifo_ok ? " PASS" : " FAIL") << std::dec << "\n";
    if (fifo_ok) ++passed;
    apb_write(REG_FCR, 0x00);  // disable FIFO

    constexpr int total_with_fifo = total + 1;
    std::cout << "\n=== " << passed << "/" << total_with_fifo << " tests passed ===\n";
    return (passed == total_with_fifo) ? 0 : 1;
}
