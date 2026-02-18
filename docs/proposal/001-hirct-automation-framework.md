# [Project Proposal] HIRCT: SoC 설계/검증 자동화 프레임워크

**작성자:** wonseok
**작성일:** 2026-02-15
**버전:** v6.0

---

## 1. 개요 (Executive Summary)

**프로젝트명:** HIRCT (HDL Intermediate Representation Compiler & Tools)

**한줄 소개:** SystemVerilog/Verilog RTL을 입력받아, 컴파일러 인프라(Slang, CIRCT)를 통해 생성된 IR을 기반으로 다종 자동화 산출물을 생성하는 LLVM 기반 통합 파이프라인.

**핵심 목표:**

1. **SSOT (Single Source of Truth):** RTL 코드가 유일한 진실 공급원. 모든 파생 산출물이 자동 생성되어 데이터 불일치를 원천 차단.
2. **Legacy IP Uplift:** 기존 Verilog IP를 수정 없이 최신 SystemVerilog/UVM 검증 환경으로 승격.
3. **Cycle-Accurate Simulation:** C++ 시뮬레이션 모델 자동 생성 + Verilator 직접 비교를 통한 정확성 입증.
4. **RTL 현대화:** IR 분석 기반 RTL 재포매팅으로 Legacy 코드 가독성 향상 (원본 무수정).

> **용어**: 이 문서에서 **SSOT**는 "Single Source of Truth"의 약어로, RTL이 모든 파생물의 유일한 원본이라는 원칙을 의미합니다.

---

## 2. 추진 배경 (Problem Statement)

### 2.1 기존 도구의 한계

- **Verilator:** 실행 속도는 빠르지만 컴파일 시간이 길고, 생성된 모델이 하드웨어 구조 정보를 상실하여 디버깅 및 SW 연동이 어렵다.
- **문서 불일치:** RTL 수정 시 레지스터 맵(CSR) 문서나 펌웨어 헤더 파일을 수동으로 업데이트해야 하므로 잦은 오류가 발생한다.
- **Legacy IP 검증의 한계:** 구형 Verilog IP는 인터페이스 구조가 단순(wire/reg)하여 최신 UVM 검증 방법론 적용 시 수작업 래핑이 필요하다.
- **반복 업무 과부하:** 테스트벤치 골격 생성, 문서화, 헤더 파일 작성 등 단순 반복 업무에 엔지니어 리소스가 과도하게 투입된다.

### 2.2 기술적 기회

- **LLVM/CIRCT:** 하드웨어 설계를 위한 MLIR 기술을 활용, 하드웨어 의미론을 유지하면서 소프트웨어 실행에 최적화된 변환이 가능.
- **Slang:** 완전한 SystemVerilog 2017 파서. CIRCT의 `circt-verilog` 프론트엔드로 통합.
- **Introspection:** 컴파일러가 회로의 모든 메타데이터(포트, 파라미터, 주소)를 파악하고 있으므로, 이를 다양한 산출물 생성에 활용할 수 있다.

---

## 3. 도구 구조 (Tool Architecture)

### 3.1 파이프라인

```
Verilog/SV (.v/.sv)
      │
      ▼
circt-verilog (Slang)        Frontend: 파싱 + Elaboration
      │
      ▼
CIRCT IR (.mlir)             Middle-end: hw/comb/seq dialect
      │
      ├── circt-opt           Passes: flatten, arcs, DCE
      │
      ▼
ModuleAnalyzer               lib/Analysis/ — 포트/연산/레지스터 추출 + Kahn 정렬
      │
      ▼
hirct-gen                    lib/Target/ — 8종 Emitter 산출물 생성
hirct-verify                 검증: 자동 드라이버 생성 + Verilator 비교
```

### 3.2 CLI 및 산출물

**CLI:** `hirct-gen` (8종 산출물 생성), `hirct-verify` (자동 등가성 검증)

**산출물 8종:** gen-model, gen-tb, gen-dpic, gen-wrapper, gen-format, gen-doc, gen-ral, gen-cocotb

> 상세 CLI 옵션(`--only`, `--top`, `-f`), 산출물별 파일/디렉토리, 테스트 기준은 [plans/summary.md](../plans/summary.md) 및 [plans/reference-commands-and-structure.md](../plans/reference-commands-and-structure.md) 참조.

### 3.3 hirct-verify

자동 검증 드라이버를 IR 포트 정보에서 생성하고, Verilator RTL과 C++ 모델을 매 사이클 직접 비교한다. 10개 시드 × 1000 사이클 = 총 10,000 사이클 비교가 기본 게이트 기준이다.

---

## 4. 출력 구조 (Output Structure)

### 4.1 핵심 원칙

- **원본 RTL 불변**: `rtl/` 디렉토리는 절대 수정하지 않는다.
- **출력 경로 규칙 (두 가지 모드)**:
  - **기본 CLI** (`hirct-gen input.v`): `output/<filename>/` — 입력 경로에 독립. 어디서든 동일한 출력 구조.
  - **순회 모드** (`make generate`, `config/generate.f` 기반): 소스 트리 미러링 적용 `rtl/<path>/<file>.v` → `output/<path>/<file>/`
- **파일명 기준**: 디렉토리명은 소스 파일명(확장자 제외). 모듈명이 아님.
- **per-module Makefile**: 각 모듈 디렉토리에 자동 생성된 Makefile로 `make test` 실행.
- **재귀 make test**: 어느 디렉토리에서든 `make test` → 그 하위 전체 테스트.

> 디렉토리 트리 상세 구조, Top 출력 디렉토리, 생성 규칙은 [plans/reference-commands-and-structure.md](../plans/reference-commands-and-structure.md) §1 참조.

---

## 5. 핵심 기술: Legacy IP Uplift (SV Wrapper)

### 5.1 개념

기존 `.v` 파일을 수정하지 않고, IR 파싱 단계에서 포트 패턴을 분석하여 SystemVerilog Interface가 적용된 Wrapper를 자동 생성한다.

### 5.2 동작 원리

```
[Legacy Verilog]                    [Auto-Generated SV Wrapper]

module old_ip (                     interface old_ip_plic_if;
  input  wire io_plic_ready,           logic ready;
  input  wire io_plic_valid,           logic valid;
  output wire io_plic_complete         logic complete;
);                                     modport master(...);
                                    endinterface
        ┌──────────────┐
        │ ModuleAnalyzer│            module old_ip_wrapper (
        │ 접두사 그룹핑 │──────▶       old_ip_plic_if.master plic,
        └──────────────┘               ...
                                    );
                                      old_ip u_core (
                                        .io_plic_ready(plic.ready),
                                        ...
                                      );
                                    endmodule
```

### 5.3 효과

- 수년 전 개발된 IP도 즉시 UVM VIP 및 최신 검증 환경에 통합 가능
- Human Error 0%: 포트 매핑이 자동 생성
- 원본 RTL 무수정: IP sign-off 상태 유지

---

## 6. 검증 전략 (Verification Strategy)

### 6.1 hirct-verify: 자동 검증

hirct-verify가 IR에서 포트를 읽어 검증 드라이버를 자동 생성하고, Verilator RTL과 C++ 모델을 매 사이클 직접 비교한다. 수작업 드라이버 불필요.

### 6.2 게이트 기준

10개 시드 × 1000 사이클 = 총 10,000 사이클 비교. Phase 2에서 전체 RTL(~1,600 파일)을 자동 순회하여 실패를 발견하고, Phase 1으로 되돌려 수정하는 피드백 루프를 실행한다.

### 6.3 실패 관리

- **XFAIL (Expected Failure)**: 알려진 미지원 모듈은 `known-limitations.md`에 등록하고, CI는 항상 Green 유지.
- **verify-decisions**: 의사결정 문서(`open-decisions.md`)의 RESOLVED 항목은 `test/`(lit) 및 `unittests/`(gtest)의 테스트 PASS로 반영을 증명한다. 별도 검증 스크립트는 사용하지 않는다.

> 검증 방법론 상세(직접 비교 패턴, 클럭/리셋 규약, 비트폭 처리, 산출물 테스트 규약)는 [plans/hirct-convention.md](../plans/hirct-convention.md) 참조.

---

## 7. 단계별 구축 계획 (Roadmap)

| Phase | 기간 | 핵심 목표 |
|-------|------|----------|
| **Phase 0** | 2일 | 환경 구성: 도구 설치 + 외부 도구 validation + 빌드 인프라 + 코딩 컨벤션 |
| **Phase 1** | 29일 | 파이프라인 기능 구현: Bootstrap + 8종 emitter + hirct-verify + CLI (모든 C++ 신규 작성) |
| **Phase 2** | 12일 | 전체 순회 테스트: ~1,600 파일 자동 검증 + 피드백 루프 |
| **Phase 3** | 5일 | 통합 및 배포: VCS co-sim + mkdocs + 패키징 |

> 상세 실행 계획(Phase별 태스크, 게이트, C++ 소스 매핑)은 [plans/summary.md](../plans/summary.md) 참조.

---

## 8. 기대 효과 (Impact)

### 8.1 검증(Verification) 팀

- **C++ Model:** RTL 완성 전 조기 검증 및 Virtual Platform 활용
- **SV Wrapper:** Legacy IP를 UVM 환경에 즉시 통합
- **UVM RAL:** RTL 변경 시 레지스터 모델 자동 갱신
- **Testbench:** 인터페이스 기반 테스트벤치 골격 자동 생성

### 8.2 소프트웨어(SW/FW) 팀

- **HAL Header + C Driver:** 레지스터 주소, 비트 필드, 접근 매크로, 드라이버 코드 자동 생성
- **HW Documentation:** 레지스터 맵, 초기화 시퀀스 등 최신 문서 실시간 제공

### 8.3 설계(Design) 팀

- **RTL Formatter:** IR 분석 기반 섹션 주석, 포트 그룹핑, 코드 스타일 일괄 적용
- **Single Source of Truth:** "설계 코드가 곧 문서이자 검증 환경" (Code is Everything)

---

## 9. 기술 스택

- **Frontend:** Slang (SystemVerilog Parser, circt-verilog 내장)
- **Middle-end:** LLVM/MLIR, CIRCT (hw/comb/seq/arc dialect)
- **Backend:** C++17, hirct-gen / hirct-verify (자체 개발)
- **Verification:** Verilator 5.020, VCS V-2023.12-SP2-7
- **Build:** CMake + Ninja (hirct-gen/hirct-verify), Make (오케스트레이션), lit (테스트)
- **Documentation:** Markdown, mkdocs

---

## 10. 의존성 관리 (Dependency Management)

### 10.1 시스템 의존성 (Phase 0에서 설치)

| 도구 | 최소 버전 | 용도 | 설치 방법 |
|------|----------|------|----------|
| CIRCT (circt-verilog, circt-opt) | LLVM 18+ | Frontend 파싱 + IR 최적화 | 사전 빌드 바이너리 또는 소스 빌드 |
| Verilator | 5.020 | RTL 기준 시뮬레이션 (hirct-verify) | 패키지 매니저 또는 소스 빌드 |
| CMake | 3.20+ | hirct-gen/hirct-verify 빌드 | 패키지 매니저 |
| Ninja | 1.10+ | 빌드 백엔드 | 패키지 매니저 |
| GCC/Clang | C++17 지원 | 컴파일러 | 시스템 기본 |
| Python | 3.10+ | 유틸리티 (lit, 리포트, triage) | 시스템 기본 |

### 10.2 Python 의존성

**필수** (`requirements.txt`):

```
black>=24.0          # Python 포매터
flake8>=7.0          # Python 린터
mypy>=1.8            # Python 타입 체커
lit>=18.0            # LLVM 테스트 러너
```

**선택** (`requirements-optional.txt`):

```
cocotb>=1.9          # Python 테스트벤치 (Phase 2+)
mkdocs>=1.6          # 문서 사이트 (Phase 3)
mkdocs-material>=9.5 # mkdocs 테마 (Phase 3)
```

설치: `pip install -r requirements.txt` (필수), `pip install -r requirements-optional.txt` (선택)

### 10.3 VCS (Phase 3 전용, 선택)

| 도구 | 버전 | 용도 |
|------|------|------|
| VCS | V-2023.12-SP2-7 | DPI-C co-simulation 3자 비교 |

> VCS는 Synopsys 상용 라이선스가 필요합니다. Phase 3 이전에는 불필요하며, Verilator만으로 검증이 가능합니다.

---

## 11. 라이선스 준수 (Compliance)

### 11.1 라이선스

이 프로젝트는 **Apache License v2.0 with LLVM Exceptions** 하에 배포됩니다 ([LICENSE](../../LICENSE)).

LLVM Exception은 Apache 2.0의 Section 4(a), 4(b), 4(d)를 면제하여, 컴파일된 바이너리 배포 시 라이선스 고지 요건을 완화합니다.

### 11.2 배포 규칙

- **소스 배포**: `LICENSE` 파일을 리포지토리 루트에 반드시 포함
- **바이너리 배포**: LLVM Exception에 의해 바이너리에 라이선스 텍스트 포함 의무 면제
- **NOTICE 파일**: 현재 해당 없음. 서드파티 코드 통합 시 NOTICE 파일 생성
- **생성 파일 헤더**: hirct-gen이 생성하는 산출물(.cpp, .sv, .md 등)에는 "Auto-generated by HIRCT" 주석을 포함하되, 라이선스 헤더는 포함하지 않음 (생성물은 사용자 프로젝트의 라이선스를 따름)

### 11.3 서드파티 의존성 호환성

| 의존성 | 라이선스 | Apache-2.0 호환 |
|--------|---------|-----------------|
| CIRCT/LLVM | Apache-2.0 with LLVM Exception | 동일 |
| Verilator | LGPL-3.0 (런타임), Artistic-2.0 (생성물) | 호환 (외부 프로세스 호출) |
| Slang | MIT | 호환 |
| lit | Apache-2.0 with LLVM Exception | 동일 |

---

## 12. 문서 참조 (SSOT)

이 proposal은 상위 제안서입니다. 구체적인 규약/정책/명령어/구조는 아래 계획서를 참조하세요:

| 문서 | 역할 |
|------|------|
| [plans/summary.md](../plans/summary.md) | 총괄 실행 계획 (Phase별 태스크, CLI, 산출물, 빌드 시스템) |
| [plans/hirct-convention.md](../plans/hirct-convention.md) | **Canonical 규약** (검증 방법론, 클럭/리셋, 비트폭, 테스트 규약) |
| [plans/open-decisions.md](../plans/open-decisions.md) | 의사결정 추적 (26건 RESOLVED + 반영 증거 검증 정책) |
| [plans/reference-commands-and-structure.md](../plans/reference-commands-and-structure.md) | 최종 구조/명령어 레퍼런스 (디렉토리 트리, Make 타겟, 스키마) |

---

## 변경 이력

| 버전 | 날짜 | 내용 |
|------|------|------|
| v1.0 | 2026-02-11 | HIRCT 최초 제안 |
| v2.0 | 2026-02-15 | SoC 자동화 프레임워크로 확장 |
| v3.0 | 2026-02-15 | Phase 재구성(기능 기준), CLI 확정(hirct-gen/hirct-verify), 출력 구조 확정(소스 트리 미러링), 산출물 8종 확정, gen-ral에 HAL+드라이버 통합, doc+guide 통합 |
| v4.0 | 2026-02-15 | 문서 슬림화: 상세 스펙은 plans로 이관, SSOT 용어 정의, XFAIL/verify-decisions 언급 추가, 문서 참조 섹션 추가 |
| v5.0 | 2026-02-16 | v2 구조 반영: CIRCT 스타일 디렉토리, lit 기반 테스트, utils/ 경로 |
| v6.0 | 2026-02-17 | §10 Dependency Management, §11 Compliance 섹션 추가, 기존 §10→§12로 재번호 (배포용 문서 보강) |