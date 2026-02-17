# HIRCT 리스크 검증 결과 (실측 기반)

> **검증일**: 2026-02-16
> **환경**: CIRCT `5e760efa9`, Verilator 5.020, RTL 1,597 .v 파일
> **목적**: 계획서 리뷰에서 도출된 3가지 리스크를 실제 도구 실행으로 검증

---

## 1. MLIR 정규화 (`circt-opt --canonicalize`)

### 검증 명령

```bash
diff <(circt-verilog LevelGateway.v) \
     <(circt-verilog LevelGateway.v | circt-opt --canonicalize)
```

### 결과: 사실상 동일 (trailing newline 1줄 차이만)

`circt-verilog`의 원본 출력이 이미 정규화된 형태이다.
`--canonicalize`를 거쳐도 op 구조, 이름, 순서가 바뀌지 않는다.

### 방향성

- 현재 계획: "필수 가드레일: `circt-opt --canonicalize` 파이프라인"
- 변경안: **"선택적 안정화 단계"로 톤 하향**
  - 원본 출력이 이미 안정적이므로 필수가 아님
  - 단, CIRCT 버전 업데이트 대비 안전장치로 유지는 권장
  - 파이프라인에 넣어도 성능 비용이 작음 (LevelGateway 기준 +225ms)

---

## 2. GenModel IR op 커버리지

### 검증 명령

```bash
# 전체 1,597 파일 파싱 성공률
for f in $(find rtl/ -name "*.v"); do circt-verilog "$f" >/dev/null 2>&1; done

# 성공 파일의 op 빈도 집계
for f in $(find rtl/ -name "*.v"); do circt-verilog "$f" 2>/dev/null; done \
  | grep -oP '^\s+%\S+ = \K[a-z]+\.\S+' | sort | uniq -c | sort -rn
```

### 결과: 590/1,597 파일 파싱 성공 (36%)

실패 원인은 대부분 `unknown module` (외부 모듈 의존성).
단일 파일 모드에서는 590개만 처리 가능하며, 나머지는 **의존 파일을 함께 입력하는 multi-file 모드 + `--top` 지정**이 필요하다.
HIRCT의 filelist(`hirct-gen -f`)는 이 목적을 위해 filelist를 파싱/확장하여 최종적으로 `circt-verilog <many-files> --top=...` 형태로 호출한다.

### 전체 op 빈도표 (590파일 기준)

| op | 빈도 | 101 매핑 테이블 | 상태 |
|---|---|---|---|
| `comb.extract` | 42,239 | 있음 | OK |
| `comb.icmp` | 21,580 | 있음 | OK |
| `hw.constant` | 21,486 | 있음 | OK |
| `comb.and` | 12,107 | 있음 | OK |
| `comb.mux` | 10,892 | 있음 | OK |
| `comb.xor` | 7,814 | 있음 | OK |
| `comb.concat` | 4,632 | 있음 | OK |
| `comb.or` | 4,524 | 있음 | OK |
| `seq.firreg` | 4,254 | 있음 | OK |
| `comb.add` | 715 | **없음** | **추가 필요** |
| `comb.sub` | 357 | **없음** | **추가 필요** |
| `seq.to_clock` | 322 | 있음 (무시) | OK |
| `comb.shru` | 313 | 있음 | OK |
| `hw.array_get` | 290 | 있음 | OK |
| `comb.shl` | 247 | 있음 | OK |
| `comb.replicate` | 242 | 있음 | OK |
| `seq.firmem` + `seq.firmem.read_port` | 241 | **없음** | **Phase 2 대응** |
| `hw.array_create` | 185 | **없음** | **추가 필요** |
| `llhd.sig` / `llhd.prb` / `llhd.process` | 156+ | **없음** | **XFAIL (다른 dialect)** |
| `hw.aggregate_constant` | 140 | **없음** | **추가 필요** |
| `hw.array_inject` | 113 | **없음** | **추가 필요** |
| `hw.bitcast` | 36 | **없음** | **추가 필요** |
| `comb.parity` | 18 | **없음** | **추가 필요** |
| `comb.mul` | 8 | **없음** | **추가 필요** |
| `comb.shrs` | 3 | **없음** | **추가 필요** |

### 방향성

**Phase 1A에서 추가해야 할 op (9개)**:

| op | C++ 매핑 | 난이도 |
|---|---|---|
| `comb.add` | `out = a + b;` | 쉬움 |
| `comb.sub` | `out = a - b;` | 쉬움 |
| `comb.mul` | `out = a * b;` | 쉬움 |
| `comb.shrs` | `out = (int)a >> b;` (산술 시프트) | 쉬움 |
| `comb.parity` | `out = __builtin_parity(a);` | 쉬움 |
| `hw.bitcast` | `out = static_cast<T>(a);` | 보통 |
| `hw.array_create` | 배열 초기화 | 보통 |
| `hw.aggregate_constant` | 구조체/배열 상수 | 보통 |
| `hw.array_inject` | `arr[idx] = val;` | 보통 |

**Phase 1A에서 미지원 Error 처리 (Phase 2 XFAIL 대상)**:

| op | 사유 | 대응 |
|---|---|---|
| `seq.firmem` | 메모리 시뮬레이션은 별도 인프라 필요 | Error + XFAIL |
| `llhd.*` | LLHD dialect — 시뮬레이션 시맨틱이 다름 | Error + XFAIL |

**파싱 성공률 36%의 의미**:

- 64%의 파일이 실패하는 주 원인은 `unknown module` (외부 의존성)
- 이는 hirct-gen의 문제가 아니라 **단일 파일 모드의 한계**
- filelist 모드(`-f`)에서는 성공률이 크게 올라갈 것으로 예상
- Phase 2 Task 202(Top 순회)에서 filelist 기반 빌드가 이를 해소

---

## 3. GenFormat scope (CIRCT ExportVerilog)

### 검증 명령

```bash
# ExportVerilog 관련 pass 확인
circt-opt --help | grep -i "export.*verilog"

# 실제 Verilog 재생성 테스트
circt-verilog LevelGateway.v | circt-opt --lower-seq-to-sv | circt-opt --export-verilog
```

### 결과: ExportVerilog 존재, Verilog 재생성 동작 확인

| pass | 설명 |
|---|---|
| `--export-verilog` | IR을 SystemVerilog 파일로 출력 |
| `--export-split-verilog` | IR을 모듈별 개별 파일로 출력 |
| `--prepare-for-emission` | ExportVerilog 전처리 |
| `--prettify-verilog` | 출력 품질 향상 |
| `--lower-seq-to-sv` | seq dialect을 sv dialect으로 변환 (ExportVerilog 전제) |

**동작 파이프라인**:

```
circt-verilog input.v
  | circt-opt --lower-seq-to-sv
  | circt-opt --export-verilog
  → 완전한 SystemVerilog 출력 (module ... endmodule)
```

**제약 사항**:

- `--export-verilog`는 seq dialect을 직접 지원하지 않음 — `--lower-seq-to-sv` 선행 필수
- `--prepare-for-emission`은 `--export-verilog`와 같은 `circt-opt` 호출에서 사용 불가 — 별도 호출 필요
- 출력에 FIRRTL 초기화 매크로(`RANDOMIZE_REG_INIT` 등)가 포함됨

### 방향성

- 현재 계획: "scope 불명확, CIRCT ExportVerilog 조사 필요"
- 변경안: **scope 확정 — CIRCT ExportVerilog 호출 + 주석 삽입**
  - GenFormat은 MLIR에서 Verilog를 자체 재구성할 필요 없음
  - `CirctRunner`로 ExportVerilog 파이프라인을 외부 프로세스 호출
  - 출력에 IR 분석 기반 섹션 주석, 포트 그룹핑 주석만 추가
  - 난이도: "어려움" → **"보통"**
  - 예상 시간: 3일 유지 (주석 삽입 로직이 본체)

---

## 4. 계획서 반영 요약

| 항목 | 현재 계획서 | 실측 기반 변경 |
|---|---|---|
| MLIR 정규화 | 필수 가드레일 | 선택적 안정화 (원본 이미 안정) |
| 101 op 매핑 테이블 | 15개 op | 24개 op (+9개 추가) |
| 101 미지원 분류 | 없음 | `seq.firmem`, `llhd.*` → Error + XFAIL |
| 파싱 성공률 | 미정 (~1,600 가정) | 36% 단일 파일 (590/1,597) |
| 105 GenFormat | scope 불명확 | CIRCT ExportVerilog 호출 + 주석 삽입 |
| 105 난이도 | 어려움 | 보통 |

---

---

## 4. circt-opt --flatten 실측 검증

### 검증 결론 (실측)

- 이 환경(CIRCT `5e760efa9`)에서 `circt-opt --flatten` 옵션은 **존재하지 않는다**.
- 대신 계층 flatten(인라인)은 `circt-opt --hw-flatten-modules` pass로 수행한다.
  - public 모듈까지 인라인하려면 pass 옵션을 `--hw-flatten-modules=hw-inline-public`로 지정한다.

### 검증 명령 (실행)

```bash
# (1) hw-flatten-modules pass 존재 확인
circt-opt --help | grep -i "hw-flatten-modules"

# (2) LevelGateway MLIR에 적용 (변환 성공 여부 확인)
circt-verilog rtl/plat/src/s5/design/Fadu_K2_S5_LevelGateway.v \
  -o /tmp/hirct-pretest/LevelGateway.mlir
circt-opt --hw-flatten-modules /tmp/hirct-pretest/LevelGateway.mlir \
  -o /tmp/hirct-pretest/LevelGateway_hw_flat.mlir

# (3) public 모듈까지 인라인하는 옵션 예시 (옵션은 = 형태로 지정)
circt-opt --hw-flatten-modules=hw-inline-public /tmp/hirct-pretest/LevelGateway.mlir \
  -o /tmp/hirct-pretest/LevelGateway_hw_flat_public.mlir
```

### 결과

- `circt-opt --flatten` → FAIL (Unknown argument)
- `circt-opt --hw-flatten-modules` → PASS (MLIR 출력 생성)

### 방향성

- 계획서의 “flatten pass” 표기는 `--hw-flatten-modules`로 통일한다.
- Phase 1의 “Flatten 또는 Error” 정책은 `circt-opt --hw-flatten-modules(필요 시 =hw-inline-public)`를 먼저 시도하고,
  실패 시 Error + 진단 메시지로 폴백한다.

---

## 5. filelist 모드 파싱 성공률 검증

### 검증 결론 (실측)

- 이 환경(CIRCT `5e760efa9`)에서 `circt-verilog -f filelist.f`는 **지원되지 않는다**.
- 대신 `circt-verilog`는 **여러 입력 파일을 인자로 받는 multi-file 모드**를 지원하며,
  `--top=<name>`과 `--timescale=...`를 함께 주면 단일 파일에서 실패(unknown module)하던 케이스가 성공한다.
- HIRCT의 filelist(`hirct-gen -f`)는 **우리 쪽에서 파싱/확장**하여, 최종적으로 `circt-verilog <many-files> --top=...` 형태로 호출하면 된다.

### 검증 명령 (실행)

```bash
# 단일 파일 모드 실패 예시 (unknown module)
circt-verilog rtl/plat/src/s5/design/Fadu_K2_S5_AXI4Buffer.v

# multi-file + top + default timescale (혼재 timescale 에러 방지)
circt-verilog --timescale=1ns/1ps --top=Fadu_K2_S5_AXI4Buffer \
  rtl/plat/src/s5/design/*.v \
  -o /tmp/hirct-pretest/AXI4Buffer_multifile_ts.mlir
```

### 결과

- 단일 파일: `unknown module 'Fadu_K2_S5_Queue_2'` 등으로 FAIL
- multi-file + `--timescale` + `--top`: PASS (MLIR 생성 성공)

### 방향성

- Phase 2 Top 순회(`-f --top`)는 “filelist 파싱”이 핵심이며, CIRCT에는 multi-file로 전달한다.
- `--timescale`은 multi-file 입력에서 timescale 정의가 섞인 RTL을 다루기 위한 **필수 가드레일 후보**다.

---

## 변경 이력

| 날짜 | 내용 |
|---|---|
| 2026-02-16 | 신규 작성: 3가지 리스크 실측 검증 |
| 2026-02-16 | §4 flatten, §5 filelist 검증 TODO 스텁 추가 |
