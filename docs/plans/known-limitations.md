# HIRCT 알려진 제약 사항 (Known Limitations)

> **최종 수정**: 2026-03-05 (pipeline-audit Stage 2 --llhd-mem2reg segfault 반영)

---

## KL-18. `--llhd-mem2reg` pass — hw.module 입력에서 segfault

**카테고리**: CIRCT upstream 버그 / pipeline-audit Stage 2
**영향 타겟**: `examples/fc6161/pipeline-audit/Makefile` PASSES_STAGE2
**심각도**: Medium (Stage 2 완전 실패 → pass 제외로 우회)

**설명**:
CIRCT `--llhd-mem2reg` pass가 `hw.module` 입력 포트(`OpResultImpl::getOwner`)를 역참조하는 과정에서 null-ptr dereference로 segfault(exit 139) 발생.
크래시 위치: `Mem2Reg.cpp:Promoter::resolveDefinitionValue`.

```
#3 mlir::detail::OpResultImpl::getOwner() const
#4 (anonymous namespace)::Promoter::resolveDefinitionValue(DriveNode*)  Mem2Reg.cpp
#5 (anonymous namespace)::Mem2RegPass::runOnOperation()               Mem2Reg.cpp
```

**재현 조건**: Stage 1(convert-moore-to-core) 출력 → `--llhd-mem2reg` 단독 실행 시 재현.
**우회 방법**: `PASSES_STAGE2`에서 `--llhd-mem2reg` 제거. sig2reg만으로 동일 변환 범위 충족 가능 여부 추후 검증 필요.
**파일 경로 키**: `examples/fc6161/pipeline-audit/Makefile`

---

## KL-1. DPI Wrapper cycle-based 출력 타이밍 불일치

**카테고리**: CModel / DPI-C 통합
**영향**: equiv 테스트벤치에서 false mismatch 발생 가능
**심각도**: Low (기능 정확성에는 영향 없음)

**설명**:
`uart_top_dpi_wrapper`는 `always @(posedge UART_CLK)` 블록 내에서 입력 설정 → `step()` → 출력 읽기를 순차적으로 수행하는 cycle-based 모델이다. RTL은 event-driven으로 신호 변화 시 즉시 조합논리가 재계산된다.

이 차이로 인해 PSEL 해제(deassert) 직후:
- **RTL**: 조합논리로 즉시 PRDATA=0 (같은 time step 내 NBA 이후)
- **CModel**: 다음 posedge에서 step() 호출 시까지 이전 값 유지

ACCESS phase(PSEL=1, PENABLE=1)에서는 RTL과 CModel이 동일한 값을 출력하므로, **비교는 반드시 ACCESS phase에서 수행**해야 한다.

**영향받는 산출물**: `equiv_tb.sv` (수동 작성 테스트벤치)
**우회 방법**: `apb_read_compare` 태스크에서 비교를 PSEL 해제 후가 아닌 ACCESS phase에서 수행

---

## KL-2. VCS 전용 시스템 태스크 미지원

**카테고리**: Verilog Import / Slang 제약
**영향 타겟**: ets (`$disable_warnings`/`$enable_warnings`), ddr_ctrl (패키지 중복 정의)
**심각도**: Low (특정 벤더 코드에만 발생)
**우회 방법**: 해당 파일에서 `$disable_warnings`/`$enable_warnings`를 빈 문으로 수동 치환, 또는 중복 include 제거
**등록일**: 2026-03-04

---

## KL-3. 64비트 초과 신호 절삭 (WideInt 미지원)

**카테고리**: GenModel / 비트폭 처리
**영향**: 65비트 이상 신호가 `uint64_t`로 절삭되어 데이터 손실 발생
**심각도**: Medium (정확성 문제이나 fc6161 커버리지에는 미영향)

**설명**:
`cpp_type_for_width()`가 65비트 이상을 `uint64_t`로 매핑하며, `ConcatOp`에서 `ow >= 64`일 때 `static_cast<uint64_t>`로 절삭한다. `ParityOp`만 `unsigned __int128`로 128비트까지 처리. WideInt 라이브러리는 미도입 상태.

이전에는 `APInt::getZExtValue()` 호출 시 비트폭 미체크로 `APInt assert` (SIGABRT)가 발생했으나, Stage 3 (`f8278e2`, `52202cc`) 수정으로 `>64비트 상수`는 하위 64비트로 truncate 후 안전하게 처리된다.

**영향받는 산출물**: `cmodel/*.cpp` (GenModel 생성 코드)
**영향받는 타겟**: `smbus` (i384 wide signal — `APInt assert` → truncate로 변경), `axi_x2p1/axi_x2p2` (wide concat)
**우회 방법**: 해당 모듈을 XFAIL로 등록. Phase 4 이후 Verilator 호환 `uint32_t[]` 래퍼 도입 시 해결 예정 (open-decisions A-1 참조)
**등록일**: 2026-03-04
**상태 갱신**: 2026-03-05 — SIGABRT 제거 (안전 truncate 적용), 데이터 정확성 이슈는 잔존

---

## KL-4. initial 블록 미지원

**카테고리**: GenModel / 초기화
**영향**: Verilog `initial` 블록이 C++ 코드로 변환되지 않음
**심각도**: Low (합성 대상 ASIC RTL에서는 리셋 시퀀스가 `always` 블록으로 표현)

**설명**:
ASIC 흐름에서 `initial`은 비합성 구문이며, 기능 검증은 리셋 시퀀스(`do_reset()`)로 대체된다. FPGA synthesis용 `initial`(레지스터 초기값)은 CIRCT elaboration에서 상수로 치환되므로 별도 처리 불필요. Testbench용 `initial`은 GenModel 범위 밖.

**우회 방법**: `do_reset()` 호출로 초기화 수행. FPGA 초기값은 CIRCT가 상수 치환.
**등록일**: 2026-03-04

---

## KL-5. llhd.prb/drv 잔존 Op (verilator -E 경로)

**카테고리**: GenModel / CIRCT Lowering 잔류
**영향 타겟**: verilator -E 경로에서 복잡한 신호 드라이브 패턴을 가진 모듈
**심각도**: Medium (기능 정확성에는 영향 없음, 커버리지 감소)

**설명**:
verilator -E → importVerilog 경로에서 일부 모듈의 MLIR에 `llhd.prb` (신호 프로빙) / `llhd.drv` (신호 드라이브) op이 잔존할 수 있다. 이는 `populateLlhdToCorePipeline()`이 process 외부의 신호 드라이브를 완전히 변환하지 못한 잔류물이다. 이 경우 GenModel 생성을 스킵하고 경고를 출력한다.

- **영향 범위**: verilator -E 경로 + 복잡한 신호 드라이브 패턴 (예: i2cm `bcm21/bcm37` — FSM 잔류로 GenModel 스킵됨)
- **i2cm 상태**: `llhd.combinational` 역방향 루프는 `c948bca` sgt 지원으로 해결. 잔존 실패는 FSM 기반 sub-module(KL-10) 이슈
- **해결 계획**: HirctSignalLoweringPass 확장 (중기), CIRCT 업스트림 개선 대기 (장기)
- **등록일**: 2026-03-04
- **상태 갱신**: 2026-03-05 — i2cm의 `llhd.combinational` 이슈는 KL-11로 분리. 잔존 영향은 FSM sub-module에 집중

### KL-5 추가 사례 (2026-03-05): ncs_cmd_v2p_sbk_remap — `llhd.sig.extract + llhd.drv` type mismatch

**모듈**: `ncs_cmd_v2p_sbk_remap`  
**파일:라인 (RTL 패턴 위치)**: `ncs_cmd_v2p_sbk_remap.v:169` (generate for 비트셀렉트), `ncs_cmd_v2p_sbk_remap.v:203` (동적 인덱스 비트셀렉트)  
**에러 loc (CIRCT 보고)**: `ncs_cmd_v2p_sbk_remap.v:96:15` — CIRCT가 failing pass 시점에 기록하는 컨텍스트 위치이며, 실제 failing op의 선언 위치(`r_cmd_mpblk_str`)와 다름

**RTL 패턴**:
```verilog
// 패턴 1: generate for 비트셀렉트 assign (wire r_cmd_go_ok[15:0])
for (g = 0; g < 16; g = g + 1) begin : CMD_GO_OK
    assign r_cmd_go_ok[g] = ((g[3:0] == w_cmd_ch_num[3:0]) && (r_num_req_os[g][3:0] != 4'd4));
end
// 패턴 2: 동적 인덱스 비트셀렉트 (reg r_req_info_push[15:0])
r_req_info_push[r_cmd_ch_str[3:0]] = 1'b1;
```

**MLIR 증거** (`--ir-llhd` 덤프 — 타입 불일치 17개):
```mlir
%55 = llhd.sig.extract %r_cmd_go_ok from %c0_i4 : <i16> -> <i1>
llhd.drv %55, %61 after %1 : i1
// signal ref type <i16>→<i1> ≠ value type i1 → DriveOp TypeMatchesRefNestedType 위반
```

**추정 원인**: 원인 A — Moore frontend가 비트셀렉트 assign을 `llhd.sig.extract + llhd.drv`로 변환할 때, `llhd.sig.extract` 결과 타입(`<i16> → <i1>` slice ref)과 drive 값 타입(`i1`) 사이의 RefType nested type 불일치를 처리하지 못함

**CIRCT upstream 이슈 여부**: Yes — `circt-verilog`(동일 CIRCT 빌드, `detectMemories=true/false` 모두)는 동일 RTL 변환 성공. `populateLlhdToCorePipeline` 내부에서 `sig.extract` + `drv` 패턴에 대한 canonicalization이 hirct 호출 경로에서는 누락되거나 순서 차이로 인해 verification fail이 먼저 발생하는 것으로 추정

**hirct 전처리 우회 가능성**: 불가 — RTL 전처리(`verilator -E`)로는 MLIR lowering 단계 타입 불일치를 우회할 수 없음

**수정 방향**:
- 단기: `v2p_tbl_stage1` / `ncs_cmd_v2p_sbk_remap` XFAIL 처리
- 중기: `VerilogLoader.cpp`에서 `populateLlhdToCorePipeline` 호출 전 `llhd.sig.extract + llhd.drv` → widened drive 재작성 canonicalization pass 추가
- 장기: CIRCT upstream에 이슈 리포트 (MooreToCore / LlhdToCore의 `sig.extract` drv handling 정합성)

---

## KL-6. hybrid-verify-matrix.py 경로 부재 (matrix-smoke.test)

**카테고리**: Integration Test / 경로 의존
**영향 타겟**: `hirct/integration_test/smoke/hybrid/matrix-smoke.test`
**심각도**: Low (hirct 코어 기능과 무관)

**설명**:
`matrix-smoke.test`는 `%S/../../../../utils/hybrid-verify-matrix.py` 경로로 `hybrid-verify-matrix.py`를 참조하나, 이 파일은 `examples/fc6161/pt_plat/utils/`에 위치하며 프로젝트 루트 `utils/`에 존재하지 않는다. 이 유틸리티는 fc6161 예제 전용이므로 hirct 코어 테스트로 부적합.

**우회 방법**: `XFAIL: *` 마커로 Expected Failure 처리.
**해결 계획**: fc6161 예제 의존 테스트를 별도 test suite로 분리하거나, 유틸리티를 hirct에 통합할 필요성이 확인되면 경로를 수정.
**등록일**: 2026-03-04

---

## KL-7. lit 테스트 pre-existing 실패 3건 (instance-crossref, instance-topo-sort, multi-module)

**카테고리**: GenModel / 인스턴스 처리
**영향 타겟**:
- `hirct/test/Target/GenModel/instance-crossref.test`
- `hirct/test/Target/GenModel/instance-topo-sort.test`
- `hirct/test/Target/GenModel/multi-module.test`
**심각도**: Medium (인스턴스 계층 출력 구조 미완성)

**설명**:
이 3개 테스트는 커밋 5cbe646 이전(1bbb1f1)에서도 동일하게 실패하며, 이번 작업의 regression이 아닌 pre-existing failure다. 직접 확인된 증거:

- `git checkout 1bbb1f1` 후 `ninja check-hirct` → 동일 3개 FAIL (54 Passed, 3 Failed)
- 실패 원인: 출력 디렉토리 내 `CyclicTop/cmodel/CyclicTop.cpp` 등 파일이 생성되지 않음 (hirct-gen이 다중 모듈 / cross-ref 케이스에서 하위 모듈 파일 경로를 생성하지 않음)

**현재 상태**: 3개 pre-existing FAIL, 나머지 55개 PASS (val-map-miss.test 포함)
**해결 계획**: 인스턴스 계층 출력 구조 구현 시 해결 예정 (Phase 4 이후)
**등록일**: 2026-03-05

---

## KL-8. g++ -c 컴파일 체크 결과 (uart_top, DW_apb_gpio)

**카테고리**: GenModel / C++ 컴파일 가능성
**등록일**: 2026-03-05

### uart_top

- **hirct-gen EXIT**: 1 (실패) — Stage 4 확인
- **실패 원인**: `uart_top`을 포함한 대부분 sub-module이 residual LLHD ops로 인해 GenModel 스킵됨 (KL-5)
  - `DW_apb_uart_tx` (16 residual), `DW_apb_uart_rx` (16), `DW_apb_uart_regfile` (46), `uart_top` (6) 등 19개 sub-module 스킵
  - top-level 모듈(`uart_top`)까지 스킵되어 전체 생성 실패
- **관련 제약**: KL-5 (llhd.prb/drv 잔존 Op), KL-10 (FSM process)

### DW_apb_gpio

- **hirct-gen EXIT**: 0 (생성 성공) — Stage 4 확인
- **성공 범위**: `DW_apb_gpio` top-level 생성 성공. 5개 sub-module 스킵 (KL-5: bcm21×2, gpio_ctrl, apbif, debounce)
- **val map miss**: 1건 잔존 (comb.extract, deep cycle — 5cbe646 documented)
- **g++ -c 결과**: 미실시 (sub-module 스킵으로 #include 누락 예상 — KL-5 해결 후 재시도 필요)
- **근본 원인**: GenModel의 SSA rename 로직이 동일 모듈 내에서 중복 멤버명을 생성하는 버그 (SSA suffix collision) — gpio_ctrl/debounce는 스킵되어 실제 컴파일 시 확인 불가

---

## KL-10. FSM ceq 기반 process — ProcessFlatten 범위 밖

**카테고리**: GenModel / LLHD Lowering
**영향 타겟**: `uart`, `gpio`, `axi_x2p1`, `axi_x2p2` (FSM 패턴 sub-module)
**심각도**: Medium (해당 sub-module GenModel 스킵, top-level 생성은 성공)

**설명**:
CIRCT의 `llhd.process` 중 FSM 패턴(`ceq` 기반 상태 비교)을 사용하는 process는 `ProcessFlattenPass`가 처리하지 못한다. `ProcessFlattenPass`는 단순 sequential `always` 블록 (induction variable 기반 루프)만 대상으로 하며, FSM 분기(`ceq/ne` 로 상태 비교 → `br` 분기)는 CIRCT의 `hw`/`comb` dialect로 완전히 변환되지 않은 채 `llhd.process` 형태로 잔존한다.

**영향 범위**:
- `uart`: 31개 process → lowering 후 FSM 잔존 sub-module (top-level GenModel은 성공)
- `gpio`: 14개 process → lowering 후 FSM 잔존 (DW_apb_gpio_ctrl/debounce)
- `axi_x2p1/axi_x2p2`: 35개 process → lowering 후 FSM 잔존 (arb, dcdr, p, s_addr_dcd, s_control)
- `i2cm`: bcm21/bcm37 FSM sub-module

**우회 방법**: 해당 FSM sub-module은 XFAIL 처리. top-level은 FSM sub-module을 외부 구현으로 가정 (수동 스텁 필요). forward declaration 자동 emit은 미구현 상태.
**해결 계획**: CIRCT FSM dialect 활용 또는 ProcessFlattenPass의 FSM 패턴 확장 (Phase 4+ 고려)
**등록일**: 2026-03-05

---

## KL-11. llhd.combinational 미지원 (UnrollProcessLoops)

**카테고리**: GenModel / LLHD Lowering
**영향 타겟**: `i2cm` (역방향 루프 잔존 2개)
**심각도**: Low (i2cm 전체 GenModel의 극히 일부 sub-module에만 영향)

**설명**:
`UnrollProcessLoops`는 `llhd.process` 내부 루프만 처리하며, `llhd.combinational` (조합 논리 블록) 내부 루프는 처리하지 않는다. `i2cm`의 일부 sub-module에서 `llhd.combinational` 내 `sgt %i, %c-1_i32` 역방향 루프 패턴(`i=N; i>=0; i--`)이 잔존한다.

역방향 루프 `sgt` 패턴 자체는 `c948bca`(sgt 지원)으로 `llhd.process` 내에서는 해결됨. `llhd.combinational` 내부로의 확장이 미구현 상태.

**잔존 상황 (Stage 4 확인, 2026-03-05)**:
- `i2cm` MLIR 재생성(circt-verilog) 후에도 `llhd.combinational` 2개 잔존 확인
- 원인: `begin=3(상수), end=-1(sgt), slt` 복합 조건 루프 — 루프 인덱스가 BBarg가 아닌 상수 `%c3_i32_0`으로 고정되어 unroll 매처(loop induction variable 기반)가 인식하지 못함
- 해당 sub-module은 GenModel 스킵(경고 출력), 나머지 82개 파일 생성은 정상

**해결 계획**: `UnrollProcessLoops`의 `CombinationalOp` visitor 추가 + 상수 기반 루프 인덱스 패턴 처리 (소규모 확장). 해결 시 이 항목 제거.
**등록일**: 2026-03-05

---

## KL-13. FSMAnalysis ICmpPredicate::ceq 미지원 (FuncModel 미생성)

**카테고리**: GenFuncModel / FSMAnalysis
**영향 타겟**: `uart` (DW_apb_uart_rx/tx/biu/fifo 등 전 서브모듈), `ncs_cmd_parity_readp_dma`, `ncs_cmd_urgent_router` 및 동일 패턴 모듈
**심각도**: High (FSM이 실제로 존재하는 모듈에서 func_model 미생성)
**등록일**: 2026-03-05 (fc6161 FuncModel 실행 결과)

**설명**:

`FSMAnalysis::identify_fsm_registers()`는 `ICmpPredicate::eq`만 FSM 상태 비교로 인식한다.
Verilog `case` 문에서 합성된 `ICmpPredicate::ceq` (case equality)는 별도 열거자이므로 `eq` 체크에 걸리지 않아 "no FSM found"를 반환한다.

```cpp
// FSMAnalysis.cpp (현재)
if (icmp.getPredicate() == ICmpPredicate::eq) { ... }
// ICmpPredicate::ceq 누락 → case 기반 FSM 전부 미탐지
```

**확인된 RTL 패턴**:
```verilog
// Verilog case 문 → CIRCT가 ICmpPredicate::ceq로 변환
always @(*) begin
  case (c_state)
    STATE_IDLE: ...
    STATE_RUN:  ...
  endcase
end
```

**영향받는 모듈 (fc6161 실행 결과)**:
- `uart`: DW_apb_uart_rx, DW_apb_uart_tx, DW_apb_uart_biu, DW_apb_uart_fifo 등 전 모듈 — func_model **0개** 생성
- `dma_requster`: ncs_cmd_parity_readp_dma (i3 cstate, i1 cpl_cstate 존재) — func_model 미생성
- `urgent_router`: ncs_cmd_urgent_router (STATE_0/1) — func_model 미생성

**추가 미탐지 원인** (uart에서 추가 확인):
`verify_mux_chain_feedback`이 `icmp → mux.cond` 직접 연결만 추적하며, `icmp → comb.and → mux.cond` 체인은 추적하지 못함.

**재현 명령어**:
```bash
./build/bin/hirct-gen examples/fc6161/pt_plat/skip-analysis-results/uart/preprocessed.v \
  --top uart_top --only func-model -o /tmp/uart_funcmodel_test
# 결과: 모든 서브모듈 "no FSM found" → func_model 미생성
```

**해결 계획**:
- 단기: `identify_fsm_registers()`에 `ICmpPredicate::ceq` 조건 추가
- 단기: `verify_mux_chain_feedback()`에 `comb.and` 체인 추적 추가
- 우선순위: High (fc6161 실제 모듈의 대부분이 `case` 기반 FSM 사용)

---

## KL-14. llhd.drv 동적 배열 인덱스 타입 불일치 (hirct-gen 자체 실패)

**카테고리**: CIRCT Lowering / moore→core
**영향 타겟**: `v2p_tbl_stage2` (ncs_cmd_v2p_blk_swap), `packet_path/normal_path`, `packet_path/urgent_path`
**심각도**: High (hirct-gen exit 1 — 전체 산출물 생성 불가)
**등록일**: 2026-03-05 (fc6161 FuncModel 실행 결과)

**설명**:

동적 변수로 배열을 인덱싱하는 `always` 블록에서 `llhd.drv` op의 타입 불일치가 발생한다.
`moore→core` lowering 단계에서 signal ref의 element type과 drive 값 타입이 맞지 않아 verification fail.

```verilog
// 실패 패턴 1: 파라미터화된 배열 동적 인덱스 드라이브
reg [DATA_WIDTH-1:0] data_reg [0:DEPTH-1];
always @(posedge clk) begin
  data_reg[wr_ptr] <= wr_data;  // 동적 인덱스 → llhd.drv type mismatch
end

// 실패 패턴 2: reg_slice_1toN (reg_slice_asym 의존 모듈)
for (i=0; i<N; i++) begin
  data_reg[sel] <= wr_data;     // sel이 런타임 변수
end
```

**CIRCT 에러**:
```
error: 'llhd.drv' op failed to verify that value type must match signal ref element type
Error: moore/llhd-to-core lowering failed
```

**영향받는 모듈 (fc6161 실행 결과)**:
- `ncs_cmd_v2p_blk_swap`: 4상태 FSM 존재하나 진입 불가
- `ncs_cmd_normal_path`, `ncs_cmd_urgent_path`: reg_slice_1toN 포함으로 실패
- `top_v2p`: 상위 두 모듈 포함으로 연쇄 실패

**재현 명령어**:
```bash
./build/bin/hirct-gen \
  /user/wonseok/fc6161-trunk-rom/rtl/pt_ncs/src/ncs_cmd_path/ncs_cmd_v2p_blk_swap.v \
  /user/wonseok/fc6161-trunk-rom/rtl/lib/fifo_reg/sync_fifo_reg.v \
  --top ncs_cmd_v2p_blk_swap --only func-model \
  --preprocess verilator \
  -o /tmp/v2p_blk_swap_test
# 결과: llhd.drv type mismatch → exit 1
```

**해결 계획**:
- XFAIL 처리 (ncs_cmd_v2p_blk_swap, packet_path 모듈)
- 중기: `VerilogLoader.cpp`에서 동적 배열 인덱스 드라이브에 대한 widened-drive 재작성 패스 추가 (KL-5 수정 방향과 동일)
- 장기: CIRCT upstream 이슈 리포트

---

## KL-15. `--preprocess verilator --pp-comments` define 주석 파서 에러

**카테고리**: VerilogLoader / 전처리
**영향 타겟**: `packet_router/normal_router`, `top_v2p` (ncs_opc.vh 포함 모든 모듈)
**심각도**: High (hirct-gen exit 1 — importVerilog parse error)
**등록일**: 2026-03-05 (fc6161 FuncModel 실행 결과)

**설명**:

``define`` 매크로 값에 inline 주석이 포함된 경우, verilator `-E --pp-comments` 전처리가 주석을 보존한 채 치환하면서 닫는 괄호 `)` 가 주석 안으로 이동해 MLIR 파서 에러가 발생한다.

**RTL 패턴**:
```verilog
// ncs_opc.vh
`define NCS_UPDATE_V2P_TBL      6'h14 // Normal port

// 사용처 (ncs_cmd_normal_router.v)
r_direction = (RD_CSS_FTL_CMD_I[5:0] == `NCS_UPDATE_V2P_TBL);
```

**전처리 후 (_preprocessed.v)**:
```verilog
r_direction = (RD_CSS_FTL_CMD_I[5:0] == 6'h14 // Normal port);
//                                                             ^ ')' 가 주석 안에 들어감
```

**에러**:
```
error: expected ')'
Error: importVerilog failed
```

**재현 명령어**:
```bash
./build/bin/hirct-gen -f /tmp/normal_router.f \
  --top ncs_cmd_normal_router --only func-model --preprocess verilator \
  --lib-dir /user/wonseok/fc6161-trunk-rom/rtl/pt_ncs/include \
  -o /tmp/normal_router_test
# 결과: expected ')' → exit 1
```

**우회 방법**: verilator 전처리 옵션에서 `--pp-comments`를 제거하면 주석이 보존되지 않아 파서 에러를 회피할 수 있으나, 다른 모듈에서 주석 제거로 인한 부작용 가능성 검토 필요.

**해결 계획**:
- 단기: `VerilogLoader.cpp`의 verilator 호출에서 `--pp-comments` 옵션 제거 또는 조건부 사용
- 단기: 전처리 후 `_preprocessed.v`에서 라인 끝 `// ...` 주석을 제거하는 포스트 프로세싱 추가

---

## KL-16. hw.array_inject unsupported → cmodel g++ 컴파일 실패

**카테고리**: GenModel / 코드 생성
**영향 타겟**: `sync_fifo_reg` (dma_requster 서브모듈), 동적 배열 쓰기를 가진 모든 모듈
**심각도**: Medium (hirct-gen 자체는 성공하나 생성된 C++ 코드가 컴파일 안 됨)
**등록일**: 2026-03-05 (fc6161 FuncModel 실행 결과)

**설명**:

`hw.array_inject` op가 GenModel에서 지원되지 않아 fallback 코드를 생성하는데, 이 fallback 코드에 두 가지 컴파일 에러가 발생한다.

**에러 1 — 중복 멤버 선언** (동일 이름의 SSA가 다른 타입으로 2번 선언):
```cpp
// sync_fifo_reg.h
uint64_t reg_ssa_;   // array 타입 fallback
uint64_t next_ssa_;
bool reg_ssa_;       // reset 신호 — 동일 이름 재선언
bool next_ssa_;      // → 컴파일 에러: redeclaration
```

**에러 2 — 배열 subscript on scalar** (uint64_t에 배열 인덱스 적용):
```cpp
// sync_fifo_reg.cpp
uint32_t t27 = static_cast<uint32_t>(t25[static_cast<size_t>(t26)]);
//                                   ^^ t25가 uint64_t인데 [] 연산자 사용
```

**재현 명령어**:
```bash
./build/bin/hirct-gen \
  /user/wonseok/fc6161-trunk-rom/rtl/pt_ncs/src/ncs_cmd_path/ncs_cmd_parity_readp_dma.v \
  /user/wonseok/fc6161-trunk-rom/rtl/lib/fifo_reg/sync_fifo_reg.v \
  --top ncs_cmd_parity_readp_dma --only func-model --preprocess verilator \
  --lib-dir /user/wonseok/fc6161-trunk-rom/rtl/pt_ncs/include \
  -o /tmp/dma_requster_test
g++ -std=c++17 -c /tmp/dma_requster_test/sync_fifo_reg/cmodel/sync_fifo_reg.cpp \
  -I /tmp/dma_requster_test/sync_fifo_reg/cmodel/
# 결과: redeclaration error + subscript error
```

**해결 계획**:
- GenModel의 `hw.array_inject` fallback 코드 생성 시 SSA 이름 충돌 방지 (suffix 추가)
- `hw.array_inject` 미지원 경고 후 배열 원소 접근을 `uint64_t` 스칼라로 표현하는 변환 로직 추가 (또는 skip)

---

## KL-17. meta.json gen-func-model skip_reason 오표시

**카테고리**: GenFuncModel / 메타데이터
**영향 타겟**: func_model 스킵된 모든 모듈
**심각도**: Low (기능 오류 없음, 진단 정확성 저하)
**등록일**: 2026-03-05 (fc6161 FuncModel 실행 결과)

**설명**:

func_model이 스킵될 때 meta.json의 `gen-func-model.reason`이 항상 `"not in --only filter"`로 표시된다.
실제 원인이 "no FSM found"인 경우에도 동일 문자열이 사용됨.

```json
// 실제 meta.json (no FSM found 케이스)
"gen-func-model": {
  "result": "skipped",
  "reason": "not in --only filter"   // ← 잘못된 이유
}
```

**해결 계획**: `main.cpp`의 emitter skip 로직에서 "not in --only filter" / "no FSM found" / "multiple FSMs" 등 스킵 원인을 별도 문자열로 구분하여 meta.json에 기록.

---

## KL-9. GenFuncModel 현재 제약 사항

**카테고리**: GenFuncModel / 기능 제약
**등록일**: 2026-03-05
**최종 갱신**: 2026-03-05 (Task 1~6 완료 반영)

### 구현 완료 항목 (이전 제약에서 해소됨)

- **복합 조건 지원**: `comb.and/or/xor/icmp eq/ne/slt/sle/sgt/sge/ult/ule/ugt/uge`, `hw.constant`, `BlockArgument` 재귀 처리 (Task 1, `bef8f1c`)
- **data register 갱신 emit**: tick() 내 상태별 `cnt_ = 0;`, `cnt_ = cnt_ + 1;` 등 갱신 코드 생성 (Task 3, `992054f`)
- **출력 포트 갱신 emit**: tick() 내 상태별 `io.busy = 1;`, `io.done = 0;` 등 출력 코드 생성 (Task 4, `992054f`)
- **다중 FSM 경고**: `fsm_views.size() > 1` 시 stderr에 "multiple FSMs found, using first" 경고 출력 (Task 6)
- **func_model Makefile 타겟**: `test-compile-funcmodel` 타겟 및 `test-artifacts`의 func_model 파일 존재 검사 (Task 5, `67d5676`)

### 잔존 제약

1. **FSM 필수**: FSM이 있는 모듈에서만 func_model/ 생성. 순수 조합 모듈 → skipped.
2. **다중 FSM 단일 처리**: 모듈 내 FSM이 2개 이상이면 첫 번째만 사용. 경고 출력됨.
3. **resolve_value_expr 미지원 패턴**: `comb.sub`, `comb.mul` 등 일부 산술 연산 미지원 → `/* unresolved: comb.sub */` fallback 출력.
4. **이종 레지스터 교차 참조**: `resolve_value_expr`에서 `seq.compreg` 결과가 FSM 소속 data_regs에 없으면 `/* unresolved: compreg */` 출력 가능. 교차 레지스터 참조 시 한계.
5. **signed/unsigned 구별 없음**: `comb.icmp slt`와 `ult` 모두 `<`로 출력. 생성된 C++ 코드에서 signed/unsigned 의미 소실.

**현재 상태**: 기본 FSM 모델 생성 가능. 상태 전이, data reg 갱신, 출력 포트 갱신, 복합 조건 모두 지원.
**해결 계획**: comb.sub/mul 추가는 수요 발생 시. 이종 레지스터 참조 및 signed/unsigned 구별은 Phase 3+.

---

## KL-12. GenModel reverse loop (sgt) 미구현

**카테고리**: GenModel / 기능 제약
**등록일**: 2026-03-05
**파일 경로 키**: `hirct/test/Target/GenModel/unroll-loop-sgt.test`

### 설명
`try_unroll_loop`에서 루프 헤더의 icmp LHS(`%i`)가 BlockArgument로 시작하므로
`try_const`가 `val` 맵에서 초기값을 찾지 못해 `false`를 반환, 언롤 자체가 실행되지 않음.

카운터 감소(`--counter`)는 `GenModel.cpp`에 이미 구현되어 있다(`reverse_loop` 플래그 기반
`if (reverse_loop) --counter; else ++counter;` 분기). 실패 원인은 카운터 증감 방향이 아니라,
loop-carried BlockArgument의 초기값을 `val` 맵(또는 predecessor branch operand)에서
역추적하는 로직이 미구현인 것이다.

**현재 상태**: XFAIL 처리. `unroll-loop-sgt.test`는 Expectedly Failed.
**해결 계획**: `try_unroll_loop` 진입 시 header BlockArgument의 initial value를
predecessor branch operand에서 역추적하여 `lhs_val`을 도출하는 로직 추가.

---
