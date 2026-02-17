# Task 104: SV Interface 래퍼 생성기 (GenWrapper)

> **목표**: 포트 분석 기반 SV Interface 래퍼 생성 (신규)
> **예상 시간**: 2일
> **파일**: `lib/Target/GenWrapper.cpp` (신규)
> **TDD**: RED → GREEN → REFACTOR

---

## 현재 상태

- 신규: GenWrapper.cpp 없음

---

## Step 1: RED — GenWrapper 미존재 (5분)

**Goal**: GenWrapper 호출 시 산출물 없음 확인

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
ls output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/wrapper/ 2>/dev/null || echo "no wrapper"
```

> **Implementation Note**: 개발 중 개별 emitter 테스트 시 `--only` 필터 사용:
> `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only wrapper`

**Expect**:
```
no wrapper
```

---

## Step 2: GREEN — 최소 래퍼 생성 (2시간)

**Goal**: 포트 분석으로 기본 modport 구조 SV 생성

**Run**:
```bash
# GenWrapper.cpp 구현 후
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
ls output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/wrapper/
head -30 output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/wrapper/*.sv
```

> **Implementation Note**: 개발 중 개별 emitter 테스트 시 `--only` 필터 사용:
> `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only wrapper`

**Expect**:
```
interface ... endinterface
modport ...
```

---

## Step 3: prefix 기반 포트 그룹핑 (2시간)

**Goal**: io_, ctrl_, data_ 등 prefix로 그룹화

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
grep -E "input|output" output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/wrapper/*.sv | head -20
```

> **Implementation Note**: 개발 중 개별 emitter 테스트 시 `--only` 필터 사용:
> `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only wrapper`

**Expect**:
```
포트가 prefix별로 그룹화된 modport
```

---

## Step 4: Gate — verilator --lint-only (30분)

**Goal**: 생성 래퍼 + 원본 RTL lint 통과

**Run**:
```bash
verilator --lint-only -Wall output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/wrapper/*.sv rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
```

**Expect**:
```
exit 0
```

---

## Step 5: 커밋

**Goal**: GenWrapper 최소 기능 완료

**Run**:
```bash
git add lib/Target/GenWrapper.cpp
git commit -m "feat(phase-1): GenWrapper - SV interface wrapper from port analysis"
```

---

## ModuleAnalyzer 변경 사항

**신규 구현** (이 태스크에서 `lib/Analysis/ModuleAnalyzer.cpp`에 추가):
- `std::map<std::string, std::vector<PortInfo>> groupPortsByPrefix() const`
- 포트 이름에서 공통 접두사(io_plic_, io_uart_ 등)를 추출하여 그룹화
- 105(gen-format)에서 재사용됨

---

## 게이트 (완료 기준)

- [ ] `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v` → `output/.../wrapper/*.sv` 존재
- [ ] `verilator --lint-only -Wall output/.../wrapper/*.sv rtl/.../Fadu_K2_S5_LevelGateway.v` → exit 0
- [ ] 생성된 SV에 `interface ... endinterface` + `modport` 포함 확인
- [ ] prefix 기반 포트 그룹핑 동작 확인
- [ ] `test/Target/GenWrapper/` lit 테스트 최소 1개 PASS
