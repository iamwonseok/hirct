# Cross-Validation Diff: VCS vs ncsim — Task 304

**Date**: 2026-02-24
**VCS**: V-2023.12-SP2-7_Full64 (`/tools/synopsys/vcs/V-2023.12-SP2-7`)
**ncsim**: 15.10-s010 (`/tools/cadence/INCISIVE151`)
**Configuration**: 10 seeds × 1000 cycles, warmup=10/20, timescale=1ns/1ps

---

## 1. 이중 게이트 비교표

| Module | Level | VCS Result | ncsim Result | 일치 여부 | 비고 |
|--------|-------|-----------|-------------|----------|------|
| LevelGateway | module | **10/10 PASS** | **10/10 PASS** | **YES** | 양 시뮬레이터 모두 CModel 정확 판정 |
| Queue_11 | module | **1/10 PASS** | **0/10 PASS** | **YES**¹ | 양 시뮬레이터 모두 CModel 버그 판정 |
| CLINT | ip-top | **0/10 PASS** | 미실행² | — | CModel 로직 버그 확인 |

¹ VCS seed=8만 PASS (우연). ncsim seed=8은 FAIL. PRNG 구현 차이(IEEE 1800 명세).
² ncsim CLINT TB 미실행. VCS 결과만 확보.

## 2. PRNG 차이 분석

VCS와 ncsim은 `process::self().srandom(seed)` / `$urandom_range()`의 PRNG 알고리즘이 다릅니다.
IEEE 1800 표준은 PRNG 구현을 규정하지 않으므로 **동일 seed ≠ 동일 랜덤 시퀀스**입니다.

| Seed | VCS Queue_11 | ncsim Queue_11 | 동일 판정? |
|------|-------------|---------------|-----------|
| 1    | FAIL (650)  | FAIL (1000)   | YES (둘 다 FAIL) |
| 8    | PASS (0)    | FAIL (992)    | NO — VCS 우연 PASS |

**결론**: per-seed 결과는 직접 비교 불가. **전체 seed 집합의 PASS/FAIL 판정**으로 비교해야 합니다.

## 3. 시뮬레이터별 차이점

| 항목 | VCS V-2023.12 | ncsim 15.10 |
|------|--------------|-------------|
| SystemVerilog 표준 | IEEE 1800-2017 | IEEE 1800-2009 |
| DPI-C 링킹 | 내장 (vcs -cpp) | 외부 shared lib (-sv_lib) |
| FSDB 지원 | 네이티브 (+Verdi PLI) | 미지원 |
| VCD 지원 | `$dumpfile()` | `$dumpfile()` (상수 문자열만) |
| C++ ABI | 시스템 libstdc++ | 번들 libstdc++ 6.0.13³ |
| 4-state | 완전 4-state | 완전 4-state |

³ IUS 15.1의 번들 libstdc++가 CXXABI_1.3.9 미지원 → `-static-libstdc++` + `LD_PRELOAD` 필요

## 4. TB 호환성

기존 TB의 `string` 변수를 `$dumpfile()`에 전달하는 코드가 ncsim에서 에러 발생:
```
ncelab: *E,STRNOT: Passing string variable to this system task/function is currently not supported.
```

**해결**: `ifdef VCS` 가드 + `reg [8*64-1:0]` 타입으로 변경 완료.

## 5. 환경 설정

```bash
# VCS
export VCS_HOME=/tools/synopsys/vcs/V-2023.12-SP2-7
export VERDI_HOME=/tools/synopsys/verdi/V-2023.12-SP2-7
export LM_LICENSE_FILE=27020@fdn37

# ncsim
export IUS_HOME=/tools/cadence/INCISIVE151
g++ -std=c++17 -shared -fPIC -static-libstdc++ -I$IUS_HOME/tools/include ...
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6 ncverilog ...
```

## 6. Waveform 아티팩트

| File | Format | Module | Seed | Cycles |
|------|--------|--------|------|--------|
| `vcs-cosim/results/vcs/Queue_11_s1.vcd` | VCD | Queue_11 | 1 | 100 |

## 7. 결론

- **LevelGateway**: VCS/ncsim 이중 게이트 일치 → CModel 정확성 **확인됨**
- **Queue_11**: VCS/ncsim 모두 FAIL → CModel 버그 **확정** (시뮬레이터 무관)
- **CLINT**: VCS에서 FAIL 확인, ncsim 추가 검증 가능
- per-seed 비교는 PRNG 차이로 무의미 → 전체 PASS/FAIL 판정 기준 채택
- IUS 15.1의 C++ ABI/SystemVerilog 호환 문제는 workaround로 해소
