# HIRCT 미합의 사항 목록 (Open Decisions)

> **작성일**: 2026-02-15
> **상태**: 전체 합의 완료
> **총 항목**: 26건 (RESOLVED 26)

---

## 범례

| 상태 | 의미 |
|------|------|
| OPEN | 합의 필요 |
| PARTIAL | 합의 완료되었으나 반영 증거 미확인 (검증 정책 참조) |
| RESOLVED | 합의 완료 + 반영 증거 확인 (날짜·결정 내용·검증 grep 기록) |

---

## A. 아키텍처 / 설계 결정 (8건)

### A-1. 65비트 초과 신호(폭 65+) 처리 방식 — RESOLVED (2026-02-15)

**결정**: Phase 1에서는 **Error 처리** (미지원 에러 + 스킵), Phase 2에서 **Verilator 호환 `uint32_t[]` 래퍼** 도입.

**Phase 2 구현 방식**: Verilator의 `VlWide<N>` (`uint32_t` 배열) 방식과 호환되는 경량 래퍼.
- hirct-verify가 Verilator 모델과 직접 비교하므로, 타입을 일치시켜 변환 비용 제거
- `std::array<uint64_t, N>`은 Verilator와 타입 불일치로 비교 시 변환 코드 필요 → 비효율

**근거**:
- `__int128`은 비표준(GCC/Clang 종속)이므로 이식성을 해침
- Verilator는 넓은 비트를 `WData` (`uint32_t[]`)로 처리 — 이와 호환되는 타입이 비교 드라이버를 단순화
- Phase 1은 "돌아가는 골격"에 집중, 65+ 비트는 Phase 2에서 실제 발견 시 대응

**반영 대상**: `docs/plans/hirct-convention.md` §3.1 "타입 매핑" 65+ 행
**검증 grep**: `grep -n "65.*Error\|uint32_t\[\]" docs/plans/hirct-convention.md`

---

### A-2. GenRAL 레지스터 존재 판정 기준 — RESOLVED (2026-02-15)

**결정**: 우선순위 명시: **1. 어노테이션(절대적) > 2. IR 패턴 > 3. 포트 이름(휴리스틱)**.
- 휴리스틱 판정 시 verbose 로그를 남겨 사용자가 인지하도록 함
- false positive 방지를 위해 휴리스틱 판정은 경고 레벨로 리포트

**근거**:
- 휴리스틱은 false positive 위험이 크므로 최하위 우선순위
- 최신 트렌드는 소스 코드에 `(* ... *)` Attribute를 남기는 것을 선호
- 어노테이션이 있으면 나머지 무시, 패턴이 매치되면 이름 검사 불필요

**반영 대상**: `docs/plans/phase-1-pipeline/107-gen-ral.md` L112–L123

---

### A-3. GenModel의 hw.instance 처리 스코프 — RESOLVED (2026-02-15)

**결정**: **CIRCT flatten pass 먼저 시도 → 성공 시 flat IR로 GenModel → 실패 시 Error + 진단 메시지**.

```
입력 IR → circt-opt --hw-flatten-modules → 성공? → GenModel (flat IR)
                                  실패? → Error("hw.instance를 flatten할 수 없음: ...")
```

> **Note (실측)**: CIRCT `5e760efa9` 기준 `circt-opt --flatten` 옵션은 존재하지 않는다.
> 계층 인라인(flatten)은 `circt-opt --hw-flatten-modules` pass로 수행한다.
> (필요 시 `--hw-flatten-modules=hw-inline-public` 등 pass 옵션 사용)

- 계층 구조 완전 지원은 Phase 2 이후로 연기
- "Flatten 또는 Error"가 아닌 **순서가 있는 폴백** — 구현자에게 명확한 가이드

**근거**:
- CIRCT에 이미 flatten pass가 존재하므로, 별도 구현 없이 활용 가능
- 계층 구조 지원은 복잡도가 기하급수적으로 증가
- Phase 1 목표는 "파이프라인 관통(Walking Skeleton)"

**반영 대상**: `docs/plans/phase-1-pipeline/101-gen-model.md` L14, L116

---

### A-4. hirct-verify의 구현 형태 — RESOLVED (2026-02-15, 경정 2026-02-15)

**결정**: **역할별 분리 (Makefile + lit 방식)**. bash 스크립트 파일(.sh) 사용 금지.

| 역할 | 도구 | 이유 |
|------|------|------|
| 비교 드라이버 소스 생성 | hirct-gen C++ (`lib/Target/GenVerify.cpp`) | IR 포트 정보가 필요 |
| per-module 빌드+실행 | per-module Makefile (`make test-verify`) | GenMakefile.cpp이 자동 생성 |
| 전체 순회/병렬/리포트 | lit (`integration_test/`) + `utils/generate-report.py` | lit이 병렬/timeout/격리 제공 |
| JSON 리포트 생성 | `utils/generate-report.py` | lit xunit XML + meta.json → JSON |
| 메인 진입점 | `make test-all` | CI와 로컬 동일 |

**근거**:
- 순수 Make로 순회/병렬/리포트/필터링까지 하면 유지보수 난도 급상승
- bash 스크립트 파일은 정책상 금지 — Makefile recipe 내 inline shell만 허용
- lit이 테스트 병렬/타임아웃/격리를 기본 제공 — CIRCT/LLVM 프로젝트 표준
- `make test-all`이 메인 진입점이 되어 CI 시스템에 무관하게 동작

**구현 위치**: `lib/Target/GenVerify.cpp` (드라이버 소스 생성), `tools/hirct-verify/main.cpp` (CLI 바이너리), per-module `Makefile` (빌드+실행), lit (`integration_test/`) (전체 오케스트레이션)

**반영 대상**: `docs/plans/phase-1-pipeline/109-verify.md` 헤더 "파일" 행
**검증 grep**: `grep -n "per-module\|GenVerify\|lit" docs/plans/phase-1-pipeline/109-verify.md`

---

### A-5. Top 모듈 판정 방법 — RESOLVED (2026-02-15)

**결정**: **`-f` 사용 시 `--top` 필수, 단일 파일은 조건부 자동**.

| 모드 | 조건 | `--top` |
|------|------|---------|
| `hirct-gen input.v` | 파일 내 모듈 1개 | 자동 지정 (불필요) |
| `hirct-gen input.v` | 파일 내 모듈 2개+ | **필수** (미지정 시 Error) |
| `hirct-gen -f filelist.f` | 항상 | **필수** (미지정 시 Error) |

**근거**:
- 단일 파일 + 단일 모듈은 자명 — 불필요한 CLI 마찰 제거
- 다중 모듈/파일에서는 파일리스트 순서를 신뢰할 수 없으므로 명시 필수
- 자동 감지("인스턴스되지 않은 모듈")는 복수 Top 시 실패

**CLI 예시**: `hirct-gen -f filelist.f --top CoreIPSubsystem`

**반영 대상**: `docs/plans/phase-1-pipeline/110-output-structure.md` L79–L97, `docs/plans/phase-1-pipeline/111-cli.md`

---

### A-6. filelist.f 형식 — RESOLVED (2026-02-15)

**결정**: **Synopsys/Cadence 표준 호환**.

**지원 문법**:
- `//` 주석
- `+incdir+<path>` include 경로
- 한 줄에 파일 경로 하나
- 와일드카드 미지원 (명시적 경로만)

**근거**:
- EDA 생태계 표준을 따라야 기존 워크플로우와 호환
- 가벼운 정규식 기반 파서로 충분

**반영 대상**: `docs/plans/phase-1-pipeline/110-output-structure.md` L85, `docs/plans/phase-2-testing/202-top-traversal.md` L17

---

### A-7. 계층 JSON 산출물 정책 — RESOLVED (2026-02-15)

**결정**: 별도의 **계층 JSON 파일은 생성하지 않는다**.

- Top 모드(`hirct-gen -f ... --top ...`)는 `output/<path>/top/` 아래에 **Top 산출물만 생성**한다.
- 모듈 계층 트리가 필요하면, 별도 JSON 파일이 아니라 **문서 산출물(`doc/*.md`) 내부에 텍스트 트리로만 포함**한다(인스턴스가 존재하는 경우에 한함).

**근거**:
- 추가 JSON 산출물은 SSOT/유지보수 포인트를 늘리고, 문서/테스트/툴의 의존성을 불필요하게 복잡하게 만든다.
- 계층 정보는 “사람이 읽는 문서” 용도로만 필요하며, 기계 판정은 `report.json`/`verify-report.json`으로 충분하다.

**반영 대상**:
- `docs/plans/phase-1-pipeline/110-output-structure.md` (Top 단계에서 계층 JSON 파일 생성 언급 제거)
- `docs/plans/phase-2-testing/202-top-traversal.md` (게이트에서 계층 JSON 파일 검사 제거)
- `docs/plans/reference-commands-and-structure.md` (전역 산출물에서 계층 JSON 파일 제거)

---

### A-8. Combinational loop 처리 레벨 — RESOLVED (2026-02-16)

**결정**: **ERROR** (hard fail). WARN이 아니라 ERROR + meta.json emitter `"fail"`.

- 감지 시: `ERROR: combinational loop detected: %a -> %b -> ... -> %a`
- meta.json: `"combinational_loop": true`, 해당 emitter `"fail"`
- 모듈은 `known-limitations.md`에 `combinational_loop` 카테고리로 등록 가능

**근거**: WARN + 계속 진행은 미정렬 op이 잘못된 순서로 emit되어 "컴파일은 되지만 시뮬레이션 불일치"를 허용한다. 이는 verify 단계에서야 실패가 발견되어 원인 추적이 어렵다. ERROR로 즉시 표면화하여 `hirct-convention.md` §5 실패 분류 체계의 `fail` 정의와 일관성을 유지한다.

**반영 대상**: `hirct-convention.md` §2.10.1, `100-bootstrap.md` Step 4 Kahn's Algorithm
**검증 grep**: `grep -n "combinational.*loop.*ERROR\|combinational_loop.*true" docs/plans/hirct-convention.md docs/plans/phase-1-pipeline/100-bootstrap.md`

---

## B. 네이밍 / 구조 정리 (6건)

### B-1. EmitCHeader.cpp (55줄)의 행방 — RESOLVED (2026-02-15)

**결정**: **삭제 (Deprecate)**.
- GenModel이 이미 `.h` 헤더를 생성하며, 포트 정보·타입 선언·API(step/reset)를 모두 포함
- EmitCHeader의 출력은 GenModel `.h`의 **부분집합** — 기능이 완전히 중복
- GenRAL에 통합하면 비-레지스터 모듈에서 C 헤더가 사라지는 문제 발생 → 통합도 부적절

**반영 대상**: `docs/plans/summary.md` §7 "C++ 소스 매핑" 삭제 예정 행, `docs/plans/phase-1-pipeline/README.md` §C++ 소스 매핑 테이블
**검증 grep**: `grep -n "EmitCHeader\|삭제" docs/plans/summary.md docs/plans/phase-1-pipeline/README.md`

---

### B-2. EmitProgGuide.cpp 통합 절차 — RESOLVED (2026-02-15)

**결정**: **GenDoc.cpp로 통합** (단일 문서 내 섹션 분리).
- 출력 파일: `doc/<module>.md` 하나에 `## Hardware Spec` + `## Programmer's Guide` 섹션
- 사용자는 하나의 잘 정리된 문서를 선호

**반영 대상**: `docs/plans/phase-1-pipeline/README.md` L42, `docs/plans/phase-1-pipeline/106-gen-doc.md`

---

### B-3. hirct-convention.md와 rules.md 역할 분리 / 불일치 — RESOLVED (2026-02-15)

**결정**: **SSOT 원칙 적용 — 하나로 통합**.
- `hirct-convention.md`를 canonical로 유지, `rules.md`는 삭제 (또는 hirct-convention.md로 리다이렉트)
- `g++` vs `gcc` 불일치도 통합 시 해소

**근거**:
- 중복 문서는 불일치의 원천
- GitHub 표준은 `CONTRIBUTING.md` 또는 단일 `coding_standard.md`

**반영 대상**: `docs/plans/hirct-convention.md`, `docs/plans/rules.md` (전체)

---

### B-4. 리네이밍 시 클래스/함수명 변경 범위 — RESOLVED (2026-02-15)

**결정**: **클래스/함수명도 파일명과 일치시킴**.
- `EmitCppModel` 클래스 → `GenModel` 클래스
- IDE 리팩토링 기능 활용, 기존 참조 전부 업데이트
- 하위 호환성(alias)은 두지 않음 — 깔끔한 전환

**근거**:
- 파일명과 클래스명이 다르면 IDE 탐색이 어려움
- 기술 부채 방지를 위해 일치시키는 것이 정석

**반영 대상**: `docs/plans/phase-1-pipeline/README.md` L35–L47

---

### B-5. hirct-verify 드라이버 파일/타겟 네이밍 규칙 — RESOLVED (2026-02-15)

**결정**: **`verify_<module>.cpp`** (모듈명 기준, 기존 관례 유지).
- 예: `verify_Fadu_K2_S5_LevelGateway.cpp`
- Top: `verify_<TopModuleName>.cpp`
- 기존 코드(`verify_levelgateway.cpp`, `verify_rvcexpander.cpp`)와 일관성 유지
- SV 테스트벤치는 `tb/` 디렉토리, C++ 비교 드라이버는 `verify/` 디렉토리로 폴더 격리

**`tb_` 접두사를 쓰지 않는 이유**: `tb/` 디렉토리의 SV 테스트벤치(GenTB 산출물)와 역할 혼동

**반영 대상**: `docs/plans/phase-1-pipeline/109-verify.md` L41, L46

---

### B-6. GenMakefile.cpp 태스크 문서 — RESOLVED (2026-02-15)

**결정**: **별도 태스크 문서 불필요**, C++ 코드 내장(embedded string) 방식.
- 외부 `Makefile.template` 파일을 두지 않고, GenMakefile.cpp 안에 템플릿 문자열 내장
- 다른 emitter(GenModel, GenTB 등)도 전부 코드 안에서 문자열로 출력 — 동일 패턴
- 바이너리 하나에 모든 것이 포함되어 배포 시 템플릿 파일 경로 문제 없음
- 110-output-structure.md에 상세 추가로 충분

**반영 대상**: `docs/plans/summary.md` L151, `docs/plans/phase-1-pipeline/110-output-structure.md`

---

## C. 도구 / Lint 정책 (3건)

### C-1. 생성 코드(output/) lint 엄격도 — RESOLVED (2026-02-15)

**결정**: **Phase별 단계 전환** + **AUTO-GENERATED 헤더 필수**.

- **Phase 1**: Policy B — `output/**`를 `make lint`에서 제외. emitter 개발 생산성 우선.
- **Phase 2 후반**: Policy A로 전환 — `output/**`를 lint에 포함. 실패 시 생성기(hirct-gen) 쪽을 수정해서 통과시킨다.
- **전환 후 반복 저해 시**: `output/**`에 한해 완화 규칙 도입(허용 예외 목록 고정).

> **경정 사유 (2026-02-17)**: 원래 "A 먼저, 필요 시 B 폴백"이었으나, Phase 1이 전체 C++을 신규 작성하는 greenfield 단계여서 생성 코드 lint 패턴이 확립되지 않은 상태에서 Policy A는 전환 조건을 거의 확실히 충족한다. `docs/plans/phase-0-setup/003-coding-convention.md`의 실행 계획과 일치시켰다.

모든 생성 파일 첫 줄에 표준 헤더 삽입:
```
// AUTO-GENERATED by hirct-gen — DO NOT EDIT
```
(Python은 `# AUTO-GENERATED by hirct-gen — DO NOT EDIT`, SV는 `// AUTO-GENERATED by hirct-gen — DO NOT EDIT`)

**효과**:
- 사람이 실수로 수동 편집하는 것을 방지
- `output/**`는 수동 수정 금지, lint 실패 시 생성기(hirct-gen) 수정으로만 해결하도록 강제
- `git diff`에서 생성 코드 변경을 빠르게 식별

**테스트 기준**:
- Phase 1: Policy B — `make lint`는 `src/**` + `tools/**`만 대상 (생성물 제외)
- Phase 2 후반: Policy A — `make lint`에서 `output/**`까지 포함해 lint가 통과해야 함
- 생성 파일은 `AUTO-GENERATED` 헤더로 수동 편집을 방지하고, 스타일/포맷 문제는 기본적으로 생성기에서 해결

**반영 대상**: `docs/plans/phase-0-setup/003-coding-convention.md` L19

---

### C-2. Verilog/UVM lint·컴파일 게이트의 표준 도구 — RESOLVED (2026-02-15)

**결정**: **이중 게이트 — VCS/ncsim은 1차 게이트, Verilator/Verible은 기본 게이트**.

VCS/ncsim 라이선스가 확보되어 있으므로:

| 게이트 | 도구 | 역할 |
|--------|------|------|
| 기본 게이트 (필수) | Verilator `--lint-only`, Verible | 라이선스 무관, CI에서 항상 실행 |
| 1차 게이트 (필수) | VCS, ncsim | 라이선스 서버 접근 가능 시 실행, SV/UVM 완전 검증 |
| 판정 기준 | VCS 결과 우선 | VCS 통과 = 확정 PASS, VCS 없으면 Verilator만으로 판정 |

**UVM RAL 게이트**: `vcs -sverilog +incdir+$UVM_HOME/src` 를 **표준 게이트**로 사용 (UVM은 VCS에서만 완전 검증 가능).

**근거**:
- VCS/ncsim 라이선스가 있으므로 적극 활용
- Verilator/Verible은 CI 환경이나 라이선스 서버 접근 불가 시 fallback

**반영 대상**: `docs/plans/phase-0-setup/003-coding-convention.md` L17, `docs/plans/phase-1-pipeline/107-gen-ral.md` L80–L93

---

### C-3. cocotb 환경 필수 여부 — RESOLVED (2026-02-15)

**결정**: **선택적 의존성 (Optional)**.
- `setup-env.sh`에 포함하되, 실패해도 경고만 출력하고 계속 진행
- 환경 변수(`COCOTB_ENABLED=1`)로 활성화 여부 결정
- Phase 1 GenCocotb 게이트: `python3 -m py_compile`만으로 충분

**근거**:
- Python 환경 설정이 꼬이기 쉬우므로 필수 강제는 진입 장벽
- cocotb 실제 실행은 Phase 2 이후에서 선택적으로

**반영 대상**: `docs/plans/phase-1-pipeline/108-gen-cocotb.md` L32

---

## D. CLI / 빌드 시스템 (4건)

### D-1. CLI `--only` 옵션 — RESOLVED (2026-02-15)

**결정**: **Phase 1에서 필수 구현**.

**구문**: `hirct-gen input.v --only model,tb,doc`
- 쉼표 구분 필터
- 잘못된 필터명 → Error + 유효 목록 출력
- 지원 필터: `model`, `tb`, `dpic`, `wrapper`, `format`, `doc`, `ral`, `cocotb`

**근거**:
- 대형 프로젝트 디버깅 시 전체 생성을 기다리는 것은 비생산적
- 개발 생산성을 위해 반드시 필요

**반영 대상**: `docs/plans/phase-1-pipeline/111-cli.md` L26

---

### D-2. `make generate` 대상 범위 — RESOLVED (2026-02-15)

**결정**: **명시적 Config (Filelist 기반)**.
- `make generate` → 설정 파일(예: `config/generate.f` 또는 Makefile 변수)에서 대상 읽기
- `rtl/**/*.v` 와일드카드는 사용하지 않음 (백업/임시 파일 포함 위험)

**반영 대상**: `docs/plans/phase-1-pipeline/111-cli.md` L59

---

### D-3. 환경 설치 스크립트(setup-env.sh) 구체 방침 — RESOLVED (2026-02-15)

**결정**: **패키지 매니저 우선 + 멱등성(Idempotency)**.

| 도구 | 설치 방식 |
|------|---------|
| Verilator | 시스템 패키지 (`apt`) 또는 소스 빌드 |
| CIRCT/LLVM | 사전 빌드 바이너리 또는 기존 로컬 빌드 경로 검증 |
| VCS/ncsim | 경로 검증만 (`vcs -ID` 출력 확인) |
| clang-format, shellcheck | 시스템 패키지 |
| cocotb | pip install (선택, 실패 시 경고만) |

**원칙**:
- 멱등성: 여러 번 실행해도 안전 (`if not exist then install`)
- 실패 처리: 필수 도구 실패 → Error 중단, 선택 도구 실패 → 경고 후 계속

**반영 대상**: `docs/plans/phase-0-setup/001-setup-env.md` L11

---

### D-4. make test-all / CI 구체 구성 — RESOLVED (2026-02-15)

**결정**: **Makefile 기반 추상화 + lit 테스트 오케스트레이션 + CI는 호스팅 플랫폼에 따라 결정**.

**make 타겟 구조** (`make test-all`이 메인 진입점):
```
make test-all
  ├── make check-hirct          → lit test/ (emitter FileCheck)
  ├── make check-hirct-unit     → gtest (C++ API)
  ├── make check-hirct-integration → lit integration_test/smoke/
  └── make report               → utils/generate-report.py

make test-traversal            → lit integration_test/traversal/ (CI 제외, nightly/수동)
```

**CI 구성**:
- **Makefile 기반**: `make test-all` 단일 명령으로 CI 시스템에 무관하게 동작
- CI 시스템 선택은 실제 호스팅 플랫폼(GitHub → Actions, GitLab → CI, 사내 → Jenkins)에 따라 결정
- VCS/ncsim 게이트: 라이선스 서버 접근 가능 환경(self-hosted runner 등)에서 실행
- Push 및 PR 이벤트에 트리거

**반영 대상**: `docs/plans/phase-2-testing/205-test-automation.md` L12

---

## E. 검증 기준 (4건)

### E-1. VCS lock-step 사이클 수 — RESOLVED (2026-02-15)

**결정**: **1000 사이클로 통일** (Verilator 기준과 동일).
- 기존 100cyc은 오기로 판단
- VCS도 10시드 × 1000cyc 기준 적용

**반영 대상**: `docs/plans/phase-3-release/301-vcs-cosimulation.md` L6

---

### E-2. VCS 4-state에서 X 비율 허용 한계 — RESOLVED (2026-02-15)

**결정**: **Warning Log + 초기 N 사이클 무시 + 이후 X 발생 시 Fail**.

**규칙**:
1. 리셋 구간(초기 N 사이클)의 X는 허용 (N은 모듈별 설정, 기본값 = 리셋 사이클 수)
2. 리셋 완료 후 X 발생 → **즉시 Fail**
3. 모든 X 발생을 Warning 로그에 기록 (사이클, 포트, 값)

**근거**:
- 4-state(VCS)와 2-state(C++) 비교 시 초기화 구간의 X는 필연적
- 리셋 후 X는 설계 또는 모델 문제이므로 Fail 처리가 적절

**반영 대상**: `docs/plans/hirct-convention.md` §2.5 "2-state vs 4-state 규칙" VCS 4-state 처리 항목
**검증 grep**: `grep -n "리셋.*X.*허용\|즉시 Fail\|Warning 로그" docs/plans/hirct-convention.md`

---

### E-3. Phase 2 PASS 비율 게이트 — RESOLVED (2026-02-15)

**결정**: **100% (Known Fail 제외)**.
- "80%"는 삭제, **XFAIL (Expected Failure) 리스트**로 관리
- CI는 항상 Green(Pass) 상태 유지
- XFAIL 모듈은 `known-limitations.md`에 사유와 함께 문서화
- Phase 3 진입 조건: XFAIL 제외 전체 PASS

**근거**:
- "80% 합격"은 위험 — 20%의 실패가 중요한 회귀(Regression)일 수 있음
- 실패 테스트를 XFAIL로 명시 관리하면 새로운 회귀를 즉시 감지 가능

**반영 대상**: `docs/plans/phase-2-testing/203-auto-verification.md` L35

---

### E-4. mkdocs 모듈 인덱스의 기준 데이터 — RESOLVED (2026-02-15)

**결정**: 모듈 인덱스는 `output/report.json`(필수)과 `output/` 디렉터리 구조(보조)를 표준 소스로 사용한다.

- `report.json`은 “무엇을 처리했고(pass/fail/skip) 어디에 있는지(path)”의 SSOT이다.
- 계층 트리(JSON)는 생성하지 않는다. 계층 정보가 필요하면 `doc/*.md` 내부 텍스트 트리로만 제공한다.

**반영 대상**: `docs/plans/phase-3-release/302-documentation.md` (계층 JSON 파일 기반 문구 제거)

---

## F. 기타 (1건)

### F-1. hirct-convention.md vs rules.md 동기화 — RESOLVED (2026-02-15)

**결정**: B-3에 따라 **파일 통합으로 해결**. 원본이 하나면 동기화 문제는 소멸.

---

## 요약

| 카테고리 | 건수 | 상태 |
|---------|------|------|
| A. 아키텍처 / 설계 결정 | 8 | 전체 RESOLVED |
| B. 네이밍 / 구조 정리 | 6 | 전체 RESOLVED |
| C. 도구 / Lint 정책 | 3 | 전체 RESOLVED |
| D. CLI / 빌드 시스템 | 4 | 전체 RESOLVED |
| E. 검증 기준 | 4 | 전체 RESOLVED |
| F. 기타 | 1 | 전체 RESOLVED |
| **합계** | **26** | **전체 RESOLVED** |

---

## 핵심 원칙 (합의 요약)

1. **Phase 1은 Walking Skeleton**: 완벽한 기능(65비트, 계층 처리)보다 **전체 파이프라인(RTL → C++ → 실행)** 관통에 집중
2. **이중 게이트**: VCS/ncsim(라이선스 보유)을 1차 게이트, Verilator/Verible을 기본 게이트로 병행
3. **SSOT**: 중복 문서(hirct-convention.md + rules.md)는 반드시 하나로 통합
4. **XFAIL 관리**: CI는 항상 Green, 실패 모듈은 XFAIL 리스트로 명시 관리
5. **명시적 설정**: 와일드카드 대신 filelist, `--top` 조건부 필수, `--only` 필터 지원
6. **Verilator 호환**: 생성 코드의 데이터 타입(`uint32_t[]`)을 Verilator와 일치시켜 비교 드라이버 단순화
7. **역할별 도구 분리**: C++ = IR 분석/코드 생성, Makefile = 진입점/정책, lit = 테스트 오케스트레이션, Python = 리포트 변환 (bash 스크립트 파일 금지)
8. **AUTO-GENERATED 헤더**: 모든 생성 파일 첫 줄에 표준 헤더로 수동 편집 방지

---

## RESOLVED 검증 정책

> **원칙**: RESOLVED 표기는 "합의 완료"가 아니라 **"반영 증거가 확인된 상태"**를 의미한다.

### 검증 조건 (전부 충족 시에만 RESOLVED 유지)

1. **결정 문서화**: 이 문서에 결정 내용, 날짜, 근거가 기록됨
2. **반영 증거**: "반영 대상" 필드의 파일에 결정 내용이 실제 존재 (앵커 + grep 패턴으로 검증)
3. **게이트 존재**: 해당 결정을 검증하는 make 타겟 또는 게이트 체크리스트가 계획 문서에 존재
4. **기능 결정은 `test/` 또는 `unittests/`의 테스트가 PASS로 증명**, 문서/네이밍 결정은 PR 리뷰 + lint

하나라도 미충족 시: RESOLVED 유지 금지 → OPEN 또는 PARTIAL로 변경.

### 반영 대상 참조 형식 (표준)

```markdown
<!-- 금지: 라인 번호 하드코딩 (문서 수정 시 즉시 깨짐) -->
**반영 대상**: `hirct-convention.md` L187

<!-- 권장: 섹션 앵커 + grep 검증 패턴 -->
**반영 대상**: `hirct-convention.md` §3.1 "타입 매핑" 65+ 행
**검증 grep**: `grep -n "65.*Error\|uint32_t\[\]" docs/plans/hirct-convention.md`
```

### 자동 검증

- 기능 결정: `test/` (lit/FileCheck) 또는 `unittests/` (gtest)에서 해당 동작을 직접 테스트
- 문서/네이밍 결정: PR 리뷰 + clang-tidy/lint
- Make 타겟: `make test-all`의 테스트 PASS가 결정 반영의 증거
- CI 게이트: `make test-all` → 테스트 실패 시 CI 실패

---

## 변경 이력

| 날짜 | 내용 |
|------|------|
| 2026-02-15 | 초안 작성 (25건 OPEN) |
| 2026-02-15 | 전체 합의 완료 (25건 RESOLVED), VCS/ncsim 이중 게이트 반영 |
| 2026-02-15 | 피드백 반영: A-1(Verilator 호환), A-3(Flatten 폴백), A-4(혼합 방식), A-5(조건부 --top), B-1(삭제), B-5(verify_ 유지), B-6(C++ 내장), C-1(AUTO-GENERATED 헤더), D-4(Makefile 추상화) |
| 2026-02-15 | 경정: A-4(Shell→Makefile+Python runner), D-4(.sh→Python runner). MISMATCH 수정: A-1, B-1, E-2 반영 대상을 앵커 기반으로 전환. RESOLVED 검증 정책 섹션 추가. |
| 2026-02-16 | v2: CIRCT 스타일 디렉토리 구조 반영. A-4(Python runner→lit, GenVerify→lib/Target/), D-4(make 타겟을 check-hirct/check-hirct-unit/check-hirct-integration으로 재구성, verify-decisions 제거), RESOLVED 검증 정책(verify-decisions.py→test/+unittests/ 직접 테스트), 전역 경로 정리(scripts/→utils/ 또는 lit). |
