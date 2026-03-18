# Task 305: GenTB Cosim TB 자동생성 계획

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** `hirct-gen`이 module 단위 `*_cosim_tb.sv`를 자동 생성해, 동일 TB로 VCS/ncsim에서 RTL vs CModel lock-step 비교를 수행할 수 있게 한다.

**Architecture:** 기존 `GenTB`의 단순 자극형 TB 생성(`emit`)은 유지하고, 병행으로 cosim 전용 TB 생성(`emit_cosim`)을 추가한다. `GenMakefile`에 `test-vcs-cosim`/`test-ncsim-cosim` 타겟을 자동 생성해 per-module 검증 루프를 통합한다.

**Tech Stack:** C++17 (`GenTB`, `GenMakefile`, `hirct-gen`), SystemVerilog DPI-C, VCS, ncsim

---

## 배경

- 현재는 `vcs-cosim/tb/*.sv`를 수동 관리하고 있어 확장성과 일관성이 낮다.
- `GenTB`는 단순 TB만 생성하며 RTL vs DPI 비교 하네스를 자동 생성하지 않는다.
- 목표는 "하나의 생성 TB"를 여러 시뮬레이터에서 재사용하는 것이다.

---

## 구현 범위

### 1) GenTB 확장 (핵심)

**파일**
- 수정: `include/hirct/Target/GenTB.h`
- 수정: `lib/Target/GenTB.cpp`

**작업**
- `emit_cosim(const std::string &output_dir)` 추가
- 생성 파일: `tb/<ModuleName>_cosim_tb.sv`
- 구성:
  - RTL 인스턴스 `u_rtl`
  - DPI wrapper 인스턴스 `u_dpi`
  - `+seed/+cycles/+warmup/+dump` plusarg
  - warmup 이후 negedge 비교
  - mismatch 로그: `seed/cycle/port/rtl/model`
  - `RESULT: PASS/FAIL` 요약
  - waveform: 기본 VCD, `ifdef VCS`에서 FSDB 지원

### 2) preset_flops 정책 반영

**파일**
- 수정: `lib/Target/GenTB.cpp`

**작업**
- `preset_flops` 감지 함수 추가
- 리셋 시퀀스:
  1. 시작 시 `preset_flops=1`
  2. 첫 posedge 후 `preset_flops=0`
  3. 비교 구간에서 고정
- 랜덤 자극에서 `clock/reset/preset_flops` 제외

### 3) GenMakefile 확장

**파일**
- 수정: `lib/Target/GenMakefile.cpp`

**작업**
- `test-vcs-cosim` 타겟 생성
- `test-ncsim-cosim` 타겟 생성
- 공통 파라미터:
  - `SEED ?= 1`
  - `CYCLES ?= 1000`
  - `WARMUP ?= 10`
  - `DUMP ?= vcd`
- ncsim ABI workaround 포함:
  - `-static-libstdc++`
  - 필요 시 `LD_PRELOAD`

### 4) hirct-gen 연결

**파일**
- 수정: `tools/hirct-gen/main.cpp`

**작업**
- 기존 `GenTB::emit` 호출 직후 `emit_cosim` 호출
- 실패 시 `meta.json`에 emitter 실패 반영되도록 오류 전파

### 5) 테스트 추가

**파일**
- 생성: `test/Target/GenTB/cosim-basic.test`

**검증 포인트**
- 생성 TB에 `u_rtl`, `u_dpi`, `RESULT:`, `MISMATCH` 존재
- plusarg(`seed/cycles/warmup/dump`) 파싱 코드 존재
- `preset_flops` 고정 시퀀스 존재

---

## 실행/검증 절차

1. `make build`
2. `ninja -C build check-hirct`
3. 샘플 모듈 생성:
   - `build/bin/hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v -o output/tmp-lg`
4. 생성물 확인:
   - `output/tmp-lg/Fadu_K2_S5_LevelGateway/tb/Fadu_K2_S5_LevelGateway_cosim_tb.sv`
5. VCS 검증:
   - `make -C output/tmp-lg/Fadu_K2_S5_LevelGateway test-vcs-cosim SEED=1 CYCLES=1000`
6. ncsim 검증:
   - `make -C output/tmp-lg/Fadu_K2_S5_LevelGateway test-ncsim-cosim SEED=1 CYCLES=1000`
7. 회귀 확인:
   - `make test-all`

---

## 완료 기준 (DoD)

- `GenTB`가 cosim TB 자동 생성
- `GenMakefile`이 VCS/ncsim cosim 타겟 자동 생성
- LevelGateway 기준 VCS/ncsim 실행 성공
- lit 신규 테스트 PASS
- `make test-all` 회귀 없음

---

## 리스크

- ncsim(IUS 15.1) C++ ABI 제약으로 환경별 링크 이슈 가능
- PRNG 구현 차이로 VCS/ncsim per-seed 1:1 결과 비교는 불가
- SoC 계층은 `hw.instance` 제약으로 후속 단계 필요

