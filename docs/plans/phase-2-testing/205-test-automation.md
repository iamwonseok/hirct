# Task 205: make test-all + CI

> **목표**: make test-all로 전체 테스트 단일 명령 실행
> **예상 시간**: 1일
> **산출물**: Makefile (test-all, check-hirct, check-hirct-unit, check-hirct-integration, report, test-traversal 타겟)
> **게이트**: make test-all 실행 시 exit 0

---

## 목표

`make test-all`로 전체 테스트를 한 번에 실행한다. 하위 디렉터리의 `make test`가 해당 서브트리를 테스트하는 recursive make 패턴을 사용한다.

**원칙**:
- CI/로컬 모두 **`make test-all` 단일 명령**을 메인 진입점으로 사용한다 (CI 시스템 독립).
- VCS/ncsim 게이트는 라이선스 서버 접근이 가능한 환경(self-hosted runner 등)에서만 활성화한다.

## 주요 작업

```
make test-all
  ├── make check-hirct            → ninja check-hirct → lit build/test/ (FileCheck)
  ├── make check-hirct-unit       → ninja check-hirct-unit → gtest 바이너리
  ├── make check-hirct-integration → ninja check-hirct-integration → lit build/integration_test/
  └── make report                 → utils/generate-report.py

make test-traversal               → lit integration_test/traversal/ (CI 제외)
```

- make check-hirct: `ninja -C build check-hirct` — CMake `add_custom_target`으로 등록된 lit FileCheck 단위 테스트. CMake `DEPENDS hirct-gen hirct-verify`로 바이너리 빌드 순서 자동 보장.
- make check-hirct-unit: `ninja -C build check-hirct-unit` — gtest C++ 유닛 테스트
- make check-hirct-integration: `ninja -C build check-hirct-integration` — lit 통합 스모크 테스트. Phase 2에서 `integration_test/CMakeLists.txt` + `lit.site.cfg.py.in` 추가.
- make report: `utils/generate-report.py` — 리포트 변환
- make test-traversal: `lit integration_test/traversal/` — 전체 순회 테스트 (CI 제외, 별도 실행)
- recursive make: 디렉터리별 make test가 해당 서브트리 테스트 (per-module Makefile, GenMakefile.cpp 자동 생성)
- make test-all: check-hirct + check-hirct-unit + check-hirct-integration + report 통합
- 중간 실패 시 즉시 exit 1 전파

> **CMake lit 통합**: Phase 1 Task 100에서 `test/CMakeLists.txt` + `test/lit.site.cfg.py.in`을 작성하여
> `ninja check-hirct` 타겟을 등록한다. Phase 2에서는 동일 패턴으로 `integration_test/CMakeLists.txt` +
> `integration_test/lit.site.cfg.py.in`을 추가하여 `ninja check-hirct-integration` 타겟을 등록한다.
> 상세: `reference-commands-and-structure.md` §13 참조.

## 로컬 테스트 실행 매뉴얼 (CIRCT 스타일)

CIRCT의 `check-*` 타겟 운용 방식과 동일하게, 빌드 완료 후 테스트 타겟을 순차 실행한다.

### 1) 사전 준비

```bash
export CIRCT_BUILD="$HOME/circt/build"
export PATH="$CIRCT_BUILD/bin:$PATH"
```

### 2) 빌드

```bash
cmake -B build -G Ninja
cmake --build build
```

### 3) Phase 1A 핵심 테스트

```bash
ninja -C build check-hirct
ninja -C build check-hirct-unit
```

확인 항목:
- lit 결과: `2/2 PASS` (현재 스모크 기준)
- gtest 결과: `1/1 PASS`
- xUnit 파일: `build/lit-check.xml` 존재

### 4) 통합 진입점 테스트 (Phase 2+)

```bash
make test-all
```

권장:
- 로컬 개발 중에는 `check-hirct`/`check-hirct-unit`를 빠르게 반복
- CI 직전에는 `make test-all`로 최종 회귀 확인

## CI 워크플로 (최소 경로)

Phase 2에서 CI 워크플로 예시 파일(`.github/workflows/ci.yml` 등)을 작성한다.

**최소 CI 경로** (VCS 라이선스 불필요):

```yaml
# .github/workflows/ci.yml (예시)
name: CI
on: [push, pull_request]
jobs:
  build-and-test:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - name: Build
        run: make build
      - name: Unit Tests
        run: make check-hirct && make check-hirct-unit
      - name: Integration Tests
        run: make check-hirct-integration
```

**VCS 확장 경로** (self-hosted runner, 라이선스 서버 접근 가능):

```yaml
  vcs-gate:
    runs-on: self-hosted
    if: env.HAVE_VCS == '1'
    steps:
      - name: VCS Compile Gate
        run: make test-all HAVE_VCS=1
```

**원칙**: CI 시스템(GitHub Actions, GitLab CI, Jenkins 등)에 독립적으로 `make test-all` 단일 명령으로 동작한다.

**아키텍처 원칙**: Makefile은 사용자 진입점(얇은 래퍼), CMake는 lit 구성(`configure_file` + `add_custom_target`) 및 빌드 의존성 관리, lit은 테스트 실행 엔진, Python(`utils/generate-report.py`)은 리포트 변환만. bash 스크립트 파일(.sh) 사용 금지.

## 게이트 (완료 기준)

- [ ] `make test-all` → exit 0 (전체 테스트 통과 시)
- [ ] `make test-all` 실행 시 출력 형식:
  ```
  [test] output/.../ModuleA ... PASS
  [test] output/.../ModuleB ... PASS
  ...
  ALL TESTS PASSED (N modules)
  ```
- [ ] 중간 실패 시 `make` 즉시 중단 (exit 1)
- [ ] recursive make: `cd output/<path>/<module>/ && make test` 동작 확인
