# HIRCT 계획-구현 Blocker 근본원인 분석 보고서

> **작성일**: 2026-03-10
> **분석 범위**: Phase 0 ~ Phase 4-F (2026-02-17 ~ 2026-03-10)
> **데이터 소스**: 에이전트 채팅 기록 (~200 세션), git 히스토리 (200+ 커밋), known-limitations.md (KL-1~KL-18), open-decisions.md (32건), Phase별 리포트

---

## Section 1: 이슈 카탈로그

Phase 0~4 전체 기간 동안 발생한 blocker/이슈를 6개 카테고리로 분류한다.

---

### 카테고리 A: IR 시맨틱 오분석 (5건)

계획 단계에서 대상 IR Op의 동작 가정이 부정확하여, 구현 후 런타임에서 예상과 다른 결과가 발생한 경우.

| ID | 이슈 | 발생 Phase | 증상 | 해결 방법 | 소요 시간 (추정) |
|----|------|-----------|------|----------|-----------------|
| A-1 | `build_clock_domain_map()` 빈 결과 반환 | Phase 4+ (3월) | pass-through 모듈(레지스터 없음)에서 `is_multi_clock=false`로 판정. `step_uart_pclk()`/`step_uart_clk()` 미생성 | `count_clock_registers_through_instances`로 인스턴스 내부 레지스터 재귀 탐색 | 4~6시간 |
| A-2 | `is_clock_port()` 휴리스틱 실패 | Phase 4+ (3월) | `_pclk` suffix 미등록으로 UART_PCLK을 clock으로 인식 못함 | suffix 목록에 `_pclk` 추가 → 이후 IR 기반 분석으로 전면 교체 | 2시간 (수정) + 8시간 (IR 기반 전환) |
| A-3 | `flatten_block` cond_br 미처리 | Phase 4+ (3월) | `cf.CondBranchOp`의 true/false dest가 모두 wait_block인 경우 → `step()` 본문 빈 출력 | `flatten_block`에 wait_block 직접 타겟 분기 추가 | 3~4시간 |
| A-4 | `CompRegOp expr(reg.getInput())` forward reference | Phase 4+ (3월) | 아직 val map에 없는 SSA를 참조 → 빈 문자열 반환 → 잘못된 C++ 코드 | `deferred_regs` 2단계 emit (정방향 참조 후처리) | 4시간 |
| A-5 | `comb.mux` 배열 타입에서 false_e 스칼라 "0" | Phase 4+ (3월) | 배열 타입 mux의 false 분기가 `"0"` 스칼라로 emit → g++ 타입 에러 | 배열 타입 감지 후 초기화 리스트로 emit | 2시간 |

---

### 카테고리 B: CIRCT Upstream 한계 (4건)

CIRCT 자체의 미지원/버그로 hirct 파이프라인이 차단된 경우. hirct 측에서 수정 불가능하거나 우회만 가능.

| ID | 이슈 | 발생 Phase | 증상 | 해결 방법 | 소요 시간 (추정) |
|----|------|-----------|------|----------|-----------------|
| B-1 | `llhd.drv` 파라미터 의존 비트폭 타입 불일치 (KL-14) | Phase 4+ (3월) | 동적 배열 인덱스 드라이브에서 `moore→core` lowering 실패 → hirct-gen exit 1 | XFAIL 처리. 중기: widened-drive 재작성 pass | BLOCKED (upstream) |
| B-2 | `moore.concat_ref` legalization 실패 | Phase 4 (3월) | verilator -E 없이 직접 import 시 6개 모듈 Stage 3a 실패 | verilator -E 전처리로 L2 concat_ref 해소 | 8시간 (verilator -E 전체 구현) |
| B-3 | `--llhd-mem2reg` segfault (KL-18) | Phase 4 (3월) | `hw.module` 입력 포트에서 null-ptr dereference → exit 139 | Stage 2 pass 목록에서 제거 | 1시간 (우회) |
| B-4 | `cf.br/cf.cond_br` 잔존 → arcilator NO-GO | Phase 4 (3월) | FSM 패턴의 control flow가 hw/comb로 완전히 lowering 안 됨 | Arc PoC CONDITIONAL GO 판정. 커스텀 pass 4종으로 부분 해소 | 16시간+ (pass 4종 구현) |

---

### 카테고리 C: Makefile/환경 설정 오류 (3건)

빌드/실행 환경의 구성 오류로 허위 PASS 또는 실패가 발생한 경우.

| ID | 이슈 | 발생 Phase | 증상 | 해결 방법 | 소요 시간 (추정) |
|----|------|-----------|------|----------|-----------------|
| C-1 | PROJECT 환경변수 미export | Phase 4 (3월) | pipeline-audit Makefile에서 `$(PROJECT)` 빈 문자열 → 경로 오류 | `export PROJECT` 추가 | 30분 |
| C-2 | HIRCT_GEN 경로 오타 | Phase 4 (3월) | `hirct-gen` 바이너리를 찾지 못해 전체 stage 실패 | 경로 수정 | 30분 |
| C-3 | 0바이트 입력 mlir → hirct-gen exit 0 (허위 pass) | Phase 4 (3월) | Stage 3b/3c가 빈 입력으로 성공 판정 → 실제로는 아무것도 생성 안 됨 | 입력 파일 크기 검증 추가 | 1시간 |

---

### 카테고리 D: 병합 후 회귀 (3건)

기능 브랜치 병합 시 기존 동작이 깨진 경우.

| ID | 이슈 | 발생 Phase | 증상 | 해결 방법 | 소요 시간 (추정) |
|----|------|-----------|------|----------|-----------------|
| D-1 | `task-batch-c` 병합 후 async-reset SSA 깨짐 | Phase 2 (2월) | 병합 후 async-reset 처리 코드의 SSA 매핑이 덮어쓰여짐 → verify mismatch | `fix: preserve async-reset SSA handling after task-batch-c merge` | 4시간 |
| D-2 | `verilator-preprocess-pipeline` 병합 후 롤백 | Phase 4 (3월) | 병합 직후 테스트 실패 → soft reset (`reset: moving to c922e60`) | 롤백 후 재병합 | 2시간 |
| D-3 | Phase 1 bugfix-gate 후속 수정 다수 | Phase 1 (2월) | BUG-1/2/3 연쇄 수정. eval_comb 타이밍, SSA regex, extract_bit_width | 개별 fix 커밋 3건 | 6~8시간 |

---

### 카테고리 E: 계획-구현 Gap (3건)

계획 문서의 가정과 실제 시스템 상태가 불일치하여 구현 시 차단된 경우.

| ID | 이슈 | 발생 Phase | 증상 | 해결 방법 | 소요 시간 (추정) |
|----|------|-----------|------|----------|-----------------|
| E-1 | 계획 파일 미존재 상태에서 구현 진행 | Phase 4+ (3월) | `.cursor/plans/` 경로에 계획 파일 없음 → 에이전트가 즉석 설계로 구현 | 계획 파일 없이 직접 구현 진행 (품질 저하 위험) | 낭비 시간 추정 불가 |
| E-2 | `--run-pass` 단일 pass만 지원 | Phase 4 (3월) | pipeline 가정으로 여러 pass를 순차 실행해야 했으나 CLI가 단일 pass만 수용 → 중간 pass 누락 | `--pipeline` 옵션 구현 + `--pipeline-checkpoint-dir` 추가 | 4시간 |
| E-3 | `signal-lowering` pass segfault 미예측 | Phase 4 (3월) | 계획에서 pass가 안정적이라 가정했으나 실제 실행 시 segfault | pass 디버깅 + 방어 코드 추가 | 3시간 |

---

### 카테고리 F: GenModel 코드 생성 버그 (8건)

GenModel이 생성하는 C++ 코드의 시맨틱/구문 오류.

| ID | 이슈 | 발생 Phase | 증상 | 해결 방법 | 소요 시간 (추정) |
|----|------|-----------|------|----------|-----------------|
| F-1 | 음수 상수 비트폭 미정규화 | Phase 3 (2월) | Queue_11 FAIL. 음수 상수가 부호 확장 없이 절삭 | `normalize negative constants to unsigned bit-width` | 4시간 |
| F-2 | SSA regex/extract_bit_width 파싱 실패 | Phase 1 (2월) | RVCExpander에서 SSA 이름 파싱 실패 → 잘못된 변수명 | regex 패턴 확장 + extract_bit_width 로직 수정 | 3시간 |
| F-3 | eval_comb() 타이밍 순서 오류 | Phase 1 (2월) | register-to-output 경로에서 eval_comb 1회만 호출 → 이전 사이클 값 출력 | `step()` 내 eval_comb() 2회 호출 (pre/post register update) | 6시간 |
| F-4 | mem-write order (write_latency=1) | Phase 2 (3월) | FIFO 메모리 쓰기 순서가 읽기 전에 발생 → 데이터 corruption | step() 내 mem-write 순서 교정 | 4시간 |
| F-5 | width parsing (concat/icmp/extract/parity) | Phase 2 (3월) | 비트폭 계산 오류로 잘못된 타입 캐스팅 → verify mismatch | 각 op별 비트폭 계산 로직 수정 (4건 일괄) | 6시간 |
| F-6 | `hw.array_inject` 미지원 (KL-16) | Phase 4+ (3월) | 배열 reg/next SSA 중복 선언 + 스칼라에 subscript 적용 → g++ 에러 | hw.array_inject 지원 추가 (배열 원소 접근 C++ 코드 생성) | 8시간 |
| F-7 | `hw.bitcast` 배열 타입 미지원 | Phase 4+ (3월) | 배열↔정수 변환 코드 미생성 → g++ 에러 | 타입 감지 후 reinterpret 코드 생성 | 3시간 |
| F-8 | `AggregateConstantOp` 배열 타입을 `uint64_t`로 선언 | Phase 4+ (3월) | 배열 상수가 스칼라로 선언 → 타입 불일치 | 배열 타입 감지 후 `std::array` 초기화 | 2시간 |

---

### 발생 빈도 요약

| 카테고리 | 건수 | 누적 소요 시간 (추정) | 주 발생 Phase |
|---------|------|---------------------|-------------|
| A: IR 시맨틱 오분석 | 5 | ~24시간 | Phase 4+ |
| B: CIRCT Upstream 한계 | 4 | ~26시간+ | Phase 4 |
| C: Makefile/환경 설정 | 3 | ~2시간 | Phase 4 |
| D: 병합 후 회귀 | 3 | ~14시간 | Phase 1~4 |
| E: 계획-구현 Gap | 3 | ~7시간+ | Phase 4 |
| F: GenModel 코드 생성 버그 | 8 | ~36시간 | Phase 1~4+ |
| **합계** | **26** | **~109시간+** | |

---

## Section 2: 근본 원인 분석 (5-Why 기법)

### 카테고리 A: IR 시맨틱 오분석

```
1. Why: clock domain map이 빈 결과를 반환 / is_clock_port()가 UART_PCLK을 미인식
2. Why: 분석 함수가 레지스터 존재에 의존 / suffix 목록이 하드코딩
3. Why: IR Op의 입력 가정(invariant)이 명시되지 않아 edge case 미식별
4. Why: 계획 단계에서 대상 IR의 Op 조합 + 모듈 구조 다양성을 열거하지 않음
5. Why: "대표 모듈 1개로 설계 → 전체 적용" 패턴. IR Op별 입출력 계약(contract) 정의 단계 부재
```

**근본 원인**: 구현 전 대상 IR Op의 타입/시맨틱 계약을 명시적으로 문서화하는 단계가 없다. 대표 사례 1~2개로 설계하고, 다양한 모듈 구조(pass-through, multi-clock, FSM, 배열 등)에 대한 edge case를 사전에 열거하지 않는다.

---

### 카테고리 B: CIRCT Upstream 한계

```
1. Why: llhd.drv 타입 불일치 / mem2reg segfault / concat_ref legalization 실패
2. Why: CIRCT의 moore→core lowering이 해당 RTL 패턴을 완전히 지원하지 않음
3. Why: hirct가 의존하는 CIRCT 기능의 지원 범위를 사전에 확인하지 않음
4. Why: CIRCT의 pass/lowering 커버리지를 실측하는 단계가 계획에 없음
5. Why: 외부 의존성(CIRCT)의 기능 경계를 "사용 시점"에서야 발견하는 구조
```

**근본 원인**: 외부 의존성(CIRCT)의 기능 커버리지를 계획 단계에서 실측하지 않는다. "CIRCT가 지원한다"는 가정하에 구현을 시작하고, 실제 RTL 패턴을 투입해야 미지원 사항을 발견한다.

---

### 카테고리 C: Makefile/환경 설정 오류

```
1. Why: PROJECT 미export / HIRCT_GEN 경로 오타 / 0바이트 입력 허위 pass
2. Why: Makefile을 수동 작성하고 실행 검증 없이 다음 단계로 진행
3. Why: "Makefile이 동작한다"는 가정. dry-run(make -n) 또는 최소 입력 smoke 테스트 미수행
4. Why: Makefile 변경 후 검증 게이트가 없음 (코드 변경에는 lit/gtest가 있지만 빌드 설정 변경에는 없음)
5. Why: 빌드/환경 설정도 "코드"로 취급하여 테스트해야 한다는 인식 부재
```

**근본 원인**: 빌드/환경 설정 변경에 대한 검증 게이트가 없다. 코드 변경에는 lit/gtest가 자동 검증하지만, Makefile/환경 변수 변경은 수동 확인에 의존한다.

---

### 카테고리 D: 병합 후 회귀

```
1. Why: task-batch-c 병합 후 async-reset SSA 깨짐 / verilator-preprocess 병합 후 롤백
2. Why: 병합 전 영향 범위 분석 없이 merge 수행
3. Why: 기능 브랜치가 독립적이라는 가정. 실제로는 GenModel 내부 상태(val map, SSA 네이밍)가 공유
4. Why: 병합 전 regression gate가 `make test-all` 수준이나, 특정 모듈 조합의 edge case를 커버하지 못함
5. Why: 브랜치별 변경이 영향을 미치는 코드 경로를 식별하는 diff-based impact analysis가 없음
```

**근본 원인**: 병합 전 diff 기반 영향 범위 분석이 없다. `make test-all`은 전체 회귀를 잡지만, 특정 모듈 조합의 교차 영향(cross-cutting concern)은 커버하지 못한다.

---

### 카테고리 E: 계획-구현 Gap

```
1. Why: 계획 파일 미존재 상태에서 구현 진행 / --run-pass가 pipeline 가정과 불일치
2. Why: 계획 문서가 현재 시스템의 실제 상태를 반영하지 않음
3. Why: 계획 작성 후 시스템이 변경되었으나 계획을 업데이트하지 않음
4. Why: 계획-시스템 간 정합성 검증 단계가 없음 (plan-readiness-check 스킬이 있으나 일관 적용 안 됨)
5. Why: plan-readiness-check가 optional로 취급됨. mandatory gate가 아님
```

**근본 원인**: plan-readiness-check가 필수 게이트가 아니다. 계획이 현재 시스템 상태와 맞는지 검증하지 않고 구현을 시작한다.

---

### 카테고리 F: GenModel 코드 생성 버그

```
1. Why: 음수 상수 절삭 / eval_comb 타이밍 / width parsing / 배열 타입 미지원
2. Why: GenModel이 처리해야 하는 Op 조합의 전체 목록이 없음
3. Why: "발견된 Op만 처리"하는 반응적(reactive) 방식. 선제적(proactive) Op 카탈로그 없음
4. Why: MLIR IR에 어떤 Op가 어떤 타입 조합으로 나타나는지 전수 조사(Op census) 없이 구현
5. Why: 구현 전 대상 IR의 Op 분포를 실측하는 단계가 없음
```

**근본 원인**: 구현 대상 IR의 Op 분포를 사전에 실측(census)하지 않는다. 실제 RTL을 변환한 MLIR에 어떤 Op가 어떤 타입 조합으로 출현하는지 파악하지 않고, 대표 사례만으로 구현한 뒤 새로운 Op를 만날 때마다 패치한다.

---

### 근본 원인 종합

6개 카테고리의 근본 원인을 종합하면 **3가지 구조적 결함**으로 수렴한다:

| # | 구조적 결함 | 관련 카테고리 |
|---|-----------|------------|
| **R1** | **IR Contract 부재**: 대상 IR Op의 타입/시맨틱/출현 분포를 사전에 정의·실측하지 않음 | A, F |
| **R2** | **외부 의존성 사전 검증 부재**: CIRCT upstream 기능 커버리지를 "사용 시점"에서야 발견 | B, E |
| **R3** | **변경 영향 범위 분석 부재**: 코드/설정 변경 후 영향 받는 경로를 식별하지 않고 전체 테스트에만 의존 | C, D |

---

## Section 3: Flow 상 추가 단계 제안

현재 운영 표준 루프:

```
진입 → plan-readiness-check → 실행 → verification-before-completion → auditing → 마무리
```

아래 5개 단계를 추가/강화하여 재발을 방지한다.

---

### Gate 1: Pre-Implementation IR Census (R1 대응)

**삽입 위치**: plan-readiness-check 이후, 실행 전

**목적**: 구현 대상 IR의 Op 분포를 실측하여, 처리해야 하는 Op 목록과 타입 조합을 사전에 확정한다.

**수행 내용**:
1. 대표 RTL 파일 3~5개를 MLIR로 변환
2. MLIR 내 고유 Op 종류 + 타입 조합을 열거 (`mlir-opt --print-ir-after-all` 또는 커스텀 스크립트)
3. 각 Op에 대해 입력/출력 타입 계약(contract)을 명시
4. 구현 계획에 "이 Op는 이 타입 조합으로 처리한다"를 기록
5. 미지원 Op는 사전에 XFAIL 또는 scope-out으로 분류

**게이트 기준**: Op census 문서가 존재하고, 모든 Op에 대해 "처리/미지원" 판정이 기록되어 있을 것.

**방지 효과**: 카테고리 A (IR 시맨틱 오분석 5건), 카테고리 F (GenModel 코드 생성 버그 8건) = **13건**

---

### Gate 2: Upstream Capability Smoke (R2 대응)

**삽입 위치**: plan-readiness-check 내부 (기존 스킬 강화)

**목적**: CIRCT upstream 기능에 의존하는 계획 항목에 대해, 실제 RTL 입력으로 해당 기능의 동작을 사전 확인한다.

**수행 내용**:
1. 계획에서 CIRCT pass/lowering에 의존하는 항목을 식별
2. 대표 RTL 1건으로 해당 pass를 실제 실행 (`circt-opt --<pass> input.mlir`)
3. 성공/실패를 기록. 실패 시 우회 방안을 계획에 추가
4. CIRCT 버전과 pass 이름을 계획에 명시

**게이트 기준**: CIRCT 의존 항목 전부에 대해 smoke 실행 결과가 기록되어 있을 것.

**방지 효과**: 카테고리 B (CIRCT Upstream 한계 4건), 카테고리 E-2/E-3 (계획-구현 Gap 2건) = **6건**

---

### Gate 3: Build/Config Smoke (R3 대응)

**삽입 위치**: 실행 단계 내, Makefile/환경 변수 변경 직후

**목적**: 빌드/환경 설정 변경이 실제로 동작하는지 즉시 검증한다.

**수행 내용**:
1. Makefile 변경 시: `make -n <target>` (dry-run)으로 명령어 확인
2. 환경 변수 변경 시: `echo $VAR` + 의존 타겟 1건 실행
3. 새로운 파이프라인 stage 추가 시: 최소 입력 1건으로 end-to-end 실행
4. 0바이트/빈 입력에 대한 방어 검증

**게이트 기준**: 빌드/설정 변경 후 dry-run + 최소 입력 smoke가 PASS일 것.

**방지 효과**: 카테고리 C (Makefile/환경 설정 오류 3건) = **3건**

---

### Gate 4: Merge Impact Analysis (R3 대응)

**삽입 위치**: finishing-a-development-branch 스킬 내, 병합 전

**목적**: 기능 브랜치 병합 전 변경이 영향을 미치는 코드 경로를 식별하고, 해당 경로의 테스트를 강화한다.

**수행 내용**:
1. `git diff main..HEAD --name-only`로 변경 파일 목록 추출
2. 변경 파일이 의존하는/의존되는 파일 식별 (include/import 그래프)
3. 영향 범위에 해당하는 lit/gtest 테스트를 선별 실행
4. GenModel 내부 상태(val map, SSA naming) 변경 시: 기존 PASS 모듈 중 대표 3개 re-verify

**게이트 기준**: `make test-all` PASS + 영향 범위 선별 테스트 PASS + 대표 모듈 re-verify PASS.

**방지 효과**: 카테고리 D (병합 후 회귀 3건) = **3건**

---

### Gate 5: Plan-Reality Sync (R2 대응)

**삽입 위치**: plan-readiness-check를 mandatory gate로 승격

**목적**: 계획 문서가 현재 시스템 상태와 일치하는지 검증한다.

**수행 내용**:
1. 계획에서 참조하는 파일/CLI 옵션/pass가 실제로 존재하는지 확인
2. 계획의 가정(전제 조건)이 현재 코드에서 유효한지 검증
3. 계획 파일 자체의 존재 여부 확인 (`.cursor/plans/` 경로)
4. 불일치 발견 시: 계획 업데이트 → 재검증 → Go/No-Go 재판정

**게이트 기준**: plan-readiness-check Go 판정 필수. No-Go 시 구현 시작 금지.

**방지 효과**: 카테고리 E (계획-구현 Gap 3건) = **3건**

---

### 추가 단계 요약 다이어그램

```
진입
  │
  ▼
plan-readiness-check ◀── [Gate 5: Plan-Reality Sync] (mandatory)
  │                   ◀── [Gate 2: Upstream Capability Smoke]
  │
  ▼
[Gate 1: IR Census] ──── Op 분포 실측 + 타입 계약 문서화
  │
  ▼
실행
  │
  ├── 코드 변경 → 기존 verification-before-completion
  ├── Makefile/환경 변경 → [Gate 3: Build/Config Smoke]
  └── 브랜치 병합 → [Gate 4: Merge Impact Analysis]
  │
  ▼
verification-before-completion
  │
  ▼
auditing-plan-report-sync
  │
  ▼
마무리
```

---

## Section 4: 우선순위 매트릭스

### Impact x Likelihood 매트릭스

```
              높은 재발 가능성          낮은 재발 가능성
           ┌────────────────────┬────────────────────┐
  높은     │  ★★★ 최우선         │  ★★ 중요            │
  피해     │                    │                    │
           │  A: IR 시맨틱 오분석  │  B: CIRCT Upstream │
           │  F: GenModel 버그    │                    │
           │                    │                    │
           ├────────────────────┼────────────────────┤
  낮은     │  ★ 개선              │  관찰               │
  피해     │                    │                    │
           │  E: 계획-구현 Gap    │  C: Makefile 오류   │
           │  D: 병합 후 회귀     │                    │
           │                    │                    │
           └────────────────────┴────────────────────┘
```

**판정 근거**:

| 카테고리 | Impact | Likelihood | 이유 |
|---------|--------|-----------|------|
| A | 높음 (디버깅 4~8시간/건) | 높음 (새 모듈마다 재발) | 새 RTL 패턴을 투입할 때마다 미처리 Op 조합 발견 |
| F | 높음 (verify mismatch → 원인 추적 어려움) | 높음 (Op 조합이 무한) | GenModel이 처리하는 Op 종류가 계속 증가 |
| B | 높음 (hirct-gen exit 1 → 전체 차단) | 낮음 (CIRCT 버전 고정 후 안정) | 동일 CIRCT 빌드 사용 시 새로운 upstream 한계 발생 빈도 낮음 |
| D | 보통 (병합 후 4~8시간 디버깅) | 보통 (병합 빈도에 비례) | 활발한 개발 기간에만 발생 |
| E | 보통 (방향 수정 + 재작업) | 보통 (계획 변경 빈도에 비례) | plan-readiness-check 적용 시 감소 |
| C | 낮음 (30분~1시간 수정) | 낮음 (일회성 설정 오류) | 환경이 안정화되면 재발 거의 없음 |

### 대책 우선순위

| 순위 | 대책 | 대상 카테고리 | 방지 건수 | 구현 비용 |
|------|------|------------|----------|----------|
| **1** | Gate 1: IR Census | A, F | 13건 | 중 (스크립트 + 문서 템플릿) |
| **2** | Gate 2: Upstream Capability Smoke | B, E | 6건 | 낮 (기존 plan-readiness-check 확장) |
| **3** | Gate 4: Merge Impact Analysis | D | 3건 | 중 (diff 분석 + 선별 테스트) |
| **4** | Gate 5: Plan-Reality Sync | E | 3건 | 낮 (기존 스킬 mandatory 승격) |
| **5** | Gate 3: Build/Config Smoke | C | 3건 | 낮 (dry-run 습관화) |

---

## Section 5: 미구현 사항 현황

### Known Limitations ↔ Blocker 매핑

현재 KL-1~KL-18 중 blocker와 직접 연관된 항목:

| KL | 카테고리 | 심각도 | 연관 Blocker | 해결 시 해소되는 Blocker |
|----|---------|--------|-------------|----------------------|
| **KL-3** | GenModel | Medium | F-5 (width parsing) | 64비트 초과 신호 정확성 회복 |
| **KL-5** | GenModel | Medium | B-4 (llhd.prb/drv 잔존) | uart_top 전체 sub-module GenModel 생성 가능 |
| **KL-10** | GenModel | Medium | B-4 (FSM process) | FSM sub-module GenModel 생성 가능 |
| **KL-13** | GenFuncModel | **High** | — (FuncModel 전용) | uart 등 case 기반 FSM func_model 생성 |
| **KL-14** | CIRCT Lowering | **High** | B-1 (llhd.drv 타입 불일치) | ncs_cmd_v2p_blk_swap 등 hirct-gen 성공 |
| **KL-15** | VerilogLoader | **High** | — (전처리 전용) | ncs_opc.vh 포함 모듈 importVerilog 성공 |
| **KL-16** | GenModel | Medium | F-6 (hw.array_inject) | 동적 배열 쓰기 모듈 g++ 컴파일 성공 |
| **KL-18** | CIRCT upstream | Medium | B-3 (mem2reg segfault) | Stage 2 pass 복원 가능 |

### Open Decisions ↔ Blocker 매핑

| OD | 상태 | 연관 영향 |
|----|------|----------|
| **G-1** (DPI 출력 타이밍) | OPEN | KL-1과 직접 연관. 결정 지연이 false mismatch 허용 범위를 불확실하게 유지 |
| **H-1** (시뮬레이터 인터페이스) | OPEN | GenModel 출력 형식에 영향. 결정 지연은 직접적 blocker는 아님 |
| **H-2** (CXXRTL PoC 범위) | OPEN | PoC 진행 중. UART Conditional Go 판정 완료. v2p는 B-1로 BLOCKED |
| **H-3** (GenFuncModel SystemC) | OPEN | 아키텍처 결정. 현재 GenFuncModel의 KL-13 해결과는 독립 |
| **H-4** (IR 스펙 재개 시점) | OPEN | Gate 1 (IR Census)와 자연스럽게 연결. IR 스펙 작업이 IR Census의 체계적 버전 |

### 해결 우선순위 (Blocker 해소 관점)

| 순위 | 항목 | 해소되는 Blocker/모듈 | 난이도 |
|------|------|---------------------|--------|
| 1 | KL-15 수정 (`--pp-comments` 제거) | packet_router, top_v2p 진입 | 낮 |
| 2 | KL-13 수정 (ceq 추가) | uart 전 sub-module func_model 생성 | 낮 |
| 3 | KL-16 수정 (array_inject 지원) | sync_fifo_reg 등 배열 모듈 g++ 성공 | 중 |
| 4 | KL-3 수정 (WideInt 도입) | smbus, axi_x2p 정확성 | 높 |
| 5 | KL-14 수정 (widened-drive pass) | ncs_cmd_v2p_blk_swap hirct-gen 성공 | 높 (CIRCT 의존) |
| 6 | KL-5/KL-10 수정 (FSM lowering) | uart_top 전체 GenModel | 높 (장기) |

---

## 부록: 타임라인 시각화

```
2/17 ─ Phase 0 시작
2/18 ─ Phase 1A/1B emitter 구현
2/19 ─ Phase 1 완료 ── D-3: BUG-1/2/3 수정 (eval_comb, SSA regex)
        │                  F-2, F-3: Phase 1 코드 생성 버그 연쇄
2/21 ─ Phase 2 Batch A
2/23 ─ Phase 2 완료 ── D-1: task-batch-c 병합 후 회귀
        │                  F-4: mem-write order
2/23 ─ Phase 3 완료 ── F-1: Queue_11 음수 상수
2/24 ─ Task 304 (cross-validation)
2/28 ─ Phase 4 설계 ── Phase 4 전환 결정
3/04 ─ Phase 4-B/C  ── B-2: concat_ref (verilator -E로 해소)
        │                  B-3: mem2reg segfault (pass 제외)
        │                  B-4: cf.br/cf.cond_br (커스텀 pass 4종)
3/05 ─ Phase 4-D/E/F ── C-1~C-3: pipeline-audit Makefile 오류
        │                    E-2: --run-pass 단일 pass 한계
3/09 ─ fc6161 audit  ── KL-13~KL-18 다수 발견
3/10 ─ CXXRTL PoC    ── A-1~A-5: IR 시맨틱 오분석 집중 발생
        │                  B-1: llhd.drv 타입 불일치 (v2p BLOCKED)
        │                  F-6~F-8: 배열 타입 코드 생성 버그
```

---

## 결론

1. **가장 큰 구조적 결함은 IR Contract 부재(R1)**. 26건 중 13건(50%)이 이 원인에 귀속된다. Gate 1(IR Census)이 최우선 대책.

2. **CIRCT upstream 한계(R2)는 예방 가능**. 4건 중 3건은 사전 smoke 테스트로 조기 발견이 가능했다. Gate 2(Upstream Capability Smoke)로 대응.

3. **병합 후 회귀(R3)는 diff 기반 분석으로 감소 가능**. Gate 4(Merge Impact Analysis)는 특히 GenModel 내부 상태 변경 시 효과적.

4. **Makefile/환경 설정 오류(C)는 가장 빠르게 해결 가능**. dry-run 습관화만으로 재발 방지.

5. **plan-readiness-check의 mandatory 승격(Gate 5)**은 비용 대비 효과가 가장 높다. 기존 스킬을 강제 적용하는 것만으로 카테고리 E 3건을 방지.
