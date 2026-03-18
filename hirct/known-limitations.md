# Known Limitations (XFAIL SSOT)

> **목적**: 알려진 제한 사항(Expected Failure)의 단일 진실 공급원
> **규칙**: lit 테스트에서 이 파일을 참조하여 XFAIL 판정. XPASS(예상 실패가 통과) = WARN (CI는 Green 유지).
> **분류 기준**: hirct-convention.md §5 "실패 분류 체계" 참조

## Hybrid Cross-Validation Baseline (2026-02-23)

- Baseline 고정(Task 1 시점): `make build && ninja -C build check-hirct` -> `44/44 PASS` (exit 0)
- 현재 테스트 상태(RED XFAIL 등록 후): `45 tests = 44 PASS + 1 XFAIL` (exit 0)
- 대표 실패 고정: `Fadu_K2_S5_Queue_11`는 `--seeds 3 --cycles 100`에서 동일 조건 mismatch 재현 (exit 2)
- Acceptance Contract:
  - Module Gate: 동일 seed/cycle mismatch 0
  - Subsystem Gate: PASS 비율 및 실패 패턴 안정성 유지
  - Top Gate: SoC smoke mismatch 0 또는 허용 리스트만 존재

| Path | Category | Origin Op | Reason | Fix Phase | Date |
|------|----------|-----------|--------|-----------|------|
| ~~test/Target/GenModel/firmem-sync-read-latency-red.test~~ | ~~xfail-red~~ | ~~seq.firmem.read_port~~ | ~~RED 단계 의도 실패 테스트~~ → `mem_rp_addr_*` 스테이징 구현으로 해소. step()에서 주소 샘플링 → 2nd eval_comb 이후 실제 읽기로 read_latency=1 시맨틱 충족 | Phase 3 완료 | 2026-02-25 |
| test/fixtures/RVCExpander.mlir | verify | seq.firreg | 복합 케이스, Verilator verify Phase 2 스모크 이관 | Phase 2 | 2026-02-19 |
| test/fixtures/MultiModule.mlir | gen-model | hw.instance | ~~ModuleAnalyzer 단일 모듈 파싱 제한~~ → Phase 2 완료 (Task 209). MultiModuleContext + hw.instance composition 지원 | Phase 2 완료 | 2026-02-22 |
| (width > 64-bit signals) | gen-model | (width) | uint64_t fallback 적용 (Task 209, Task A). 65+bit 상위 비트 손실 있음. 정밀 지원(uint32_t[])은 Phase 3 | Phase 2 부분완료 | 2026-02-22 |
| ~~hw.array_get (unknown-size)~~ | gen-model | hw.array_get | ~~포트 배열 unknown-size → emit_unsupported_op~~ → `!hw.array<NxT>` result_type에서 배열 크기 추출로 해소. bcm57 등 llhd.sig 기반 배열도 지원 | Phase 3+ 완료 | 2026-02-25 |
| rtl/plat/src/uart/ (top) | parse_error | circt-verilog | %m format specifier unsupported in multi-file mode | Phase 3+ | 2026-02-20 |
| rtl/plat/src/gpio/ (top) | parse_error | circt-verilog | %m format specifier unsupported in multi-file mode | Phase 3+ | 2026-02-20 |
| rtl/plat/src/edma/ (top) | parse_error | circt-verilog | Missing external deps (crc32c, ecc_od1_sfifo_utp) | Phase 3+ | 2026-02-20 |
| rtl/plat/src/s5mc/design/Fadu_K2_S5MC_TLXbar.v | infra-error | circt-verilog | circt-verilog segfault (SIGSEGV, exit 139) | Phase 3+ | 2026-02-20 |
| (foundry clock cells: `clk_mux2`, `clk_or2`, `clk_and2`, `clk_buf`) | ~~unknown_module~~ | circt-verilog | ~~파운드리 PDK 클록 셀~~ → Task C에서 `rtl/lib/stubs/` stub 파일로 해소. 11 files 전부 mlir pass | Phase 2 완료 | 2026-02-23 |
| (foundry ICG: `clk_gate` port conflict) | ~~unknown_module~~ | circt-verilog | ~~`sim_lib/clk_gate.v` 포트 불일치~~ → Task C에서 foundry 포트 stub(`rtl/lib/stubs/clk_gate.v`) + `--timescale` 기본값 추가로 해소 | Phase 2 완료 | 2026-02-23 |
| (clock gating wrappers: `S5_EICG_wrapper`, `S5MC_EICG_wrapper`) | ~~unknown_module~~ | circt-verilog | ~~RTL 소스 없음~~ → **재조사 결과**: 소스는 `S5_EICG.v`/`S5MC_EICG.v` 내 존재. 파일명≠모듈명으로 -y 해소 불가 → Task C에서 별도 stub 파일 생성으로 해소. EICG 소스 4파일 + 소비자 일부 mlir pass | Phase 2 완료 | 2026-02-23 |
| (SRAM macros: `sfifo_utp_*` — 47 unique) | unknown_module | circt-verilog | 동기 FIFO SRAM 매크로 (메모리 컴파일러 생성), 107 files 영향 | Phase 3+ | 2026-02-22 |
| (SRAM macros: `ssd_ctrl_sp_*`, `ssd_ctrl_utp_*` — 62 unique) | unknown_module | circt-verilog | SSD 컨트롤러 SRAM 매크로, 63 files 영향 | Phase 3+ | 2026-02-22 |
| (SRAM macros: `sfifo_sp_*` — 24 unique) | unknown_module | circt-verilog | 동기 FIFO single-port SRAM 매크로, 50 files 영향 | Phase 3+ | 2026-02-22 |
| (RISC-V SRAM: `data_arrays_*_ext`, `tag_array_*_ext`, `periphsram0_ext`, `MaskROM`) | unknown_module | circt-verilog | RISC-V SoC 캐시/메모리 블록 (Rocket/BOOM), 27 files 영향 | Phase 3+ | 2026-02-22 |
| (SRAM macros: `ncs_sp_*`, `ncs_utp_*` — 17 unique) | unknown_module | circt-verilog | NCS SRAM 매크로, 17 files 영향 | Phase 3+ | 2026-02-22 |
| (SRAM macros: `ppu_sp_*`, `host_ppu_sp_*` — 12 unique) | unknown_module | circt-verilog | PPU SRAM 매크로, 12 files 영향 | Phase 3+ | 2026-02-22 |
| (project SRAM: `hil_*`, `hpp_*`, `nil_*`, `pka_*`, `css_*` 등 — 36 unique) | unknown_module | circt-verilog | 프로젝트별 SRAM/ROM 매크로 (다종), 21 files 영향 | Phase 3+ | 2026-02-22 |
| (IP blocks: `crc32c`, `ecc_od1_sfifo_utp_*`) | unknown_module | circt-verilog | 외부 IP 블록 (CRC, ECC), 4 files 영향 | Phase 3+ | 2026-02-22 |
| (project misc: `mw_append_sp_*`, `descbuf_sp_*` 등 — 32 unique) | unknown_module | circt-verilog | 기타 프로젝트별 메모리 매크로, 37 files 영향 | Phase 3+ | 2026-02-22 |
| (seq.firmem / memory op) | gen-model | seq.firmem | ~~seq.firmem 미지원~~ → Task A에서 FirMemEmitter + array-type 지원 추가. deferred worklist로 순서 의존성 해소 | Phase 2 완료 | 2026-02-23 |
| (comb.concat > 64-bit) | gen-model | comb.concat | ~~64-bit 초과 concat 실패~~ → Task A에서 uint64_t fallback 완화. 정밀 wide-bit 지원은 Phase 3 | Phase 2 부분완료 | 2026-02-23 |
| (hw.instance SSA cross-ref) | gen-model | hw.instance | ~~SSA cross-reference 미지원~~ → Task A에서 deferred worklist + MultiModuleContext SSA 해소 개선 | Phase 2 완료 | 2026-02-23 |
| ~~(llhd.sig 19 modules)~~ | gen-model | llhd.sig | ~~LLHD dialect — event-driven model, cycle-accurate 변환 불가~~ → llhd.sig/prb/drv/process 기본 지원 추가. process flatten(cond_br→mux)으로 단순 프로세스 변환 가능. ~~**남은 제한**: process body에 for-loop 미지원~~ → loop unrolling 구현(adabe24) + signal 멤버 승격 + edge detection old_ 변수 추적. uart equiv-ncsim PASS (9/9 checks, 0 mismatches) | Phase 3+ 완료 | 2026-02-25 |
| ~~(llhd.process loop unrolling)~~ | gen-model | llhd.process | ~~LLHD process cf.br 루프 정적 unrolling 미구현~~ → loop unrolling(adabe24) + signal 클래스 멤버 승격(eval_comb 로컬→private 멤버) + edge detection old_ 추적(step()에서 이전 값 저장, wakeup 블록에서 old_ 참조)으로 생성 모델 동작 확인 | Phase 3+ 완료 | 2026-02-25 |
| ~~(GenDPIC: reset port setter missing)~~ | ~~gen-dpic~~ | ~~GenDPIC~~ | ~~DPI wrapper가 리셋 포트를 cmodel에 전달하지 않음~~ → GenDPIC emitter에서 리셋 포트 skip 조건 제거로 해소. `is_clock_port`만 skip, `is_reset_port`는 setter 생성 대상에 포함 | Phase 3 완료 | 2026-02-24 |

## Phase 3 VCS 3자 비교 결과 (Task 301 — 10 seeds × 1000 cycles)

> **실행일**: 2026-02-23
> **VCS 버전**: V-2023.12-SP2-7_Full64
> **대상**: LevelGateway (PASS 대표), Queue_11 (FAIL 3자비교)

| Module | RTL-vs-CModel (Verilator) | RTL-vs-CModel (VCS) | 판정 |
|--------|---------------------------|----------------------|------|
| Fadu_K2_S5_LevelGateway | **PASS** (10/10) | **PASS** (10/10) | hirct-gen 모델 정확 |
| Fadu_K2_S5_Queue_11 | **FAIL** (0/10) | **FAIL** (0/10) | hirct-gen 코드 생성 버그 — Stage A-1 음수 상수 수정 후에도 FAIL (`io_deq_bits` mismatch, 0/1000 cycles). FIFO 메모리 인덱싱/읽기 로직 추가 원인 존재 |

> **결론**: Queue 계열 FAIL은 Verilator 버그가 아닌 hirct-gen 코드 생성 버그로 확정.
> `io_deq_bits` 포트에서 일관된 mismatch (5,876/10,000 사이클). 수정 대상: GenModel의 FIFO 메모리 인덱싱/wrap 로직.
> **Stage A-1 재검증 (2026-02-23)**: 음수 상수 정규화(`-3`→`29`) + comb.icmp operand masking 적용 후 Queue_11 재측정 — 여전히 FAIL (0/1000 cycles, seed=42). 음수 상수는 부분 원인이나, FIFO read/write 포인터 로직 및 `seq.firmem` 기반 메모리 접근에 추가 버그 존재.
> 상세: `vcs-cosim/results/cosim-report.md`

---

## Phase 2 검증 결과 (Task 203 — 10 seeds × 1000 cycles, baseline)

> **실행일**: 2026-02-20
> **입력**: gen-model=pass 274개 모듈
> **결과**: output/verify-report.json (archived)

| Category | Count | 비율 |
|----------|-------|------|
| **PASS** (전 시드 통과) | 131 | 47.8% |
| **FAIL** (mismatch 발견) | 121 | 44.2% |
| **SKIP** (컴파일 실패) | 22 | 8.0% |

---

## Phase 2 검증 결과 (Task D — 10 seeds × 1000 cycles, Task C + step() 수정 반영)

> **실행일**: 2026-02-23
> **입력**: gen-model=pass 1,121개 모듈 (Task C: ssa_to_ident, concat, icmp, extract, parity 수정 + step() mem-write 순서 수정)
> **결과**: output/verify-report.json

| Category | Count | 비율 |
|----------|-------|------|
| **PASS** (전 시드 통과) | 133 | 11.9% |
| **FAIL** (mismatch 발견) | 224 | 20.0% |
| **SKIP** (컴파일 실패) | 764 | 68.2% |

### Task B 대비 변화

| 항목 | Task B | Task D | 변화 |
|------|--------|--------|------|
| 대상 모듈 | 1,094 | 1,121 | +27 |
| PASS | 132 | 133 | +1 |
| FAIL | 192 | 224 | +32 (신규 모듈 유래) |
| SKIP | 770 | 764 | -6 (ssa 스코핑 수정 효과) |

### Task D 주요 변경 사항

- **GenModel step() 메모리 쓰기 순서 수정**: `eval_comb() → reg update → mem write → eval_comb()` 에서 `eval_comb() → reg update → eval_comb() → mem write` 로 변경. `write_latency=1` 시맨틱에 맞게 메모리 쓰기를 second eval_comb 뒤로 이동
- **firmem-write-latency.test 회귀 테스트 추가**: step() 내부 순서를 FileCheck으로 검증 (lit 44/44)
- **Queue 동기 읽기 포트 미모델링 확인**: `seq.firmem.read_port`에 clock 인자가 있는 경우 (synchronous read, `read_latency=1`), 현재 모델은 combinational read로 처리. 읽기 레지스터 추가 필요 (Phase 3+)

### FAIL 모듈 패턴 (Task D)

| Pattern | Count | 설명 |
|---------|-------|------|
| Queue 계열 | 60 | `seq.firmem` 동기 읽기 포트 미모델링 (read_latency=1 unimplemented) |
| secded_hamming | 38 | enc 19 + dec 19 — ECC XOR 체인 로직 오류 잔존 |
| bridge (async) | 14 | 비동기 브릿지 메모리/포인터 로직 |
| Fadu_K2 | 69 | 다양한 모듈별 이슈 |
| other | 43 | 기타 |

### SKIP — 컴파일 실패 (764건)

| Pattern | Count | 원인 |
|---------|-------|------|
| MODMISSING (서브모듈 누락) | 633 | Verilator include path에 서브모듈 RTL 없음 |
| Verilator 경고/오류 | ~5 | WIDTHEXPAND, NEEDTIMINGOPT 등 |
| C++ 컴파일 오류 | ~126 | 일부 gen-model 코드 생성 문제 잔존 |

---

## Phase 2 검증 결과 (Task B — 10 seeds × 1000 cycles, Task 209 + Task A 반영)

> **실행일**: 2026-02-23
> **입력**: gen-model=pass 1,094개 모듈 (Task 209: 654→Task A: 1,094)
> **결과**: output/verify-report.json

| Category | Count | 비율 |
|----------|-------|------|
| **PASS** (전 시드 통과) | 132 | 12.1% |
| **FAIL** (mismatch 발견) | 192 | 17.6% |
| **SKIP** (컴파일 실패) | 770 | 70.4% |

### Task 203 대비 변화

| 항목 | Task 203 | Task B | 변화 |
|------|----------|--------|------|
| 대상 모듈 | 274 | 1,094 | +820 (×4.0) |
| PASS | 131 | 132 | +1 (기존 통과 유지) |
| FAIL | 121 | 192 | +71 (신규 모듈 유래) |
| SKIP | 22 | 770 | +748 (서브모듈 누락 주도) |

### SKIP — 컴파일 실패 (770건)

| Pattern | Count | 원인 |
|---------|-------|------|
| MODMISSING (서브모듈 누락) | 607 | hw.instance로 새로 합성된 멀티모듈의 서브모듈 RTL이 Verilator include path에 없음 |
| undeclared `ssa__` variable | 131 | GenModel이 SSA 변수명을 잘못 생성 (comb.icmp 등 중간 결과, Task 203 대비 ×7 증가) |
| C++ 컴파일 오류 | 21 | GenModel 코드 생성 버그 (멤버 접근, 타입 불일치, wide-bit 관련) |
| Verilator 경고/오류 | 10 | WIDTHEXPAND, NEEDTIMINGOPT 등 Verilator strict 모드 거부 |
| 기타 | 1 | top-module 미발견 등 |

> **수정 사항 (2026-02-23)**: SSA 변수 스코핑 버그 수정 (`ssa_to_ident` 충돌 방지), 음수 상수 정규화 width>=64 처리, `comb.divu`/`divs`/`modu`/`mods` 지원 추가, hirct-verify `--lib-dir` 옵션 추가.
>
> **추가 수정 (2026-02-23, Task C)**:
> - `comb.extract` result_width 마스킹 추가 (상위 비트 누출 방지)
> - `comb.parity` 64비트 초과 입력 XOR 분할 지원
> - `comb.concat` 결과 너비 파싱 수정 (입력 타입 합산으로 변경)
> - `comb.icmp` 결과 너비를 항상 1비트로 고정 (입력 타입과 무관)
> - `mask_ssa` 메모리 쓰기 포트 enable 통합
> - `seq.firmem.read_port` enable 신호 처리 추가
> - CI workflow `|| true` 제거 + `continue-on-error` 전환
>
> **검증**: secded_hamming_enc_d10_p5, secded_hamming_dec_d10_p5, secded_hamming_enc_d12_p6 모두 PASS (3 seeds × 100 cycles). LevelGateway 회귀 없음. lit 43/43 PASS. 전체 재측정(Task B 재실행) 필요.

### FAIL — mismatch (192건)

| Sub-category | Count | 설명 |
|--------------|-------|------|
| 전 시드 실패 | 179 | 일관된 모델 로직 오류 → op 매핑 수정 대상 |
| 일부 시드 실패 | 13 | 시드 의존적 — 초기화/리셋 로직 의심 (Task 203과 동일 13건) |

### FAIL 모듈 패턴

| Pattern | Count | 설명 |
|---------|-------|------|
| Queue 계열 | 59 | FIFO deq/enq 로직 mismatch — Stage A-1 음수 상수 수정으로 코드 변경 확인되나 여전히 FAIL. FIFO 메모리 읽기/인덱싱 추가 수정 필요 |
| secded_hamming | 38 | ECC 인코더/디코더 XOR 체인 |
| TileLink/Bus | 19 | CLINT, TLXbar 등 버스 프로토콜 |
| divider | 8 | 나눗셈/모듈로 연산 |
| other | 68 | 기타 (다양한 모듈) |

### 주요 mismatch 포트 (상위)

| Port | Occurrences | 관련 모듈군 |
|------|-------------|------------|
| `io_deq_bits_data` | 326 | Queue 계열 (FIFO 데이터 경로) |
| `io_deq_bits_size` | 244 | Queue 계열 (사이즈 필드) |
| `io_deq_bits_opcode` | 199 | Queue 계열 (opcode 필드) |
| `p`, `q` | 190 | secded_hamming_enc/dec (ECC) |
| `uecc_error` | 187 | secded_hamming_dec (에러 감지) |
| `io_deq_bits_id` | 156 | Queue 계열 (ID 필드) |
| `io_deq_bits_source` | 153 | Queue 계열 (source 필드) |
| `auto_in_d_bits_opcode` | 119 | TileLink 버스 모듈 |

### Batch C / 향후 작업 대상 요약

- ~~**MODMISSING 633건 해소**~~: Makefile `generate` 타겟에 `--lib-dir` 3개 경로 추가(s5/design, s5mc/design, riscv_e21)로 96.2% 해소 경로 확보 (2026-02-25). SRAM 매크로/외부 IP는 영구 XFAIL (known-limitations §SRAM 참조)
- ~~**ssa__ 131건**~~: `ssa_to_ident()` 충돌 방지 인코딩으로 수정 완료 (2026-02-23). `.`→`_2e`, `-`→`_2d` hex 인코딩, `_`→`__` 이스케이프
- ~~**secded_hamming 38건**~~: `comb.concat` 결과 너비 + `comb.icmp` 결과 너비 + `comb.extract` 마스킹 수정으로 해소 (2026-02-23, Task C). enc/dec 대표 모듈 PASS 확인
- ~~**divider 8건**~~: `comb.divu/divs/modu/mods` 구현 완료 (2026-02-23)
- ~~**Queue FIFO 60건**~~: ~~`seq.firmem.read_port` 동기 읽기(clock 인자 존재) 미모델링~~ → `mem_rp_addr_*` 주소 스테이징 + 2nd eval_comb 이후 `mem_rp_data_*` 업데이트로 read_latency=1 시맨틱 구현 (2026-02-25). 재검증 필요
- **TileLink/Bus 19건**: Queue 수정 후 재평가 필요 (내부적으로 Queue 사용)
- **시드 의존적 실패 13건**: 초기화 값/리셋 시퀀스 검증 필요
- 상세 triage는 Task 206 (`utils/triage-failures.py`) 에서 자동 수행
