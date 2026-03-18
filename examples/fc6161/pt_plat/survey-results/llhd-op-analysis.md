# LLHD/CF 잔존 Op 전수 분석 보고서

> Task 2-1 산출물 | 생성일: 2026-03-02
> MLIR dump 소스: `circt-verilog --ir-hw` (MooreToCore + LlhdToCore pipeline)

## 1. 타겟별 잔존 Op 통계

### 1.1 총괄 테이블

| Op | uart | gpio | wdt | ptimer | 합계 | Result |
|---|---:|---:|---:|---:|---:|---|
| `llhd.constant_time` | 10 | 5 | 3 | 2 | **20** | 1 (time) |
| `llhd.sig` | 23 | 13 | 5 | 2 | **43** | 1 (signal ref) |
| `llhd.prb` | 23 | 13 | 5 | 2 | **43** | 1 (probed value) |
| `llhd.process` (result) | 23 | 12 | 5 | 2 | **42** | N (value+valid pairs) |
| `llhd.process` (no-result) | 8 | 2 | 0 | 0 | **10** | 0 |
| `llhd.drv` (conditional) | 23 | 12 | 5 | 2 | **42** | 0 (side-effect) |
| `llhd.drv` (unconditional) | 0 | 32 | 0 | 0 | **32** | 0 (side-effect) |
| `llhd.wait` | 23 | 12 | 5 | 2 | **42** | 0 (terminator) |
| `llhd.halt` | 8 | 2 | 0 | 0 | **10** | 0 (terminator) |
| `cf.br` | 66 | 51 | 8 | 4 | **129** | 0 (terminator) |
| `cf.cond_br` | 150 | 28 | 11 | 18 | **207** | 0 (terminator) |
| `sim.fmt.literal` | 8 | 2 | 0 | 0 | **10** | 1 (format string) |
| `sim.proc.print` | 8 | 2 | 0 | 0 | **10** | 0 (side-effect) |
| **합계** | **373** | **186** | **47** | **34** | **640** | |

### 1.2 타겟별 모듈 구성

| Target | hw.module 수 | llhd 포함 모듈 수 | MLIR 라인 수 |
|---|---:|---:|---:|
| uart | 39 | ~18 | 2,356 |
| gpio | 10 | ~6 | 1,376 |
| wdt | 12 | ~3 | 325 |
| ptimer | 6 | ~2 | 591 |

## 2. Op별 상세 분석

### 2.1 `llhd.constant_time` — 시간 상수

**형태**: `%0 = llhd.constant_time <ns, delta, epsilon>`

**관측된 값 2종**:
- `<0ns, 1d, 0e>` — 1-delta delay (combinational latch/process에서 사용)
- `<0ns, 0d, 1e>` — 1-epsilon delay (sequential register process에서 사용)

**분포**:

| 값 | uart | gpio | wdt | ptimer |
|---|---:|---:|---:|---:|
| `<0ns, 1d, 0e>` | 2 | 2 | 0 | 0 |
| `<0ns, 0d, 1e>` | 8 | 3 | 3 | 2 |

**use-def 관계**: `llhd.drv`의 `after` operand로만 사용됨.

**GenModel 실패 원인**: result-producing op이지만 `llhd.*` namespace blanket skip으로 val map에 미등록. `llhd.drv`가 이 값을 operand로 참조할 때 "not ready" 발생.

**Pass 설계 인사이트**: GenModel에서 time 개념이 불필요하므로, `llhd.drv` lowering 시 `after` operand를 무시하거나, `llhd.constant_time`을 제거 가능.

### 2.2 `llhd.sig` — 신호 선언 (internal signal)

**형태**: `%name = llhd.sig %init_val : type`

**역할**: `hw.module` 내부에서 intermediate signal을 선언. process의 drv target이 되고, prb를 통해 읽힘.

**관측된 타입**: `i1`, `i2`, `i3`, `i4`, `i6`, `i8`, `i10`, `i32`, `!hw.array<32xi8>`, `!hw.array<32xi10>`

**use-def 체인**:
```
llhd.sig %init → (1) llhd.drv %sig, %value after %time [if %cond]
                  (2) llhd.prb %sig → %probed_value → hw.output / comb ops
```

**GenModel 실패 원인**: val map 미등록 → drv/prb 모두 operand "not ready".

**Pass 설계 인사이트**: `llhd.sig`+`llhd.drv`+`llhd.prb` 3-tuple은 `sv.reg`+`sv.assign`으로, 또는 직접 wire connection으로 lowering 가능. conditional drv는 `comb.mux`로 변환 가능.

### 2.3 `llhd.prb` — 신호 프로브 (signal read)

**형태**: `%val = llhd.prb %sig : type`

**역할**: `llhd.sig`로 선언된 signal의 현재 값을 읽음. 결과가 `hw.output`, `comb.*` op 등에서 사용됨.

**분포**: `llhd.sig`와 항상 1:1 대응 (sig 1개당 prb 1개).

**Pass 설계 인사이트**: process flatten 후 `llhd.sig`→wire 치환하면 `llhd.prb`는 단순 wire read로 대체.

### 2.4 `llhd.process` — 프로세스 블록

두 가지 패턴이 관측됨:

#### Pattern A: Result-producing process

**형태**: `%N:M = llhd.process -> type1, type2, ... { ... }`

**내부 구조** (CFG):
```
^bb0:  cf.br ^bb1(%init_values)                     ← 초기화
^bb1:  llhd.wait yield (%results), (%sensitivity), ^bb2  ← 대기/감지
^bb2:  [edge detection / logic]                      ← 로직
       cf.cond_br / cf.br → ^bb1 or ^bbN            ← 분기
^bb3~N: [case/mux/loop 로직]                         ← 추가 분기
```

**BB 수 범위**: 최소 3개 (단순 mux) ~ 최대 12개 (복잡 case 문)

**Result 패턴**: 항상 `(value, valid_flag)` 쌍으로 반환.
- `-> i32, i1` (가장 흔함: 32-bit value + 1-bit valid)
- `-> i32, i1, i32, i1` (multi-output: gpio `%261:4`)
- `-> !hw.array<32xi8>, i1` (uart memory)

**결과 소비**: `llhd.drv %sig, %N#0 after %time if %N#1` — result#0은 값, result#1은 valid.

#### Pattern B: No-result process (sim display)

**형태**:
```
llhd.process {
  sim.proc.print %fmt
  llhd.halt
}
```

**역할**: 시뮬레이션 시작 시 DesignWare IP info 메시지 출력. C++ model에서는 완전히 불필요.

**분포**: uart 8개, gpio 2개 (모두 DW CDC IP).

**Pass 설계 인사이트**: `llhd.process { sim.proc.print; llhd.halt }` 패턴은 무조건 DCE(dead code elimination).

### 2.5 `llhd.drv` — 신호 드라이브

**형태**: `llhd.drv %target, %value after %time [if %cond] : type`

#### Conditional drive (42개)

항상 result-producing process와 쌍:
```mlir
%N:2 = llhd.process -> i32, i1 { ... }
llhd.drv %sig, %N#0 after %0 if %N#1 : i32
```

**의미**: process가 valid 신호(`%N#1 = true`)를 줄 때만 signal을 업데이트.

#### Unconditional drive (32개, gpio only)

```mlir
llhd.drv %debounce_d1, %debounce_d1_0 after %1 : i32
llhd.drv %debounce_d1, %debounce_d1_1 after %1 : i32
... (32회 반복)
```

**원인**: Verilog for-loop가 unroll되어 32개 bit별 개별 drive 생성. 모두 같은 signal(`debounce_d1`)을 대상으로 하며, 각각 다른 value source를 가짐. 이는 `seq.firreg` 기반 clock-domain 레지스터의 비트별 drive.

**Pass 설계 인사이트**: multi-drive to same signal은 priority encoder 또는 최종 값 선택 로직으로 합성 필요. gpio의 경우 bit-wise OR/AND 연산으로 치환 가능.

### 2.6 `llhd.wait` — 감지 대기 (process 내부 terminator)

**형태**: `llhd.wait yield (%results), (%sensitivity_list), ^bbN(%prev_args)`

**관측된 sensitivity 패턴 3종**:

| 패턴 | Sensitivity | 의미 | 예시 |
|---|---|---|---|
| Clock-edge | `(%clk, %rst_n : i1, i1)` | posedge clk / negedge rst_n | uart bcm57 mem |
| Clock-like | `(%dbclk, %dbclk_res : i1, i1)` | gpio debounce clock | gpio debounce |
| Combinational | `(%sig1, %sig2, ... : types)` | 입력 변경 시 재평가 | ptimer PRDATA mux, wdt prdata mux |

**Edge detection 로직** (Clock-edge 패턴):
```mlir
^bb2(%prev_rst: i1, %prev_clk: i1):
  %posedge = comb.xor bin %prev_clk, %true   // prev was 0
  %posedge = comb.and bin %posedge, %clk      // now is 1 → posedge
  %negedge_rst = comb.and bin %prev_rst, %neg_rst_n  // reset fell
  cf.cond_br %edge_or_reset, ^bb3, ^bb1(...)
```

**Pass 설계 인사이트**: `llhd.wait`의 sensitivity가 clock/reset이면 → `seq.firreg`로 변환 가능. 그 외는 `comb.*` 조합 로직으로 변환.

### 2.7 `llhd.halt` — 프로세스 종료

**형태**: `llhd.halt`

**역할**: process 실행을 영구 중지. `sim.proc.print` + `llhd.halt` 패턴에서만 관측.

**Pass 설계 인사이트**: Pattern B process 전체를 DCE.

### 2.8 `cf.br` / `cf.cond_br` — 제어 흐름 (process 내부)

**역할**: process 내부 CFG의 분기 명령. `llhd.process` region 외부에서는 관측되지 않음.

**`cf.br` 패턴**:
- `cf.br ^bb1(%init_vals)` — 초기화 진입
- `cf.br ^bb1(%computed_val, %true)` — 결과 확정 후 wait로 복귀
- `cf.br ^bbN(%loop_vars)` — loop 반복

**`cf.cond_br` 패턴**:
- Edge detection: `cf.cond_br %posedge, ^logic, ^wait`
- Reset check: `cf.cond_br %reset, ^reset_path, ^normal_path`
- Case/mux: `cf.cond_br %cmp, ^match(%val, %true), ^next_case`
- Loop guard: `cf.cond_br %cmp_slt, ^loop_body, ^exit`

**Pass 설계 인사이트**: process flatten 시 CFG를 structured control flow로 변환해야 함. 대부분 if-else chain (case문) 또는 for-loop 패턴이므로 `scf.if`/`scf.for` 또는 직접 `comb.mux` chain으로 변환 가능.

### 2.9 `sim.fmt.literal` / `sim.proc.print` — 시뮬레이션 전용

**형태**:
```mlir
%0 = sim.fmt.literal "Information: ..."
sim.proc.print %0
```

**역할**: DesignWare IP의 CDC 방식 정보 출력. C++ model에 불필요.

**Pass 설계 인사이트**: `sim.*` op은 전부 DCE. 포함된 `llhd.process { ... llhd.halt }` 블록 통째로 제거.

## 3. Clock/Reset 패턴 분석

### 3.1 관측된 clock/reset 패턴 분류

#### Type 1: Clock-edge sensitive process (sequential)

```
llhd.wait yield (%val, %valid), (%clk, %rst_n : i1, i1), ^bb2(%prev_rst, %prev_clk)
```

- **관측 위치**: uart `DW_apb_uart_bcm57` (memory), gpio `DW_apb_gpio_debounce`
- **특징**: sensitivity에 clock/reset만 있음, prev 값으로 edge detection
- **RTL 원본**: `always @(posedge clk or negedge rst_n)`

#### Type 2: Combinational sensitivity process

```
llhd.wait yield (%val, %valid), (%input1, %input2, ... : types), ^bb2
```

- **관측 위치**: ptimer PRDATA mux, wdt PRDATA/prdata mux, gpio apbif, uart regfile
- **특징**: sensitivity에 데이터 신호들만 있음, edge detection 없음
- **RTL 원본**: `always @(*)` 또는 `always_comb`

#### Type 3: Multi-signal combinational

```
llhd.wait yield (%val, %valid), (%many_signals : many_types), ^bb2
```

- **관측 위치**: uart `DW_apb_uart` top의 PRDATA 선택 (40개 operand)
- **특징**: 매우 많은 sensitivity 신호 (address decode + 다수 레지스터)
- **RTL 원본**: 복잡한 case 문

### 3.2 Multi-clock 이슈

**관측 사실**: 4개 타겟 모두 단일 clock domain이 지배적.
- uart: `pclk`(APB) + `sclk`(serial) 2개 clock → CDC module(`DW_apb_uart_bcm25/23`)에서 처리
- gpio: `pclk`(APB) + `dbclk`(debounce) 2개 clock → `DW_apb_gpio_debounce`에서 처리
- wdt: 단일 clock (`pclk`)
- ptimer: `PCLK`(APB) + `TIMER_CLK`(timer domain, 최대 10개) 다중 clock이나 CDC는 `seq.firreg` 2-FF synchronizer로 이미 lowering 완료

**CDC module 패턴**: clock crossing은 `seq.firreg` 체인(2-FF synchronizer)으로 이미 lowering 완료. `llhd.process`는 CDC 모듈이 아닌 combinational/sequential 로직에서만 잔존.

**결론**: multi-clock은 pass 설계에 blocking issue가 아님. CDC는 `seq.firreg`로 이미 처리됨.

## 4. GenModel 실패 원인 매핑

### 4.1 Root Cause Chain

```
GenModel.cpp L3366-3371: llhd.*/cf.* namespace → blanket skip
  ↓
llhd.constant_time: result %0 val map 미등록
llhd.sig: result %sig val map 미등록
llhd.prb: result %val val map 미등록 (operand %sig도 미등록)
llhd.process: result %N:M val map 미등록
  ↓
GenModel.cpp L3402-3458 C-8: process 내부 operand val map lookup 실패
  → "not ready" 판정
  ↓
llhd.drv: operand (%sig, %value, %time) 모두 val map에 없음
  → unresolved process/drive error
```

### 4.2 Op별 val map 미등록 영향

| Op | Result 수 | val map 미등록 | 영향받는 consumer |
|---|---:|---|---|
| `llhd.constant_time` | 1 | O | `llhd.drv` (after operand) |
| `llhd.sig` | 1 | O | `llhd.drv` (target), `llhd.prb` (operand) |
| `llhd.prb` | 1 | O | `hw.output`, `comb.*` ops |
| `llhd.process` (result) | N | O | `llhd.drv` (value + cond operand) |
| `sim.fmt.literal` | 1 | O | `sim.proc.print` (but DCE-able) |

### 4.3 해결 전략 우선순위

1. **DCE 대상** (10 process): `llhd.process { sim.proc.print; llhd.halt }` 통째 제거
2. **단순 치환** (43 sig + 43 prb + 20 time): sig→wire, prb→wire read, time→삭제
3. **Process flatten** (42 result process): CFG를 structured logic으로 변환
4. **Drive lowering** (74 drv): conditional → mux, unconditional → direct assign

## 5. 커스텀 Pass 설계 인사이트

### 5.1 제안 pass 파이프라인

```
Pass 1: LlhdSimCleanup
  - sim.fmt.literal + sim.proc.print + llhd.halt → 포함 process 통째 DCE
  - 선행 조건 없음

Pass 2: LlhdProcessFlatten
  - llhd.process 내부 CFG 분석
  - Pattern 매칭:
    (a) Combinational mux/case → comb.mux chain
    (b) Clock-edge sequential → seq.firreg
    (c) Loop-based bit operation → unrolled comb ops
  - llhd.wait → sensitivity 분석으로 패턴 결정
  - 결과: process body가 flat한 op sequence로 변환

Pass 3: LlhdSignalLowering
  - llhd.sig + llhd.drv + llhd.prb → hw wire/reg 치환
  - llhd.constant_time → 삭제
  - conditional drv: comb.mux로 변환
  - unconditional multi-drv: merge 로직 생성
```

### 5.2 Process CFG 패턴 정리

| 패턴 | BB 수 | 구조 | 변환 대상 | 관측 횟수 |
|---|---:|---|---|---:|
| Simple mux (1-level) | 3 | wait → compare → br back | `comb.mux` | 6 |
| If-else chain (case) | 4-12 | wait → cond_br chain → br back | `comb.mux` chain | 28 |
| Clock-edge + reset | 4-8 | wait → edge detect → reset/logic → br back | `seq.firreg` | 4 |
| Clock-edge + loop | 6-8 | wait → edge → loop(slt guard) → br back | `seq.firreg` + unrolled | 4 |
| Sim display (halt) | 1 | print → halt | DCE | 10 |

### 5.3 핵심 관찰

1. **Result-producing process는 항상 `(value, valid_flag)` 쌍을 반환**하며, `llhd.drv ... if %valid` 패턴으로 소비됨. 이는 conditional assignment를 LLHD 수준에서 표현한 것으로, `comb.mux`의 enable 신호로 직접 매핑 가능.

2. **모든 `llhd.sig`는 `llhd.drv`와 `llhd.prb`에 의해서만 사용됨.** `hw.output`에 직접 전달되지 않으므로, sig→wire 치환 후 drv→assign, prb→read로 일관된 lowering 가능.

3. **Process 내부의 `comb.*` op은 이미 core dialect**이므로 process flatten 시 그대로 재사용 가능. 새로운 op을 생성하는 것이 아니라 기존 op을 process 밖으로 이동하는 것.

4. **gpio unconditional multi-drv 32개는 for-loop unroll 결과**. 이들은 같은 signal에 서로 다른 비트를 쓰는 패턴이므로, bit-concat + single assign으로 합성 가능.

5. **Clock-edge detection 패턴은 정형화되어 있음**: `prev_clk/prev_rst` block arg → xor/and로 posedge/negedge → `seq.firreg`로 1:1 매핑 가능.

6. **Process 내/외부 op 분류**: process 외부 llhd op은 `llhd.constant_time`, `llhd.sig`, `llhd.prb`, `llhd.drv` (4종). Process 내부 전용: `llhd.wait`, `llhd.halt`, `cf.br`, `cf.cond_br` (4종). `sim.*`은 항상 process 내부.

## 6. 부록: MLIR Dump 파일 위치

| Target | 파일 | 라인 수 |
|---|---|---:|
| uart | `mlir-dumps/uart-lowered.mlir` | 2,356 |
| gpio | `mlir-dumps/gpio-lowered.mlir` | 1,376 |
| wdt | `mlir-dumps/wdt-lowered.mlir` | 325 |
| ptimer | `mlir-dumps/ptimer-lowered.mlir` | 591 |
