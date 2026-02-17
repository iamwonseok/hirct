# Task 109: 자동 검증 (hirct-verify)

> **목표**: IR 포트 정보로 verify_<module>.cpp 자동 생성, Verilator 모델 + 비교 드라이버 빌드, 다중 시드 실행
> **예상 시간**: 3일
> **Phase**: 1A (Core Pipeline)
> **파일**: `lib/Target/GenVerify.cpp` (드라이버 생성), `tools/hirct-verify/main.cpp` (별도 바이너리), per-module `Makefile` (`make test-verify` 타겟)
> **TDD**: RED → GREEN → REFACTOR

---

## 현재 상태

- 수동 검증 드라이버 없음. GenVerify emitter를 신규 작성하여 자동 생성한다.
- GenVerify emitter 및 hirct-verify CLI를 Task 100 스켈레톤 기반으로 구현한다.

---

## Step 1: RED — 자동 생성 없음 (10분)

**Goal**: hirct-verify 호출 시 수동 드라이버만 존재 확인

**Run**:
```bash
hirct-verify rtl/plat/src/s5/design/Fadu_K2_S5_PMPChecker.v 2>&1 | head -5
```

**Expect**:
```
hirct-verify 바이너리 미존재
```

---

## Step 2: GREEN — GenVerify.cpp로 verify_<module>.cpp 자동 생성 (3시간)

**Goal**: hirct-gen 내 GenVerify emitter가 IR 포트 정보로 verify_<module>.cpp 자동 생성

**Run**:
```bash
hirct-verify rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
ls output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/verify/verify_*.cpp
grep "VLevelGateway\|LevelGateway" output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/verify/verify_*.cpp | head -5
```

**Expect**:
```
verify_Fadu_K2_S5_LevelGateway.cpp (모듈명 기준)
VModule, Model 인스턴스화
```

---

## Step 3: Verilator 모델 빌드 연동 (2시간)

**Goal**: Verilator로 RTL 래퍼 빌드. RTL 원본 경로는 per-module Makefile의 `RTL_SRC` 변수에 절대경로로 저장됨.

**Run**:
```bash
hirct-verify rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
# Verilator 빌드 (per-module Makefile이 RTL_SRC 절대경로를 사용)
cd output/plat/src/s5/design/Fadu_K2_S5_LevelGateway && make -n test-verify | head -5
```

> **경로 규칙**: GenMakefile.cpp는 `RTL_SRC`에 프로젝트 루트 기준 **절대경로**를 기록한다.
> 이를 통해 output 하위 디렉토리에서 실행해도 원본 RTL에 안정적으로 접근 가능하다.

**Expect**:
```
verilator --cc <절대경로>/rtl/.../Fadu_K2_S5_LevelGateway.v ...
```

---

## Step 4: 비교 드라이버 빌드 (2시간)

**Goal**: verify_<module> 바이너리 빌드 (g++ + Verilator obj + hirct-gen C++ 모델)

**Run**:
```bash
hirct-verify rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
# 스크립트가 빌드까지 수행
cd output/plat/src/s5/design/Fadu_K2_S5_LevelGateway && make test-verify
```

**Expect**:
```
PASS: ... pass, 0 fail
```

---

## Step 5: 다중 시드 실행 (1시간)

**Goal**: 10 seeds × 1000 cycles

**Run**:
```bash
for seed in 42 123 456 789 1024 2048 4096 8192 16384 32768; do
  (cd output/plat/src/s5/design/Fadu_K2_S5_LevelGateway && make test-verify SEED=$seed) || exit 1
done
echo "ALL SEEDS PASS"
```

**Expect**:
```
ALL SEEDS PASS
```

---

## Step 6: Gate — LevelGateway + RVCExpander PASS (30분)

**Goal**: 두 모듈 모두 자동 생성 드라이버로 PASS

**Run**:
```bash
cd output/plat/src/s5/design/Fadu_K2_S5_LevelGateway && make test-verify
cd output/plat/src/s5/design/Fadu_K2_S5_RVCExpander && make test-verify
```

**Expect**:
```
PASS (둘 다)
```

---

## Step 7: 커밋

**Goal**: hirct-verify 자동 검증 기능

**Run**:
```bash
git add lib/Target/GenVerify.cpp tools/hirct-verify/main.cpp
git commit -m "feat(phase-1): hirct-verify - auto-generate verify driver, per-module Makefile, multi-seed run"
```

---

## 완료 체크리스트

- [ ] verify_<module>.cpp 자동 생성
- [ ] Verilator RTL 모델 빌드
- [ ] 비교 드라이버 빌드
- [ ] 10 seeds × 1000 cyc 실행
- [ ] LevelGateway, RVCExpander 자동 드라이버 PASS
- [ ] GenVerify.cpp emitter 구현 + CMakeLists.txt 수정
- [ ] `test/Target/GenVerify/` lit 테스트 최소 1개 PASS
- [ ] 커밋 완료

---

## Verilator obj_dir 캐시 규칙

`make test-verify`는 Verilator `obj_dir/`를 캐시하여 다중 시드 실행 시 빌드를 반복하지 않는다:

- `obj_dir/` 존재 + RTL 원본(RTL_SRC) 미변경 → `verilator --cc --build` 스킵
- `make test-verify SEED=N` → obj_dir 재사용, 드라이버만 재실행
- `make test-verify-rebuild` → 강제 재빌드 (obj_dir 삭제 후 재생성)

이 규칙은 GenMakefile.cpp가 생성하는 per-module Makefile에 반영한다.
