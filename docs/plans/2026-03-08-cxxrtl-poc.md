# CXXRTL PoC: UART 모듈 Yosys CXXRTL 변환 및 기존 테스트 검증

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Yosys CXXRTL로 UART RTL을 C++로 변환하고, 기존 HIRCT GenModel과 동일한 테스트(standalone + GenVerify 수준)를 통과하는지 확인하여 CXXRTL 활용 가능성을 판정한다.

**Architecture:** preprocessed.v(21,041줄, 22개 모듈)를 Yosys로 읽어 CXXRTL C++ 백엔드로 출력한 뒤, 기존 cmodel_standalone_test.cpp의 테스트 시나리오를 CXXRTL 모델에 맞게 어댑터를 작성하여 실행한다.

**Tech Stack:** Yosys (oss-cad-suite), g++, Make, 기존 HIRCT 테스트 인프라

**참조 전략 문서:** `docs/plans/2026-03-08-hirct-product-strategy.md` Section 4 (CXXRTL 중간 단계 활용)

---

### Task 1: Yosys 설치 및 환경 확인

**Goal:** oss-cad-suite에서 Yosys를 설치하고 CXXRTL 백엔드가 동작하는지 확인한다.

**Files:**
- 없음 (환경 설정만)

**Step 1: oss-cad-suite 설치**

```bash
cd /tmp
wget https://github.com/YosysHQ/oss-cad-suite-build/releases/download/2025-03-03/oss-cad-suite-linux-x64-20250303.tgz
tar xzf oss-cad-suite-linux-x64-20250303.tgz
export PATH="/tmp/oss-cad-suite/bin:$PATH"
```

**Step 2: 버전 확인**

Run: `yosys --version`
Expected: `Yosys 0.4x+` (CXXRTL 지원 버전)

**Step 3: CXXRTL 기본 동작 확인 — 최소 Verilog 테스트**

```bash
cat > /tmp/test_cxxrtl.v << 'EOF'
module counter(input clk, input rst, output reg [7:0] count);
  always @(posedge clk)
    if (rst) count <= 0;
    else count <= count + 1;
endmodule
EOF

yosys -p "read_verilog /tmp/test_cxxrtl.v; write_cxxrtl /tmp/test_cxxrtl.cc"
```

Run: `test -f /tmp/test_cxxrtl.cc && echo "CXXRTL_OK" || echo "CXXRTL_FAIL"`
Expected: `CXXRTL_OK`

**Step 4: 최소 모델 컴파일 및 실행**

```bash
cat > /tmp/test_cxxrtl_main.cc << 'EOF'
#include <backends/cxxrtl/cxxrtl.h>
#include "test_cxxrtl.cc"

int main() {
    cxxrtl_design::p_counter top;
    top.p_rst.set<bool>(true);
    top.p_clk.set<bool>(false);
    top.step();
    top.p_clk.set<bool>(true);
    top.step();
    top.p_rst.set<bool>(false);
    for (int i = 0; i < 10; i++) {
        top.p_clk.set<bool>(false); top.step();
        top.p_clk.set<bool>(true); top.step();
    }
    printf("count = %d\n", top.p_count.get<uint8_t>());
    return (top.p_count.get<uint8_t>() == 10) ? 0 : 1;
}
EOF

g++ -std=c++17 -I /tmp/oss-cad-suite/share/yosys/ -o /tmp/test_cxxrtl /tmp/test_cxxrtl_main.cc
/tmp/test_cxxrtl
```

Run: `/tmp/test_cxxrtl`
Expected: `count = 10` (exit 0)

---

### Task 2: UART preprocessed.v를 CXXRTL로 변환

**Goal:** 실제 UART RTL(22개 모듈, 21,041줄)을 CXXRTL로 변환하고 컴파일한다.

**Files:**
- Input: `examples/fc6161/pt_plat/skip-analysis-results/uart/preprocessed.v`
- Input: `examples/fc6161/pt_plat/config/stubs/*.v` (클럭 게이트 스텁)
- Create: `examples/fc6161/pt_plat/cxxrtl-poc/uart_cxxrtl.cc` (CXXRTL 출력)
- Create: `examples/fc6161/pt_plat/cxxrtl-poc/Makefile`

**Step 1: PoC 디렉토리 생성**

```bash
mkdir -p examples/fc6161/pt_plat/cxxrtl-poc
```

**Step 2: Yosys로 UART CXXRTL 변환**

```bash
yosys -p "
  read_verilog examples/fc6161/pt_plat/config/stubs/*.v;
  read_verilog examples/fc6161/pt_plat/skip-analysis-results/uart/preprocessed.v;
  hierarchy -top uart_top;
  proc; flatten; opt;
  write_cxxrtl examples/fc6161/pt_plat/cxxrtl-poc/uart_cxxrtl.cc
"
```

Run: `test -f examples/fc6161/pt_plat/cxxrtl-poc/uart_cxxrtl.cc && wc -l examples/fc6161/pt_plat/cxxrtl-poc/uart_cxxrtl.cc`
Expected: 파일 존재, 수천줄 이상

**주의**: `flatten`을 사용하면 모듈 계층이 소실된다. 모듈 계층 보존이 필요하면:

```bash
yosys -p "
  read_verilog examples/fc6161/pt_plat/config/stubs/*.v;
  read_verilog examples/fc6161/pt_plat/skip-analysis-results/uart/preprocessed.v;
  hierarchy -top uart_top;
  proc; opt;
  write_cxxrtl -header examples/fc6161/pt_plat/cxxrtl-poc/uart_cxxrtl.cc
"
```

`-header` 옵션으로 별도 .h도 생성. `flatten` 없이 시도하여 모듈 계층 보존 여부 확인.

**Step 3: 에러 발생 시 기록**

Yosys 변환에서 에러가 발생하면, 에러 메시지를 `examples/fc6161/pt_plat/cxxrtl-poc/yosys-errors.log`에 기록하고 원인을 분석한다. 가능한 이슈:
- SystemVerilog 구문 미지원 (preprocessed.v는 Verilog-2001이므로 문제 없을 가능성 높음)
- 클럭 게이트 스텁 누락
- 블랙박스 처리 필요

**Step 4: 컴파일 확인**

```bash
g++ -std=c++17 -c \
  -I /tmp/oss-cad-suite/share/yosys/ \
  examples/fc6161/pt_plat/cxxrtl-poc/uart_cxxrtl.cc \
  -o examples/fc6161/pt_plat/cxxrtl-poc/uart_cxxrtl.o
```

Run: `test -f examples/fc6161/pt_plat/cxxrtl-poc/uart_cxxrtl.o && echo "COMPILE_OK"`
Expected: `COMPILE_OK`

---

### Task 3: CXXRTL 모델 기본 동작 테스트

**Goal:** CXXRTL UART 모델이 리셋 후 기본 동작하는지 확인한다.

**Files:**
- Create: `examples/fc6161/pt_plat/cxxrtl-poc/basic_test.cpp`

**Step 1: 기본 테스트 작성**

기존 `cmodel_standalone_test.cpp`의 리셋 + 레지스터 read 시나리오를 CXXRTL API로 재구현한다. CXXRTL API는 GenModel의 `step()`과 다르므로 어댑터가 필요:

```cpp
// basic_test.cpp
#include <cstdio>
#include <cstdint>
#include <cassert>
#include <backends/cxxrtl/cxxrtl.h>
#include "uart_cxxrtl.cc"

static void clock_tick(cxxrtl_design::p_uart__top &m) {
    m.p_UART__PCLK.set<bool>(false);
    m.step();
    m.p_UART__PCLK.set<bool>(true);
    m.step();
}

int main() {
    cxxrtl_design::p_uart__top model;

    // Reset
    model.p_UART__PRESETn.set<bool>(false);
    model.p_UART__RESETn.set<bool>(false);
    model.p_SCAN__MODE.set<bool>(false);
    model.p_PSEL.set<bool>(false);
    model.p_PENABLE.set<bool>(false);
    model.p_PWRITE.set<bool>(false);
    for (int i = 0; i < 5; i++) clock_tick(model);

    // Release reset
    model.p_UART__PRESETn.set<bool>(true);
    model.p_UART__RESETn.set<bool>(true);
    for (int i = 0; i < 5; i++) clock_tick(model);

    // APB read: IIR register (offset 0x08) — default should be 0x01 (no interrupt)
    model.p_PSEL.set<bool>(true);
    model.p_PENABLE.set<bool>(false);
    model.p_PWRITE.set<bool>(false);
    model.p_PADDR.set<uint32_t>(0x08);
    clock_tick(model);
    model.p_PENABLE.set<bool>(true);
    clock_tick(model);

    uint32_t prdata = model.p_PRDATA.get<uint32_t>();
    printf("IIR read: 0x%02x\n", prdata & 0xFF);

    model.p_PSEL.set<bool>(false);
    model.p_PENABLE.set<bool>(false);

    printf("BASIC TEST PASS\n");
    return 0;
}
```

**주의**: CXXRTL의 신호 이름은 Verilog 이름에서 특수문자가 변환된다 (예: `UART_PCLK` → `p_UART__PCLK`). 정확한 이름은 Task 2에서 생성된 헤더를 보고 확인한다.

**Step 2: 컴파일 및 실행**

```bash
g++ -std=c++17 \
  -I /tmp/oss-cad-suite/share/yosys/ \
  -o examples/fc6161/pt_plat/cxxrtl-poc/basic_test \
  examples/fc6161/pt_plat/cxxrtl-poc/basic_test.cpp

./examples/fc6161/pt_plat/cxxrtl-poc/basic_test
```

Run: `./examples/fc6161/pt_plat/cxxrtl-poc/basic_test`
Expected: `BASIC TEST PASS` (exit 0)

---

### Task 4: GenModel 결과와 비교

**Goal:** 동일 입력 시퀀스에서 CXXRTL 모델과 기존 GenModel의 출력이 일치하는지 확인한다.

**Files:**
- Create: `examples/fc6161/pt_plat/cxxrtl-poc/compare_test.cpp`

**Step 1: 비교 테스트 작성**

GenModel의 `uart_top` 클래스와 CXXRTL 모델을 동시에 구동하여 매 사이클 출력을 비교한다.

```cpp
// compare_test.cpp
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cassert>

// GenModel
#include "../../output/uart_top/cmodel/uart_top.h"

// CXXRTL
#include <backends/cxxrtl/cxxrtl.h>
#include "uart_cxxrtl.cc"

int main(int argc, char **argv) {
    int cycles = (argc > 1) ? atoi(argv[1]) : 100;
    int seed = (argc > 2) ? atoi(argv[2]) : 42;
    srand(seed);

    // GenModel
    uart_top gm;
    gm.do_reset();

    // CXXRTL
    cxxrtl_design::p_uart__top cx;

    // Reset CXXRTL
    cx.p_UART__PRESETn.set<bool>(false);
    cx.p_UART__RESETn.set<bool>(false);
    cx.p_SCAN__MODE.set<bool>(false);
    cx.p_PSEL.set<bool>(false);
    for (int i = 0; i < 5; i++) {
        cx.p_UART__PCLK.set<bool>(false); cx.step();
        cx.p_UART__PCLK.set<bool>(true); cx.step();
    }
    cx.p_UART__PRESETn.set<bool>(true);
    cx.p_UART__RESETn.set<bool>(true);

    int mismatches = 0;
    for (int cyc = 0; cyc < cycles; cyc++) {
        // Random inputs
        bool psel = rand() & 1;
        bool penable = rand() & 1;
        bool pwrite = rand() & 1;
        uint32_t paddr = (rand() & 0x1F) << 2;
        uint32_t pwdata = rand();

        // Apply to GenModel
        gm.PSEL = psel;
        gm.PENABLE = penable;
        gm.PWRITE = pwrite;
        gm.PADDR = paddr;
        gm.PWDATA = pwdata;
        gm.step();

        // Apply to CXXRTL
        cx.p_PSEL.set<bool>(psel);
        cx.p_PENABLE.set<bool>(penable);
        cx.p_PWRITE.set<bool>(pwrite);
        cx.p_PADDR.set<uint32_t>(paddr);
        cx.p_PWDATA.set<uint32_t>(pwdata);
        cx.p_UART__PCLK.set<bool>(false); cx.step();
        cx.p_UART__PCLK.set<bool>(true); cx.step();

        // Compare outputs
        uint32_t gm_prdata = gm.PRDATA;
        uint32_t cx_prdata = cx.p_PRDATA.get<uint32_t>();
        if (gm_prdata != cx_prdata) {
            printf("MISMATCH cyc=%d: GenModel PRDATA=0x%08x, CXXRTL PRDATA=0x%08x\n",
                   cyc, gm_prdata, cx_prdata);
            mismatches++;
        }
    }

    if (mismatches == 0)
        printf("COMPARE PASS: %d cycles, seed=%d\n", cycles, seed);
    else
        printf("COMPARE FAIL: %d mismatches in %d cycles\n", mismatches, cycles);

    return (mismatches == 0) ? 0 : 1;
}
```

**Step 2: 컴파일 및 실행**

```bash
g++ -std=c++17 \
  -I /tmp/oss-cad-suite/share/yosys/ \
  -I examples/fc6161/pt_plat/output/uart_top/ \
  -o examples/fc6161/pt_plat/cxxrtl-poc/compare_test \
  examples/fc6161/pt_plat/cxxrtl-poc/compare_test.cpp

./examples/fc6161/pt_plat/cxxrtl-poc/compare_test 1000 42
```

Run: `./examples/fc6161/pt_plat/cxxrtl-poc/compare_test 1000 42`
Expected: `COMPARE PASS: 1000 cycles, seed=42` (exit 0)

---

### Task 5: PoC 결과 문서화 및 Go/No-Go 판정

**Goal:** PoC 결과를 기록하고 H-2 미결 결정을 업데이트한다.

**Files:**
- Create: `examples/fc6161/pt_plat/cxxrtl-poc/README.md`
- Modify: `docs/plans/open-decisions.md` (H-2 상태 업데이트)

**Step 1: 결과 문서 작성**

Go/No-Go 판정표:

| 기준 | 결과 | 비고 |
|------|------|------|
| Yosys 변환 성공 | ? | preprocessed.v 22개 모듈 |
| g++ 컴파일 성공 | ? | C++17 |
| 리셋 후 기본 동작 | ? | basic_test |
| 모듈 계층 보존 | ? | flatten 없이 시도 |
| GenModel과 출력 일치 | ? | compare_test 1000cyc |

**Step 2: H-2 업데이트**

`docs/plans/open-decisions.md`의 H-2를 결과에 따라:
- 전체 통과 → RESOLVED (선택안 기록)
- 부분 실패 → PARTIAL (실패 항목 + 후속 조치 기록)
- 전체 실패 → RESOLVED (No-Go, GenModel 유지 결정)

**Step 3: Commit**

```bash
git add examples/fc6161/pt_plat/cxxrtl-poc/
git commit -m "poc: CXXRTL UART conversion and comparison test results"
```

---

## 의존성 순서

```
Task 1 (Yosys 설치)
  → Task 2 (UART CXXRTL 변환)
    → Task 3 (기본 동작 테스트)
      → Task 4 (GenModel 비교)
        → Task 5 (결과 문서화)
```

모든 태스크는 순차 의존. 각 단계에서 실패하면 원인을 기록하고 No-Go 판정 가능.

## 예상 이슈 및 대응

| 이슈 | 가능성 | 대응 |
|------|--------|------|
| Yosys가 preprocessed.v 파싱 실패 | 낮음 (Verilog-2001) | 에러 위치 확인, 문제 구문 수정 |
| 클럭 게이트 스텁 미인식 | 중간 | stubs/*.v를 먼저 읽도록 순서 조정 |
| CXXRTL 신호 이름 매핑 불일치 | 높음 | 생성 헤더에서 실제 이름 확인 후 테스트 코드 수정 |
| GenModel과 CXXRTL 출력 불일치 | 중간 | 리셋 타이밍, 클럭 엣지 차이 확인. 몇 사이클 스킵 후 비교 시도 |
| flatten 없이 CXXRTL 변환 실패 | 중간 | flatten 사용으로 폴백, 모듈 계층은 No-Go 기록 |
