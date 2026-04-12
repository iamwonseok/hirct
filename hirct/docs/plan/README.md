# Plan Change Tracking

`hirct/docs/plan` 아래 문서는 작업 계획의 현재판을 담는다. 수정이 누적되면 현재판만 보고는 의도 변화와 판단 근거를 추적하기 어려워질 수 있으므로, 아래 규칙을 따른다.

## 기본 원칙

- git commit이 1차 이력이다.
- 계획의 의미가 바뀌는 수정은 잡다한 문서 수정과 섞지 않고 별도 commit으로 남긴다.
- milestone closure 기준, evidence class, verification 전제, scope in/out 변경은 "의미 변경"으로 간주한다.
- 단순 오탈자, 문장 다듬기, 링크 수정은 의미 변경이 아니면 묶어도 된다.

## 권장 커밋 규칙

- 계획 변경 commit 메시지는 `docs(plan): ...` 또는 `docs(M0): ...` 같이 범위를 드러낸다.
- 한 commit에는 가능하면 한 milestone의 한 의미 변경만 담는다.
- plan과 truthful contract(`v1-scope.md`, `known-limitations.md`)를 함께 바꿀 때는 왜 같이 바꾸는지 commit message에 드러낸다.

## 문서 내 추적 규칙

- 계획 실행 중 새 사실이 드러나면, 기존 문구를 덮어쓰기만 하지 말고 closure rule / risk note / evidence rule에 반영한다.
- 현재판을 유지하되, 큰 방향 전환이 생기면 새 파일을 추가로 만든다.
  - 예: `YYYY-MM-DD-<milestone>-plan-v2.md`
  - 또는 `YYYY-MM-DD-<topic>-decision.md`
- roadmap 수준 결정이 바뀌면 `2026-04-12-v1-export-roadmap.md`에도 같은 날 반영해 상위 기준과 하위 계획이 엇갈리지 않게 한다.

## 언제 새 파일을 만들까

아래 중 하나면 기존 파일 수정만 하지 말고 새 파일 또는 별도 decision 문서를 고려한다.

- milestone 목표 자체가 바뀜
- verification 전제가 바뀜
- evidence class 체계가 바뀜
- 구현 milestone이 문서 milestone으로 축소/확대됨
- 다음 milestone의 입력 조건이 바뀜

## 최소 운영 방법

최소한 아래 둘은 항상 지킨다.

1. 의미 있는 plan 변경은 별도 commit으로 남긴다.
2. 왜 바뀌었는지 한두 줄이 commit message나 decision 문서에 드러나게 한다.
