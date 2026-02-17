# Task 101: C++ cycle-accurate 모델 생성기 (GenModel)

> **목표**: GenModel C++ cycle-accurate 모델 생성기 신규 작성 (IR operation 커버리지 확장)
> **예상 시간**: 5일
> **Phase**: 1A (Core Pipeline)
> **파일**: `lib/Target/GenModel.cpp` (Task 100 Bootstrap 스켈레톤 확장)
> **TDD**: RED → GREEN → REFACTOR

---

## 현재 상태

- Task 100(Bootstrap)에서 `lib/Target/GenModel.cpp` 스켈레톤 생성 완료
- 스켈레톤 상태: 포트 정보로 빈 .h + .cpp 생성 가능
- 이 태스크에서 IR operation 해석 로직을 추가하여 **cycle-accurate 모델**로 완성
- **Phase 1A 지원 (24개 op)**: comb.and/or/xor/mux/icmp/extract/concat/shl/shru/add/sub/mul/shrs/parity/replicate, seq.firreg/compreg/to_clock(무시), hw.constant/output/array_get/array_create/array_inject/aggregate_constant/bitcast (실측 근거: `risk-validation-results.md` §2)
- **Phase 1A 미지원 → Error + XFAIL**: seq.firmem/firmem.read_port, llhd.sig/prb/process
- **hw.instance**: CIRCT flatten pass로 해소, 실패 시 Error + 진단 메시지 (open-decisions A-3)

---

## Step 1: RED — Bootstrap 스켈레톤 상태 확인 (20분)

**Goal**: Bootstrap 스켈레톤이 빌드되고 최소 출력이 가능함을 확인

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only model
ls output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/cmodel/
g++ -std=c++17 -c output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/cmodel/*.cpp
```

**Expect**:
```
.h + .cpp 파일 존재, 컴파일 성공 (빈 step() 함수)
```

---

## Step 2: GREEN — IR operation 해석 로직 추가 (4시간)

**Goal**: MLIR의 comb/seq operation을 C++ 코드로 변환하는 로직 구현

GenModel이 IR의 각 operation을 순서대로 C++ 문장으로 번역한다.
ModuleAnalyzer에서 토폴로지 정렬된 operation 목록을 받아 순차적으로 emit.

**핵심 매핑** (590개 .v 파일의 MLIR 출력에서 실측된 전체 operation, 빈도순):

| CIRCT IR Op | C++ 코드 | 빈도 | 비고 |
|---|---|---|---|
| `comb.extract` | `out = (src >> lo) & mask;` | 42,239 | `comb.extract %src from <lo>` |
| `comb.icmp` | `out = (a op b);` | 21,580 | eq/ne/slt/ult 등 predicate |
| `hw.constant` | `const <type> name = <value>;` | 21,486 | 상수 정의 |
| `comb.and` | `out = a & b;` | 12,107 | 다중 피연산자 가능 |
| `comb.mux` | `out = sel ? a : b;` | 10,892 | `comb.mux bin` 형식 |
| `comb.xor` | `out = a ^ b;` | 7,814 | |
| `comb.concat` | `out = (hi << width) \| lo;` | 4,632 | |
| `comb.or` | `out = a \| b;` | 4,524 | 다중 피연산자 가능 |
| `seq.firreg` | `reg_next = ...;` (step()에서 `reg = reg_next;`) | 4,254 | 레지스터 |
| `comb.add` | `out = a + b;` | 715 | |
| `comb.sub` | `out = a - b;` | 357 | |
| `seq.to_clock` | (무시) | 322 | 클럭 변환용 |
| `comb.shru` | `out = a >> b;` | 313 | 논리 시프트 |
| `hw.array_get` | `out = arr[idx];` | 290 | |
| `comb.shl` | `out = a << b;` | 247 | |
| `comb.replicate` | `out = replicate(val, N);` | 242 | 비트 반복 |
| `hw.array_create` | `arr = {a, b, c};` | 185 | 배열 초기화 |
| `hw.aggregate_constant` | 구조체/배열 상수 | 140 | |
| `hw.array_inject` | `arr[idx] = val;` | 113 | 배열 원소 대입 |
| `hw.bitcast` | `out = static_cast<T>(a);` | 36 | 타입 변환 |
| `comb.parity` | `out = __builtin_parity(a);` | 18 | XOR 축소 |
| `comb.mul` | `out = a * b;` | 8 | |
| `comb.shrs` | `out = (signed)a >> b;` | 3 | 산술 시프트 |
| `seq.compreg` | `reg_next = ...;` (step()에서 `reg = reg_next;`) | — | 동기 리셋 레지스터 (seq.firreg와 동일 패턴) |
| `hw.output` | (출력 포트 할당) | — | `io_out = <value>;` |

**Phase 1A 미지원 → Error 처리 (Phase 2 XFAIL 대상)**:

| CIRCT IR Op | 빈도 | 사유 | 대응 |
|---|---|---|---|
| `seq.firmem` / `seq.firmem.read_port` | 241 | 메모리 시뮬레이션은 별도 인프라 필요 | Error + skip + XFAIL |
| `llhd.sig` / `llhd.prb` / `llhd.process` | 156+ | LLHD dialect — 시뮬레이션 시맨틱이 다름 | Error + skip + XFAIL |

> (근거: `docs/plans/risk-validation-results.md` §2 — 590/1,597 파일 실측)

**미지원 op 처리**: 매핑 테이블에 없는 operation을 만나면:
1. stderr에 에러 출력: `ERROR: unsupported op '<opName>' in module '<moduleName>'`
2. 해당 모듈의 gen-model을 **hard fail** 처리 (placeholder=0 금지 — 가짜 성공 방지)
3. `meta.json`에 `"gen-model": {"result": "fail", "reason": "unsupported op: <opName>"}` 기록
4. 다른 emitter(gen-tb, gen-doc 등)는 계속 실행 (ModuleAnalyzer 정보는 여전히 유효)
5. `known-limitations.md`에 등록된 경우 lit에서 XFAIL 처리

> **근거**: placeholder=0은 "컴파일은 되지만 시뮬레이션 불일치"를 허용하여,
> verify 단계에서야 실패가 발견된다. 이는 실패 원인 추적을 어렵게 하고,
> `hirct-convention.md` §5 실패 분류 체계(`fail` = emitter 실패)와 충돌한다.
> hard fail은 실패를 즉시 표면화하여 XFAIL 관리와 일관성을 유지한다.

**Run**:
```bash
cmake --build build
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v --only model
g++ -std=c++17 -c output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/cmodel/*.cpp
```

**Expect**:
```
step() 함수에 실제 연산 로직 포함, 컴파일 성공
```

---

## Step 3: hw.instance — CIRCT flatten 폴백 TDD (2시간)

**Goal**: CIRCT flatten pass 먼저 시도 → 성공 시 flat IR로 GenModel → 실패 시 Error + 진단 메시지

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
# hw.instance 포함 모듈이 있으면 GenModel가 처리, 컴파일 에러 없음 확인
g++ -std=c++17 -c output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/cmodel/*.cpp 2>&1 | tail -5
```

> **Implementation Note**: 개발 중 IR을 보존하려면:
> `hirct-gen rtl/.../hierarchical_module.v -o build/_scratch/out`
> (생성된 .mlir 파일이 output 디렉토리에 보존됨)

**Expect**:
```
생성된 C++ 모델이 hw.instance 참조 처리
```

---

## Step 4: 누락 op 추가 (op별 TDD) (각 1~2시간)

**Goal**: IR에서 발견되는 미지원 op逐一 추가

**Run**:
```bash
hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v
# comb.neg, arith.addi 등 발견 시 GenModel가 처리
g++ -std=c++17 -c output/plat/src/s5/design/Fadu_K2_S5_LevelGateway/cmodel/*.cpp
```

> **Implementation Note**: 개발 중 개별 emitter 테스트 시 `--only` 필터 사용:
> `hirct-gen <해당 op 포함 모듈>.v --only model`

**Expect**:
```
exit 0
```

---

## Step 5: 커밋

**Goal**: GenModel IR op 커버리지 확장 완료

**Run**:
```bash
git add lib/Target/GenModel.cpp include/hirct/Target/GenModel.h
git commit -m "feat(phase-1): GenModel — cycle-accurate C++ model generator with IR op coverage"
```

---

## 게이트 (완료 기준)

- [ ] `hirct-gen rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v` → `output/.../cmodel/Fadu_K2_S5_LevelGateway.h` + `.cpp` 존재
- [ ] `g++ -std=c++17 -c output/.../cmodel/Fadu_K2_S5_LevelGateway.cpp` → exit 0
- [ ] GenModel 출력 C++ 모델이 LevelGateway의 모든 IR op를 포함
- [ ] GenModel 출력 C++ 모델이 RVCExpander의 모든 IR op를 포함
- [ ] hw.instance: CIRCT flatten pass 시도 → 실패 시 Error + 진단 메시지 출력 확인
- [ ] `test/Target/GenModel/` lit 테스트 최소 1개 PASS (예: `// RUN: hirct-gen %s --only model | FileCheck %s`)
