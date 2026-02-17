# Task 102: SV 테스트벤치 골격 생성기 (GenTB)

> **목표**: GenTB 신규 작성 (테스트벤치 골격 생성기), Verilog 컨벤션 적용
> **예상 시간**: 0.5일
> **파일**: `lib/Target/GenTB.cpp` (신규 작성)
> **TDD**: RED → GREEN → REFACTOR

---

## 현재 상태

- Bootstrap(Task 100)에서 emitter 인프라 구축 완료. GenTB를 신규 작성한다.

---

## Step 1: RED — 현재 출력 확인 (15분)

**Goal**: GenTB가 생성할 테스트벤치 구조 설계

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
ls output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/tb/
head -40 output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/tb/*.sv 2>/dev/null | head -50
```

> **Implementation Note**: 개발 중 개별 emitter 테스트 시 `--only` 필터 사용:
> `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only tb`

**Expect**:
```
기존 테스트벤치 구조 확인
```

---

## Step 2: GREEN — 신규 작성 (20분)

**Goal**: GenTB 신규 작성

**Run**:
```bash
# lib/Target/GenTB.cpp 신규 작성
cmake --build build
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
ls output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/tb/
```

> **Implementation Note**: 개발 중 개별 emitter 테스트 시 `--only` 필터 사용:
> `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only tb`

**Expect**:
```
테스트벤치 산출물 생성
```

---

## Step 3: Verilog 컨벤션 적용 (30분)

**Goal**: .cursor/convention/verilog.md 규칙 반영

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
# 포맷, 네이밍 등 확인
ls output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/tb/
```

> **Implementation Note**: 개발 중 개별 emitter 테스트 시 `--only` 필터 사용:
> `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only tb`

**Expect**:
```
컨벤션 준수 출력
```

---

## Step 4: Gate — verilator --lint-only (15분)

**Goal**: 테스트벤치 + 원본 RTL lint 통과

**Run**:
```bash
verilator --lint-only -Wall output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/tb/*.sv rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
```

**Expect**:
```
exit 0
```

---

## Step 5: 커밋

**Goal**: GenTB 신규 작성 및 컨벤션 적용

**Run**:
```bash
git add lib/Target/GenTB.cpp
git commit -m "feat(phase-1): GenTB 신규 작성, apply Verilog convention"
```

---

## 게이트 (완료 기준)

- [ ] `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v` → `output/.../tb/*.sv` 존재
- [ ] `verilator --lint-only -Wall output/.../tb/*.sv rtl/.../Fadu_K2_S5_LevelGateway.v` → exit 0
- [ ] GenTB 신규 작성 완료
- [ ] Verilog 컨벤션 (.cursor/convention/verilog.md) 적용 확인
- [ ] `test/Target/GenTB/` lit 테스트 최소 1개 PASS
