# Task 105: IR 기반 RTL 포매터 (GenFormat)

> **목표**: IR 보조 RTL 주석/섹션 삽입, 원본 RTL 미수정, rtl/ 서브디렉에 출력
> **예상 시간**: 3일
> **Phase**: 1B (Remaining Emitters — Phase 2와 병행 가능)
> **파일**: `lib/Target/GenFormat.cpp` (신규)
> **TDD**: RED → GREEN → REFACTOR
>
> **Scope 확정** (실측 검증 완료, `risk-validation-results.md` §3):
>
> CIRCT에 `--export-verilog`, `--export-split-verilog`, `--prettify-verilog` pass가 존재한다.
> 따라서 MLIR에서 Verilog를 자체 재구성할 필요 없이, CIRCT ExportVerilog 파이프라인을 호출한다.
>
> **GenFormat 구현 방식**:
> 1. `CirctRunner`로 ExportVerilog 파이프라인 호출:
>    `circt-verilog input.v | circt-opt --lower-seq-to-sv | circt-opt --export-verilog`
> 2. 출력된 Verilog에 IR 분석 기반 주석 삽입 (섹션 구분, 포트 그룹핑)
> 3. `rtl/` 서브디렉토리에 저장
>
> **제약 사항**:
> - `--export-verilog`는 `seq` dialect을 직접 지원하지 않음 — `--lower-seq-to-sv` 선행 필수
> - 출력에 FIRRTL 초기화 매크로(`RANDOMIZE_REG_INIT` 등)가 포함됨 — **제거 정책**: GenFormat이 후처리로 `ifdef RANDOMIZE` 블록을 strip한다 (원본 RTL에 없는 코드이므로)
>
> **난이도**: 보통 (주석 삽입 로직이 본체, Verilog 재구성은 CIRCT에 위임)

---

## 현재 상태

- 신규: GenFormat.cpp 없음
- CIRCT ExportVerilog 동작 확인 완료 (LevelGateway 기준)

---

## Step 1: RED — GenFormat 미존재 (5분)

**Goal**: GenFormat 호출 시 산출물 없음 확인

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
ls output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/rtl/ 2>/dev/null || echo "no rtl subdir"
```

> **Implementation Note**: 개발 중 개별 emitter 테스트 시 `--only` 필터 사용:
> `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only format`

**Expect**:
```
no rtl subdir
```

---

## Step 2: GREEN — 섹션 주석 삽입 (3시간)

**Goal**: Parameters, Signals, Instances, Comb, Seq 섹션 주석 생성

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
grep -E "// --- |// === " output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/rtl/*.v | head -15
```

> **Implementation Note**: 개발 중 개별 emitter 테스트 시 `--only` 필터 사용:
> `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only format`

**Expect**:
```
// --- Parameters ---
// --- Signals ---
// --- Instances ---
// --- Comb ---
// --- Seq ---
```

---

## Step 3: 포트 그룹핑 (GenWrapper 로직 재사용) (1시간)

**Goal**: 104 태스크(GenWrapper)와 동일 prefix 기반 그룹핑

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
# 포트 그룹 주석 확인
grep "io_\|ctrl_\|data_" output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/rtl/*.v | head -10
```

> **Implementation Note**: 개발 중 개별 emitter 테스트 시 `--only` 필터 사용:
> `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only format`

**Expect**:
```
포트별 그룹 주석
```

---

## Step 4: 토폴로지 순서 코드 (2시간)

**Goal**: IR 의존 순서로 assign/always 블록 재배치

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
# 토폴로지 순서 확인 (원본과 다를 수 있으나 의미 동등)
```

> **Implementation Note**: 개발 중 개별 emitter 테스트 시 `--only` 필터 사용:
> `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only format`

**Expect**:
```
의존 순서 반영
```

---

## Step 5: 원본 RTL 미수정 (10분)

**Goal**: 원본 파일은 읽기만, output/rtl/ 에 새 파일 출력

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
diff rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/rtl/*.v 2>/dev/null | head -5 || true
```

> **Implementation Note**: 개발 중 개별 emitter 테스트 시 `--only` 필터 사용:
> `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only format`

**Expect**:
```
원본과 다름 (주석/순서 추가된 새 파일)
```

---

## Step 6: Gate — verilator --lint-only (15분)

**Goal**: 포맷된 RTL lint 통과

**Run**:
```bash
verilator --lint-only -Wall output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/rtl/*.v
```

**Expect**:
```
exit 0
```

---

## Step 7: 커밋

**Goal**: GenFormat 기본 기능 완료

**Run**:
```bash
git add lib/Target/GenFormat.cpp
git commit -m "feat(phase-1): GenFormat - IR-assisted RTL annotation and section comments"
```

---

## ModuleAnalyzer 변경 사항

**재사용**: `groupPortsByPrefix()` (104에서 추가됨)
**신규 구현**: `std::vector<Statement> topoSortStatements() const`
- IR 의존성 그래프에서 토폴로지 순서를 계산하여 assign/always 블록 재배치

---

## 게이트 (완료 기준)

- [ ] `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v` → `output/.../rtl/*.v` 존재
- [ ] `verilator --lint-only -Wall output/.../rtl/*.v` → exit 0
- [ ] 섹션 주석 포함 확인: `grep -c '// ---' output/.../rtl/*.v` → 1 이상
- [ ] 원본 RTL 미수정: `diff rtl/.../Fadu_K2_S5_LevelGateway.v` → 변경 없음
- [ ] 최소 등가성 확인: 포맷 RTL로 Verilator 재빌드 → 기존 verify 드라이버로 PASS (전제: Phase 0 pre-test에서 원본 RTL verify PASS 확인됨)
- [ ] `test/Target/GenFormat/` lit 테스트 최소 1개 PASS
