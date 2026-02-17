# Task 202: Top 순회 (filelist 기반)

> **목표**: filelist 기반으로 Top 모듈 산출물 생성
> **예상 시간**: 2일
> **산출물**: `output/<path>/top/`

---

## 목표

filelist를 사용해 Top 모듈 산출물을 생성한다. `hirct-gen -f filelist.f --top <TopModule>`로 filelist 기반 빌드를 수행하고, Top 산출물은 `output/<path>/top/`에 생성한다.

## Top 모듈 식별 방법

Top 모듈 목록은 다음 순서로 식별한다:

1. **수동 목록 (canonical)**: `config/top-modules.txt`에 Top 모듈명과 해당 filelist 경로를 기록
2. 형식: `<TopModuleName> <filelist_path>` (한 줄에 하나)
3. 예시:
   ```
   CoreIPSubsystem  rtl/plat/src/s5/design/filelist.f
   UartTop          rtl/plat/src/uart/filelist.f
   ```
4. Phase 2 시작 시 RTL 디렉토리 구조를 탐색하여 기존 filelist.f 파일들을 수집하고, 각 filelist 내 최상위 모듈을 식별하여 `config/top-modules.txt`를 초기 생성한다.

> **원칙**: 자동 탐지는 초기 수집용이며, 최종 목록은 반드시 수동 검증 후 확정한다.

## 주요 작업

- `config/top-modules.txt` 생성 (Top 모듈명 + filelist 경로)
- 각 filelist에 대해 `hirct-gen -f filelist.f --top <TopModule>` 실행
- output/<path>/top/ 산출물 구조 검증 (cmodel/, wrapper/, doc/ 등)
- 실패 시 원인 문서화 (`known-limitations.md`에 `multi_module` 카테고리로 등록)

## 게이트 (완료 기준)

- [ ] `config/top-modules.txt` 존재 + 최소 1개 Top 모듈 등록
- [ ] `hirct-gen -f filelist.f --top <TopModule>` → exit 0
- [ ] `output/<path>/top/` 디렉토리 생성
- [ ] `output/<path>/top/` 아래 `Makefile` 및 `cmodel/ wrapper/ doc/` 등 산출물 존재 확인
- [ ] `output/<path>/top/meta.json` 존재 + `"top"` 키에 올바른 모듈명
- [ ] `cd output/<path>/top/ && make test-compile` → exit 0 (Top cmodel 컴파일 확인)
- [ ] 실패 Top 모듈은 `known-limitations.md`에 등록됨
