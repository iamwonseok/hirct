---
name: session-handoff
description: Use when ending a session and need to prepare context for the next session, when asked to create a handoff prompt, or when switching between work contexts
---

# Session Handoff

## Overview

현재 세션의 상태를 표준 형식으로 정리하여, 다음 세션에서 컨텍스트 손실 없이 작업을 이어갈 수 있게 한다.

**Core principle:** 다음 세션의 에이전트가 이 문서만 읽고 바로 실행할 수 있어야 한다.

**Announce at start:** "I'm using the session-handoff skill to prepare the handoff."

## When to Use

- 세션 종료 시
- 사용자가 "handoff 정리해줘", "다음에 이어서 하게 준비해줘" 요청 시
- 작업 컨텍스트 전환 시

## 수집 항목 (자동)

아래 정보를 자동으로 수집한다:

```bash
git branch --show-current
git status --short
git log --oneline -5
git worktree list
```

## 출력 형식

```markdown
## Session Handoff

### 현재 상태
- **브랜치**: `<current-branch>`
- **워크트리**: `<worktree-path>` (있는 경우)
- **미커밋 변경**: 있음/없음

### 완료 사항
- [x] 항목 1
- [x] 항목 2

### 남은 작업
- [ ] 항목 A (우선순위: 높음)
- [ ] 항목 B (우선순위: 중간)

### 이슈/블로커
- (있는 경우만 기록)

### 다음 세션 실행 프롬프트
\```
@<기준 계획 파일> 을 기반으로 남은 작업을 진행해줘.

실행 원칙:
- @.cursor/convention 준수
- 이슈 발생 시 기록 후 계속 진행
- 완료 후 verification-before-completion 실행

남은 작업:
1. <구체적 작업 1>
2. <구체적 작업 2>
\```
```

## 체크리스트

handoff 작성 전 확인:
1. 미커밋 변경사항이 있으면 커밋 또는 stash 처리
2. worktree가 남아있으면 정리 필요 여부 확인
3. 계획 문서의 체크리스트가 현재 상태와 일치하는지 확인

## Common Mistakes

| 실수 | 수정 |
|------|------|
| 수치를 하드코딩 | "현재 main 기준 재측정"으로 기술 |
| 남은 작업이 모호 | 구체적 파일 경로와 실행 명령 포함 |
| worktree 상태 누락 | `git worktree list` 결과 반드시 포함 |
| 계획 파일 경로 누락 | 다음 프롬프트에 기준 문서 경로를 `@` 형식으로 명시 |

## Integration

**Called by:**
- **finishing-a-development-branch** (Step 5 이후) - merge/cleanup 완료 후
- 사용자 직접 요청

**Pairs with:**
- **plan-readiness-check** - 다음 세션에서 handoff 프롬프트 실행 전 readiness 재확인
- **auditing-plan-report-sync** - 세션 종료 전 plan↔report 동기화 확인
