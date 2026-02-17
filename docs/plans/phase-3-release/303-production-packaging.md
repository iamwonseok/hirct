# Task 303: 프로덕션 패키징

> **목표**: 신규 환경에서 setup → build → hirct-gen end-to-end 동작 확인, README 완성
> **예상 시간**: 2일
> **산출물**: `README.md`, 버전 태그
> **게이트**: 신규 환경에서 setup-env.sh → make build → hirct-gen input.v → 산출물 생성

---

## 목표

프로덕션 배포를 위해 README를 완성하고, 신규 환경에서 3단계 Quick Start가 동작하는지 검증한다.

## 주요 작업

- make test-all 전체 회귀 실행
- README: 프로젝트 소개, 아키텍처 요약
- README: 설치 (utils/setup-env.sh + make build)
- README: Quick Start 3단계
  1. setup-env.sh 실행
  2. make build
  3. hirct-gen input.v → 모든 산출물 확인
- README: 산출물 목록 (GenModel, GenRAL, SV wrapper, DPI-C, HW Doc 등)
- README: 테스트 (make test, make test-all, make report)
- 신규 환경 시뮬레이션: 클린 디렉터리에서 setup → build → hirct-gen → 산출물 확인
- 버전 태깅 (v0.1.0 등)
- Dockerfile 작성 (Ubuntu 22.04 + 필수 도구 버전 고정, 재현 가능 빌드 환경)
- SECURITY.md 작성 (보안 취약점 보고 절차, 외부 프로세스 호출 관련 입력 검증 정책)
- CHANGELOG.md 초안 작성 (Keep a Changelog 형식)
- 버전 정책 확정: Semantic Versioning (MAJOR.MINOR.PATCH), 태그 규칙 (`v0.1.0` 형식)

## 게이트 (완료 기준)

- [ ] 클린 디렉토리에서 3단계 Quick Start:
  1. `./utils/setup-env.sh` → 모든 도구 버전 출력 확인
  2. `make build` → `test -x build/tools/hirct-gen/hirct-gen` (바이너리 존재)
  3. `hirct-gen rtl/.../Fadu_K2_S5_LevelGateway.v` → `output/.../` 에 8종 산출물 생성
- [ ] `make test-all` → exit 0
- [ ] README.md Quick Start 3단계 완성 및 정확성 확인
- [ ] `Dockerfile` 존재 + `docker build .` → 이미지 빌드 성공
- [ ] `SECURITY.md` 존재 + 보안 취약점 보고 절차 포함
- [ ] `CHANGELOG.md` 존재 + Keep a Changelog 형식 준수
- [ ] `git tag v0.1.0` → Semantic Versioning 규칙 준수
