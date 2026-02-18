---
name: auditing-plan-report-sync
description: Use when reviewing phase progress, when plan checklists might be stale, when report evidence is missing, or when asked to audit docs/plans vs docs/report consistency
---

# Plan ↔ Report 동기화 감사

## Overview

plan 파일(체크리스트)과 report 파일(근거 데이터)의 정합성을 한번에 검증하고 수정한다.

**SSOT 원칙**: plan = 뭘 해야 하고 뭘 했는지 (`[x]`/`[ ]`), report = 그 근거 (실측 테이블).

## When to Use

- Phase 완료 후 또는 태스크 완료 후 체크리스트/리포트 갱신 필요 시
- "체크리스트가 안 맞다", "리포트가 없다", "진행 상태 파악이 안 된다" 증상
- 커밋 후 plan↔report 동기화 점검

## 감사 절차

### 1단계: 현황 수집 (병렬)

동시에 실행:
- `docs/plans/` 하위 모든 `*.md` 파일에서 `- [ ]`, `- [x]` 체크리스트 수집
- `docs/report/` 하위 모든 `*.md` 파일 목록 수집
- `git log --oneline -10` 최근 커밋 확인
- `git status --short` 미추적/변경 파일 확인

### 2단계: 정합성 검증

아래 5가지 항목을 검증하고 결과를 테이블로 출력:

| 검증 항목 | 방법 |
|----------|------|
| **A. plan↔report 1:1 매핑** | plan의 각 태스크 파일에 대응하는 report 파일이 존재하는가? |
| **B. 체크리스트 ↔ 실측 일치** | plan의 `[x]` 항목이 report에 근거(게이트 번호, 실측값)가 있는가? |
| **C. 체크리스트 ↔ 현실 일치** | plan의 `[ ]` 항목이 실제로 미완료인가? (파일 존재, 도구 설치 등 확인) |
| **D. report에 근거 링크** | plan 체크리스트에 report 파일 링크가 있는가? |
| **E. 중복 제거** | report에 체크리스트가 중복되어 있지 않은가? (report는 근거만) |

### 3단계: 불일치 목록 출력

검증 결과를 아래 형식으로 보고:

```
## 감사 결과

| # | 파일 | 검증 | 문제 | 심각도 |
|---|------|------|------|--------|
| 1 | plans/phase-0-setup/001.md | B | G10 [x]인데 report에 FAIL | 높음 |
| 2 | report/phase-0-setup/ | A | 002 report 파일 없음 | 중간 |
| ...
```

### 4단계: 수정 (사용자 확인 후)

불일치 목록을 보여주고, 수정 범위를 사용자에게 확인한 뒤 일괄 수정:
- plan 체크리스트 `[x]`/`[ ]` 갱신
- report 파일 생성 또는 갱신
- 근거 링크 추가
- 중복 체크리스트 제거

### 5단계: 검증

수정 후 2단계를 재실행하여 불일치 0건 확인.

## Quick Reference

```
plan 파일 위치:  docs/plans/phase-{N}-*/
report 파일 위치: docs/report/phase-{N}-*/

plan 체크리스트 형식:
  - [x] 항목 설명 — G번호: 실측 요약
  - [ ] 항목 설명 — 사유 (FAIL/SKIP/N/A/미실행)

report 근거 테이블 형식:
  | # | 게이트 | 결과 | 실측 |
  |---|--------|------|------|

plan → report 링크 형식:
  > 근거: [Task NNN 리포트](../../report/phase-X-xxx/NNN-name.md)
```

## Common Mistakes

| 실수 | 수정 |
|------|------|
| report에 체크리스트를 중복 배치 | report는 근거 테이블만, 체크리스트는 plan에만 |
| plan `[x]`인데 report에 근거 없음 | report에 게이트 테이블 추가 |
| plan `[ ]`인데 실제로는 완료됨 | 실측 확인 후 `[x]`로 갱신 + report에 근거 추가 |
| report만 갱신하고 plan 미갱신 | 항상 plan↔report 양방향 동기화 |
| 게이트 번호 없이 `[x]` 표시 | 반드시 근거 참조 (G번호 또는 실측 요약) 포함 |
