#include <cstdio>
#include "test_counter.cc"

int main() {
    cxxrtl_design::p_counter top;

    top.p_rst.set<bool>(true);
    top.p_clk.set<bool>(false); top.step();
    top.p_clk.set<bool>(true);  top.step();

    top.p_rst.set<bool>(false);
    for (int i = 0; i < 10; i++) {
        top.p_clk.set<bool>(false); top.step();
        top.p_clk.set<bool>(true);  top.step();
    }
    uint8_t count = top.p_count.get<uint8_t>();
    printf("count = %d\n", count);
    return (count == 10) ? 0 : 1;
}
