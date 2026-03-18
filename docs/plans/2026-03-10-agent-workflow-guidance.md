# 에이전트 작업 지시 Flow 가이드

> **작성일**: 2026-03-10
> **근거**: [blocker-retrospective.md](2026-03-10-blocker-retrospective.md), [blocker-root-cause-analysis.md](2026-03-10-blocker-root-cause-analysis.md)
> **목적**: 사용자가 에이전트에게 작업을 줄 때 어떤 flow로 지시하면 blocker 재발을 줄일 수 있는지, `.cursor` 자산을 어떻게 활용하면 좋은지를 정리한다.

---

## 1. 현재 `.cursor` 자산 인벤토리

### 1.1 Rules (7개) — 상시 또는 파일 스코프 제약

| 파일 | 적용 범위 | 역할 |
|------|----------|------|
| `hirct-project.mdc` | alwaysApply | 프로젝트 운영 헌법: 정책, SSOT 계층, 표준 루프 |
| `agent-behavior.mdc` | alwaysApply | worktree 금지, BLOCKED 증거 필수, reason-first |
| `plan-docs-hirct.mdc` | `docs/plans/**/*.md` | 마이크로 스텝 템플릿, RESOLVED 정책 |
| `cpp-hirct.mdc` | C++ 파일 | C++ 컨벤션 |
| `python-hirct.mdc` | Python 파일 | Python 컨벤션 |
| `makefile-hirct.mdc` | Makefile | Make 컨벤션 |
| `verilog-hirct.mdc` | Verilog/SV 파일 | Verilog 컨벤션 |

### 1.2 Commands (3개) — 사용자가 직접 호출하는 진입점

| 커맨드 | 연결 skill | 용도 |
|--------|-----------|------|
| `brainstorm` | `brainstorming` | 설계 전 요구사항/대안 탐색 |
| `write-plan` | `writing-plans` | 구현 계획 작성 |
| `execute-plan` | `executing-plans` | 계획 배치 실행 |

### 1.3 Skills (17개) — 운영 단계별 분류

| 단계 | skill | 호출 시점 |
|------|-------|----------|
| **부트스트랩** | `using-superpowers` | 모든 대화 시작 시 자동 |
| **설계** | `brainstorming` | 기능/행동 변경 요청 시 |
| **계획** | `writing-plans` | 설계 확정 후 구현 계획 작성 시 |
| **계획 검증** | `plan-readiness-check` | 계획 작성 완료 후 (mandatory gate) |
| **실행 (배치)** | `executing-plans` | 별도 세션에서 배치 실행 |
| **실행 (서브에이전트)** | `subagent-driven-development` | 같은 세션에서 태스크별 실행 |
| **실행 (병렬 디버깅)** | `dispatching-parallel-agents` | 독립적 실패 여러 건 동시 조사 |
| **격리 작업공간** | `using-git-worktrees` | 구현 격리 필요 시 (사용자 승인 필수) |
| **디버깅** | `systematic-debugging` | 버그/테스트 실패/예상 밖 동작 |
| **TDD** | `test-driven-development` | 구현/버그픽스 시 테스트 우선 |
| **완료 검증** | `verification-before-completion` | 완료 선언 직전 (mandatory gate) |
| **코드리뷰 요청** | `requesting-code-review` | 구현 완료 후 |
| **코드리뷰 수신** | `receiving-code-review` | 외부 피드백 반영 시 |
| **브랜치 마감** | `finishing-a-development-branch` | merge/PR/보류/폐기 결정 |
| **감사** | `auditing-plan-report-sync` | plan↔report 정합성 점검 |
| **세션 인계** | `session-handoff` | 세션 종료 시 |
| **스킬 작성** | `writing-skills` | 새 스킬 생성/편집 시 |

---

## 2. Blocker 원인과 `.cursor` 자산 매핑

회고/RCA 문서에서 추출한 5개 원인 축, 3개 구조적 결함, 6개 예방 게이트를 현재 `.cursor` 자산과 대응시킨다.

### 2.1 원인 축 → 현재 커버리지

| 원인 축 | 건수 | 현재 자산 커버리지 | 공백 |
|---------|------|-----------------|------|
| C1 계획 품질 | 4 | `plan-readiness-check` (optional) | mandatory 승격 필요. Baseline Sync Gate 없음 |
| C2 환경 전제 | 7 | `agent-behavior.mdc` (BLOCKED 증거 필수 규칙만) | 절차화된 skill 없음. Environment/CWD Gate 없음 |
| C3 기술 조사 | 8 | `systematic-debugging` (가설 선행 방지) | Reproduce-and-Evidence Gate, Artifact Reality Check 미통합 |
| C4 검증 | 3 | `verification-before-completion` | Verification Scope Lock (minimum bar vs stretch) 없음 |
| C5 세션 연결 | 1 | `session-handoff` + `auditing-plan-report-sync` | 통합 게이트 없음. handoff에 "다음 첫 명령" 강제 없음 |

### 2.2 구조적 결함 → 게이트 대응

| 결함 | 대응 게이트 | 현재 자산 상태 |
|------|-----------|--------------|
| R1 IR Contract 부재 | Gate 1: IR Census | 자산 없음 (HIRCT 도메인 전용) |
| R2 외부 의존성 미검증 | Gate 2: Upstream Smoke | `plan-readiness-check` 확장으로 커버 가능 |
| R3 변경 영향 미분석 | Gate 3: Build/Config Smoke, Gate 4: Merge Impact | `finishing-a-development-branch`에 부분 통합 가능 |

### 2.3 예방 게이트 → `.cursor` 자산 매핑

| 게이트 | 기존 자산 | 필요 조치 |
|--------|----------|----------|
| Gate 1: Baseline Sync | 없음 | `plan-readiness-check` 강화 또는 별도 gate |
| Gate 2: Environment/CWD | `agent-behavior.mdc` 규칙만 | `verify-before-blocked` skill 신설 |
| Gate 3: Reproduce-and-Evidence | `systematic-debugging` | skill 내 gate 절차 강화 |
| Gate 4: Artifact Reality Check | 없음 | `systematic-debugging` 또는 `verification-before-completion`에 통합 |
| Gate 5: Verification Scope Lock | `verification-before-completion` | minimum bar / stretch 분리 절차 추가 |
| Gate 6: Plan-Report-Handoff Sync | `auditing` + `session-handoff` | 통합 체크리스트 강화 |

---

## 3. 추천 작업 지시 Flow

### 3.1 진입점 라우팅 (사용자 판단 기준)

```
내 요청이 뭐지?
  ├─ 기능 추가 / 행동 변경  →  /brainstorm 또는 /write-plan
  ├─ 버그 / 테스트 실패     →  직접 systematic-debugging 지시
  ├─ 문서 정합성 / 상태 정리 →  직접 auditing-plan-report-sync 지시
  ├─ 코드리뷰 피드백 반영    →  직접 receiving-code-review 지시
  └─ 세션 마감 / handoff    →  직접 session-handoff 지시
```

### 3.2 전체 운영 흐름

```
[진입: 사용자 지시]
    │
    ▼
선택 1: 기능/행동 변경 ──────────────────────────────────────────────────────
    │
    ├─ brainstorm (설계가 불명확할 때)
    │      └─ 설계 문서 → write-plan
    │
    └─ write-plan (설계가 확정됐을 때)
           │
           ▼
       [Gate 1] Baseline Sync: plan이 현재 저장소와 동기화?
           │
           ▼
       plan-readiness-check (7항목 + mandatory gate)
           │
           ▼
       [Gate 2] Environment/CWD: 모든 명령에 cwd/절대경로?
           │
           ▼
       execute-plan 또는 subagent-driven-development
           │  (수정 전마다)
           ├─ [Gate 4] Artifact Reality Check
           │
           ▼
       [Gate 5] Verification Scope Lock: minimum bar 먼저
           │
           ▼
       verification-before-completion
           │
           ▼
       auditing-plan-report-sync
           │
           ▼
       session-handoff

선택 2: 버그/실패 ──────────────────────────────────────────────────────────
    │
    ▼
[Gate 3] Reproduce-and-Evidence: 재현 명령 + 2종 증거 먼저
    │
    ▼
systematic-debugging (Phase 1~4)
    │  (수정 전마다)
    ├─ [Gate 4] Artifact Reality Check
    │
    ▼
verification-before-completion
    │
    ▼
session-handoff

선택 3: 문서 정합성 / docs-only ─────────────────────────────────────────────
    │
    ▼
auditing-plan-report-sync
    │
    ▼
session-handoff
(TDD, worktree, branch-finish 생략 가능)

선택 4: 세션 마감 ──────────────────────────────────────────────────────────
    │
    ▼
[Gate 6] Plan-Report-Handoff Sync
    │
    ▼
session-handoff
```

---

## 4. 사용자 지시 체크리스트

에이전트에게 작업을 줄 때 아래 항목을 프롬프트에 포함한다.

### 4.1 필수 항목 (모든 작업)

| # | 항목 | 예시 |
|---|------|------|
| 1 | **세션 목표 1개** | "이번 세션 목표: `DW_apb_uart` GenModel seed=42 1000cyc mismatch 0 달성" |
| 2 | **완료 판정 기준** | "minimum pass bar: `make test-compare SEED=42 CYCLES=1000` exit 0" |
| 3 | **기준 문서 경로** | "`@docs/plans/2026-03-10-uart-genmodel.md` 기반으로 진행" |
| 4 | **현재 branch** | "현재 브랜치: `feat/uart-genmodel`, worktree 사용 안 함" |
| 5 | **수정 범위** | "수정 대상: `lib/Target/GenModel/`" |
| 6 | **비범위 (scope-out)** | "GenDPIC, GenWrapper는 이번 범위 밖" |

### 4.2 조건부 항목

| 조건 | 추가 항목 | 예시 |
|------|----------|------|
| 버그/디버깅 | **재현 명령** | "`make test-compare MODULE=DW_apb_uart SEED=42` → TXD mismatch at cycle 487" |
| 버그/디버깅 | **확인할 artifact** | "IR dump (`--dump-ir`), 생성 C++ 코드, 비교 로그 중 IR과 생성 코드를 볼 것" |
| CIRCT 의존 | **upstream smoke** | "CIRCT `--convert-moore-to-core` pass를 대표 RTL 1건으로 먼저 확인" |
| 설정/Makefile 변경 | **dry-run 요구** | "Makefile 수정 후 `make -n <target>`으로 dry-run 통과 확인" |
| worktree 허용 | **격리 방식** | "worktree 생성 허용. 브랜치명: `feat/uart-fix`" |
| stretch 검증 | **stretch 기준** | "minimum 통과 후에만 `5seed 10000cyc`으로 stretch 검증" |
| 세션 종료 | **handoff 요구** | "종료 시 handoff에 '다음 첫 명령', '남은 blocker + 재현 명령' 포함" |

---

## 5. 작업 유형별 프롬프트 템플릿

### 5.1 기능 추가 / 행동 변경

```
@docs/plans/<관련 plan 파일> 을 기반으로 <기능명>을 구현해줘.

## 세션 목표
<1줄 목표>

## 완료 기준
- minimum pass bar: <명령 + 기대 결과>

## 실행 컨텍스트
- 브랜치: <branch명>
- worktree: 사용 안 함 / 허용 (브랜치명: <name>)
- cwd: <절대 경로>

## 수정 범위
- 대상: <파일/디렉토리>
- 비범위: <제외 항목>

## 실행 원칙
- plan-readiness-check 통과 후 구현 시작
- 설정 변경 시 dry-run 먼저
- 완료 전 verification-before-completion 실행
- 종료 시 session-handoff
```

### 5.2 버그 디버깅

```
<증상 1줄 설명>을 근본원인부터 분석해줘.

## 세션 목표
<버그 해결 1줄>

## 재현 정보
- 재현 명령: <command>
- 증상: <무엇이 / 어디서 / 어떻게 틀렸는지>
- 대상 signal/seed/cycle: <범위>

## 완료 기준
- minimum pass bar: <명령 + 기대 결과>

## 실행 컨텍스트
- 브랜치: <branch명>
- cwd: <절대 경로>

## 확인할 artifact
- IR dump / 생성 코드 / 비교 로그 중: <무엇을 볼지>

## 실행 원칙
- systematic-debugging 스킬을 따라 Phase 1(증거 수집)부터 시작
- 가설 수립 전 재현 + 2종 증거 확보 필수
- 수정 전마다 현재 생성물 fresh 확인
- 완료 전 verification-before-completion 실행
- 종료 시 session-handoff (남은 blocker + 재현 명령 포함)
```

### 5.3 문서 정합성 / docs-only 정리

```
docs/plans와 docs/report의 정합성을 점검하고 동기화해줘.

## 세션 목표
plan↔report 불일치 0건 달성

## 대상 범위
- docs/plans/phase-<N>-*/
- docs/report/phase-<N>-*/

## 실행 원칙
- auditing-plan-report-sync 스킬을 따라 5단계 진행
- TDD, worktree, branch-finish는 생략
- 수정 전 diff 보여주고 확인 받을 것
- 종료 시 session-handoff
```

### 5.4 Phase 감사

```
Phase <N>의 plan 대비 실제 진행 상태를 감사해줘.

## 세션 목표
Phase <N> 감사 보고서 완성

## 대상 범위
- docs/plans/phase-<N>-*/
- docs/report/phase-<N>-*/
- known-limitations.md
- open-decisions.md

## 실행 원칙
- auditing-plan-report-sync 스킬 전체 5단계 진행
- 불일치 목록을 심각도 기준으로 정렬해 보고
- 수정은 내 확인 후에만 진행
```

### 5.5 세션 handoff

```
이번 세션을 마감하고 다음 세션 준비를 해줘.

## 필수 포함 항목
- plan 체크리스트 최신화
- 남은 blocker: 원인 축(C1~C5) + 재현 명령
- 다음 세션 첫 명령 (실행 가능한 형태)
- 환경/경로 이슈 발견 시 convention 반영 여부

## 실행 원칙
- session-handoff 스킬을 따라 진행
- plan↔report 동기화 확인 (auditing-plan-report-sync)
- 미커밋 변경은 커밋 또는 stash 처리
```

---

## 6. 사용자 comment: 지시를 더 정확하게 하려면

회고/RCA에서 반복 실패를 일으킨 지시 패턴과 개선 방향을 정리한다.

### 6.1 피해야 할 지시 패턴

| 패턴 | 왜 실패하는가 | 개선 |
|------|-------------|------|
| "이 버그 고쳐줘" (재현 정보 없음) | 에이전트가 가설부터 세우고 반복 시도 (Pattern B) | 재현 명령 + 증상 + signal/cycle 범위를 같이 줘야 한다 |
| "plan 따라서 진행해" (plan이 stale) | plan이 현재 저장소와 안 맞으면 세션 시간 소진 (Pattern A) | "plan이 현재 상태와 맞는지 먼저 확인해" 또는 plan-readiness-check 명시 |
| "전부 테스트해" (범위 미지정) | 검증 범위가 중간에 확장되고 기준선 흔들림 (C4) | minimum pass bar 1개 + stretch는 별도 지시 |
| cwd/경로 미명시 | 경로 오류 연쇄 (Pattern C) | working_directory 또는 절대 경로를 프롬프트에 포함 |
| "BLOCKED인 것 같아" (실행 안 해본 채) | 에이전트도 추측으로 BLOCKED 처리 | "먼저 실행해보고 실패 로그를 보여줘" |
| 세션 목표 여러 개 | 어느 것도 완료 못하고 종료 | 세션 목표는 1개로 고정 |

### 6.2 효과적인 지시의 구조

```
[목표] 1개
[기준] minimum pass bar 1개
[맥락] branch, cwd, 관련 plan, 이전 handoff
[범위] 수정 대상 + 비범위
[증거 요구] 재현 명령, 확인할 artifact
[검증 요구] 완료 전 verification, stretch는 별도
[종료 요구] handoff 형식 (다음 첫 명령, 남은 blocker)
```

### 6.3 "자명한 후속 작업"은 지시 안 해도 됨

`agent-behavior.mdc`의 "reason-first decision" 규칙에 따라, 아래는 에이전트가 알아서 수행한다:
- 문서 동기화
- 커밋 메시지 작성
- 테스트 실행
- lint 확인

이런 것까지 지시에 넣으면 프롬프트만 길어지고 핵심이 묻힌다.

---

## 7. `.cursor` 자산 변경안 명세

현재 자산의 공백과 충돌을 해소하기 위해 아래 변경을 제안한다.

### 7.1 새 rule 추가

#### `workflow-routing-hirct.mdc` (alwaysApply)

**목적**: 요청 유형별 첫 진입 skill을 규칙으로 강제해서, 에이전트가 라우팅을 추론에 의존하지 않게 한다.

**내용 골자**:
```
요청 유형 → 필수 첫 skill:
- 기능/행동 변경 → brainstorming 또는 writing-plans
- 버그/실패 → systematic-debugging
- 문서 정합성 → auditing-plan-report-sync
- 코드리뷰 피드백 → receiving-code-review
- 세션 마감 → session-handoff

docs-only 작업 판정:
- 수정 대상이 docs/ 하위만이면 TDD, worktree, branch-finish 생략 가능
```

#### `report-docs-hirct.mdc` (globs: `docs/report/**/*.md`)

**목적**: `docs/report/` 규약을 rule로 승격해서, `auditing-plan-report-sync` skill에만 의존하던 report 형식을 파일 스코프로 고정한다.

**내용 골자**:
```
report 파일 형식:
- 근거 테이블만 포함 (체크리스트는 plan에만)
- plan과 1:1 또는 명시된 매핑
- 게이트 번호, 실측값, exit code 기반 기록
- 주관적 판단 금지
```

### 7.2 새 skill 추가

#### `verify-before-blocked/SKILL.md`

**목적**: `agent-behavior.mdc`의 "BLOCKED 증거 필수" 규칙을 절차화된 skill로 보강한다.

**트리거**: 에이전트가 BLOCKED를 선언하려 할 때, 또는 환경/도구 문제가 의심될 때.

**절차 골자**:
1. `utils/setup-env.sh` 실행 → exit code 확인
2. 의심 명령을 실제 실행 → 실패 로그 캡처
3. 환경/도구/권한/입력/규칙 중 어디서 실패했는지 분류
4. BLOCKED / PARTIAL / READY 판정 템플릿 출력
5. BLOCKED일 경우 재현 명령 + 실패 로그를 기록

**근거**: 회고 문서 C2 원인 축 7건. "추측으로 BLOCKED 처리"가 반복됨.

#### `docs-maintenance-and-sync/SKILL.md`

**목적**: 문서-only 작업 전용 경량 flow를 제공해서, 구현형 흐름(TDD/worktree/branch-finish)이 불필요하게 적용되는 걸 방지한다.

**트리거**: 수정 대상이 `docs/` 하위만일 때. plan/report 동기화, retrospective 작성, convention 갱신 등.

**절차 골자**:
1. 관련 rules 확인 (`plan-docs-hirct.mdc`, `report-docs-hirct.mdc`)
2. plan/report/status 차이 수집 (`auditing-plan-report-sync`와 연동)
3. 수정 범위 확인 후 일괄 수정
4. 검증: 불일치 0건 확인
5. 구현용 TDD/worktree/branch-finish 흐름 생략

**근거**: 현재 시스템은 구현 중심으로 잘 짜여 있지만, 문서 운영에 같은 강도를 적용하면 과도하다.

### 7.3 새 command 추가

현재 command는 `brainstorm`, `write-plan`, `execute-plan`의 3개뿐이다. 디버깅/감사/handoff/리뷰 피드백은 command 없이 skill 이름을 직접 알아야 한다. 아래 4개를 추가하면 사용자 진입 경로가 명확해진다.

| 커맨드 파일 | 연결 skill | 설명 |
|------------|-----------|------|
| `debug.md` | `systematic-debugging` | 버그/실패 근본원인 조사 시작 |
| `audit-plan-report.md` | `auditing-plan-report-sync` | plan↔report 정합성 감사 |
| `handoff.md` | `session-handoff` | 세션 마감 및 다음 세션 인계 |
| `review-feedback.md` | `receiving-code-review` | 코드리뷰 피드백 수신 및 반영 |

각 command의 형식은 기존 command와 동일:
```markdown
---
description: <설명>
disable-model-invocation: true
---
Invoke the superpowers:<skill-name> skill and follow it exactly as presented to you
```

### 7.4 기존 자산 보정

#### `finishing-a-development-branch/SKILL.md` — worktree cleanup 규칙 정합성

**문제**: Step 5에서 Option 1, 2, 4를 cleanup 대상으로 쓰지만, Common Mistakes/Red Flags는 1, 4만 cleanup하라고 쓴다.

**수정**: Quick Reference 테이블 기준으로 통일한다:
- Option 1 (Merge locally): cleanup
- Option 2 (Create PR): keep (PR 검토/수정 여지)
- Option 3 (Keep as-is): keep
- Option 4 (Discard): cleanup

Step 5의 "For Options 1, 2, 4:" → "For Options 1, 4:" 로 변경.

#### `plan-readiness-check/SKILL.md` — mandatory gate 명시

**문제**: 현재 skill 본문에 mandatory라는 표현이 없어서 optional로 취급되는 경우가 있다.

**수정**: Overview에 아래 문구 추가:
```
이 스킬은 구현 착수 전 mandatory gate이다. No-Go 시 구현을 시작할 수 없다.
```

#### `verification-before-completion/SKILL.md` — minimum bar / stretch 분리

**문제**: 현재 skill은 "검증 명령을 실행하라"만 강제하고, 검증 범위(minimum vs stretch)를 분리하지 않는다.

**수정**: Gate Function에 아래 단계 추가:
```
0. SCOPE: 사용자가 지정한 minimum pass bar와 stretch check를 구분한다.
   - minimum bar 미달 시 stretch로 넘어가지 않는다.
   - stretch 결과는 별도 보고한다.
```

#### `session-handoff/SKILL.md` — "다음 첫 명령" 강제

**문제**: 현재 템플릿에 "다음 세션 실행 프롬프트"는 있지만, "다음 세션 첫 명령"이 실행 가능한 형태인지 검증하지 않는다.

**수정**: 체크리스트에 아래 항목 추가:
```
4. handoff의 "다음 세션 첫 명령"이 copy-paste로 바로 실행 가능한 형태인가?
5. 남은 blocker가 있으면 원인 축(C1~C5)과 재현 명령이 기록되었는가?
```

#### `using-git-worktrees/SKILL.md` — 프로젝트 규칙 우선 조건

**문제**: skill은 worktree 생성을 전제하지만, `agent-behavior.mdc`는 기본 금지한다. 에이전트가 혼동한다.

**수정**: skill 상단에 아래 guard 추가:
```
## Project Override
이 프로젝트는 agent-behavior.mdc에서 worktree를 기본 금지한다.
사용자가 명시적으로 "worktree 허용" 또는 "worktree 생성해"라고 지시한 경우에만 이 skill을 진행한다.
그렇지 않으면 현재 브랜치에서 작업한다.
```

---

## 8. 우선 보정 포인트 요약

변경안 전체 중 즉시 효과가 큰 항목을 우선순위로 정렬한다.

| 순위 | 항목 | 유형 | 근거 |
|------|------|------|------|
| 1 | `workflow-routing-hirct.mdc` 신설 | rule | 에이전트 라우팅 오류 방지. 모든 세션에 적용 |
| 2 | command 4종 추가 | command | 사용자 진입 경로 명확화. 비용 최소 |
| 3 | `plan-readiness-check` mandatory 승격 | skill 보정 | Gate 5: 카테고리 E 3건 방지. 1줄 추가 |
| 4 | `finishing-a-development-branch` cleanup 규칙 통일 | skill 보정 | 문서 정합성. 1줄 변경 |
| 5 | `verification-before-completion` minimum/stretch 분리 | skill 보정 | Gate 5: 카테고리 C4 3건 방지 |
| 6 | `session-handoff` 첫 명령 + blocker 강제 | skill 보정 | Gate 6: 카테고리 C5 방지 |
| 7 | `using-git-worktrees` project override guard | skill 보정 | worktree 정책 충돌 해소 |
| 8 | `verify-before-blocked` 신설 | skill | 카테고리 C2 7건 방지 |
| 9 | `report-docs-hirct.mdc` 신설 | rule | plan↔report drift 방지 |
| 10 | `docs-maintenance-and-sync` 신설 | skill | docs-only 작업 과도 절차 방지 |

---

## 9. `.cursor` 변경안 파일 초안

다음 단계에서 바로 생성/수정할 수 있도록 각 파일의 전문 초안을 기록한다.

### 9.1 새 rule: `.cursor/rules/workflow-routing-hirct.mdc`

```markdown
---
description: 요청 유형별 에이전트 첫 진입 skill 라우팅. 모든 세션에 적용.
alwaysApply: true
---

# Workflow Routing (HIRCT)

## 진입 라우팅 테이블

사용자 요청을 받으면 아래 테이블에서 해당 유형의 **필수 첫 skill**을 먼저 호출한다.

| 요청 유형 | 필수 첫 skill | 비고 |
|----------|--------------|------|
| 기능 추가 / 행동 변경 | `brainstorming` 또는 `writing-plans` | 설계 불확실 → brainstorming, 확정 → writing-plans |
| 버그 / 테스트 실패 / 예상 밖 동작 | `systematic-debugging` | Phase 1(증거 수집)부터 시작 |
| 문서 정합성 / plan↔report 동기화 | `auditing-plan-report-sync` | docs-only flow 적용 |
| 코드리뷰 피드백 반영 | `receiving-code-review` | 기술 검증 후 반영 |
| 세션 마감 / handoff | `session-handoff` | Gate 6 포함 |

## docs-only 판정

수정 대상이 `docs/` 하위만이면 다음을 생략할 수 있다:
- `test-driven-development`
- `using-git-worktrees`
- `finishing-a-development-branch`

## mandatory gate

`plan-readiness-check`는 구현 착수 전 mandatory gate이다. No-Go 시 구현을 시작할 수 없다.
```

### 9.2 새 rule: `.cursor/rules/report-docs-hirct.mdc`

```markdown
---
description: HIRCT report 문서 규약 — 근거 테이블 전용, plan과 1:1 매핑
globs: "docs/report/**/*.md"
alwaysApply: false
---

# Report Document Convention (HIRCT)

## 기본 원칙
- report 파일은 **근거 테이블만** 포함한다. 체크리스트는 `docs/plans/`에만 둔다.
- 각 report 파일은 대응하는 plan 파일과 1:1 또는 명시된 매핑을 가진다.

## 근거 테이블 형식

| # | 게이트 | 결과 | 실측 |
|---|--------|------|------|
| 1 | G번호 | PASS/FAIL | exit code + 핵심 수치 |

## 금지 사항
- 주관적 판단 ("잘 동작하는 것 같다") 금지. exit code와 수치만 기록.
- 체크리스트 중복 배치 금지.
- 게이트 번호 없이 PASS/FAIL 표기 금지.
```

### 9.3 새 command: `.cursor/commands/debug.md`

```markdown
---
description: 버그/테스트 실패/예상 밖 동작의 근본원인을 체계적으로 조사한다
disable-model-invocation: true
---
Invoke the superpowers:systematic-debugging skill and follow it exactly as presented to you
```

### 9.4 새 command: `.cursor/commands/audit-plan-report.md`

```markdown
---
description: plan과 report의 정합성을 감사하고 동기화한다
disable-model-invocation: true
---
Invoke the superpowers:auditing-plan-report-sync skill and follow it exactly as presented to you
```

### 9.5 새 command: `.cursor/commands/handoff.md`

```markdown
---
description: 세션을 마감하고 다음 세션 인계 문서를 준비한다
disable-model-invocation: true
---
Invoke the superpowers:session-handoff skill and follow it exactly as presented to you
```

### 9.6 새 command: `.cursor/commands/review-feedback.md`

```markdown
---
description: 코드리뷰 피드백을 기술적으로 검증한 뒤 반영한다
disable-model-invocation: true
---
Invoke the superpowers:receiving-code-review skill and follow it exactly as presented to you
```

### 9.7 새 skill: `.cursor/skills/verify-before-blocked/SKILL.md`

```markdown
---
name: verify-before-blocked
description: BLOCKED 선언 전 실행 증거 수집 절차. 환경/도구 문제가 의심되거나 에이전트가 BLOCKED를 선언하려 할 때 사용.
---

# Verify Before BLOCKED

## Overview

BLOCKED를 추측으로 선언하지 않는다. 실행을 시도하고, 실패 로그를 증거로 남긴 뒤에만 BLOCKED를 판정한다.

**Core principle:** 증거 없는 BLOCKED는 BLOCKED가 아니다.

## 절차

### Step 1: 환경 확인
- `source utils/setup-env.sh` 실행 → exit code 확인
- 필요한 도구/바이너리가 PATH에 있는지 `which <tool>` 확인

### Step 2: 의심 명령 실행
- BLOCKED를 유발한다고 생각하는 명령을 실제로 실행한다.
- 출력과 exit code를 캡처한다.

### Step 3: 실패 분류

| 실패 유형 | 증상 | 다음 행동 |
|----------|------|----------|
| 도구 미설치 | `command not found` | setup-env.sh 확인 → 설치 가이드 제시 |
| 경로 오류 | `No such file or directory` | 절대 경로로 재시도 |
| 권한 문제 | `Permission denied` | 권한 변경 또는 에스컬레이션 |
| 입력 오류 | 0바이트 파일, 잘못된 포맷 | 입력 생성 경로 재확인 |
| 외부 의존성 | segfault, 미지원 기능 | XFAIL 분류 + upstream 이슈 기록 |

### Step 4: 판정

| 판정 | 기준 |
|------|------|
| **READY** | 명령 실행 성공. 원래 작업 계속 진행 |
| **PARTIAL** | 일부 실패하지만 우회 가능. 우회안을 기록하고 진행 |
| **BLOCKED** | 실행 불가. 재현 명령 + 실패 로그 + 원인 축(C1~C5) 기록 |

### Step 5: BLOCKED 기록 형식

BLOCKED 판정 시 아래 형식으로 기록:
```
## BLOCKED: [1줄 요약]
- **원인 축**: C1~C5 중 해당
- **재현 명령**: `<copy-paste 가능한 명령>`
- **실패 로그**: <핵심 에러 메시지>
- **시도한 우회**: <있으면 기록>
- **에스컬레이션**: 사용자 확인 필요 사항
```

## Common Mistakes

| 실수 | 수정 |
|------|------|
| "설치 안 된 것 같다" → BLOCKED 선언 | 실제로 실행해보고 `command not found` 확인 후 판정 |
| setup-env.sh 미확인 | 항상 먼저 source 후 재시도 |
| 경로 추측 | `ls`, `which`, `find`로 실제 경로 확인 |

## Integration

**Called by:**
- 모든 skill에서 BLOCKED 상황이 의심될 때
- `agent-behavior.mdc`의 "BLOCKED 증거 필수" 규칙 집행

**Pairs with:**
- `systematic-debugging` - 환경 문제가 버그와 얽혀있을 때
- `plan-readiness-check` - 환경 전제조건 검증 시
```

### 9.8 새 skill: `.cursor/skills/docs-maintenance-and-sync/SKILL.md`

```markdown
---
name: docs-maintenance-and-sync
description: 문서-only 작업(plan/report 동기화, retrospective, convention 갱신) 전용 경량 flow. 수정 대상이 docs/ 하위만일 때 사용.
---

# Docs Maintenance & Sync

## Overview

문서-only 작업에 구현형 흐름(TDD/worktree/branch-finish)을 적용하면 과도하다. 이 skill은 문서 작업에 최적화된 경량 flow를 제공한다.

## When to Use

- 수정 대상이 `docs/` 하위 파일만
- plan/report 동기화, retrospective 작성, convention 갱신
- known-limitations.md, open-decisions.md 정리
- 구현 코드 변경 없음

## 절차

### Step 1: 관련 규칙 확인
해당 파일에 적용되는 rule을 확인한다:
- `docs/plans/**/*.md` → `plan-docs-hirct.mdc`
- `docs/report/**/*.md` → `report-docs-hirct.mdc`

### Step 2: 현황 수집
- `auditing-plan-report-sync` 1~2단계 실행 (현황 수집 + 정합성 검증)
- 또는 대상 문서를 직접 읽어 현재 상태 파악

### Step 3: 수정 계획
- 변경 범위를 사용자에게 보여주고 확인
- diff preview를 제공

### Step 4: 수정 실행
- 확인받은 범위만 수정
- 커밋

### Step 5: 검증
- 수정 후 불일치 0건 확인 (`auditing-plan-report-sync` 2단계 재실행)

## 생략 가능 항목

이 skill 사용 시 아래는 생략한다:
- `test-driven-development` (코드 변경 없음)
- `using-git-worktrees` (문서 충돌 위험 낮음)
- `finishing-a-development-branch` (merge 판단 불필요)

## Integration

**Pairs with:**
- `auditing-plan-report-sync` - 핵심 검증 로직 재사용
- `session-handoff` - 문서 작업 종료 후 인계
```

### 9.9 기존 skill 보정 diff 요약

| 파일 | 변경 내용 | 변경량 |
|------|----------|--------|
| `finishing-a-development-branch/SKILL.md` | Step 5: "For Options 1, 2, 4:" → "For Options 1, 4:" | 1줄 |
| `plan-readiness-check/SKILL.md` | Overview에 mandatory gate 문구 추가 | 1줄 |
| `verification-before-completion/SKILL.md` | Gate Function에 SCOPE 단계(minimum/stretch 분리) 추가 | 5줄 |
| `session-handoff/SKILL.md` | 체크리스트에 "다음 첫 명령 실행 가능" + "blocker 원인 축+재현 명령" 항목 추가 | 2줄 |
| `using-git-worktrees/SKILL.md` | 상단에 Project Override guard 추가 | 5줄 |

---

## 10. 적용 방법

1. **즉시**: 이 가이드의 섹션 4 체크리스트와 섹션 5 템플릿을 다음 세션부터 사용한다.
2. **1단계**: 우선순위 1~4 (rule 1개, command 4개, skill 보정 2개)를 먼저 반영한다.
   - `workflow-routing-hirct.mdc` 생성
   - `debug.md`, `audit-plan-report.md`, `handoff.md`, `review-feedback.md` 생성
   - `plan-readiness-check/SKILL.md` mandatory 문구 추가
   - `finishing-a-development-branch/SKILL.md` cleanup 규칙 통일
3. **2단계**: 우선순위 5~7 (skill 보정 3개)을 반영한다.
   - `verification-before-completion/SKILL.md` minimum/stretch 분리
   - `session-handoff/SKILL.md` 첫 명령 + blocker 강제
   - `using-git-worktrees/SKILL.md` project override guard
4. **3단계**: 우선순위 8~10 (새 skill 2개, rule 1개)을 반영한다.
   - `verify-before-blocked/SKILL.md` 생성
   - `report-docs-hirct.mdc` 생성
   - `docs-maintenance-and-sync/SKILL.md` 생성
5. **측정**: 파일럿 2주 후 아래 지표로 효과 판정.
   - 재지시율 (동일 유형 반복 지시 감소 여부)
   - plan-readiness No-Go 재발률
   - 가설 변경 횟수 (디버깅 세션)
   - handoff 불일치 건수
   - BLOCKED 오판 건수
