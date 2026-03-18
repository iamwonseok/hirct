# HIRCT Parent Session 전수 분석 보고서

> **작성일**: 2026-03-12
> **분석 대상**: parent transcript 전수 조사 (`subagents` 제외)
> **범위**: 2026-02 ~ 2026-03 HIRCT 프로젝트 세션, 관련 계획/회고/리포트 문서
> **목적**: 모든 parent 세션의 대화 흐름을 바탕으로, 프로젝트의 목표/목적, 주요 시도, 현재 상태, 회고를 재구성한다.

---

## 1. Executive Summary

이번 전수 조사는 `agent-transcripts`의 **parent session 251개**를 기준으로 수행했다. 이 중 **2026-02 세션이 162개**, **2026-03 세션이 89개**였고, 실제 프로젝트의 큰 흐름은 다음 다섯 단계로 압축된다.

1. **자동 생성 파이프라인 구축**
   - HIRCT를 `RTL -> CIRCT/MLIR -> 다종 산출물 생성` 툴체인으로 세우는 단계
2. **대규모 검증 체계화**
   - 전체 순회, verify, triage, XFAIL 관리로 "만들었다"가 아니라 "증명한다"로 전환
3. **릴리즈/문서화**
   - co-sim, mkdocs, packaging, quickstart까지 묶어 제품 형태를 갖추는 단계
4. **CIRCT 내장 아키텍처 전환**
   - 외부 프로세스 + 텍스트 파싱의 구조 한계를 인정하고, in-process MLIR API 기반으로 피벗
5. **CXXRTL 기준 정확도 수렴**
   - CXXRTL을 비교 기준선으로 삼아 HIRCT GenModel의 cycle accuracy를 좁히는 단계

가장 중요한 결론은 두 가지다.

- 기술적으로는, HIRCT가 이미 **PoC 단계를 지나 실제 생성/검증/회귀 루프를 운영하는 도구**가 되었다.
- 운영적으로는, 반복 blocker의 절반 이상이 구현 난이도보다도 **stale plan**, **가설 선행 디버깅**, **환경 전제 누락**에서 시작되었다.

즉 이 프로젝트의 핵심 교훈은 "더 많은 기능"보다 **더 틀릴 수 없는 시작 조건**이 더 큰 생산성 향상을 만든다는 점이다.

---

## 2. 분석 범위와 방법

### 2.1 분석 대상

- transcript: parent transcript 251개
- 제외: `subagents/*.jsonl`
- 교차 검증 문서:
  - `docs/plans/summary.md`
  - `docs/plans/draft-hirct-project-retrospective.md`
  - `docs/plans/2026-03-10-agent-workflow-guidance.md`
  - `docs/plans/2026-03-10-blocker-retrospective.md`
  - `docs/plans/2026-03-10-blocker-root-cause-analysis.md`
  - `docs/plans/2026-03-12-model-comparison-and-prompting-report.md`

### 2.2 읽는 기준

이번 보고서는 세션을 단순 나열하지 않고 아래 네 가지 질문으로 재구성했다.

1. **무엇을 만들려 했는가**
2. **왜 그 방향을 택했는가**
3. **실제로 무엇을 시도했고 어디까지 갔는가**
4. **무엇이 반복적으로 사람 개입을 요구했는가**

### 2.3 주의사항

- 이 문서는 **parent transcript 기준** 분석이다.
- transcript는 실제 대화 흐름을 보여 주지만, 세부 구현 상태의 최신 truth는 코드/문서와 다를 수 있으므로 관련 문서와 교차 검증했다.
- 인용은 parent session만 사용했다.

---

## 3. 프로젝트의 목표와 목적

### 3.1 프로젝트의 한 줄 정의

HIRCT는 **SystemVerilog/Verilog RTL을 입력으로 받아 CIRCT/MLIR 기반 분석을 수행하고, C/C++ 모델과 테스트/문서/브릿지 산출물을 자동 생성하는 파이프라인**으로 운영되었다.

### 3.2 transcript가 보여 준 실제 목표

parent transcript를 기준으로 보면, 프로젝트의 실제 목표는 단순히 "C++ 파일을 생성하는 것"이 아니었다. 세션 전반에서 반복 확인되는 목표는 다음 네 가지였다.

1. **수동 C 모델 작성을 자동화**
   - RTL 변경 시 재작성 비용을 줄이고, 반복 가능한 생성 체계를 만든다.
2. **사내 시뮬레이터에 연결 가능한 모듈 단위 모델 확보**
   - Verilator처럼 flatten된 블랙박스가 아니라, 계층과 이름이 살아 있는 모델이 필요했다.
3. **자동 검증 가능한 생성기 구축**
   - `hirct-gen`과 `hirct-verify`를 통해 "생성"과 "정확성 확인"을 분리하지 않고 함께 굴린다.
4. **생성기를 도구 체인으로 승격**
   - build, lint, lit, gtest, docs, packaging, co-sim, handoff까지 포함한 운영 체계를 갖춘다.

### 3.3 transcript가 보여 준 실제 목적

초기에는 "산출물을 만들어 보자"가 중심이었지만, 세션이 진행될수록 목적은 더 선명해졌다.

- **초기 목적**: walking skeleton 확보
- **중기 목적**: 모든 모듈을 돌려 보고 실패를 분류할 수 있는 체계 확보
- **후기 목적**: reference와 비교해 cycle-accurate correctness를 좁히기
- **운영 목적**: 에이전트 기반 개발을 재현 가능하게 만들기

이 변화는 transcript의 초점이 `feature addition -> traversal -> architecture pivot -> compare loop -> workflow retrospective`로 이동하는 모습에서 분명히 드러난다.

---

## 4. parent session이 보여 준 프로젝트 연대기

### 4.1 Phase 0: 환경과 규약 정착

대표 세션:
- [Phase 0 Kickoff](2cbea0e6-31c1-4fd8-9adf-f3ad338ff727)

이 단계의 핵심은 "코드를 짜기 시작한다"가 아니라 **작업이 반복 가능한 상태를 만드는 것**이었다. transcript에서 환경 설정, 도구 검증, 규약 정리, 빌드 기반 정착이 먼저 나오고, 이것이 이후 phase의 공통 기반으로 쓰였다.

### 4.2 Phase 1: 파이프라인과 emitter의 골격 완성

대표 세션:
- [Phase 1 Gate](ce1e9592-95bb-4527-a533-b872f22194b3)
- [Bugfix Gate](2522e545-5c6d-45a8-9c4a-f234511a2e75)

이 구간에서 transcript는 `hirct-gen`, emitter, lit/gtest/lint 게이트, regression bugfix를 중심으로 움직인다. 중요한 점은 이 시점부터 이미 "기능 구현"과 "게이트 기반 완료 판정"이 함께 간다는 것이다.

### 4.3 Phase 2: 전수 순회와 triage 체계

대표 세션:
- [Phase 2 Batch A](0a437a5e-54eb-4b6f-a057-c410cee033a5)
- [Batch C Complete](4f0b0725-c353-4de0-a077-551ae48d2a72)

이 시기부터 transcript의 언어가 바뀐다. "무엇을 추가할까"보다 **몇 개가 통과했고, 무엇이 실패했고, 왜 분류되는가**가 중심이 된다. 프로젝트가 기능 중심에서 **증거 중심**으로 넘어간 구간이다.

### 4.4 Phase 3: release, co-sim, packaging

대표 세션:
- [Phase 3 Release](32b9459f-acaa-4a88-a5e8-1751c3e41fe5)

HIRCT를 단순 개발 산출물이 아니라 **릴리즈 가능한 툴체인처럼 보이게 만드는 단계**였다. VCS co-sim, mkdocs, packaging, quickstart가 붙으면서 "기술 데모"와 "배포 가능한 도구"의 경계가 바뀌었다.

### 4.5 2026-02-28 피벗: CIRCT 내장 전환

대표 세션:
- [CIRCT Pivot](e9600201-c7d9-44f2-b234-95ca08fec3c6)
- [Legacy Removal](6427abec-f748-4cdd-ad86-d248d52bfc7b)

이 시점이 parent transcript 전체에서 가장 큰 구조 전환점이다. 기존의 외부 프로세스 호출과 텍스트 파싱 경로가 반복적으로 발목을 잡자, transcript는 이를 "부분 수정"이 아니라 **아키텍처 전환 문제**로 재정의한다.

이후의 세션들은 거의 모두 `in-process CIRCT/MLIR`, `IRAnalysis`, `legacy 제거`, `custom pass`라는 전제 위에서 움직인다.

### 4.6 후기 단계: CXXRTL 기준선과 정확도 수렴

대표 세션:
- [CXXRTL Strategy](a54603ae-5d9a-4a9b-adc6-d91c370ba422)
- [CXXRTL Compare](22a129a2-498d-4cfe-a980-c19c12ccac2d)
- [Array Inject Fix](ffd27fc3-3939-42c1-95cc-407b64e661cd)
- [UART TXD Debug](27c6407a-7c89-4179-90f8-06d9e6f0b1c1)

이 단계에서 transcript의 초점은 "파이프라인이 있느냐"에서 "reference와 얼마나 맞느냐"로 옮겨간다. CXXRTL은 경쟁자가 아니라 **golden reference / bridge**로 작동했고, HIRCT GenModel은 그 기준선에 맞춰 조정되는 대상으로 등장한다.

### 4.7 최종 확장: 운영 회고와 모델 보고

대표 세션:
- [Workflow Guidance](3a2acb15-9d02-48e3-ba04-0b9a4a7eb0be)
- [Model Report](589834fc-68b2-4cba-8cde-1fa31249d454)

후반 transcript는 기술 구현뿐 아니라, **에이전트를 어떻게 써야 하는가** 자체를 산출물로 끌어올린다. 이것은 프로젝트가 코드 생성기 개발을 넘어 **에이전트-운영 체계 설계**까지 포함하는 단계로 진입했음을 보여 준다.

---

## 5. 실제로 무엇을 시도했는가

### 5.1 성공적으로 정착한 시도

#### A. 자동 생성 파이프라인 구축

- `hirct-gen` / `hirct-verify`
- 다종 emitter
- lit/gtest/lint/build 게이트

이 축은 transcript 전반에서 가장 안정적으로 누적되었다. 세부 버그는 많았지만, "생성기 프레임워크 자체"는 프로젝트 후반에 이르러 이미 전제로 취급된다.

#### B. CIRCT 내장 아키텍처 전환

- 외부 `circt-verilog` 호출 + 텍스트 파싱의 한계 인식
- MLIR API 직접 순회
- `IRAnalysis` 기반 공통 분석 계층
- legacy analyzer/runner 제거

이 시도는 transcript와 문서가 가장 강하게 일치하는 성공 사례다. 이후의 디버깅이 모두 이 구조 위에서 진행된다는 점이 결정적이다.

#### C. CXXRTL을 기준선으로 사용하는 검증 루프

- UART 비교
- compare harness 정비
- NC/VCS 연결 시도
- v2p 같은 다른 모듈로 확대 검증

이 흐름은 후기 세션의 공통 축이다. transcript는 CXXRTL을 "HIRCT를 포기하게 만드는 대체재"가 아니라, **HIRCT를 더 정확하게 고치게 만드는 기준선**으로 사용한다.

### 5.2 부분 성공한 시도

#### A. `hw.array_inject` 및 FIFO 관련 GenModel 보강

이 축은 UART 비교에서 매우 중요했다. transcript상 PRDATA/INTR mismatch를 줄이는 데 분명한 진전이 있었고, generated 수준에서는 `5/5 PASS`까지 도달한 적이 있다. 다만 source-level durable fix로 완전히 닫혔다고 보기는 어려웠다.

#### B. clock-domain / multi-clock 정합화

clock domain 분석과 multi-clock step 생성은 여러 차례 시도되었고 일부 효과를 냈다. 하지만 transcript 기준으로 보면 "안정적으로 일반화됐다"고 하기는 아직 이르다.

#### C. 운영 게이트 정리

`plan-readiness`, `verification-before-completion`, `handoff sync`, `evidence-first` 같은 규율은 회고 문서로 잘 정리되었다. 하지만 transcript는 이 규율이 **문서화는 되었지만 항상 자동으로 적용되지는 않았음**도 동시에 보여 준다.

### 5.3 보류되거나 아직 열린 시도

#### A. UART TXD durable source fix

후기 transcript의 가장 직접적인 오픈 이슈다. generated patch 수준의 성공과 source-level closure 사이에 차이가 있었고, 최종 fresh 재검증에서 TXD mismatch가 남았다.

#### B. `ncs_cmd_v2p_blk_swap` 확대 검증

CXXRTL standalone은 통과했지만, HIRCT 쪽은 `llhd.drv` 타입 불일치 등 구조적 blocker로 멈췄다. 즉 확대 검증의 방향은 맞았지만, HIRCT가 아직 모든 실전 모듈을 소화하는 수준은 아니었다.

#### C. 운영 루프의 완전 제도화

문서와 transcript 모두 plan-readiness와 gate의 필요성을 강하게 보여 주지만, 실제 세션 흐름을 보면 여전히 우회나 늦은 적용이 존재했다.

---

## 6. 현재 상태

### 6.1 기술 상태

parent transcript 기준 현재 상태는 아래처럼 요약할 수 있다.

- HIRCT는 이미 **실제 생성/검증/회귀를 수행하는 도구** 단계에 들어와 있다.
- 핵심 아키텍처는 **CIRCT in-process + IRAnalysis + custom lowering**로 정리되었다.
- CXXRTL은 **실용적인 reference/bridge**로 가치를 입증했다.
- UART 축에서 **PRDATA/INTR는 상당히 수렴**했지만, **TXD durable source-level closure는 아직 남아 있다**.
- UART 밖으로의 안정적 확대 검증은 아직 진행 중이며, 일부 모듈은 구조적 known limitation에 막혀 있다.

### 6.2 운영 상태

- 계획, 검증, handoff, retrospective 문서는 많이 성숙했다.
- 그러나 transcript는 여전히 사람의 개입이 필요한 순간이 분명하다고 보여 준다.
- 특히 아래 순간에는 사람 개입이 결정적이었다.
  - 목표를 재정의할 때
  - 의미론을 선택할 때
  - blocker가 진짜 기술 한계인지 환경 문제인지 구분할 때
  - "이번 세션 done"의 기준을 고정할 때

### 6.3 한 줄 현재 상태

**플랫폼과 운영 체계는 상당히 성숙했지만, cycle-accurate correctness의 마지막 몇 개 갭을 실제 reference 비교로 닫아 가는 중**이라고 보는 것이 가장 정확하다.

---

## 7. transcript가 보여 준 반복 blocker 패턴

### 7.1 Pattern A: Stale Plan Cascade

가장 자주 보인 패턴 중 하나는 **계획이 현재 저장소 상태보다 뒤처진 채 실행에 들어가는 것**이었다. 그 결과 `No-Go`, 루트 원인 재규명, 환경 재확인, 세션 시간 소진이 연쇄적으로 이어졌다.

대표 세션:
- [GenDPIC IR Fix](c7c8b341-7a15-49fd-a088-fee84b71744a)
- [TXD Readiness Recovery](c02b7990-2ae4-4489-a6ee-9c00ddb9f2f8)

### 7.2 Pattern B: Hypothesis-First Debugging

IR, 생성 코드, 비교 로그를 보기 전에 "아마 이것이 원인일 것"이라고 출발한 경우, 거의 항상 우회와 재해석을 반복했다. UART, array_inject, multi-clock, TXD mismatch가 모두 이 패턴을 여러 번 보여 줬다.

대표 세션:
- [Array Inject Recovery](ffd27fc3-3939-42c1-95cc-407b64e661cd)
- [UART Compare Retry](22a129a2-498d-4cfe-a980-c19c12ccac2d)

### 7.3 Pattern C: Environment Assumption Drift

환경, 경로, build dir, tool path, license를 추정하고 시작했다가 나중에 바로잡는 패턴이 반복됐다. transcript가 말하는 blocker의 상당수는 실제 기술 한계가 아니라 **잘못된 시작 조건**이었다.

대표 세션:
- [CXXRTL DPI Bringup](a54603ae-5d9a-4a9b-adc6-d91c370ba422)
- [CXXRTL NC VCS](cadff050-65d8-4182-a479-019fb5729c62)

### 7.4 추측성 BLOCKED 선언

실행 근거 없이 "막힌 것 같다"고 판단한 뒤, 실제로는 경로나 도구 설정 문제였던 경우도 반복됐다. 이 점은 `verify-before-blocked` 류 규율이 왜 필요했는지 transcript가 그대로 증명한다.

### 7.5 약한 handoff

좋지 않은 handoff는 추측만 남기고 재현 명령과 다음 첫 명령을 남기지 못했다. 반대로 좋은 handoff는 다음 세션이 곧바로 재현부터 시작할 수 있게 만들었다.

---

## 8. 사람 개입이 필요했던 순간

### 8.1 목표와 의미를 재정의할 때

사람이 가장 효과적으로 개입한 순간은 "코드를 대신 짜는 순간"보다 **무엇을 정말 맞추려는지 정의하는 순간**이었다.

예:
- cycle-accurate와 pure functional의 구분
- GenModel과 GenFuncModel의 역할 구분
- CXXRTL을 경쟁자가 아니라 기준선으로 둘지의 결정
- 특정 mismatch를 기능 오류로 볼지 timing gap으로 볼지의 판단

### 8.2 외부 환경 지식이 필요할 때

license, simulator version, 실제 설치 경로, toolchain 제약은 transcript만으로는 안정적으로 추론되지 않았다. 이 구간은 사람이 정보를 주는 순간 진행이 빨라졌다.

### 8.3 done 기준을 고정할 때

`seed=42 1000cyc`, `5/5 compare`, `minimum pass bar`, `stretch check` 같은 기준은 사람이 못 박아 줄 때 세션이 수렴했다. 기준이 없으면 에이전트는 계속 탐색을 연장하는 경향을 보였다.

---

## 9. 회고: 이 프로젝트가 남긴 교훈

### 9.1 기술 교훈

1. **구조 문제는 부분 수정으로 오래 버티면 비용이 커진다**
   - CIRCT 내장 피벗이 이를 증명했다.
2. **reference가 있어야 의미 있는 정확도 디버깅이 가능하다**
   - CXXRTL이 후기 세션에서 핵심 역할을 했다.
3. **generated success와 source-level closure는 다르다**
   - UART compare 세션이 이 차이를 반복해서 보여 줬다.

### 9.2 운영 교훈

1. **가장 큰 생산성 향상은 새 기능이 아니라 시작 조건의 고정에서 나왔다**
2. **plan-readiness는 문서가 아니라 실행 게이트여야 한다**
3. **evidence-first가 없으면 디버깅은 거의 항상 오래 걸린다**
4. **handoff는 요약이 아니라 실행 가능한 다음 시작점이어야 한다**

### 9.3 강한 결론

- HIRCT 프로젝트의 반복 blocker는 구현 난이도보다도 **stale plan**, **가설 선행 디버깅**, **환경 전제 누락**에서 먼저 발생했다.
- parent transcript 기준으로 보면, 가장 큰 생산성 향상 요인은 새 기능 추가가 아니라 **plan-readiness**, **evidence-first**, **handoff sync** 같은 운영 게이트였다.
- 사람 개입이 가장 가치 있었던 순간은 코드를 직접 쓰는 때보다 **목표와 의미를 재정의하는 순간**이었다.

---

## 10. 문서와 transcript의 정합성 평가

### 10.1 일치하는 점

- `draft-hirct-project-retrospective.md`가 말하는 큰 흐름, 즉 **CIRCT 내장 전환이 game changer였다**는 평가는 transcript와 잘 맞는다.
- `summary.md`가 말하는 phase 구조와 상위 목표도 transcript의 큰 줄기와 일치한다.
- `agent-workflow-guidance.md`, `blocker-retrospective.md`, `blocker-root-cause-analysis.md`는 실제 반복 패턴을 꽤 정확하게 추상화하고 있다.

### 10.2 transcript가 더 강하게 드러내는 점

- 문서보다 transcript가 더 직접적으로 보여 주는 사실은, blocker의 상당수가 **기술 난제 자체보다 기준선 없는 착수**에서 시작됐다는 점이다.
- 또 문서 인상상 UART/CXXRTL 축이 꽤 정리된 것처럼 보일 수 있지만, transcript 기준 최신 오픈 이슈는 **TXD durable source fix**다.
- generated patch로 성공한 것과 source-level에서 재생성 가능한 closure를 구분해야 한다는 점도 transcript가 더 선명하게 보여 준다.

---

## 11. 인용 가능한 대표 세션

- [Phase 0 Kickoff](2cbea0e6-31c1-4fd8-9adf-f3ad338ff727)
- [Phase 1 Gate](ce1e9592-95bb-4527-a533-b872f22194b3)
- [Bugfix Gate](2522e545-5c6d-45a8-9c4a-f234511a2e75)
- [Phase 2 Batch A](0a437a5e-54eb-4b6f-a057-c410cee033a5)
- [Batch C Complete](4f0b0725-c353-4de0-a077-551ae48d2a72)
- [Phase 3 Release](32b9459f-acaa-4a88-a5e8-1751c3e41fe5)
- [CIRCT Pivot](e9600201-c7d9-44f2-b234-95ca08fec3c6)
- [Legacy Removal](6427abec-f748-4cdd-ad86-d248d52bfc7b)
- [CXXRTL Strategy](a54603ae-5d9a-4a9b-adc6-d91c370ba422)
- [CXXRTL Compare](22a129a2-498d-4cfe-a980-c19c12ccac2d)
- [Array Inject Recovery](ffd27fc3-3939-42c1-95cc-407b64e661cd)
- [TXD Readiness Recovery](c02b7990-2ae4-4489-a6ee-9c00ddb9f2f8)
- [Workflow Guidance](3a2acb15-9d02-48e3-ba04-0b9a4a7eb0be)
- [Model Report](589834fc-68b2-4cba-8cde-1fa31249d454)

---

## 12. 최종 결론

모든 parent 세션의 대화 흐름을 기준으로 보면, HIRCT 프로젝트는 다음과 같이 요약된다.

- **무엇을 했는가**
  - RTL 기반 자동 생성 파이프라인을 만들고, 이를 검증 가능한 도구 체인으로 키웠다.
- **왜 했는가**
  - 수동 C 모델 작성 비용을 줄이고, 사내 시뮬레이터에 연결 가능한 모듈 단위 모델을 확보하기 위해서다.
- **무엇을 시도했는가**
  - emitter/verify/co-sim/release, Phase 2 traversal, CIRCT 내장 피벗, CXXRTL 기준 비교, 운영 게이트 정리까지 폭넓게 시도했다.
- **현재 어디까지 왔는가**
  - 플랫폼과 운영 체계는 상당히 성숙했고, 남은 핵심은 일부 cycle-accurate correctness gap과 확대 검증이다.
- **무엇을 배웠는가**
  - 더 똑똑한 에이전트보다도, **틀릴 수 없는 시작 조건**이 더 큰 성과를 만든다.

한 줄로 줄이면:

**HIRCT는 "자동 생성기 PoC"를 넘어서 "실제 생성·검증·회귀를 운영하는 도구"가 되었고, 남은 과제는 구조 정립이 아니라 마지막 정확도 갭과 운영 습관의 완성이다.**
