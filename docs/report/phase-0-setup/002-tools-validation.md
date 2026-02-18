# Task 002 게이트 검증 리포트: 외부 도구 체인 Validation

> **검증 일시**: 2026-02-18
> **브랜치**: `feature/hirct-phase0`
> **환경**: Ubuntu 24.04 (x86_64), kernel 6.14.0-27-generic
>
> **역할**: Task 002 검증 항목의 실측 데이터를 기록한다.
> Task 001과 중복되는 항목(G11~G14, G22)은 [Task 001 리포트](001-setup-env.md) 참조.

---

## 종합 판정: [V] ALL PASS (7/7 + 추가 2건)

---

## Task 001 중복분

| 항목 | Task 001 근거 | 결과 |
|------|-------------|------|
| circt-verilog → LevelGateway MLIR | G11 | [V] PASS (784 bytes) |
| verilator → LevelGateway 빌드 | G12 | [V] PASS (22,698 bytes) |
| g++ C++17 컴파일 | G13 | [V] PASS |
| Python stdlib import | G14 | [V] PASS |
| vcs -ID | G22 | [V] PASS (V-2023.12-SP2-7_Full64) |

---

## Task 002 고유 항목

### circt-verilog RVCExpander 변환 (§1단계)

| 항목 | 결과 | 실측 |
|------|------|------|
| `circt-verilog` → RVCExpander.v MLIR 변환 | [V] PASS | 12,672 bytes |

### Verilator RVCExpander 빌드 (§2단계)

| 항목 | 결과 | 실측 |
|------|------|------|
| `verilator --cc` → RVCExpander 빌드 | [V] PASS | `libVFadu_K2_S5_RVCExpander.a` 생성 |

---

## 추가 검증 (문서 §1단계 심화)

### circt-opt flatten pass

| 항목 | 결과 | 실측 |
|------|------|------|
| `circt-opt --hw-flatten-modules` LevelGateway | [V] PASS | 785 bytes (flatten 후) |

### multi-file + --top 테스트

| 항목 | 결과 | 실측 |
|------|------|------|
| `circt-verilog --top=Fadu_K2_S5_AXI4Buffer` multi-file | [V] PASS | 15,591 bytes |
| unknown module 에러 | 없음 | `--timescale=1ns/1ps` 사용 |
