---
name: plan-readiness-check
description: Use when a plan is written or updated and needs verification before execution, when asked to review a plan for agent-in-the-loop feasibility, or when dry-run validation is requested
---

# Plan Readiness Check

## Overview

계획 문서의 실행 가능성을 검증하고 Go/No-Go 판정을 내린다.

**Core principle:** 계획이 agent-in-the-loop으로 바로 실행 가능한 수준인지 확인한 뒤에만 실행을 시작한다.

**Announce at start:** "I'm using the plan-readiness-check skill to verify this plan."

## When to Use

- `writing-plans` 스킬로 계획 작성 완료 직후
- 사용자가 "검토해줘", "실행해도 되는지 확인", "dry-run" 요청 시
- 계획 문서가 변경된 뒤 재실행 전

## 검증 체크리스트

아래 7개 항목을 순서대로 검증하고, 각 항목에 PASS/FAIL/WARN을 부여한다.

| # | 검증 항목 | 방법 |
|---|----------|------|
| 1 | **태스크 완전성** | 모든 태스크에 Goal/Files/Steps/Run/Expect가 있는가? |
| 2 | **파일 경로 유효성** | 참조된 파일 경로가 실제로 존재하거나 생성 대상으로 명시되었는가? |
| 3 | **의존성 순서** | 태스크 간 의존성이 실행 순서와 일치하는가? |
| 4 | **범위 적정성** | 현재 Phase/Task scope를 벗어나는 과잉 구현이 없는가? |
| 5 | **SSOT 충돌** | 계획 내용이 L0 규칙/L1 문서와 충돌하지 않는가? |
| 6 | **검증 명령 존재** | 각 태스크에 실행 가능한 검증 명령(Run + Expect)이 있는가? |
| 7 | **환경 전제조건** | 필요한 도구/환경이 `utils/setup-env.sh`로 충족되는가? (추측 BLOCKED 금지) |

## 판정 기준

```
Go:   FAIL 0개, WARN 2개 이하
No-Go: FAIL 1개 이상
```

## 출력 형식

```markdown
## Plan Readiness Report

| # | 항목 | 결과 | 비고 |
|---|------|------|------|
| 1 | 태스크 완전성 | PASS | — |
| 2 | 파일 경로 유효성 | FAIL | Task 3: `lib/foo.cpp` 미존재 |
| ...

**판정: Go / No-Go**
**사유**: [1줄 요약]
```

No-Go 시 구체적 수정 사항을 항목별로 제시한다.

## Common Mistakes

| 실수 | 수정 |
|------|------|
| 검증 없이 "실행 가능합니다" 선언 | 7개 항목 전부 확인 후 판정 |
| 파일 존재 여부를 추측 | `ls`/`test -f`로 실제 확인 |
| scope 과잉을 간과 | 현재 Phase 범위와 대조 |
| SSOT 충돌을 무시 | L0 규칙 → L1 문서 순서로 교차 검증 |

## Integration

**Called by:**
- **writing-plans** (Readiness Gate) - 계획 작성 완료 후 필수 호출
- 사용자 직접 요청

**Pairs with:**
- **executing-plans** - readiness 통과 후 실행
- **subagent-driven-development** - readiness 통과 후 실행
