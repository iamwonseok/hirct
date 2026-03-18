# Cross-Validation Module Gate — Task 304

**Date**: 2026-02-24
**VCS Version**: V-2023.12-SP2-7_Full64
**ncsim Version**: 15.10-s010 (IUS, Cadence INCISIVE151)
**Configuration**: 10 seeds × 1000 cycles, warmup=10, timescale=1ns/1ps

---

## 1. Module Gate 결과 요약

| Module | VCS (10s×1000c) | ncsim (10s×1000c) | 판정 |
|--------|----------------|-------------------|------|
| Fadu_K2_S5_LevelGateway | **10/10 PASS** | **10/10 PASS** | CModel 정확 — 이중 게이트 확인 |
| Fadu_K2_S5_Queue_11 | **1/10 PASS** (seed=8) | **0/10 PASS** | CModel 버그 — hirct-gen FIFO 로직 (XFAIL) |
| DW_apb_uart_bcm99 | **10/10 PASS** | (VCS only) | CModel 정확 — GenDPIC rst_d_n 패치 필요 |

## 2. VCS LevelGateway 상세

**Command**:
```
VCS_HOME=/tools/synopsys/vcs/V-2023.12-SP2-7
vcs -full64 -sverilog -timescale=1ns/1ps +define+VCS \
  -P $VERDI_PLI_DIR/novas.tab $VERDI_PLI_DIR/pli.a \
  -LDFLAGS "-L$VERDI_PLI_DIR -lnovas" \
  -cpp g++ -CFLAGS "-std=c++17 -I.../cmodel -I.../dpi" \
  rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v \
  .../dpi/Fadu_K2_S5_LevelGateway_dpi.{sv,cpp} \
  .../cmodel/Fadu_K2_S5_LevelGateway.cpp \
  vcs-cosim/tb/tb_Fadu_K2_S5_LevelGateway.sv \
  -o vcs-cosim/simv_LevelGateway
```

**Exit code**: 0 (build), 0 (all 10 seeds)

**핵심 로그**:
```
VCS LevelGateway: 10 PASS / 0 FAIL (10 seeds)
[PASS] seed=1: 1000 matches, 0 mismatches
...
[PASS] seed=10: 1000 matches, 0 mismatches
```

## 3. ncsim LevelGateway 상세

**Command**:
```
g++ -std=c++17 -shared -fPIC -static-libstdc++ \
  -I.../cmodel -I.../dpi -I$IUS_HOME/tools/include \
  .../dpi/Fadu_K2_S5_LevelGateway_dpi.cpp \
  .../cmodel/Fadu_K2_S5_LevelGateway.cpp \
  -o libdpi_lg.so

LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6 \
ncverilog +access+r +sv -timescale 1ns/1ps -64bit \
  -sv_root . -sv_lib libdpi_lg \
  .../Fadu_K2_S5_LevelGateway.v \
  .../Fadu_K2_S5_LevelGateway_dpi.sv \
  .../tb_Fadu_K2_S5_LevelGateway.sv \
  +ncsimargs+"+seed=N +cycles=1000"
```

**Exit code**: 0 (build), 0 (all 10 seeds)

**핵심 로그**:
```
ncsim LevelGateway: 10 PASS / 0 FAIL (10 seeds)
[PASS] seed=1: 1000 matches, 0 mismatches
RESULT: PASS
Simulation complete via $finish(1) at time 10200 NS + 0
```

**IUS 호환성 주의**: IUS 15.1의 libstdc++.so.6 (CXXABI_1.3.9 미지원)와 시스템 g++ 13.3.0 충돌. `-static-libstdc++` + `LD_PRELOAD` 조합으로 해소.

## 4. VCS Queue_11 per-seed 상세

**Exit code**: 0 (build), 2 (9/10 seeds FAIL)

| Seed | Matches | Mismatches | Result |
|------|---------|------------|--------|
| 1    | 350     | 650        | FAIL   |
| 2    | 452     | 548        | FAIL   |
| 3    | 568     | 432        | FAIL   |
| 4    | 121     | 879        | FAIL   |
| 5    | 6       | 994        | FAIL   |
| 6    | 402     | 598        | FAIL   |
| 7    | 981     | 19         | FAIL   |
| 8    | 1000    | 0          | **PASS** |
| 9    | 998     | 2          | FAIL   |
| 10   | 393     | 607        | FAIL   |

## 5. ncsim Queue_11 상세

| Seed | Matches | Mismatches | Result |
|------|---------|------------|--------|
| 1    | 0       | 1000       | FAIL   |
| 8    | 8       | 992        | FAIL   |

**나머지 seed**: ncverilog loop 실행 완료 전 timeout. 확인된 2개 seed 모두 FAIL.

## 6. Waveform 증거

| File | Format | Seed | Cycles | Size |
|------|--------|------|--------|------|
| `vcs-cosim/results/vcs/Queue_11_s1.vcd` | VCD | 1 | 100 | 17K |

## 7. Mismatch 요약

| Module | 주요 mismatch 포트 | 패턴 | 귀속 |
|--------|-------------------|------|------|
| Queue_11 | `io_deq_bits` | FIFO dequeue 데이터 불일치 | hirct-gen `seq.firmem.read_port` 동기 읽기 미구현 |

## UART Cosim 파일럿 (2026-02-24)

**Module**: DW_apb_uart_bcm99 (Double Register Synchronizer)
**선정 이유**: UART 계열 유일하게 clock+reset+register 보유하면서 hirct-gen 전 emitter PASS

### 선별 결과 (uart.f 25개 후보)

| 결과 | 모듈 수 | 사유 |
|------|---------|------|
| hirct-gen PASS (sequential) | 1 (bcm99) | clock+reset+register, 서브모듈 의존 없음 |
| hirct-gen PASS (combinational) | 4 (bcm00_and/or/ck_inv, rst) | 상태 없음, cosim 무의미 |
| hirct-gen PASS (C++ 버그) | 1 (bcm03) | generate 블록 `.` 멤버명 C++ 무효 |
| llhd.sig | 7 | event-driven, GenModel 미지원 |
| %m parser | 5 | circt-verilog %m 제한 (via bcm21/bcm25) |
| 미테스트 | 7 | 상위 모듈 의존성 이슈 |

### VCS Cosim 결과

| Seed | Matches | Mismatches | Result |
|------|---------|------------|--------|
| 1-10 | 1000 each | 0 each | **PASS** |

**조건**: 10 seeds × 1000 cycles, warmup=10, timescale=1ns/1ps
**GenDPIC 패치 필요**: `rst_d_n` setter 수동 추가 (GenDPIC emitter 버그)

### 재현 커맨드

```
build/bin/hirct-gen rtl/plat/src/uart/DW_apb_uart_bcm99.v \
  -o output/plat/src/uart/DW_apb_uart_bcm99 --lib-dir rtl/lib/stubs

# DPI 패치: dpi.h/dpi.cpp/dpi.sv에 set_rst_d_n() 추가

VCS_ENV vcs -full64 -sverilog -timescale=1ns/1ps \
  -P $VERDI_HOME/share/PLI/VCS/LINUX64/novas.tab \
  $VERDI_HOME/share/PLI/VCS/LINUX64/pli.a \
  -cpp g++ -CFLAGS "-std=c++17 -I<cmodel> -I<dpi>" \
  rtl/plat/src/uart/DW_apb_uart_bcm99.v \
  <dpi.sv> <dpi.cpp> <cmodel.cpp> \
  vcs-cosim/tb/tb_DW_apb_uart_bcm99.sv \
  -o vcs-cosim/simv_bcm99

for seed in $(seq 1 10); do
  ./vcs-cosim/simv_bcm99 +seed=$seed +cycles=1000
done
```

## 8. 결론

- **LevelGateway**: VCS/ncsim 이중 게이트 **PASS** — CModel 정확성 교차 검증 완료
- **Queue_11**: VCS/ncsim 이중 게이트 **FAIL** — hirct-gen 코드 생성 버그 확정 (XFAIL)
- **DW_apb_uart_bcm99**: VCS **PASS** — UART 계열 첫 cosim 파일럿 성공. GenDPIC 리셋포트 setter 미생성 버그 발견 및 수동 패치로 해소
- VCS seed=8 PASS는 우연(해당 랜덤 시퀀스가 FIFO 버그를 트리거하지 않음)
- VCS/ncsim PRNG 구현이 다르므로 동일 seed≠동일 시퀀스 (IEEE 1800 명세 준수)
