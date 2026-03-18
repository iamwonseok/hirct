# Blocker Retrospective & Prevention Gates

> **작성일**: 2026-03-10
> **목적**: 지금까지 세션에서 반복된 blocker/미구현 항목의 원인을 분류하고, 재발 방지를 위한 운영 게이트와 체크리스트를 정의한다.

---

## 1. Blocker Taxonomy

16개 부모 세션에서 추출한 blocker를 5개 원인 축과 7개 발생 시점으로 교차 분류한다.

### 1.1 원인별 분류

| ID | 원인 축 | 건수 | 대표 사례 |
|----|---------|------|-----------|
| C1 | **계획 품질** | 4 | plan readiness No-Go (stale plan, Goal/Files/Steps/Run/Expect 부족, 범위 과잉, SSOT 충돌) |
| C2 | **환경 전제** | 7 | `hirct/` cwd 누락, `setup-env.sh` 경로 오류, worktree build 참조, `vlogan`/`ncvlog` 미설치, VCS segfault, `ar` SIGBUS, `build/` vs `hirct/build/` 혼동 |
| C3 | **기술 조사** | 8 | `hw.array_inject` 미지원, `llhd.drv` 타입 불일치, Yosys 동적 for-loop, `& ()` 빈 표현식, multi-clock step 미생성, signal-lowering segfault, CXXRTL posedge 감지, process flatten 실패 |
| C4 | **검증** | 3 | TXD mismatch 미해결, seed 범위 뒤늦은 확장, DPI clock 트리거 타이밍 오류 |
| C5 | **세션 연결** | 1 | dirty worktree + main 브랜치로 착수 보류 |

### 1.2 발생 시점별 분류

| 시점 | 건수 | blocker IDs |
|------|------|-------------|
| 계획 | 3 | plan readiness No-Go, survey 범위 과잉, `is_clock_port` 루트원인 오분석 |
| 착수 | 4 | `vlogan` 미설치, `llhd.drv` 타입 불일치, `setup-env.sh` 실패, dirty worktree |
| 구현 | 3 | `& ()` 빈 표현식, checkpoint 과잉 생성, process flatten `proc_results` 비어있음 |
| 빌드 | 3 | VCS segfault, `ar` SIGBUS, worktree build 참조 |
| 검증 | 7 | multi-clock step 미생성, bcm57 컴파일 에러, signal-lowering segfault, TXD mismatch, DPI clock 트리거, CXXRTL posedge 감지, `build/bin` 경로 혼동 |
| handoff | 3 | plan/report 불일치, 이월 프롬프트만 작성, 다음 세션 기준선 불명확 |

### 1.3 미구현/이월 항목 목록

| 항목 | 멈춘 원인 축 | 현재 상태 |
|------|-------------|-----------|
| `DW_apb_uart_tx` TXD mismatch 근본 수정 | C3+C4 | 원인 조사 중 세션 종료, readiness No-Go 반복 |
| `ncs_cmd_v2p_blk_swap` GenModel 비교 | C3 | `llhd.drv` 타입 불일치로 생성 불가 |
| GenDPIC inter-module clock 분석 | C1+C3 | `is_clock_port` 루트원인 오분석 후 계획 재작성 |
| `hirct-gen --strict` 옵션 | C1 | 범위 과잉으로 BLOCKED |
| GenModel IR 스펙 (멀티클럭 알고리즘) | C1 | 브레인스토밍 문서로만 이월 |
| GenWrapper/GenTB 흡수 결정 | C1 | 스펙만 결정, 코드 미변경 |
| stage-0 RTL timescale 실패 19건 | C2 | hirct-gen 외부 RTL 이슈로 범위 밖 처리 |
| `setup-env.sh` cwd 전제 정리 | C2 | 발견됐으나 운영 단계로 고정 안 됨 |

---

## 2. Root Cause Mapping

### 2.1 연쇄 패턴

Blocker는 단독으로 발생하지 않는다. 실제 transcript에서 관찰된 연쇄 패턴 3개:

**Pattern A: Stale Plan Cascade**
```
계획 stale → readiness No-Go → 계획 재작성 → 환경/경로 재확인 → 세션 시간 소진
```
- 발생 세션: c02b7990, c7c8b341
- 핵심 원인: 계획이 이전 세션의 결과를 반영하지 않은 채 실행 시도

**Pattern B: Hypothesis-First Debugging**
```
생성물 미확인 → 가설 수립 → 구현 → 빌드 실패/결과 불일치 → 가설 수정 → 반복
```
- 발생 세션: ffd27fc3 (array_inject 후 CompRegOp deferred, 이름 충돌, bitcast, comb.mux 순차 발견), 22a129a2 (multi-clock 판정), a54603ae (DPI clock 트리거)
- 핵심 원인: IR/생성 코드/로그를 먼저 보지 않고 "아마 이것이 원인"으로 시작

**Pattern C: Environment Assumption Drift**
```
명령어 실행 → 경로 오류/도구 미설치 → 경로 탐색 → 우회 → 다른 경로에서 재실패
```
- 발생 세션: c02b7990 (setup-env.sh), ffd27fc3 (build/bin 혼동), 60d1e1e4 (worktree build), e968b21d (worktree build)
- 핵심 원인: 계획에 cwd와 절대 경로가 명시되지 않음

### 2.2 원인 → 게이트 매핑

| 원인 축 | 연쇄 패턴 | 방지 게이트 |
|---------|-----------|------------|
| C1 계획 품질 | Pattern A | **Baseline Sync Gate** + **plan-readiness-check** |
| C2 환경 전제 | Pattern C | **Environment/CWD Gate** |
| C3 기술 조사 | Pattern B | **Reproduce-and-Evidence Gate** + **Artifact Reality Check** |
| C4 검증 | Pattern B | **Verification Scope Lock** |
| C5 세션 연결 | Pattern A | **Plan-Report-Handoff Sync** |

---

## 3. Prevention Gates

기존 운영 루프(`brainstorming → writing-plans → plan-readiness-check → executing → verification → audit → handoff`)에 아래 6개 게이트를 삽입한다. 기존 스킬과 중복되는 부분은 강화 항목으로 표시한다.

### Gate 1: Baseline Sync Gate (신규)
- **삽입 위치**: `writing-plans` 직후, `plan-readiness-check` 직전
- **목적**: 계획이 현재 저장소 상태와 동기화되었는지 확인
- **통과 기준**:
  1. plan 본문의 "현재 상태" 섹션이 실제 `git status`, `git branch`, output 디렉토리 내용과 일치
  2. 이전 세션 handoff에서 "남은 작업"으로 적은 항목이 plan에 반영됨
  3. stale 상태 표기(완료인데 본문에 실행 대상, 또는 그 반대) 0건
  4. 이번 세션의 종료 조건이 1개로 고정됨
- **실패 시**: plan 본문 갱신 후 재검증

### Gate 2: Environment/CWD Gate (기존 readiness-check 항목 7 강화)
- **삽입 위치**: `plan-readiness-check` 통과 직후, 구현 착수 직전
- **목적**: 모든 명령이 올바른 cwd에서 실행 가능한지 사전 확인
- **통과 기준**:
  1. plan의 모든 `Run` 명령에 `working_directory` 또는 절대 경로가 명시됨
  2. `utils/setup-env.sh`를 명시된 cwd에서 실행해 exit 0 확인
  3. 빌드 바이너리 경로(`hirct/build/bin/hirct-gen` 등)가 존재하고 최신인지 `ls -la` 확인
  4. 테스트 타겟(`make test-compare` 등)이 명시된 디렉토리에서 `make -n` dry-run 가능
- **실패 시**: plan의 경로/cwd를 수정하고 Gate 1부터 재진행

### Gate 3: Reproduce-and-Evidence Gate (기존 systematic-debugging 강화)
- **삽입 위치**: 버그 수정 착수 직전 (디버깅 태스크에만 적용)
- **목적**: 가설 선행 방지, 증거 기반 디버깅 강제
- **통과 기준**:
  1. 실패 재현 명령 1개와 그 출력 로그가 기록됨
  2. "무엇이 / 어느 cycle에서 / 어떻게 틀렸는지"를 한 문장으로 기술 가능
  3. 대상 신호, seed, cycle 범위가 명시됨
  4. root cause 주장 전에 최소 2종 증거(IR + 생성 코드, 또는 로그 + 테스트)를 확보
- **실패 시**: 가설 수립 금지, 먼저 `--dump-ir`, 생성 코드 확인, 비교 로그 확인부터 진행

### Gate 4: Artifact Reality Check (신규)
- **삽입 위치**: 코드 수정 직전마다 (구현 루프 내)
- **목적**: 수정 전 실제 생성물이 가설과 일치하는지 확인
- **통과 기준**:
  1. 수정하려는 코드 경로의 현재 생성 결과물을 직접 읽어 확인
  2. IR dump, emitted C++ 코드, 비교 테스트 출력 중 최소 1개를 fresh하게 확인
  3. "이 수정이 이 증거의 이 부분을 고친다"를 명시
- **실패 시**: 생성물 확인부터 다시 수행

### Gate 5: Verification Scope Lock (기존 verification-before-completion 강화)
- **삽입 위치**: 검증 시작 시점
- **목적**: 검증 범위가 세션 중간에 확장되어 기준선이 흔들리는 것을 방지
- **통과 기준**:
  1. **Minimum Pass Bar** 명시: 예) `seed=42 1000cyc, PRDATA+INTR+TXD 0 mismatch`
  2. **Stretch Check** 분리: 예) `5seed 10000cyc` — minimum 통과 후에만 실행
  3. minimum pass bar 미달 시 stretch check로 넘어가지 않음
  4. 검증 결과는 exit code + 핵심 수치로 기록 (주관적 판단 금지)
- **실패 시**: minimum bar를 먼저 통과시킨 후 stretch로 진행

### Gate 6: Plan-Report-Handoff Sync (기존 auditing + session-handoff 통합 강화)
- **삽입 위치**: 세션 종료 직전
- **목적**: 다음 세션이 탐색 없이 바로 시작 가능하도록 보장
- **통과 기준**:
  1. plan 체크리스트의 각 항목 상태가 실제 결과와 1:1 일치
  2. 남은 blocker가 있으면 원인 축(C1~C5)과 재현 명령이 기록됨
  3. handoff 프롬프트에 "다음 세션 첫 명령"이 실행 가능한 형태로 포함
  4. 이번 세션에서 발견한 환경/경로 이슈가 plan 또는 convention에 반영됨
- **실패 시**: 누락 항목 보완 후 handoff 재작성

---

## 4. 제안 흐름

```
Brainstorming
    |
    v
WritePlan
    |
    v
[Gate 1] Baseline Sync Gate --- plan stale? --> 갱신 후 재검증
    |
    v
PlanReadinessCheck (7항목)
    |
    v
[Gate 2] Environment/CWD Gate --- 경로/도구 실패? --> plan 수정, Gate 1 복귀
    |
    v
[Gate 3] Reproduce-and-Evidence Gate (디버깅 태스크만)
    |
    v
Implement / Debug
    |  (수정 전마다)
    +--[Gate 4] Artifact Reality Check --- 생성물 불일치? --> 가설 재검토
    |
    v
[Gate 5] Verification Scope Lock
    |  minimum bar 통과
    v
VerificationBeforeCompletion
    |
    v
[Gate 6] Plan-Report-Handoff Sync
    |
    v
SessionHandoff
```

---

## 5. Operating Checklist

다음 세션부터 모든 구현/디버깅 세션 시작 시 아래 10항을 순서대로 확인한다.

### 착수 전 (Gates 1-2)
- [ ] **BSG-1**: plan "현재 상태"가 `git status`/`git branch`와 일치하는가?
- [ ] **BSG-2**: 이전 handoff의 "남은 작업"이 plan에 반영되었는가?
- [ ] **BSG-3**: 이번 세션 종료 조건이 1개로 고정되었는가?
- [ ] **ENV-1**: 모든 `Run` 명령에 working_directory 또는 절대 경로가 있는가?
- [ ] **ENV-2**: `setup-env.sh`가 명시된 cwd에서 exit 0인가?

### 구현/디버깅 중 (Gates 3-4)
- [ ] **REP-1**: 수정 전 실패 재현 로그와 재현 명령이 기록되었는가?
- [ ] **ART-1**: 수정 전 생성물(IR/코드/로그)을 fresh하게 확인했는가?
- [ ] **ART-2**: root cause 주장에 2종 이상 증거가 있는가?

### 검증/마무리 (Gates 5-6)
- [ ] **VER-1**: minimum pass bar를 통과했는가? (stretch 전에)
- [ ] **SYN-1**: plan 상태, 실제 결과, handoff가 동기화되었는가?

---

## 6. 적용 범위

- **즉시 적용**: Operating Checklist 10항 (모든 구현/디버깅 세션)
- **점진 적용**: Gate 3-4는 디버깅 태스크에만, Gate 5는 비교 테스트가 있는 태스크에만
- **측정 지표**: plan-readiness No-Go 재발률, 가설 변경 횟수, 검증 범위 변경 횟수, handoff 불일치 건수
