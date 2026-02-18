# HIRCT 공통 규약 (Conventions)

> **버전**: 8.0
> **작성일**: 2026-02-15
> **원칙**: "있는 것을 최대한 활용, 없는 것만 만든다"

---

## 0. RTL 입력 경로 독립성

> **핵심 원칙**: hirct-gen은 입력 RTL 경로에 의존하지 않는다.

이 저장소의 `rtl/` 디렉토리는 **개발/테스트용 예제**이다. 실제 배포 환경에서 RTL은 완전히 다른 경로에 위치한다:

```
# 예제 (이 저장소)
hirct-gen rtl/plat/src/s5/design/LevelGateway.v

# 실제 환경 (임의 경로)
hirct-gen /project/soc/ip/uart/src/uart_top.v
hirct-gen -f /project/soc/build/filelist.f --top SocTop
```

**출력 경로 결정 규칙**:

| 입력 모드 | 출력 경로 | 예시 |
|----------|----------|------|
| 단일 파일 | `output/<filename>/` | `output/uart_top/` |
| 단일 파일 + `-o` | `<output_dir>/<filename>/` | `custom/uart_top/` |
| filelist + `--top` | `output/<top_name>/` | `output/SocTop/` |
| filelist + `--top` + `-o` | `<output_dir>/<top_name>/` | `custom/SocTop/` |

> **Note**: `rtl/` prefix를 strip하는 "소스 트리 미러링"은 `config/generate.f` 기반 순회(`make generate`)에서만 적용한다.
> 이 경우 filelist 내 경로에서 공통 prefix를 자동 감지하여 strip한다.

### 0.1 multi-file 모드 정책 (filelist → circt-verilog 변환)

**실측 사실**: `circt-verilog`는 `-f filelist.f` 옵션을 **지원하지 않는다** (CIRCT `5e760efa9` 기준).
대신 여러 입력 파일을 인자로 나열하는 multi-file 모드를 지원한다. (근거: `risk-validation-results.md` §5)

**변환 규칙**: hirct-gen이 filelist를 자체 파싱하여 `circt-verilog` 인자로 변환한다.

```
hirct-gen -f filelist.f --top CoreIP [--timescale 1ns/1ps]
  ↓ hirct-gen이 filelist.f 파싱 (주석 제거, +incdir+ 처리)
  ↓ CirctRunner::runCirctVerilogMulti(files, top, timescale)
  ↓ circt-verilog --timescale=1ns/1ps --top=CoreIP file1.v file2.v ...
```

**`--timescale` 정책**:

| 모드 | timescale 전달 | 기본값 |
|------|---------------|--------|
| 단일 파일 (`hirct-gen input.v`) | 전달하지 않음 (파일 자체 선언 사용) | — |
| multi-file (`hirct-gen -f ... --top ...`) | **항상 전달** | `1ns/1ps` |
| 사용자 오버라이드 | `--timescale <value>` | — |

**근거**: multi-file 입력에서 timescale 정의가 섞인 RTL을 다루면 `circt-verilog`가 "timescale 충돌" 에러를 발생시킨다. `--timescale`을 명시하면 모든 파일에 동일 기준을 강제하여 에러를 방지한다.

---

## 1. 검증 방법론 (Verification Method)

### 1.1 핵심 원칙: 직접 비교

**동일한 입력 → 양쪽 실행 → 출력 직접 비교**

```
          동일한 입력 (랜덤 시드 고정)
                    │
          ┌─────────┴─────────┐
          ▼                   ▼
    ┌──────────┐       ┌──────────┐
    │ Verilator│       │ hirct-gen│
    │ RTL 모델 │       │ C++ 모델 │
    └────┬─────┘       └────┬─────┘
         │                  │
         ▼                  ▼
      rtl.io_out        model.io_out
         │                  │
         └──────┬───────────┘
                ▼
          assert(==)
          PASS or FAIL
```

**하나의 비교 드라이버**가 양쪽 모델을 동시에 실행하고 매 사이클 직접 비교합니다.
CSV 파일 생성, 별도 diff 도구 모두 불필요합니다.

### 1.2 비교 드라이버 패턴

```cpp
#include "VModuleName.h"       // Verilator RTL 모델
#include "ModuleName.h"        // hirct-gen C++ 모델

int main(int argc, char** argv) {
    int num_cycles = 1000;
    unsigned seed = 42;
    if (argc > 1) num_cycles = atoi(argv[1]);
    if (argc > 2) seed = atoi(argv[2]);

    VModuleName rtl;
    ModuleName  model;
    
    // 리셋 (모든 state 정의 상태로 진입)
    rtl.reset = 1; rtl.clock = 0; rtl.eval();
    rtl.clock = 1; rtl.eval(); rtl.reset = 0;
    model.do_reset();
    
    srand(seed);
    int pass = 0, fail = 0;
    
    for (int cyc = 0; cyc < num_cycles; cyc++) {
        // 1. 랜덤 입력 (제약 함수 적용)
        apply_random_inputs(rtl, model);  // 모듈별 제약 포함
        
        // 2. 양쪽 실행
        rtl.clock = 0; rtl.eval();
        rtl.clock = 1; rtl.eval();
        model.step();
        
        // 3. 직접 비교
        if (rtl.io_out != model.io_out) {
            printf("FAIL cycle %d seed %u: RTL=0x%x Model=0x%x\n",
                   cyc, seed, rtl.io_out, model.io_out);
            fail++;
            if (fail >= 10) break;  // 10개 초과 시 중단
        } else {
            pass++;
        }
    }
    printf("%s: %d pass, %d fail (seed=%u)\n",
           fail ? "FAIL" : "PASS", pass, fail, seed);
    return fail > 0 ? 1 : 0;
}
```

### 1.3 mismatch 디버깅

mismatch가 발생한 경우에만 VCD를 활용합니다:

```cpp
VerilatedVcdC* vcd = nullptr;
if (dump_vcd) {
    Verilated::traceEverOn(true);
    vcd = new VerilatedVcdC;
    rtl.trace(vcd, 99);
    vcd->open("debug.vcd");
}
```

| 도구 | 용도 | 사용 시점 |
|------|------|---------|
| 비교 드라이버 | 정확도 게이트 | **매번** |
| VCD + GTKWave | 불일치 원인 분석 | **mismatch 발생 시만** |

### 1.4 3자 비교 진단 (Phase 3)

Phase 3에서 VCS가 추가되면 3자 비교로 mismatch 원인을 특정합니다:

```
              동일한 입력 (랜덤 시드 고정)
                        │
          ┌─────────────┼─────────────┐
          ▼             ▼             ▼
    ┌──────────┐  ┌──────────┐  ┌──────────┐
    │ hirct-gen│  │ Verilator│  │   VCS    │
    │ C++ 모델 │  │ RTL 모델 │  │ RTL 모델 │
    └────┬─────┘  └────┬─────┘  └────┬─────┘
         ▼             ▼             ▼
      output_A      output_B      output_C
              \        |        /
               ▼       ▼       ▼
            3자 비교 → 원인 특정
```

**진단 매트릭스:**

| hirct-gen | Verilator | VCS | 진단 | 조치 |
|-----------|-----------|-----|------|------|
| = | = | = | 정상 | — |
| ≠ | = | = | **hirct-gen 버그** | 101 gen-model 수정 |
| = | ≠ | = | **Verilator 버그** | XFAIL + Verilator 이슈 리포트 |
| = | = | ≠ | **VCS 버그** (극히 드묾) | VCS 버전 확인 |
| ≠ | ≠ | = | 공통 가정 실패 | hirct-gen 모델 재검토 |

Phase 1~2에서 hirct-gen ≠ Verilator인 mismatch가 Phase 3에서 VCS로 교차 검증됩니다.

---

## 2. 클럭/리셋 규약 (Clock/Reset Convention)

### 2.1 신호 이름 패턴

| 유형 | 허용 패턴 | 극성 |
|------|---------|------|
| 클럭 | `clock`, `clk`, `CLK`, `*_clk`, `clk_*` | posedge |
| 리셋 (active-high) | `reset`, `rst`, `RST` | 1 = reset |
| 리셋 (active-low) | `reset_n`, `rst_n`, `RST_N`, `*_n` | 0 = reset |

### 2.2 리셋 처리

```cpp
bool is_active_low_reset(const std::string& name) {
    return name.find("_n") != std::string::npos ||
           name.find("_N") != std::string::npos;
}
```

### 2.3 클럭/리셋 포트는 랜덤 입력에서 제외

```cpp
bool should_randomize(const std::string& port_name) {
    static const std::vector<std::string> excluded = {
        "clock", "clk", "reset", "rst", "reset_n", "rst_n"
    };
    for (const auto& ex : excluded) {
        if (port_name.find(ex) != std::string::npos) return false;
    }
    return true;
}
```

### 2.4 입력 제약 (Constraint)

모듈에 따라 유효하지 않은 입력 조합이 있을 수 있습니다.
검증 드라이버에 모듈별 제약 함수를 둡니다:

```cpp
void apply_random_inputs(VModule& rtl, Module& model) {
    uint32_t val = rand();
    rtl.io_in = val;
    model.io_in = val;
    // 모듈별 제약 (필요시 추가)
}
```

### 2.5 2-state vs 4-state 규칙

| 상황 | 도구 | state | 규칙 |
|------|------|-------|------|
| Phase 0~2 | Verilator | 2-state | X 없음, 비교 직접 가능 |
| Phase 3 (301) | VCS | 4-state | X 가능, 아래 규칙 적용 |

**VCS 4-state 처리:**
- 리셋 구간(초기 N 사이클)의 X는 허용 (N은 모듈별 설정, 기본값 = 리셋 사이클 수)
- 리셋 완료 후 X 발생 → **즉시 Fail**
- 모든 X 발생을 Warning 로그에 기록 (사이클, 포트, 값)

### 2.6 게이트 기준: 다중 시드

```bash
for seed in 42 123 456 789 1024 2048 4096 8192 16384 32768; do
  ./verify_module 1000 $seed || exit 1
done
echo "ALL SEEDS PASS"
```

### 2.7 다중 클럭 도메인 정책

Phase 1에서는 **단일 클럭 도메인만 지원**한다.

| 조건 | 동작 |
|------|------|
| 클럭 포트 1개 | 정상 처리 |
| 클럭 포트 0개 (순수 조합 로직) | 아래 §2.8 참조 |
| 클럭 포트 2개+ | **Error** + meta.json `"fail"` + `known-limitations.md`에 등록 |

**감지 방법**: ModuleAnalyzer가 `seq.firreg`/`seq.compreg` op의 `clock` 피연산자를 수집하여 고유 클럭 수를 계산한다. 고유 클럭이 2개 이상이면 다중 클럭 도메인으로 판정한다.

Phase 2에서 실제 다중 클럭 모듈이 발견되면 대응 방안을 결정한다.

### 2.8 순수 조합 로직 모듈 (Combinational-Only)

클럭/리셋 포트가 없고 `seq.firreg`/`seq.compreg`도 없는 순수 조합 로직 모듈:

| 항목 | 동작 |
|------|------|
| GenModel | `step()` 대신 `eval_comb()`만 생성. `do_reset()`은 no-op |
| GenVerify | 클럭 toggle 없이 입력 변경 → `eval()` → 즉시 출력 비교 (1000회 반복) |
| GenTB | 클럭/리셋 없는 테스트벤치 골격 생성 |
| 판정 | ModuleAnalyzer의 `hasRegisters()` == false && 클럭 포트 == 0 |

### 2.9 Inout 포트 처리

Phase 1에서는 **inout 포트를 지원하지 않는다**.

| 조건 | 동작 |
|------|------|
| inout 포트 존재 | **Error** + meta.json `"fail"`, reason: `"inout ports not supported"` |
| inout 포트 없음 | 정상 처리 |

inout 포트는 `known-limitations.md`에 `inout_port` 카테고리로 등록할 수 있다.

### 2.10 GenWrapper 접두사 그룹핑 알고리즘

GenWrapper와 GenFormat이 사용하는 포트 그룹핑 알고리즘:

**알고리즘: Trie 기반 2단계 접두사 추출**

```
입력: 포트 이름 목록 (clock/reset 제외)
출력: {접두사 → [포트 목록]} 매핑

1. 각 포트 이름을 '_'로 분할: "io_plic_valid" → ["io", "plic", "valid"]
2. 분할된 토큰으로 Trie 구축
3. Trie를 DFS 순회하며 "분기점" (자식 2개 이상) 탐색
4. 분기점까지의 경로 = 접두사: "io_plic_" (자식: valid, ready, complete)
5. 최소 그룹 크기 = 2 (1개짜리는 "ungrouped"로 분류)
6. 전체 포트가 동일 접두사면 한 단계 더 진입하여 다음 분기점 탐색
```

**예시** (LevelGateway):

| 포트 | 접두사 | 그룹 |
|------|--------|------|
| io_interrupt | io_ | io (ungrouped — 그룹 내 1개) |
| io_plic_valid | io_plic_ | io_plic |
| io_plic_ready | io_plic_ | io_plic |
| io_plic_complete | io_plic_ | io_plic |

결과: `io_plic` 인터페이스 (3포트) + `io_interrupt` 개별 포트

**엣지 케이스**:

| 조건 | 동작 |
|------|------|
| 포트 이름에 '_' 없음 | ungrouped로 분류 |
| 전체 포트가 동일 접두사 | 1단계 더 진입 (io_ → io_plic_, io_interrupt_) |
| 최소 그룹 크기 미달 (1개) | ungrouped로 분류 |
| 접두사 깊이 3 이상 | 2단계까지만 탐색 (과도한 분할 방지) |

### 2.10.1 Combinational Loop 처리 정책

Combinational loop(조합 논리 순환 참조)은 **설계 버그**이다. hirct-gen이 자동으로 수정할 수 없다.

**정의**: clock/register를 거치지 않고 신호가 자기 자신에게 되돌아오는 경로.

```
# 정상 (레지스터 피드백 — 순환이 아님):
%9 = comb.mux %7, %6, %inFlight : i1
%inFlight = seq.firreg %9 clock %8 ...   ← register가 끊어줌

# 비정상 (combinational loop):
%a = comb.and %b, %input : i1
%b = comb.or %a, %other : i1            ← %a와 %b가 서로 참조, register 없음
```

**감지 방법**: Kahn's Algorithm (§100-bootstrap.md)에서 seq.firreg/seq.compreg를 절단점으로 처리한 후에도 미방문 노드가 남으면 combinational loop.

**처리**:

| 단계 | 동작 |
|------|------|
| 감지 | Kahn's Algorithm 정렬 후 미방문 노드 존재 |
| 보고 | stderr: `ERROR: combinational loop detected: %a → %b → %a` |
| 기록 | meta.json: `"combinational_loop": true`, emitter `"fail"` |
| 분류 | known-limitations.md에 `combinational_loop` 카테고리로 등록 가능 |
| 해결 | **설계자가 원본 RTL에서 루프를 끊어야 함** (레지스터 삽입 등) |

> **왜 자동 수정이 불가능한가**: 레지스터를 삽입하면 1 클럭 지연이 추가되어 원래 설계의 타이밍과 기능이 변경된다.
> CIRCT도 대부분의 combinational loop을 컴파일 시점에 거부하지만, 예외적으로 통과할 수 있으므로 방어적 처리가 필요하다.

### 2.11 hw.module 파라미터 처리

**실측 결과**: CIRCT(`circt-verilog`)는 elaboration 시점에 모든 Verilog `parameter`를 상수로 치환한다.

```
// 원본 Verilog
parameter RESET_VALUE = 0;

// MLIR 출력 (파라미터가 상수로 치환됨)
hw.module @AsyncResetReg(in %d : i1, out q : i1, ...) {
  %false = hw.constant false     ← RESET_VALUE=0이 false로 치환
  ...
}
```

**결론**: hirct-gen이 보는 MLIR에는 파라미터 선언이 없다. 따라서:

| 항목 | 영향 | 대응 |
|------|------|------|
| GenModel | 없음 | 모든 값이 상수로 확정됨 — 추가 처리 불필요 |
| GenDoc | 경미 | 원본 .v에서 `parameter` 라인을 별도 파싱하여 문서에 포함 (선택적) |
| GenWrapper | 없음 | 포트 타입이 확정됨 |
| filelist 모드 (`-f`) | 주의 | 다중 파일에서 동일 모듈을 다른 파라미터로 인스턴스화한 경우, elaboration 결과가 하나로 통합됨 |

**Phase 1 정책**: 파라미터는 CIRCT의 Slang 프론트엔드가 elaboration 시점에 **무조건** 상수로 치환한다. hirct-gen에서 별도 처리 불필요.

> **파라미터가 다른 인스턴스**: 동일 모듈을 다른 파라미터로 인스턴스화하면,
> CIRCT가 별도 모듈(`@Module_P0`, `@Module_P1`)로 분리한다.
> hirct-gen은 각각을 독립 모듈로 처리하므로 추가 로직 불필요.

GenDoc에서 원본 .v의 `parameter` 선언을 추출하여 문서에 포함하는 것은 Phase 2 이후 선택 사항.

### 2.12 GenRAL 주소 할당 전략

MLIR에는 메모리 맵(register address) 정보가 없다. `seq.firreg`로 레지스터 존재를 알 수 있지만 CSR 주소는 IR에 인코딩되지 않는다.

**핵심 원칙: offset 기반 — base address는 외부에서 주입**

GenRAL은 항상 **상대 offset**만 생성한다. 절대 주소(base address)는 사용 시점에서 결정된다.

**Phase 1 정책: 순차 자동 할당 (offset)**

```
레지스터 발견 순서 × 4바이트 = offset
  seq.firreg %reg0 → offset 0x00
  seq.firreg %reg1 → offset 0x04
  seq.firreg %reg2 → offset 0x08
  ...
```

| 항목 | 정책 |
|------|------|
| offset 간격 | 4바이트 고정 (32비트 레지스터 가정) |
| 할당 순서 | MLIR에서 `seq.firreg`/`seq.compreg`가 나타나는 순서 |
| 64비트 레지스터 | Phase 1: 2개 연속 슬롯(0x00, 0x04) 할당. Phase 2에서 개선 |
| 주소 오버라이드 | Phase 1: 미지원. Phase 2: 어노테이션 기반 (`(* hirct_addr = 0x100 *)`) |

**IP 테스트 vs Chip-Top 테스트:**

| 시나리오 | base address | 사용 방법 |
|---------|-------------|----------|
| **IP 단위 테스트** | 사용자가 직접 지정 (예: `0x0`) | `ip_read(IP_BASE, REG_OFFSET)` |
| **Chip-Top 통합** | SoC 메모리 맵에서 결정 | `ip_read(SOC_UART_BASE, REG_OFFSET)` |
| **filelist 모드 (`-f --top`)** | Phase 2: Top의 주소 디코더에서 추출 | 자동 base 할당 (Phase 2) |

**HAL 헤더 생성 규칙**:

```c
// AUTO-GENERATED by hirct-gen — DO NOT EDIT
#ifndef UART_HAL_H
#define UART_HAL_H

#include <stdint.h>

// Register offsets (base address는 사용 시점에서 주입)
#define UART_REG_CTRL_OFFSET    0x00
#define UART_REG_STATUS_OFFSET  0x04
#define UART_REG_DATA_OFFSET    0x08

// Access helpers (base를 인자로 받음)
static inline uint32_t uart_read(volatile void* base, uint32_t offset) {
    return *(volatile uint32_t*)((uint8_t*)base + offset);
}
static inline void uart_write(volatile void* base, uint32_t offset, uint32_t val) {
    *(volatile uint32_t*)((uint8_t*)base + offset) = val;
}

// Convenience macros (base 필요)
#define UART_READ_CTRL(base)        uart_read(base, UART_REG_CTRL_OFFSET)
#define UART_WRITE_CTRL(base, val)  uart_write(base, UART_REG_CTRL_OFFSET, val)

#endif
```

> **한계**: Phase 1의 순차 자동 할당은 실제 메모리 맵과 일치하지 않을 수 있다.
> Phase 2에서 어노테이션 기반 offset 지정(`(* hirct_offset = 0x100 *)`) 또는
> 별도 memory map 파일(`.map`)을 도입하여 해소한다.

---

## 3. 비트폭 처리 규약 (Bit Width Convention)

### 3.1 타입 매핑

| IR 비트폭 | C++ 타입 |
|----------|----------|
| 1 | `bool` |
| 2~8 | `uint8_t` |
| 9~16 | `uint16_t` |
| 17~32 | `uint32_t` |
| 33~64 | `uint64_t` |
| 65+ | Phase 1: **Error** (미지원 에러 + 스킵), Phase 2: **Verilator 호환 `uint32_t[]` 래퍼** |

### 3.2 64비트 초과 처리

Phase 1에서 65비트 이상 신호를 만나면 Error를 출력하고 해당 모듈을 스킵합니다.
Phase 2에서 실제 65+ 비트 신호가 발견되면 Verilator의 `VlWide<N>` (`uint32_t[]` 배열) 방식과 호환되는 경량 래퍼를 도입합니다.
hirct-verify가 Verilator 모델과 직접 비교하므로, 타입을 일치시켜 변환 비용을 제거합니다.

---

## 4. 산출물 테스트 규약 (Artifact Testing Convention)

> v3.0 추가 섹션

### 4.1 이중 게이트 원칙

각 Phase의 게이트는 **두 가지를 동시에 확인**합니다:
1. **C++ 모델 정확도:** Verilator 직접 비교 PASS
2. **산출물 컴파일:** 생성된 모든 산출물이 컴파일 성공

### 4.2 산출물별 테스트 기준

| 산출물 | 테스트 명령 | PASS 기준 |
|--------|-----------|---------|
| C++ Model | `verify_<module> 1000 <seed>` | 10시드 × 1000cyc 전체 PASS |
| SV Wrapper | `verilator --lint-only <wrapper>.sv <original>.v` | exit 0 (lint 통과) |
| UVM RAL | `vcs -sverilog +incdir+$UVM_HOME/src <ral>.sv` (또는 lint) | exit 0 |
| DPI-C | `g++ -std=c++17 -c <dpi>.cpp` | exit 0 |
| Testbench | `verilator --lint-only <tb>.sv <original>.v` | exit 0 |
| HW Doc + Prog Guide | `[ -s <doc>.md ]` (비어있지 않음) | 파일 크기 > 0 |

### 4.3 자동화 타겟

**per-module Makefile** (`GenMakefile.cpp`이 자동 생성, `output/<path>/<module>/Makefile`):

```makefile
make test-compile     # cmodel 컴파일 확인 (g++ -c)
make test-verify      # cmodel vs Verilator 등가성 검증 (10시드 × 1000cyc)
make test-artifacts   # 나머지 산출물 컴파일/lint 확인
make test             # 위 3개 전부
```

**루트 Makefile** (프로젝트 루트):

```makefile
make check-hirct          # lit test/ (emitter FileCheck)
make check-hirct-unit     # gtest (C++ API 단위 테스트)
make check-hirct-integration  # lit integration_test/smoke/ (E2E)
make test-all             # 위 전체 + report
make test-traversal       # Phase 2 전체 순회 (~1600, CI 제외)
```

### 4.3.1 VCS 옵셔널 분리

VCS/ncsim 라이선스가 없는 환경에서도 `make test-all`이 Green을 유지해야 한다:

```makefile
# 루트 Makefile
HAVE_VCS ?= $(shell which vcs >/dev/null 2>&1 && echo 1 || echo 0)
```

| 타겟 | HAVE_VCS=0 | HAVE_VCS=1 |
|------|-----------|-----------|
| `make test-artifacts` (per-module) | Verilator lint만 | + VCS 컴파일 |
| `make check-hirct-integration` | Verilator 기반 | + VCS co-sim |
| GenRAL UVM 게이트 | `verilator --lint-only` | + `vcs -sverilog` |

**원칙**: 기본 게이트(Verilator only)만으로 Phase 1~2를 완료할 수 있다. VCS는 추가 확신을 제공하는 보강 게이트이다.

### 4.3.2 내부 프로파일링 인프라 (Profiling Infrastructure)

hirct-gen은 변환 시간 계측과 timeout 관리를 위한 내부 프로파일링 인프라를 갖춘다.

**설계 원칙**: 엄격한 성능 목표를 Phase 1에서 설정하지 않는다. 대신 **계측 인프라**를 구축하여 Phase 2에서 실측 기반으로 최적화한다.

**Phase 1 필수 인프라**:

| 인프라 | 설명 | 구현 위치 |
|--------|------|----------|
| **단계별 타이밍** | MLIR 파싱, 분석, emitter별 소요 시간 측정 | `tools/hirct-gen/main.cpp` |
| **per-module 타이밍** | 모듈별 총 소요 시간을 `meta.json`에 기록 | `meta.json: "elapsed_ms": 1234` |
| **slow 모듈 경고** | 30초 초과 시 stderr에 `WARN: slow module <name> (<N>s)` | 기본 threshold = 30s |
| **timeout** | 외부 프로세스(circt-verilog, circt-opt, verilator) 개별 timeout | `CirctRunner::setTimeout()` |
| **--verbose 타이밍** | `hirct-gen --verbose` 시 단계별 소요 시간 출력 | CLI 옵션 |

**meta.json 타이밍 필드** (Phase 1 추가):

```json
{
  "elapsed_ms": 1234,
  "timing": {
    "parse_ms": 50,
    "analyze_ms": 30,
    "emit_ms": {
      "gen-model": 200,
      "gen-tb": 50,
      "gen-doc": 30
    }
  }
}
```

**timeout 정책**:

| 대상 | 기본 timeout | 초과 시 동작 |
|------|-------------|------------|
| `circt-verilog` (MLIR 변환) | 60초 | Error + meta.json `"mlir": "fail"`, reason: `"timeout"` |
| `circt-opt` (flatten 등) | 60초 | Error + 해당 pass 스킵 |
| emitter 단일 모듈 | 120초 | Error + meta.json emitter `"fail"`, reason: `"timeout"` |
| `verilator --cc` (빌드) | 300초 | Error + verify 스킵 |
| lit 테스트 (per-module) | 300초 (LIT_TIMEOUT) | lit TIMEOUT 분류 |

**Phase 2 최적화 TODO**:

- [ ] 1,600 파일 순회 후 실측 데이터 수집 (소요 시간 분포)
- [ ] 병목 식별: 파싱 vs 분석 vs emitter 중 어디가 느린지 분석
- [ ] 대형 모듈(2,000줄+) 최적화: 파서 성능, 메모리 사용량
- [ ] 병렬 emitter 실행 검토 (현재 순차 → 독립 emitter 병렬화)
- [ ] CirctRunner 캐시: 동일 MLIR 재파싱 방지 (변경 감지 기반)
- [ ] Verilator obj_dir 캐시: 이미 구현된 캐시 규칙의 효과 측정
- [ ] `--fast` 모드: GenModel + GenMakefile만 생성하는 단축 경로
- [ ] 전체 순회 목표 시간 설정 (실측 후 결정)

### 4.4 생성 코드 표준 헤더

모든 생성 파일 첫 줄에 다음 헤더를 삽입합니다:

| 언어 | 헤더 |
|------|------|
| C/C++ | `// AUTO-GENERATED by hirct-gen — DO NOT EDIT` |
| SystemVerilog | `// AUTO-GENERATED by hirct-gen — DO NOT EDIT` |
| Python | `# AUTO-GENERATED by hirct-gen — DO NOT EDIT` |
| Makefile | `# AUTO-GENERATED by hirct-gen — DO NOT EDIT` |

**효과**: 수동 편집 방지, lint 도구 자동 제외, git diff에서 생성 코드 식별.

### 4.5 meta.json (per-module 메타데이터)

**위치**: `output/<path>/<file>/meta.json`

hirct-gen이 각 모듈 처리 시 **항상** 생성합니다.
부분 실패(일부 emitter만 실패)해도 항상 기록합니다 — emitter 루프의 try/catch 후 메타를 기록하는 구조이므로, meta.json이 없으면 infra-error로 간주합니다.

**최소 스키마**:

```json
{
  "path": "rtl/.../foo.v",
  "top": "ModuleName",
  "mlir": "pass | fail",
  "reason": "",
  "emitters": {
    "gen-model": { "result": "pass | fail | skipped", "reason": "" },
    "gen-ral":   { "result": "skipped", "detection": "none", "reason": "no register indicators" }
  }
}
```

**필드 설명**:

| 필드 | 설명 |
|------|------|
| `path` | 입력 RTL 파일의 상대 경로 |
| `top` | 최상위 모듈 이름 |
| `mlir` | CIRCT MLIR 변환 결과 (`pass` / `fail`) |
| `reason` | MLIR 변환 실패 사유 (`mlir: "fail"` 시 **필수**, `pass` 시 생략 또는 빈 문자열). §4.6 reason 접두사 표준 적용. |
| `emitters.<name>.result` | 각 emitter 결과 (`pass` / `fail` / `skipped`) |
| `emitters.<name>.reason` | 실패 또는 스킵 사유 (빈 문자열 = 성공) |

**`reason` (최상위) vs `emitters.<name>.reason` 구분**:
- 최상위 `reason`: MLIR 변환 단계 실패 사유 (파싱/타임아웃/다중 모듈/flatten 실패 등). `mlir: "fail"` 시에만 의미가 있다.
- `emitters.<name>.reason`: MLIR은 성공했지만 특정 emitter가 실패/스킵된 사유.
- triage 1차 분기: `mlir`이 `"fail"`이면 최상위 `reason`을 보고 분류하고, `"pass"`이면 `emitters`를 순회하며 분류한다.

**GenRAL 전용 필드**: `detection` — 레지스터 탐지 방식을 기록합니다.

| detection 값 | 의미 |
|---|---|
| `annotation` | RTL 어노테이션으로 탐지 |
| `ir_pattern` | IR 패턴 매칭으로 탐지 |
| `port_heuristic` | 포트 이름 휴리스틱으로 탐지 |
| `none` | 레지스터 표지 미발견 (skipped) |

---

### 4.6 meta.json 확장 선택 키 (Agent Triage 지원)

Agent 자동 분류(Phase 2 `206-agent-triage.md`)를 지원하기 위한 선택 키:

| 키 | 타입 | 설명 | 생성 조건 |
|----|------|------|----------|
| `unsupported_ops` | string[] | 미지원 op 목록 | GenModel fail 시 |
| `combinational_loop` | bool | 조합 루프 감지 여부 | Kahn 정렬 실패 시 |
| `elapsed_ms` | int | 총 처리 시간 (ms) | 항상 |
| `timing` | object | 단계별 소요 시간 (`parse_ms`, `analyze_ms`, `emit_ms`) | `--verbose` 또는 항상 |
| `tool_versions` | object | `{"circt": "5e760efa9", "verilator": "5.020"}` | 항상 |
| `stderr_tail` | string | 실패 시 stderr 마지막 5줄 | fail 시 |

**reason 필드 표준 접두사** (규칙 기반 triage 정확도를 위해):

| 접두사 | 예시 | 분류 매핑 |
|--------|------|----------|
| `unsupported op:` | `"unsupported op: seq.firmem"` | `unsupported_op` |
| `timeout:` | `"timeout: circt-verilog (60s)"` | `timeout` |
| `parse error:` | `"parse error: unknown module 'foo'"` | `parse_error` |
| `multiple modules:` | `"multiple modules: 3 found, --top required"` | `multi_module` |
| `flatten error:` | `"flatten error: hw-flatten-modules failed"` | `flatten_error` |
| `combinational loop:` | `"combinational loop: %a -> %b -> %a"` | `combinational_loop` |
| `inout port:` | `"inout ports not supported"` | `inout_port` |
| `multi clock:` | `"multiple clock domains (2)"` | `multi_clock` |
| `wide signal:` | `"65+ bit signal: %data (128 bits)"` | `wide_signal` |

---

## 5. 실패 분류 체계 (Failure Classification SSOT)

모든 Phase에서 일관된 실패 분류를 위한 단일 진실 공급원(SSOT):

| 분류 | 정의 | 기록 위치 | 판정 주체 | 조치 |
|------|------|----------|----------|------|
| **pass** | emitter 정상 완료 | `meta.json` emitters.<name>.result | hirct-gen | — |
| **fail** | emitter 실패 (수정 가능) | `meta.json` + `verify-report.json` | hirct-gen / hirct-verify | Phase 1 해당 태스크 되돌림 |
| **skipped** | 조건 미충족 (예: GenRAL 레지스터 없음) | `meta.json` emitters.<name>.result | hirct-gen | — (정상 동작) |
| **infra-error** | meta.json 누락 / 파싱 실패 / 도구 오류 | `report.json` | generate-report.py | 인프라 수정 |
| **xfail** | 알려진 제한 (수정 보류) | `known-limitations.md` | 수동 등록 | XFAIL 사유 문서화 |
| **xpass** | xfail 대상이 예상과 달리 통과 | lit 출력 (WARN) | lit | XFAIL 항목 재검토 |

**판정 흐름**:

```
hirct-gen 실행
  ├─ meta.json 생성 실패 → infra-error
  ├─ MLIR 변환 실패 → meta.json: mlir="fail"
  └─ emitter 루프
       ├─ 조건 미충족 → skipped
       ├─ emitter 실행 성공 → pass
       └─ emitter 실행 실패 → fail

hirct-verify 실행
  ├─ mismatch → verify-report.json: result="fail"
  └─ 전체 일치 → verify-report.json: result="pass"

Phase 2 순회
  ├─ known-limitations.md에 등록된 모듈 → xfail (lit에서 XFAIL 처리)
  └─ xfail 대상이 통과 → xpass (WARN, CI는 Green 유지)
```

---

### 5.1 lit XFAIL 연동 메커니즘

`known-limitations.md`의 Markdown 테이블을 lit에서 참조하여 XFAIL 판정하는 방법:

**파싱 로직** (`utils/parse_known_limitations.py`):
1. `known-limitations.md`의 `| Path | Category | Reason | Date |` 테이블을 파싱
2. `Path` 열을 set으로 수집 → `xfail_paths`
3. `Category` 열로 분류 가능 (unsupported_op, multi_module, parse_error, timeout 등)

**lit.cfg.py 연동**:
1. `lit.cfg.py`가 시작 시 `parse_known_limitations.load_xfail_paths()` 호출
2. 각 테스트의 입력 파일 경로(`%s`)가 `xfail_paths`에 포함되면 `xfail=True` 표기
3. XFAIL 모듈이 통과(XPASS)하면 WARN 출력 (CI Green 유지, 사람이 XFAIL 항목 재검토)

**구현 위치**:
- `utils/parse_known_limitations.py` — Markdown 파서 (Phase 2 Task 205에서 구현)
- `test/lit.cfg.py` — 단위 테스트용 (Phase 0 Task 001에서 스켈레톤)
- `integration_test/lit.cfg.py` — 통합 테스트용

---

## 6. 마이크로 스텝 템플릿 (Micro-Step Template)

**모든 태스크 문서는 이 형식을 따릅니다.**

### 6.1 스텝 형식

```markdown
### Step N: [1줄 설명] (예상: N분)

**Goal**: 이 스텝이 달성하려는 것 (1줄)

**Run**:
\`\`\`bash
단일_명령
\`\`\`

**Expect**:
\`\`\`
기대 결과 1줄
\`\`\`
```

### 6.2 스텝 분해 원칙

| 원칙 | 나쁜 예 | 좋은 예 |
|------|--------|--------|
| **1 스텝 = 1 검증** | "스크립트 작성" | "빈 스크립트 실행 → exit 0" |
| **점진적 확장** | "전체 기능 구현" | "컴파일만 → 1사이클 → 1000사이클" |
| **있는 것 활용** | "새 인프라 구축" | "기존 도구로 시도 → 실패 지점만 수정" |

---

## 7. 도구 목록

> **Note**: hirct-gen/hirct-verify 및 모든 C++ 소스는 Phase 1에서 처음부터 신규 작성한다.
> 아래 테이블에서 "Phase 1 신규"로 표시된 항목은 Task 100(Bootstrap) 이후 사용 가능하다.

| 도구 | 경로 | 상태 |
|------|------|------|
| hirct-gen | `build/tools/hirct-gen/hirct-gen` | Phase 1 신규 (Task 100+) |
| hirct-verify | `build/tools/hirct-verify/hirct-verify` | Phase 1 신규 (Task 100+) |
| GenModel | `lib/Target/GenModel.cpp` | Phase 1 신규 (Task 100 스켈레톤 + Task 101 완성) |
| GenDPIC | `lib/Target/GenDPIC.cpp` | Phase 1 신규 (Task 103) |
| GenTB | `lib/Target/GenTB.cpp` | Phase 1 신규 (Task 102) |
| GenDoc | `lib/Target/GenDoc.cpp` | Phase 1 신규 (Task 106) |
| ModuleAnalyzer | `lib/Analysis/ModuleAnalyzer.cpp` | Phase 1 신규 (Task 100) |
| Verilator | `/usr/bin/verilator` | 5.020 설치됨 |
| VCS | `/tools/synopsys/vcs/...` | 설치됨 |
| CIRCT | `$HOME/circt/build/bin/` | 설치됨 |

---

## 8. 문서 표기 규약 (Documentation Markers)

### 이모지 금지

문서(`.md` 파일)에서 상태 표시용 이모지 사용을 **금지**한다. 대신 텍스트 마커를 사용한다:

| 용도 | 금지 | 허용 |
|------|------|------|
| 통과/성공 | U+2705, U+2713 | `[V]` 또는 `PASS` |
| 실패 | U+274C, U+2717 | `[X]` 또는 `FAIL` |
| 경고 | U+26A0 | `[!]` 또는 `WARN` |
| 정보 | U+2139 | `[i]` 또는 `INFO` |
| 미해당 | -- | `N/A` |

**적용 범위**: 저장소 전체 (docs/, .cursor/, README 등)

**자동 변환**: `pre-commit` hook (`fix-emoji`)이 커밋 시 자동으로 이모지를 텍스트 마커로 변환한다.
로직은 `.pre-commit-config.yaml`에 인라인되어 있으며, 별도 스크립트 파일 없음.
수동 실행: `pre-commit run fix-emoji --all-files`

**근거**: 이모지는 터미널/diff 도구에서 깨지거나 폭이 불일치할 수 있고, grep/검색에 불편하다.

---

## 변경 이력

| 버전 | 날짜 | 변경 내용 |
|------|------|---------|
| 1.0 | 2026-02-15 | 초안 |
| 2.0 | 2026-02-15 | 직접 비교 방식, 기존 도구 목록, 과잉 제거 |
| 3.0 | 2026-02-15 | 산출물 테스트 규약 추가 (섹션 4), 이중 게이트 원칙 |
| 4.0 | 2026-02-15 | open-decisions 반영: 65+ Error, X 처리 강화, 3자 비교 진단, AUTO-GENERATED 헤더, EmitCHeader 삭제 |
| 5.0 | 2026-02-16 | v2 구조 반영: 루트 Makefile을 lit 기반 타겟으로 교체, meta.json per-module 메타데이터 규약 추가 (§4.5), 도구 목록을 CIRCT 스타일 경로로 갱신 (emitter rename, hirct-verify 추가, shell wrapper 제거) |
| 6.0 | 2026-02-16 | dry-run 반영: 도구 목록을 "전부 신규 작성"으로 수정, rename/merge 표현 제거 |
| 7.0 | 2026-02-16 | 리뷰 반영: §5 실패 분류 체계(Failure Classification SSOT) 추가, 기존 §5→§6, §6→§7로 재번호 |
| 8.0 | 2026-02-18 | 문서 표기 규약 추가 (§8): 이모지 금지, 텍스트 마커 사용, pre-commit fix-emoji hook |