# HIRCT 제품 전략

> 작성일: 2026-03-08
> 목적: HIRCT 산출물 전략, 경쟁 도구 비교, 실행 로드맵을 정의한다.

---

## 1. 핵심 비전

칩 전체 RTL을 C/C++ 모델로 자동 변환하여 사내 제품 시뮬레이터에 연결, FW 선행 개발을 가능하게 한다.

### 1.1 현재 문제

사내에서 Verilog 모듈(v2p_tbl_stage1, v2p_tbl_stage2 등)을 수동으로 C 모델로 변환하고 있다.
수동 변환은 시간 소모, 오류 가능성, 유지보수 부담이 크며, RTL 변경 시마다 재작업이 필요하다.

### 1.2 업계 Pain Point

CHIPS Alliance의 Caliptra 프로젝트 sw-emulator가 대표적 사례다.
RTL 동작을 Rust로 수동 구현한 주변장치 모델로, 업계 전반에서 "RTL → C 모델" 수동 변환이 pain point로 존재한다.

### 1.3 HIRCT 차별점

- CIRCT/MLIR 기반 IR 분석으로 RTL에서 직접 모델 자동 생성
- 레지스터 스펙(IP-XACT/SystemRDL)이 없어도 RTL 자체가 유일한 입력
- 모듈 계층 보존으로 사내 시뮬레이터 구조에 자연스럽게 대응

---

## 2. 경쟁 도구 서베이

### 2.1 카테고리 A — RTL → 모델 자동 생성

| 도구 | 유형 | 출력 | 모듈계층 | 비고 |
|------|------|------|---------|------|
| ARM CycleModels (cbuild) | 상용 | SystemC | 보존 | 유일한 상용 RTL→모델. ARM 라이선스 필요 |
| Verilator | 오픈소스 | C++/SystemC | flatten | 업계 표준. 빠르고 성숙하지만 모듈 계층 flatten, 코드 난독화 |
| Yosys CXXRTL | 오픈소스 | C++ | 보존 | 모듈 계층 보존, 블랙박스 지원, 100% debug coverage |
| CIRCT arcilator | 오픈소스 | C++ (LLVM JIT) | flatten | CIRCT 내장. cf.br/cf.cond_br 잔존 시 실패 (범용 NO-GO) |
| ESSENT | 오픈소스 | C++ | flatten | UCSC, FIRRTL 기반 |
| NVIDIA GEM | 오픈소스 | CUDA (GPU) | flatten | Verilator 대비 5-64x 속도. DAC 2025 Best Paper 후보 |

### 2.2 카테고리 B — 스펙 → 모델 (RTL이 입력이 아님)

| 도구 | 입력 | 출력 |
|------|------|------|
| Synopsys Virtualizer | IP 벤더 TLM 라이브러리 | 가상 플랫폼 |
| Cadence VSP + nccodegen | IP-XACT / SystemRDL | SystemC TLM 템플릿 |
| Siemens Vista Model Builder | 포트/레지스터/메모리 선언 | SystemC TLM 스켈레톤 |
| Agnisys IDesignSpec | SystemRDL / IP-XACT / JSON | RTL + C API + UVM |
| DDGEN (Vayavya Labs) | DPS + RTS 스펙 | bare-metal 드라이버 |

### 2.3 카테고리 C — 학술

| 도구 | 입력 → 출력 | 비고 |
|------|-------------|------|
| v2c | Verilog → ANSI-C | 검증용 넷리스트, 가독성 없음 |
| RTL-C + DEEQ | HLS RTL → C | HLS 전용, ~300x 속도, SMT 등가성 |
| OmniSim | HLS 설계 → C sim | MICRO 2025, 35.9x 속도 |

### 2.4 핵심 결론

"RTL에서 사내 시뮬레이터용 경량 C/C++ 모델 자동 생성"을 하는 도구는 상용/오픈소스 모두 없다.
카테고리 A는 모듈 계층 flatten 또는 런타임 의존성 문제가 있고, 카테고리 B는 RTL이 아닌 별도 스펙을 요구한다.

---

## 3. GenModel vs Verilator 구체적 차이

사내 시뮬레이터 연동 관점에서의 비교:

| 항목 | Verilator | HIRCT GenModel |
|------|-----------|----------------|
| 런타임 의존 | libverilated 필수 | 없음 (cstdint만) |
| 모듈 계층 | flatten | 보존 |
| 이름 보존 | 변형/해시화 | 원본 그대로 |
| 서브모듈 단위 교체 | 어려움 | 자연스러움 |
| 증분 빌드 | 전체 재빌드 | 변경 모듈만 |
| 코드 가독성 | 불가 | 가능 |
| 사내 API 적응 | 래퍼 수동 작성 | 코드 생성 템플릿 수정 |
| 정확성 검증 | 20년 검증 | 자체 검증 필요 (GenVerify) |

Verilator는 "빠르고 정확한 블랙박스 시뮬레이터"이고, GenModel은 "사내 시뮬레이터에 맞출 수 있는 투명한 모듈 단위 모델"이다.

Verilator의 강점(속도, 정확성, 생태계)은 대체가 아닌 보완 대상이다.
GenModel은 Verilator가 제공하지 못하는 모듈 투명성과 시뮬레이터 통합 유연성에 집중한다.

---

## 4. CXXRTL 중간 단계 활용 전략

Yosys CXXRTL은 GenModel과 유사한 접근(모듈 계층 보존, standalone C++, 이름 보존)을 이미 구현하고 있다.
CXXRTL을 전면 경쟁이 아닌 단계적 활용 대상으로 설정한다.

### 4.1 단기 — CXXRTL PoC

- CXXRTL로 대상 모듈의 C++ 모델 생성 PoC 확보
- 사내 시뮬레이터 연동 래퍼 구현
- 현재 수동 변환을 즉시 자동화하여 ROI 확보
- CXXRTL의 API(step/get/set)를 사내 시뮬레이터 API에 매핑하는 thin wrapper 개발

### 4.2 중기 — HIRCT 고유 가치 구축

CXXRTL이 하지 못하는 영역에 집중:

- CIRCT/MLIR IR 분석 기반 SMOKE 테스트 자동 생성
- RTL에서 레지스터 맵 역추출 (AddressDecodingAnalysis)
- Programmer's Guide 자동 생성
- 이 단계에서 HIRCT는 "모델 생성기"가 아닌 "RTL 분석 플랫폼"으로 차별화

### 4.3 장기 — 자체 GenModel 전환 판단

- 사내 시뮬레이터 API 확정 후 CXXRTL 한계 평가
- SystemVerilog 지원 부족, 커스텀 최적화 불가 등 한계가 드러나면 자체 GenModel 전환
- 전환 판단 기준: CXXRTL wrapper 유지보수 비용 > 자체 구현 비용

---

## 5. 4축 산출물 전략

기존 emitter 8종을 4개 축으로 재구성한다.

### 축 1 — 레지스터 맵 + SMOKE 테스트 + Programmer's Guide + IP-XACT

**핵심 차별 영역**. RTL에서 자동으로 레지스터 주소, 필드, 접근 속성을 추출한다.

| 산출물 | 내용 |
|--------|------|
| 레지스터 맵 | 주소, 필드, RW/RO/WO, 리셋값 |
| SMOKE 테스트 | RW 확인, 리셋값 확인, RO 보호 확인 |
| Programmer's Guide | 레지스터 설명, 사용법, 제약사항 |
| IP-XACT | 표준 레지스터 스펙 포맷 |

기존 도구(Agnisys, DDGEN 등)는 전부 별도 스펙이 입력이다. RTL 자체가 입력인 것이 차별점.

해당 emitter: GenRAL, GenDoc, (신규 SMOKE 테스트 생성기)

### 축 2 — 테스트 인터페이스 (cocotb/UVM + DPI-C bridge)

포트/프로토콜(APB/AXI) 분석으로 cocotb, UVM 스캐폴드를 자동 생성한다.

- cocotb: Python 기반 테스트벤치 — 빠른 검증 루프
- UVM: 업계 표준 검증 방법론 — 정식 검증 환경
- DPI-C bridge: 축 3의 FuncModel을 RTL sim에 연결하는 인터페이스

해당 emitter: GenTB, GenCocotb, GenDPIC

### 축 3 — FuncModel (pure C 본체 + 환경별 래퍼)

pure C로 모델 본체(read/write + tick API)를 구현하고, 환경별 래퍼로 다양한 시뮬레이션 환경에 연결한다.

| 래퍼 | 대상 환경 |
|------|----------|
| SystemC TLM | 가상 플랫폼 |
| DPI-C | VCS / Questa RTL sim |
| QEMU | 에뮬레이터 |
| standalone | 단위 테스트, 사내 시뮬레이터 |

모델 본체가 pure C이면 어디에든 붙일 수 있다.

해당 emitter: GenFuncModel (재설계), GenDPIC

### 축 4 — Cycle-Accurate 모델 (유지, 최소 투자)

기존 GenModel을 유지하고 known-limitations 미해결 항목만 해소한다.

- GenModel: cycle-accurate C++ 모델
- GenVerify: RTL vs CModel 비교 검증 드라이버
- 사내 시뮬레이터 연결이 주 용도

해당 emitter: GenModel, GenVerify

---

## 6. 드라이버/SMOKE 테스트 자동 생성의 현실성

### 6.1 RTL에서 추출 가능한 정보

| 정보 | 추출 방법 | 자동 생성 가능? |
|------|----------|--------------|
| 레지스터 주소 오프셋 | 주소 디코딩 로직 분석 | O (AddressDecodingAnalysis) |
| 필드 비트 위치 | 비트 연산 분석 | O |
| 접근 속성 (RW/RO/WO) | 쓰기 로직 유무 | O |
| 리셋값 | seq.firreg 리셋값 | O |
| 인터럽트 소스 | 인터럽트 출력 → 마스크 경로 | O (부분적) |
| 메모리 구조 (depth/width) | seq.firmem | O |
| 프로토콜 (APB/AXI/AHB) | 포트 패턴 인식 | O |

### 6.2 RTL에서 추출 불가능한 정보

| 정보 | 이유 |
|------|------|
| 초기화 순서/시퀀스 | 설계 의도에 해당, 구조적 추론 불가 |
| baud rate 등 계산 공식 | 설계 지식 필요 |
| 에러 조건의 의미 | 의미론적 이해 필요 |
| 타임아웃/폴링 전략 | 사용 패턴에 의존 |

### 6.3 SMOKE 테스트 자동 생성 가능 항목

| 테스트 항목 | 방법 |
|------------|------|
| RW 레지스터 read/write 접근성 | 기대값 write → read-back 비교 |
| 리셋값 확인 | reset 후 기대값 비교 |
| RO 레지스터 쓰기 불가 확인 | write 후 값 불변 확인 |
| 필드 마스크 확인 | 비트 단위 write/read |
| 인터럽트 비활성화 확인 | 마스크 레지스터 설정 후 출력 확인 |
| 메모리 read/write 확인 | 주소 범위 내 접근 확인 |

### 6.4 판정

SMOKE 테스트(레지스터 접근성/리셋값/보호 확인)는 RTL에서 완전히 자동 생성 가능하다.
완전한 bare-metal 드라이버는 초기화 시퀀스, 계산 공식 등 추가 입력(YAML 어노테이션/IP-XACT)이 필요하며, 현재 목표가 아니다.

---

## 7. 논문 서베이 요약

### 7.1 First-Class Verification Dialects for MLIR

Fehr et al., PLDI 2025

MLIR pass의 의미 보존을 형식적으로 검증한다. 5개 dialect에 SMT 인코딩을 적용하여 upstream MLIR에서 5개 miscompilation 버그를 발견했다.
hw/comb/seq에 적용하면 HIRCT pass별 정확성 증명이 가능하다.

### 7.2 HEC: Equivalence Verification for Code Transformation

2025

MLIR 프론트엔드, e-graph 기반 equality saturation. 10만 줄 MLIR을 ~40분에 처리한다.
mlir-opt에서 실제 버그를 발견했으며, HIRCT 파이프라인 검증에 활용 가능하다.

### 7.3 CIRCT circt-lec

hw/comb/seq dialect LEC 도구. MLIR → SMT-LIB → Z3 경로로 동작한다.
기본 기능은 동작하나 대규모 설계는 미검증 상태다.
GenModel/GenFuncModel의 정확성 검증 도구로 활용 가능하다.

### 7.4 v2c: A Verilog to C Translator

Mukherjee et al., TACAS 2016

Verilog → ANSI-C 변환. cycle-accurate, bit-precise하지만 합성 시맨틱스 기반으로 가독성이 없다.
GenModel의 접근과 목표가 다르나 cycle-accurate 변환의 학술적 근거를 제공한다.

### 7.5 RTL-C + DEEQ

IIT Guwahati, 2022

HLS RTL에서 cycle-accurate C를 추출하여 ~300x 속도를 달성했다.
DEEQ로 SMT 기반 등가성 검증을 수행한다. HLS RTL 전용이라 범용성은 제한적이다.

### 7.6 GoldenFuzz

2025

C++ Golden Reference Model 기반 하드웨어 보안 fuzzing.
RISC-V에서 기존 취약점 전부 + 5개 신규 취약점을 발견했다.
빠른 C++ 모델이 대규모 fuzzing을 가능하게 함을 실증하며, GenModel의 활용 사례를 확장한다.

---

## 8. 미결 결정

### H-1: 사내 시뮬레이터 인터페이스 확정

**상태**: OPEN

**배경**: FuncModel/GenModel의 래퍼 설계는 사내 시뮬레이터의 API에 의존한다.
현재 pure C, SystemC TLM, 커스텀 인터페이스 중 어떤 형태인지 확정되지 않았다.

**선택안**:
- A) pure C API (read/write/tick) — 이식성 최대, 성능 오버헤드 최소
- B) SystemC TLM — 업계 표준, 가상 플랫폼 통합 용이
- C) 커스텀 인터페이스 — 사내 시뮬레이터 전용 최적화

### H-2: CXXRTL PoC 범위 및 일정

**상태**: OPEN

**배경**: Section 4의 단기 전략(CXXRTL PoC)을 실행하려면 대상 모듈과 일정을 확정해야 한다.

**선택안**:
- A) 소규모 모듈 1개 (v2p_tbl_stage1) — 2주 내 완료 가능
- B) 중규모 IP 1개 (주소 디코더 포함) — 4주, 축 1 검증도 병행
- C) Phase 2 대상 전체 — 8주+, 전면 전환

### H-3: GenFuncModel의 SystemC TLM 전환 여부 및 시기

**상태**: OPEN

**배경**: 축 3 FuncModel은 pure C 본체를 기본으로 하되, 가상 플랫폼 연동 시 SystemC TLM 래퍼가 필요하다.
TLM 래퍼 개발은 SystemC 의존성을 도입하며, 사내에서 SystemC 사용 여부가 불확실하다.

**선택안**:
- A) pure C 본체만 우선, TLM은 수요 확인 후 — 최소 의존성
- B) pure C + TLM 래퍼 동시 개발 — 가상 플랫폼 즉시 지원
- C) TLM 우선 — 가상 플랫폼이 주 타겟인 경우

### H-4: GenModel IR 스펙 작업 재개 시점

**상태**: OPEN

**배경**: `2026-03-06-genmodel-ir-spec-brainstorm.md`에서 IR 스펙 논의가 시작되었으나, 제품 전략 확정 전에 IR 스펙을 확정하면 방향이 엇나갈 수 있다.

**선택안**:
- A) 본 전략 문서 확정 후 즉시 재개 — 전략 방향 반영 가능
- B) CXXRTL PoC 결과 확인 후 재개 — PoC에서 얻은 교훈 반영
- C) 축 1(레지스터 맵/SMOKE) 구현 후 재개 — 핵심 차별 영역 우선
