# Task 103: DPI-C VCS 래퍼 생성기 (GenDPIC)

> **목표**: GenDPIC 신규 작성 (DPI-C VCS 래퍼 생성기), GenModel 인터페이스 계약 수정 (reset_i/do_reset/eval_comb)
> **예상 시간**: 1일
> **파일**: `lib/Target/GenDPIC.cpp` (신규 작성)
> **TDD**: RED → GREEN → REFACTOR

---

## 현재 상태

- Bootstrap(Task 100)에서 emitter 인프라 구축 완료. GenDPIC를 신규 작성한다.
- GenDPIC는 GenModel의 do_reset/step/eval_comb 시그니처에 의존

---

## Step 1: RED — 인터페이스 불일치 확인 (30분)

**Goal**: GenModel 출력의 C++ 인터페이스(do_reset/step/eval_comb) 확인

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
grep -E "reset_i|do_reset|eval_comb" output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/cmodel/*.cpp output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/cmodel/*.h output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/dpi/*.cpp output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/dpi/*.h
```

> **Implementation Note**: 개발 중 개별 emitter 테스트 시 `--only` 필터 사용:
> `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only model`
> `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only dpic`

**Expect**:
```
불일치 항목 목록
```

---

## Step 2: GREEN — GenModel 인터페이스에 맞춤 (2시간)

**Goal**: reset_i (active-high/low), do_reset(), eval_comb() 시그니처 통일

**Run**:
```bash
# GenDPIC가 GenModel do_reset/eval_comb 호출
grep "do_reset\|eval_comb" build/_scratch/dpic/*dpi*.cpp
```

**Expect**:
```
GenModel 시그니처와 일치
```

---

## Step 3: 신규 작성 (30분)

**Goal**: GenDPIC 신규 작성

**Run**:
```bash
# lib/Target/GenDPIC.cpp 신규 작성
cmake --build build
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
```

**Expect**:
```
DPI-C 산출물 정상 생성
```

---

## Step 4: Gate — g++ -c + vcs -sverilog (30분)

**Goal**: DPI-C cpp 컴파일, VCS SV 컴파일

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
g++ -std=c++17 -c -Ioutput/plat/src/s5/design/Fadu_K2_S5_LevelGateway/dpi output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/dpi/*.cpp
# VCS 사용 가능 시
vcs -sverilog -cpp g++ -CFLAGS "-std=c++17" output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/dpi/*.sv output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/dpi/*.cpp 2>&1 | tail -5
```

**Expect**:
```
exit 0
```

---

## Step 5: 커밋

**Goal**: GenDPIC 신규 작성 및 인터페이스 수정

**Run**:
```bash
git add lib/Target/GenDPIC.cpp
git commit -m "feat(phase-1): GenDPIC 신규 작성, fix GenModel interface contract"
```

---

## 게이트 (완료 기준)

- [ ] `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v` → `output/.../dpi/*.h` + `*.cpp` + `*.sv` 존재
- [ ] `g++ -std=c++17 -c output/.../dpi/*dpi*.cpp` → exit 0
- [ ] `vcs -sverilog output/.../dpi/*.sv output/.../dpi/*.cpp` → exit 0 (VCS 사용 가능 시)
- [ ] GenModel 인터페이스 계약 (do_reset/step/eval_comb) 일치 확인
- [ ] GenDPIC 신규 작성 완료
- [ ] `test/Target/GenDPIC/` lit 테스트 최소 1개 PASS
