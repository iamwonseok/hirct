# SoC Verilog/SystemVerilog Coding Convention Rules

> **Standard**: Verilog-2005 (IEEE 1364-2005), SystemVerilog-2017 (IEEE 1800-2017)
>
> **References**:
> - [Sutherland HDL Coding Guidelines](http://www.sutherland-hdl.com/online_verilog_ref/vlog_ref_top.html)
> - [Lowpower Design Guidelines (Synopsys)]
> - [RISC-V Coding Style Guide](https://github.com/chipsalliance/chisel-style-guide)

## 1. File Structure (V-01)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| V-01-01 | M | 하나의 파일에는 하나의 모듈만 정의한다. (testbench, package 제외) | No | - | - |
| V-01-02 | M | 파일명은 모듈명과 동일하게 작성한다: `my_module.v` → `module my_module` | No | - | - |
| V-01-03 | M | Verilog 소스는 `.v`, SystemVerilog 소스는 `.sv` 확장자를 사용한다. | No | - | - |
| V-01-04 | M | 파일의 줄바꿈 형식은 Unix 스타일(LF)을 사용한다. | Yes | pre-commit | mixed-line-ending |
| V-01-05 | M | 파일의 마지막에 빈 줄 하나를 추가한다. | Yes | pre-commit | end-of-file-fixer |
| V-01-06 | O | 파일 상단에 목적, 저자, 변경 이력을 기술하는 주석 블록을 포함한다. | No | - | - |
| V-01-07 | M | `timescale은 별도 파일이나 filelist에서 관리하며, 모듈 파일 안에 `timescale을 넣지 않는다. | No | - | - |

## 2. Formatting (V-02)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| V-02-01 | M | 들여쓰기는 공백(Space) 2칸을 사용한다. (탭 금지) | Yes | verible-verilog-format | - |
| V-02-02 | M | 줄의 끝(Trailing whitespace)에 공백 문자를 남기지 않는다. | Yes | pre-commit | trailing-whitespace |
| V-02-03 | O | 코드 한 줄은 최대 100자를 넘지 않도록 작성한다. | Yes | verible-verilog-lint | line-length |
| V-02-04 | M | begin/end 블록의 begin은 해당 구문과 같은 줄에 위치한다. | Yes | verible-verilog-format | - |
| V-02-05 | M | 연산자 양쪽에 공백을 하나씩 추가한다: `a = b + c;` | Yes | verible-verilog-format | - |
| V-02-06 | M | 콤마(,) 뒤에는 공백을 추가한다. | Yes | verible-verilog-format | - |
| V-02-07 | M | 포트 선언은 ANSI 스타일을 사용한다. (모듈 헤더에 타입+방향 포함) | No | - | - |
| V-02-08 | O | 포트 선언에서 타입/방향/이름의 열(column)을 정렬한다. | No | - | - |

```verilog
// Good: ANSI style, aligned
module my_module (
  input  wire        clk,
  input  wire        rst_n,
  input  wire [31:0] data_in,
  output reg  [31:0] data_out,
  output wire        valid
);

// Bad: non-ANSI style
module my_module (clk, rst_n, data_in, data_out, valid);
  input  clk;
  input  rst_n;
  input  [31:0] data_in;
  output [31:0] data_out;
  output valid;
```

## 3. Naming (V-03)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| V-03-01 | M | 모듈명, 신호명, 변수명은 snake_case를 사용한다. | Yes | verible-verilog-lint | naming |
| V-03-02 | M | 파라미터/localparams는 UPPER_SNAKE_CASE를 사용한다. | Yes | verible-verilog-lint | naming |
| V-03-03 | M | 클럭 신호에는 `clk` 또는 `clock` 접두사/이름을 사용한다. | No | - | - |
| V-03-04 | M | Active-low 리셋에는 `_n` 접미사를 사용한다: `rst_n`, `reset_n` | No | - | - |
| V-03-05 | M | Active-low 신호에는 일관되게 `_n` 접미사를 사용한다. | No | - | - |
| V-03-06 | O | 인터페이스 신호 그룹에는 공통 접두사를 사용한다: `axi_awaddr`, `axi_awvalid` | No | - | - |
| V-03-07 | M | 파이프라인 스테이지에는 `_d1`, `_d2` 또는 `_q`, `_qq` 접미사를 사용한다. | No | - | - |
| V-03-08 | M | 인스턴스명에는 `u_` 또는 `i_` 접두사를 사용한다: `u_fifo`, `i_arbiter` | No | - | - |
| V-03-09 | M | generate 블록에는 의미 있는 레이블을 붙인다. | Yes | verible-verilog-lint | generate-label |
| V-03-10 | O | 헝가리안 표기법(타입 접두어)을 사용하지 않는다. | No | - | - |

```verilog
// Good
parameter DATA_WIDTH = 32;
localparam ADDR_BITS = $clog2(DEPTH);

wire [DATA_WIDTH-1:0] data_in;
reg  [DATA_WIDTH-1:0] data_out_d1;  // pipeline stage 1
reg  [DATA_WIDTH-1:0] data_out_d2;  // pipeline stage 2

my_fifo #(.DEPTH(16)) u_tx_fifo (
  .clk     (clk),
  .rst_n   (rst_n),
  .wr_data (tx_data),
  .rd_data (tx_fifo_out)
);

genvar gi;
generate
  for (gi = 0; gi < NUM_CHANNELS; gi = gi + 1) begin : gen_channel
    channel u_channel (.clk(clk), ...);
  end
endgenerate
```

## 4. Module Structure (V-04)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| V-04-01 | M | 모듈 내부 구성 순서: 파라미터 → 포트 → 내부 신호 → 인스턴스 → 조합 로직 → 순차 로직 | No | - | - |
| V-04-02 | O | 각 섹션을 주석 블록으로 구분한다. | No | - | - |
| V-04-03 | M | 하나의 always 블록에는 하나의 관심사만 다룬다. | No | - | - |
| V-04-04 | M | 순차 로직과 조합 로직을 하나의 always 블록에 섞지 않는다. | No | - | - |
| V-04-05 | M | 포트 연결은 이름 기반(named association)을 사용한다. 위치 기반(positional) 금지. | Yes | verible-verilog-lint | - |

```verilog
module my_module #(
  parameter DATA_WIDTH = 32,
  parameter DEPTH      = 16
) (
  input  wire                  clk,
  input  wire                  rst_n,
  input  wire [DATA_WIDTH-1:0] data_in,
  output reg  [DATA_WIDTH-1:0] data_out
);

  // ============================================================
  // Parameters / Localparams
  // ============================================================
  localparam ADDR_BITS = $clog2(DEPTH);

  // ============================================================
  // Internal Signals
  // ============================================================
  wire [DATA_WIDTH-1:0] internal_data;
  reg  [ADDR_BITS-1:0]  wr_ptr;

  // ============================================================
  // Sub-module Instances
  // ============================================================
  my_fifo #(.DEPTH(DEPTH)) u_fifo (
    .clk     (clk),
    .rst_n   (rst_n),
    .wr_data (data_in),
    .rd_data (internal_data)
  );

  // ============================================================
  // Combinational Logic
  // ============================================================
  assign data_out_next = internal_data;

  // ============================================================
  // Sequential Logic
  // ============================================================
  always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      data_out <= {DATA_WIDTH{1'b0}};
    end else begin
      data_out <= data_out_next;
    end
  end

endmodule
```

## 5. Coding Style (V-05)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| V-05-01 | M | 순차 로직에는 non-blocking 할당(`<=`)만 사용한다. | Yes | verible-verilog-lint | blocking-assignment-in-always-ff |
| V-05-02 | M | 조합 로직에는 blocking 할당(`=`)만 사용한다. | Yes | verible-verilog-lint | - |
| V-05-03 | M | 하나의 always 블록에서 blocking과 non-blocking을 혼용하지 않는다. | Yes | verible-verilog-lint | - |
| V-05-04 | M | 조합 로직 always 블록은 `always @(*)` 또는 `always_comb`을 사용한다. | Yes | verible-verilog-lint | always-comb |
| V-05-05 | M | 순차 로직 always 블록은 `always @(posedge clk)` 또는 `always_ff`를 사용한다. | No | - | - |
| V-05-06 | M | Latch 생성을 방지한다: 조합 로직 always 블록에서 모든 경로에 출력을 할당한다. | Yes | Lint tools | latch-inferred |
| V-05-07 | M | if-else 체인에서 마지막 else를 반드시 작성한다. (조합 로직) | No | - | - |
| V-05-08 | M | case 문에서 default를 반드시 작성한다. | Yes | verible-verilog-lint | case-missing-default |
| V-05-09 | M | 비트폭 불일치를 피한다. 리터럴에 비트폭을 명시한다: `8'd0` 대신 `{WIDTH{1'b0}}` | Yes | verilator | WIDTH |
| V-05-10 | M | 정수 리터럴에 비트폭과 진법을 명시한다: `32'hDEAD_BEEF` | No | - | - |
| V-05-11 | O | 긴 숫자에 밑줄(_)을 사용하여 가독성을 높인다: `32'hDEAD_BEEF` | No | - | - |
| V-05-12 | M | tri-state 버퍼는 Top-level I/O에서만 사용하고, 내부 로직에서는 사용하지 않는다. | No | - | - |

```verilog
// Good: separate combinational and sequential
always @(*) begin  // or always_comb
  next_state = current_state;  // default assignment (latch 방지)
  case (current_state)
    IDLE:    if (start) next_state = RUNNING;
    RUNNING: if (done)  next_state = IDLE;
    default: next_state = IDLE;
  endcase
end

always @(posedge clk or negedge rst_n) begin  // or always_ff
  if (!rst_n) begin
    current_state <= IDLE;
  end else begin
    current_state <= next_state;
  end
end

// Bad: mixed blocking/non-blocking, inferred latch
always @(posedge clk) begin
  if (enable) begin
    data = data_in;    // blocking in sequential - BAD
    data_out <= data;  // mixed assignment - BAD
  end
  // missing else → latch on data_out - BAD
end
```

## 6. Reset Convention (V-06)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| V-06-01 | M | 비동기 리셋은 sensitivity list에 포함한다: `always @(posedge clk or negedge rst_n)` | No | - | - |
| V-06-02 | M | 리셋 조건은 always 블록의 첫 번째 조건으로 작성한다. | No | - | - |
| V-06-03 | M | 리셋 시 모든 레지스터를 명시적으로 초기화한다. | No | - | - |
| V-06-04 | O | 프로젝트 내에서 리셋 극성(active-high/active-low)을 통일한다. | No | - | - |

```verilog
// Good: async active-low reset
always @(posedge clk or negedge rst_n) begin
  if (!rst_n) begin
    counter <= '0;        // 명시적 초기화
    state   <= IDLE;
  end else begin
    counter <= counter + 1'b1;
    state   <= next_state;
  end
end
```

## 7. Parameterization (V-07)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| V-07-01 | M | 매직 넘버 대신 parameter/localparam을 사용한다. | No | - | - |
| V-07-02 | M | 인터페이스 관련 값(비트폭, 깊이 등)은 parameter로, 내부 상수는 localparam으로 정의한다. | No | - | - |
| V-07-03 | M | parameter 기본값을 반드시 지정한다. | No | - | - |
| V-07-04 | O | 파생 상수는 localparam으로 자동 계산한다: `localparam ADDR_W = $clog2(DEPTH);` | No | - | - |

## 8. Synthesis Safety (V-08)

| ID | Category | Rule Description | Detectable | Tool | Error Code |
|----|----------|------------------|------------|------|------------|
| V-08-01 | M | `initial` 블록은 시뮬레이션 전용(testbench)에서만 사용한다. 합성 코드 금지. | No | - | - |
| V-08-02 | M | `#delay`는 합성 코드에서 사용하지 않는다. | Yes | Lint tools | delay-in-synthesis |
| V-08-03 | M | `force`/`release`는 합성 코드에서 사용하지 않는다. | No | - | - |
| V-08-04 | M | Full case/parallel case 지시자(`// synopsys full_case`) 대신 default를 명시한다. | No | - | - |
| V-08-05 | M | 클럭 게이팅은 전용 셀(ICG)을 통해서만 수행한다. 로직으로 클럭을 직접 게이팅하지 않는다. | No | - | - |

## 9. Comments (V-09)

| ID | Category | Rule Description |
|----|----------|------------------|
| V-09-01 | M | 모듈 상단에 모듈 목적, 주요 포트 설명, 동작 개요를 기술한다. |
| V-09-02 | O | 모듈 내부를 논리적 섹션(Parameters, Signals, Instances, Comb, Seq)으로 나눠 주석 블록을 단다. |
| V-09-03 | M | 주석은 `//`를 사용한다. `/* */`는 대규모 블록 비활성화에만 사용한다. |
| V-09-04 | O | TODO/FIXME 주석에는 담당자와 이슈 번호를 명시한다: `// TODO(wonseok): #123 ...` |
| V-09-05 | M | 모든 주석과 식별자는 영어를 사용한다. |

---

## Tools

| Tool | Purpose | Installation |
|------|---------|--------------|
| verible-verilog-lint | Verilog/SV 린터 | `apt install verible` 또는 GitHub releases |
| verible-verilog-format | Verilog/SV 포매터 | 동일 |
| verilator --lint-only | 빠른 lint 확인 | `apt install verilator` |
| Slang (slang-lint) | SV 2017 완전 지원 린터 | CIRCT 빌드 시 포함 |

### Usage Example

```bash
# Lint
verible-verilog-lint --rules=-line-length my_module.v

# Format
verible-verilog-format --inplace my_module.v

# Quick lint with Verilator
verilator --lint-only -Wall my_module.v
```

---

## Legend

- **M (Mandatory)**: 필수 준수 규칙
- **O (Optional)**: 권장 규칙
