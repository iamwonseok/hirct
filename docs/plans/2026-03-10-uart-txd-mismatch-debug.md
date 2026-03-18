# UART TXD Mismatch Debug Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** UART GenModel vs CXXRTL 비교에서 TXD mismatch의 근본 원인을 특정하고, 최소 수정으로 5시드 x 10000사이클 TXD 0 mismatch를 달성한다.

**Architecture:** 먼저 현재 생성된 C++ 모델과 비교 하네스를 이용해 TX 시작 경로를 재현하고, `tx_start -> sync_tx_start -> TXD` 데이터 흐름을 계측해 끊기는 지점을 특정한다. 원인이 emitter 버그인지 생성물/비교 하네스 문제인지 분리한 뒤, 최소 수정만 적용하고 생성물을 재생성해 다중 시드로 검증한다.

**Tech Stack:** C++17, hirct-gen, generated cmodel, CXXRTL compare harness, GNU Make

---

## Scope And Guardrails

- 이번 작업 범위는 `원인 특정 -> 최소 수정 -> 5시드 x 10000사이클 검증`까지만 한정한다.
- `systematic-debugging` 스킬을 따라 원인 확인 전 추측성 수정은 금지한다.
- `compare_test.cc`가 TXD mismatch를 PASS/FAIL에 반영하도록 먼저 바로잡아 검증 신뢰도를 확보한다.
- 기존 사용자 변경인 `.gitignore`, `docs/plans/risk-validation-results.md`, `examples/fc6161/pt_plat/config/env.mk`는 건드리지 않는다.
- 구현 완료 주장 전에는 `verification-before-completion` 기준으로 fresh verification을 다시 실행한다.

---

### Task 1: 재현 기준선과 검증 게이트 정렬

**Goal:** TXD mismatch가 현재도 재현되는지 확인하고, 비교 하네스가 TXD mismatch를 실제 실패로 집계하도록 검증 기준을 먼저 바로잡는다.

**Files:**
- Modify: `examples/fc6161/pt_plat/cxxrtl-poc/compare_test.cc`
- Read: `examples/fc6161/pt_plat/cxxrtl-poc/Makefile`
- Read: `examples/fc6161/pt_plat/output/uart_top/cmodel/uart_top.h`

**Steps:**
1. `compare_test.cc`의 최종 PASS/FAIL 계산에 `txd_mismatches`를 포함시킨다.
2. 필요하면 결과 출력 문구를 조정해 TXD mismatch가 있는 경우 `COMPARE FAIL`이 되도록 맞춘다.
3. `make compare`로 빌드가 유지되는지 확인한다.
4. `make test-compare CYCLES=10000`으로 현재 기준선 mismatch를 다시 확보한다.

**Run:**
```bash
make compare
make test-compare CYCLES=10000
```

**Expect:**
```text
compare_test 빌드 성공.
현재 상태에서는 최소 1개 이상 seed에서 TXD mismatch로 FAIL.
```

---

### Task 2: TX 시작 경로 계측으로 근본 원인 특정

**Goal:** `DW_apb_uart_regfile.tx_start`, `DW_apb_uart_sync.sync_tx_start`, 최종 `UART0_TXD` 중 어느 단계에서 GenModel이 CXXRTL과 갈라지는지 증거를 수집한다.

**Files:**
- Create: `examples/fc6161/pt_plat/cxxrtl-poc/uart_txd_probe.cc`
- Read: `examples/fc6161/pt_plat/output/uart_top/DW_apb_uart/cmodel/DW_apb_uart.cpp`
- Read: `examples/fc6161/pt_plat/output/uart_top/DW_apb_uart_sync/cmodel/DW_apb_uart_sync.cpp`
- Read: `examples/fc6161/pt_plat/output/uart_top/cmodel/uart_top.h`
- Read: `examples/fc6161/pt_plat/cxxrtl-poc/extended_test.cc`

**Steps:**
1. 기존 direct register access 패턴을 재사용해 UART를 직접 설정하는 probe 프로그램을 만든다.
2. probe에서 최소 다음 이벤트를 로그로 남긴다: THR write cycle, GenModel `tx_start`, GenModel `sync_tx_start`, GenModel `UART0_TXD`, CXXRTL `UART0_TXD`.
3. 대기 구간을 2000사이클 이상으로 늘려 양쪽 TX 시작 시점을 비교한다.
4. 결과를 바탕으로 root cause를 아래 중 하나로 분류한다.
   - A: regfile에서 `tx_start`가 생성되지 않음
   - B: sync 단계에서 `sync_tx_start`가 전달되지 않음
   - C: TX 블록이 `sync_tx_start`를 받아도 TXD를 구동하지 않음
   - D: compare harness clocking/ordering 문제가 원인임

**Run:**
```bash
make compare
g++ -std=c++17 -O2 -I "$(python3 -c 'import yowasp_yosys, pathlib; print(pathlib.Path(yowasp_yosys.__file__).parent / "share/include/backends/cxxrtl/runtime")')" -I . -I ../output/uart_top -I .. -o uart_txd_probe uart_txd_probe.cc $(find ../output/uart_top -path "*/cmodel/*.cpp" -print)
./uart_txd_probe 2000
```

**Expect:**
```text
THR write 이후 TX 시작 경로의 어느 단계가 멈추는지 로그로 확인 가능.
원인이 A/B/C/D 중 하나로 명확히 분류됨.
```

---

### Task 3: 최소 수정 적용

**Goal:** Task 2에서 확인한 root cause 한 군데만 수정해 GenModel TX 시작 경로를 정상화한다.

**Files:**
- Modify: `hirct/lib/Target/GenModel.cpp` 또는
- Modify: `examples/fc6161/pt_plat/cxxrtl-poc/compare_test.cc` 또는
- Regenerate: `examples/fc6161/pt_plat/output/uart_top/**`

**Steps:**
1. root cause가 emitter 쪽이면 `hirct/lib/Target/GenModel.cpp`를 최소 수정한다.
2. root cause가 생성물 재생성 문제면 `hirct/build/bin/hirct-gen`으로 `uart_top` 생성물을 다시 만든다.
3. root cause가 compare harness ordering 문제면 `compare_test.cc` 또는 probe에서 확인한 동일 ordering fix만 반영한다.
4. 수정 후 가장 작은 재현 커맨드부터 다시 실행해 fix가 symptom과 직접 연결되는지 확인한다.

**Run:**
```bash
ninja -C hirct/build hirct-gen
hirct/build/bin/hirct-gen -o examples/fc6161/pt_plat/output --top uart_top --lib-dir examples/fc6161/pt_plat/config/stubs examples/fc6161/pt_plat/skip-analysis-results/uart/preprocessed.v
make -C examples/fc6161/pt_plat/cxxrtl-poc compare
./examples/fc6161/pt_plat/cxxrtl-poc/uart_txd_probe 2000
```

**Expect:**
```text
수정 후 probe 로그에서 THR write 이후 GenModel과 CXXRTL의 TX 시작 시점 차이가 사라지거나, mismatch 구간이 직접 감소했음을 확인.
```

---

### Task 4: 최종 검증

**Goal:** 공식 비교 경로에서 5시드 x 10000사이클 동안 PRDATA/INTR/TXD 모두 0 mismatch임을 fresh evidence로 확인한다.

**Files:**
- Read: `examples/fc6161/pt_plat/cxxrtl-poc/Makefile`
- Read: `examples/fc6161/pt_plat/cxxrtl-poc/compare_test.cc`

**Steps:**
1. `make test-compare CYCLES=10000`을 실행한다.
2. 필요하면 seed별 출력에서 PRDATA/INTR/TXD 결과를 확인한다.
3. 최종 결과를 기록하되, 하나라도 mismatch가 남으면 completion claim 없이 남은 증상을 보고한다.

**Run:**
```bash
make -C examples/fc6161/pt_plat/cxxrtl-poc test-compare CYCLES=10000
```

**Expect:**
```text
5개 seed 모두 COMPARE PASS.
PRDATA, INTR, TXD mismatch 모두 0.
명령 exit code 0.
```

---

## Exit Criteria

- root cause가 로그와 함께 한 문장으로 설명 가능할 것
- 수정은 root cause 한 곳에만 집중할 것
- `make -C examples/fc6161/pt_plat/cxxrtl-poc test-compare CYCLES=10000` fresh run이 exit 0일 것
- verification 단계 전에는 성공을 주장하지 않을 것
