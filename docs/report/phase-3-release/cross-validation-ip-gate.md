# Cross-Validation IP-Top Gate — Task 304

**Date**: 2026-02-24
**VCS Version**: V-2023.12-SP2-7_Full64
**Configuration**: 10 seeds × 1000 cycles, warmup=20

---

## 1. IP-Top Gate 결과 요약

| Module | CModel | DPI | Verify | VCS Cosim | 판정 |
|--------|--------|-----|--------|-----------|------|
| Fadu_K2_S5_CLINT | Y | Y | fail | **0/10 PASS** | CModel 로직 버그 (TileLink 타이머 카운터) |
| Fadu_K2_S5_TLPLIC | Y | Y | skip | 미실행 | Verilator 컴파일 실패 → VCS 시도 필요 |
| Fadu_K2_S5_PeripheryBus | Y | Y | skip | 미실행 | 서브모듈 누락 |
| Fadu_K2_S5_RocketTile | N | N | — | — | CModel 미생성 (hw.instance 깊이) |
| Fadu_K2_S5_CoreIPSubsystem | N | N | — | — | CModel 미생성 |

## 2. VCS CLINT 상세

**Command**:
```
vcs -full64 -sverilog -timescale=1ns/1ps +define+VCS \
  -P $VERDI_PLI_DIR/novas.tab $VERDI_PLI_DIR/pli.a \
  -LDFLAGS "-L$VERDI_PLI_DIR -lnovas" \
  -cpp g++ -CFLAGS "-std=c++17 -I.../cmodel -I.../dpi" \
  rtl/plat/src/s5/design/Fadu_K2_S5_CLINT.v \
  .../dpi/Fadu_K2_S5_CLINT_dpi.{sv,cpp} \
  .../cmodel/Fadu_K2_S5_CLINT.cpp \
  vcs-cosim/tb/ip-top/tb_Fadu_K2_S5_CLINT.sv \
  -o vcs-cosim/results/ip-top/simv_CLINT
```

**Exit code**: 0 (build), 2 (all 10 seeds FAIL)

**핵심 로그 (per-seed)**:
```
seed=1: RESULT: FAIL 186 mismatches
seed=2: RESULT: FAIL 215 mismatches
seed=3: RESULT: FAIL 214 mismatches
seed=4: RESULT: FAIL 209 mismatches
seed=5: RESULT: FAIL 201 mismatches
seed=6: RESULT: FAIL 191 mismatches
seed=7: RESULT: FAIL 215 mismatches
seed=8: RESULT: FAIL 182 mismatches
seed=9: RESULT: FAIL 219 mismatches
seed=10: RESULT: FAIL 202 mismatches
```

## 3. CLINT Mismatch 분석

- **mismatch 비율**: ~18-22% (182~219 / 1000 cycles)
- **주요 포트**: `auto_in_d_bits_data` (64-bit TileLink 응답 데이터)
- **추정 원인**: CLINT 내부 `time_` 64-bit 카운터와 `timecmp_0` 레지스터 비교 로직의 CModel 불일치
- **verify-report 상태**: `fail` (Verilator 검증에서도 동일 mismatch 확인)

## 4. IP-Top 제한 사항

| 제한 | 영향 | 해소 시점 |
|------|------|----------|
| TLPLIC: Verilator SKIP → VCS 미시도 | 서브모듈 컴파일 문제 | hirct-gen 개선 후 |
| PeripheryBus: 서브모듈 누락 | 인스턴스 하위 모듈 RTL 부재 | filelist 보강 후 |
| RocketTile/CoreIPSubsystem: CModel 미생성 | hw.instance 깊이 제한 | GenModel 계층 지원 후 |

## 5. TB 경로

- `vcs-cosim/tb/ip-top/tb_Fadu_K2_S5_CLINT.sv`

## 6. 결론

- IP-top gate는 CLINT 1개 모듈에서만 VCS cosim 실행 가능
- CLINT: **FAIL** (CModel 로직 버그 — TileLink 타이머 카운터 불일치)
- 나머지 IP-top 모듈은 CModel 미생성 또는 컴파일 실패로 SKIP
- IP-top gate 확대를 위해서는 GenModel의 hw.instance 깊이 지원 확장이 필수
