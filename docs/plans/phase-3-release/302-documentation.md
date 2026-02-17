# Task 302: mkdocs 문서화

> **목표**: mkdocs로 생성된 doc/ 산출물 기반 문서 사이트 빌드
> **예상 시간**: 1일
> **산출물**: `mkdocs.yml`, docs/ 구조, `make docs` 타겟
> **게이트**: make docs 성공

---

## 목표

hirct-gen이 생성한 doc/ 산출물을 활용해 mkdocs 문서 사이트를 구성한다. 모듈 인덱스는 **output/report.json**(표준 소스)과 `output/` 디렉터리 구조를 기반으로 자동 생성한다.

## 주요 작업

- mkdocs, mkdocs-material 설치 확인
- mkdocs.yml 생성 (theme, nav 설정)
- docs/index.md 작성 (프로젝트 소개)
- 모듈 인덱스 자동 생성: `utils/generate-report.py` (report.json 기반)
- make docs 타겟: 인덱스 생성 + mkdocs build
- docs/quickstart.md (3단계 Quick Start)
- mkdocs build 성공 확인

## 게이트 (완료 기준)

- [ ] `make docs` → exit 0
- [ ] `test -d site/ && test -f site/index.html` → 정적 파일 존재
- [ ] `mkdocs serve` → 로컬 브라우저에서 문서 사이트 렌더링 확인 (수동)
