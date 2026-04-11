# Arcilator Crash / Upstream CIRCT Instability — Risk Registry

> **작성일**: 2026-04-11
> **분류**: Risk Bookkeeping (문서 전용, 코드 수정 없음)
> **목적**: arcilator/CIRCT 경로에서 관찰된 crash·segfault·비정상 종료 사례를 reference risk로 정리하고, 현재 v1 exporter(`arc_to_cpp`)의 scope 기준에서 입력 안정화 경로를 문서화한다.
> **SSOT 관계**:
> - 코드 레벨 XFAIL: `hirct/known-limitations.md` (lit 테스트 참조)
> - 계획 레벨 제약: `docs/plans/known-limitations.md`
> - blocker 분석: `docs/plans/2026-03-10-blocker-root-cause-analysis.md`
> - 본 문서는 위 SSOT를 **깨지 않는** 보조 risk view이며, crash/instability 사례의 의사결정 경로를 추적한다.

---

## 1. Risk 사례 카탈로그

### R-1. `--llhd-mem2reg` pass — null-ptr dereference segfault

| 항목 | 내용 |
|------|------|
| **Symptom** | `--llhd-mem2reg` 단독 실행 시 SIGSEGV (exit 139). `OpResultImpl::getOwner()` null dereference |
| **Suspected Trigger Pattern** | `hw.module` 입력 포트를 `DriveNode*`로 역참조하는 `Mem2Reg.cpp:Promoter::resolveDefinitionValue` 경로. Stage 1(`convert-moore-to-core`) 출력에 `hw.module` 입력이 mem2reg의 대상 포인터로 잘못 노출되는 경우 |
| **재현 조건** | Stage 1 MLIR → `--llhd-mem2reg` 단독 (fc6161 pipeline-audit) |
| **KL 참조** | KL-18 (`hirct/known-limitations.md`, `docs/plans/known-limitations.md`) |
| **Blocker RCA 참조** | B-3 (`2026-03-10-blocker-root-cause-analysis.md`) |
| **현재 우회** | PASSES_STAGE2에서 `--llhd-mem2reg` 제거. sig2reg만 사용 |
| **v1 exporter scope 판정** | **scope 밖**. `arc_to_cpp`는 arcilator가 이미 `arc.model`/`arc.state` 수준까지 lowering 완료한 Arc dialect MLIR을 입력으로 받는다. `--llhd-mem2reg`는 LLHD→HW lowering 단계이며, v1 exporter 진입 전에 실패한다. |
| **upstream stabilization 가능성** | 가능하나 CIRCT upstream 수정 필요. hirct에서 `--llhd-mem2reg` 없이 `--llhd-sig2reg`로 동일 범위 충족 여부 검증 필요 |

---

### R-2. `cf.br` / `cf.cond_br` 잔존 → arcilator `arc-conv` FAIL

| 항목 | 내용 |
|------|------|
| **Symptom** | arcilator 파이프라인에서 `arc-conv` pass 실패 (exit ≠ 0). Arc dialect MLIR 미생성 |
| **Suspected Trigger Pattern** | `llhd.process` 내부의 Verilog `for` 루프 → `cf.br`/`cf.cond_br` 생성. CIRCT의 `llhd-deseq`가 control flow를 완전히 제거하지 못한 채 arcilator에 전달. FSM 패턴(`ceq` 상태 비교)도 동일 경로로 실패 |
| **재현 조건** | UART, GPIO 등 `unresolved_process` 타겟 (`cf.br` 존재) |
| **KL 참조** | KL-10 (FSM process), Arc PoC 결과 (`2026-03-04-arc-poc-results.md`) |
| **Blocker RCA 참조** | B-4 (`2026-03-10-blocker-root-cause-analysis.md`) |
| **현재 우회** | HIRCT 커스텀 pass 4종 (UnrollProcessLoops, ProcessFlatten, ProcessDeseq V2, SignalLowering)으로 `cf.br` 부분 제거. 단순 모듈(WDT, PTIMER)만 arcilator 호환 |
| **v1 exporter scope 판정** | **scope 밖 (입력 도달 불가)**. `arc_to_cpp`가 Arc dialect MLIR을 입력으로 받으려면 arcilator가 `arc-conv`까지 성공해야 한다. `cf.br` 잔존 시 Arc MLIR 자체가 생성되지 않으므로, v1 exporter에는 이 입력이 도달하지 않는다. |
| **upstream stabilization 가능성** | 중기. HIRCT `UnrollProcessLoops`가 180/266 llhd.process 해소. 잔여 FSM 패턴은 CIRCT FSM dialect 또는 `ProcessFlattenPass` 확장 필요. 커스텀 pass 4종 적용 후 arcilator 종단 검증(Arc PoC Task #4) 미완 |

---

### R-3. `circt-verilog` segfault (TLXbar)

| 항목 | 내용 |
|------|------|
| **Symptom** | `circt-verilog` 실행 시 SIGSEGV (exit 139) |
| **Suspected Trigger Pattern** | `Fadu_K2_S5MC_TLXbar.v` — 대형 TileLink 크로스바 모듈. CIRCT Slang frontend의 메모리/재귀 한계 추정 |
| **재현 조건** | `circt-verilog rtl/plat/src/s5mc/design/Fadu_K2_S5MC_TLXbar.v` |
| **KL 참조** | `hirct/known-limitations.md` (테이블 27행: "infra-error / circt-verilog segfault") |
| **현재 우회** | XFAIL 처리. Phase 3+ 이관 |
| **v1 exporter scope 판정** | **scope 밖 (upstream 파싱 단계 실패)**. circt-verilog가 MLIR을 생성하지 못하므로, 이후 어떤 경로도 이 모듈에 접근 불가 |
| **upstream stabilization 가능성** | CIRCT upstream 버그 리포트 필요. verilator -E 전처리 후 재시도 가능성 있으나 미검증 |

---

### R-4. `signal-lowering` pass segfault

| 항목 | 내용 |
|------|------|
| **Symptom** | HIRCT `HirctSignalLowering` pass 실행 중 segfault |
| **Suspected Trigger Pattern** | 특정 LLHD IR 패턴에서 방어 코드 부재. 계획에서 pass가 안정적이라 가정했으나 실측에서 segfault 발생 |
| **재현 조건** | 특정 fc6161 모듈 (상세 미기록) |
| **Blocker RCA 참조** | E-3 (`2026-03-10-blocker-root-cause-analysis.md`) |
| **현재 우회** | pass 디버깅 + 방어 코드 추가로 해소 |
| **v1 exporter scope 판정** | **scope 밖**. signal-lowering은 LLHD→HW 변환 단계. v1 exporter(`arc_to_cpp`)는 Arc MLIR 입력만 받으므로 이 pass를 거치지 않음 |
| **upstream stabilization** | HIRCT 자체 수정으로 해소됨. 추가 조치 불필요 |

---

### R-5. `APInt::getZExtValue()` SIGABRT (>64비트 상수)

| 항목 | 내용 |
|------|------|
| **Symptom** | GenModel이 65비트 이상 상수를 `getZExtValue()`로 추출 시 LLVM APInt assert 발동 → SIGABRT |
| **Suspected Trigger Pattern** | `hw.constant` op의 결과 비트폭 > 64 → `APInt::getZExtValue()` 호출 시 assertion failure |
| **재현 조건** | smbus i384 wide signal 포함 모듈 |
| **KL 참조** | KL-3 (`docs/plans/known-limitations.md`) |
| **현재 우회** | Stage 3 수정(`f8278e2`, `52202cc`)으로 >64비트 상수를 하위 64비트로 안전 truncate. SIGABRT 제거됨. 데이터 정확성 이슈 잔존 |
| **v1 exporter scope 판정** | **부분적으로 scope 내**. `arc_to_cpp` emitter가 Arc MLIR 내 상수를 C++ 리터럴로 emit할 때, 65비트 이상 상수의 표현 방법이 결정되어야 함. 현재 `arc_to_cpp`는 `uint64_t` 기반이므로, arcilator가 64비트 초과 상수를 포함한 Arc MLIR을 생성하면 동일한 truncation 문제 발생 가능 |
| **upstream stabilization** | v1 exporter에서 >64비트 입력을 명시적으로 거부(error + skip)하는 것이 안전. Phase 2에서 `uint32_t[]` 래퍼 도입 시 해소 (open-decisions A-1) |

---

### R-6. `llhd.drv` 동적 배열 인덱스 타입 불일치 (lowering FAIL)

| 항목 | 내용 |
|------|------|
| **Symptom** | `moore→core` lowering에서 `llhd.drv` op의 signal ref type ≠ drive value type → verification fail, hirct-gen exit 1 |
| **Suspected Trigger Pattern** | 동적 변수로 배열을 인덱싱하는 `always` 블록 (`data_reg[wr_ptr] <= wr_data`) |
| **재현 조건** | ncs_cmd_v2p_blk_swap, packet_path 모듈 |
| **KL 참조** | KL-14, KL-5 추가 사례 (`docs/plans/known-limitations.md`) |
| **Blocker RCA 참조** | B-1 (`2026-03-10-blocker-root-cause-analysis.md`) |
| **현재 우회** | XFAIL 처리 |
| **v1 exporter scope 판정** | **scope 밖 (입력 도달 불가)**. lowering이 실패하므로 Arc MLIR이 생성되지 않음 |
| **upstream stabilization** | CIRCT upstream 이슈. 중기에 widened-drive 재작성 pass 가능 |

---

## 2. Scope 판정 요약

| Risk | Crash Type | v1 Exporter 도달 여부 | 판정 |
|------|-----------|---------------------|------|
| R-1 | SIGSEGV (CIRCT pass) | 도달 불가 (LLHD lowering 단계) | **Deferred** — upstream |
| R-2 | arcilator FAIL (non-crash) | 도달 불가 (Arc MLIR 미생성) | **Deferred** — upstream + custom pass |
| R-3 | SIGSEGV (circt-verilog) | 도달 불가 (파싱 단계) | **Deferred** — upstream |
| R-4 | SIGSEGV (HIRCT pass) | 도달 불가 (LLHD lowering 단계) | **Resolved** — 방어 코드 추가 |
| R-5 | SIGABRT (APInt assert) | **부분 도달 가능** (>64bit 상수) | **Risk** — v1에서 입력 거부 필요 |
| R-6 | lowering FAIL (type mismatch) | 도달 불가 (lowering 실패) | **Deferred** — upstream |

**결론**: R-5(>64비트 상수)만 v1 exporter scope에 부분적으로 걸린다. 나머지는 전부 upstream lowering/파싱 단계에서 차단되므로 v1 exporter에 도달하지 않는다.

---

## 3. Upstream Input Stabilization 경로

### 3.1 v1 exporter(`arc_to_cpp`)가 안전하게 받을 수 있는 입력 조건

| 조건 | 설명 | 검증 방법 |
|------|------|----------|
| arcilator `state-alloc` pass 통과 | Arc dialect MLIR이 `arc.model` + `arc.alloc_state` (byte offset) 수준까지 lowering 완료 | `arcilator --until=state-alloc input.mlir` exit 0 |
| `cf.br` / `cf.cond_br` 부재 | 제어 흐름이 완전히 제거된 상태 | `grep -c "cf.br\|cf.cond_br" input_arc.mlir` = 0 |
| 64비트 이하 신호만 포함 | >64비트 상수/포트가 없음 | 별도 width 검증 스크립트 또는 emitter 내부 guard |
| `llhd.*` op 부재 | LLHD dialect이 완전히 lowering됨 | `grep -c "llhd\." input_arc.mlir` = 0 |

### 3.2 현재 안정적으로 Arc MLIR을 생성할 수 있는 모듈 범위

Arc PoC Phase A 결과 기준:

| 모듈 | arcilator 상태 | Arc MLIR 용량 | v1 exporter 입력 가능 |
|------|---------------|-------------|---------------------|
| WDT | PASS (state-alloc, 154B) | 744줄 | **Yes** |
| PTIMER | PASS (state-alloc, 1,533B) | 7,824줄 | **Yes** |
| UART | FAIL (cf.br 잔존) | N/A | No — UnrollProcessLoops 후 재시도 필요 |
| GPIO | FAIL (cf.br 잔존) | N/A | No — 동일 |

### 3.3 Upstream stabilization 로드맵 (비구현, 문서만)

```
[현재 → 단기]
1. HIRCT 커스텀 pass 4종 적용 후 arcilator 종단 재검증 (Arc PoC Task #4, 미완)
   → UART/GPIO가 arcilator 호환이 되면 v1 exporter 입력 범위 확대
2. v1 exporter에 >64비트 입력 guard 추가 (R-5 대응)

[중기]
3. CIRCT upstream: llhd.drv 타입 불일치 수정 (R-6)
4. CIRCT upstream: mem2reg null-ptr fix (R-1)
5. ProcessFlattenPass FSM 패턴 확장 (R-2)

[장기]
6. CIRCT upstream: circt-verilog 대형 모듈 안정성 (R-3)
7. WideInt 지원 (R-5 근본 해결)
```

---

## 4. 의사결정 포인트 (현재 문서에 빠진 것들)

### DP-1: HIRCT 커스텀 pass 4종 적용 후 arcilator 종단 검증 시점

- **현재 상태**: Arc PoC Task #4 "arcilator 종단 검증 — async reset 우회 후 arc-conv 통과 확인"이 "대기" 상태
- **필요한 결정**: 커스텀 pass 적용 후 UART/GPIO를 arcilator로 재시도하여 Arc MLIR 생성 여부 확인. 이 결과가 v1 exporter의 입력 범위를 결정
- **권고**: v1 exporter 안정화 전에 이 검증을 선행해야 함

### DP-2: v1 exporter의 >64비트 입력 처리 정책

- **현재 상태**: `arc_to_cpp` emitter는 `uint64_t` 기반. >64비트 상수가 Arc MLIR에 포함될 경우의 동작 미정의
- **선택안**:
  - A: 명시적 error + skip (안전, 현재 권장)
  - B: 하위 64비트 truncate + warning (KL-3과 동일 전략)
  - C: `uint32_t[]` 배열 표현 (Phase 2 작업)
- **권고**: A안(error + skip)을 v1 기본값으로 채택하고, B/C는 Phase 2 이후

### DP-3: `arc_to_cpp` v1 입력 검증 게이트 필요 여부

- **현재 상태**: `arc_to_cpp`의 `parser.py`는 텍스트 기반 MLIR 파싱. 입력이 Arc dialect인지, 잔존 LLHD/cf op이 있는지 검증하지 않음
- **선택안**:
  - A: 파서 진입 시 `llhd.`/`cf.br`/`cf.cond_br` 존재 검사 → error
  - B: 미지원 op 발견 시 warning + skip (현재 GenModel과 동일 전략)
  - C: 별도 검증 단계 없음 (arcilator가 이미 걸러줌)
- **권고**: A안. arcilator 경로가 아닌 수동 MLIR 입력 시에도 안전하도록

### DP-4: Regression evidence 기준 — 언제 "arcilator 경로 안정"을 선언할 수 있는가

- **필요 증거**:
  1. WDT + PTIMER: arcilator → `arc_to_cpp` → g++ → verify PASS (최소 2개 모듈 종단 통과)
  2. UART + GPIO: 커스텀 pass 적용 후 arcilator → Arc MLIR 생성 성공
  3. Arc MLIR → `arc_to_cpp` → g++ 컴파일 성공 (UART/GPIO 각 1개 이상 서브모듈)
  4. lit 테스트에 arcilator 경로 smoke 테스트 등록
- **현재 충족**: WDT/PTIMER의 Arc MLIR 생성까지만 확인. g++ 컴파일 및 verify는 미실행
- **차이**: 4개 증거 중 0개 완전 충족

---

## 5. 이 문서가 docs-only로 남아야 하는 이유

1. **v1 exporter에 도달하는 crash는 R-5 하나뿐이며, 이것도 "부분적"**: 나머지 5개 risk는 전부 upstream 단계에서 차단됨. 코드 수정의 근거가 충분하지 않음.
2. **Arc PoC Task #4(종단 검증)가 미완**: 커스텀 pass 적용 후 arcilator 재시도 결과가 나오기 전에 v1 exporter를 수정하면 scope가 확정되지 않은 상태에서 작업하게 됨.
3. **risk registry는 의사결정의 입력**: 이 문서의 목적은 "어떤 crash가 있고, 어디서 막히고, 무엇을 결정해야 하는지"를 정리하는 것. 실제 코드 수정은 DP-1~DP-4의 결정 후에 진행해야 함.
4. **기존 SSOT를 깨지 않음**: KL-18, KL-3, KL-5, KL-10, KL-14는 이미 `known-limitations.md`에 등록되어 있음. 본 문서는 이들을 "arcilator/v1 exporter 관점의 risk view"로 재조합한 것.

### 보수적 원칙

- **"arcilator 경로 안정" 또는 "v1 exporter arcilator 지원"을 선언하지 않는다.** DP-4의 4개 regression evidence가 모두 충족되기 전까지, 어떤 문서에서도 지원 선언을 하지 않는다.
- **이 문서는 SSOT를 대체하지 않는다.** 개별 crash의 권위 있는 기록은 `known-limitations.md`(KL-*)와 `2026-03-10-blocker-root-cause-analysis.md`(B-*)에 있다. 본 문서는 이들을 arcilator/v1 exporter scope라는 렌즈로 재조합한 보조 뷰이다.
- **risk 판정을 낙관적으로 변경하지 않는다.** "Deferred"를 "Resolved"로 변경하려면, 해당 risk가 실제로 재현 불가능함을 증명하는 regression evidence(재현 명령어 + 결과 로그)가 필요하다.

---

## 6. known-limitations.md와의 역할 분리

| 측면 | `known-limitations.md` (SSOT) | 본 문서 (risk registry) |
|------|-------------------------------|------------------------|
| **목적** | 모든 알려진 제약을 코드 레벨에서 추적 | arcilator/v1 exporter 관점의 crash risk만 추적 |
| **범위** | KL-1~KL-18 전체 (DPI, GenModel, FSM, 전처리 등) | R-1~R-6 (crash/segfault/FAIL 사례만) |
| **판정 기준** | 심각도(Low/Medium/High), 카테고리별 | v1 exporter scope 도달 여부 (Deferred/Resolved/Risk) |
| **수정 권한** | SSOT — 새 제약 발견 시 여기에 먼저 등록 | 보조 뷰 — SSOT 변경 후 여기에 반영 |
| **의사결정** | 우회 방법, 해결 계획 | upstream stabilization 경로, regression evidence 기준 |

---

## 변경 이력

| 날짜 | 내용 |
|------|------|
| 2026-04-11 | 초안 작성. R-1~R-6 카탈로그, scope 판정, upstream stabilization 경로, DP-1~DP-4 의사결정 포인트 |
| 2026-04-11 | docs-only 보조 작업으로 보강. 보수적 원칙 명시화, KL 문서와의 역할 분리 표 추가, Section 번호 재정리 |
