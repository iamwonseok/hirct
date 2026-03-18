# HIRCT 미실행 아이디어 및 잠재 시나리오 보고서

> **작성일**: 2026-03-12
> **목적**: 지금까지 문서와 parent session에서 제안되었지만 아직 본격 실행되지 않았거나 부분만 실행된 아이디어를 정리하고, 향후 어떤 시나리오가 잠재적으로 유망한지 선택지 관점에서 정리한다.
> **경계**: 이 문서는 `현재 상태 보고서`가 아니라 `미래 옵션 보고서`다. 확인된 사실과 현재 상태는 `docs/plans/2026-03-12-parent-session-transcript-report.md`를 우선 참조한다.

---

## 1. 왜 별도 문서가 필요한가

기존 보고서들은 `무엇을 했는가`, `무엇이 남았는가`, `현재 어디까지 왔는가`를 정리하는 데 초점이 있었다. 하지만 실제 세션과 전략 문서를 보면, 실행하지는 않았지만 충분히 괜찮았던 아이디어가 여러 번 제안되었고, 이런 항목은 현재 상태 문서 안에 섞어 쓰면 사실과 가정이 뒤섞이기 쉽다.

그래서 이 문서는 다음 질문만 다룬다.

1. **좋았지만 아직 본격 실행하지 않은 아이디어는 무엇인가**
2. **왜 그 아이디어가 여전히 유망한가**
3. **왜 아직 실행되지 않았는가**
4. **언제 어떤 조건이 되면 다시 꺼내야 하는가**

---

## 2. 현재 상태 기준선

잠재 시나리오를 평가하기 위한 최소한의 기준선만 적는다.

- HIRCT는 이미 `CIRCT in-process + IRAnalysis + custom lowering` 구조로 정리되었다.
- `hirct-gen`, `hirct-verify`, emitter, traversal, report, co-sim, packaging까지 한 바퀴 돌아가는 수준에는 도달했다.
- CXXRTL은 UART와 일부 확대 검증에서 유효한 reference/bridge로 확인되었다.
- 하지만 현재 성능/커버리지/정확성 신뢰도는 여전히 Verilator보다 열세다.
- 즉 앞으로의 아이디어는 "무조건 더 만든다"가 아니라, **지금의 약점을 보완하면서도 HIRCT만의 차별점을 키우는가**를 기준으로 평가해야 한다.

상세한 현재 상태는 `docs/plans/2026-03-12-parent-session-transcript-report.md`를 본다.

---

## 3. 아이디어 수집 원천

이 문서는 아래 출처를 기반으로 정리했다.

- `docs/plans/open-decisions.md`
- `docs/plans/2026-03-08-hirct-product-strategy.md`
- `docs/plans/2026-03-06-genmodel-ir-spec-brainstorm.md`
- `docs/plans/draft-hirct-project-retrospective.md`
- `docs/plans/2026-03-12-parent-session-transcript-report.md`
- [Product Strategy](cced793b-e904-4d5d-8403-bf8770fed89e)
- [CXXRTL DPI Bringup](a54603ae-5d9a-4a9b-adc6-d91c370ba422)
- [Workflow Guidance](3a2acb15-9d02-48e3-ba04-0b9a4a7eb0be)
- [IR Spec Brainstorm](12f9f509-3b82-41a8-8957-ad9e5dc78bf1)

---

## 4. 미실행 아이디어 인벤토리

### 4.1 제품 방향 결정 계열

| 아이디어 | 현재 상태 | 왜 유망한가 | 왜 아직 안 했는가 | 다시 꺼낼 조건 |
|---|---|---|---|---|
| **사내 시뮬레이터 인터페이스 확정** (`pure C API` vs `SystemC TLM` vs 커스텀) | 미실행 | 이 결정이 나야 GenModel/FuncModel/CXXRTL bridge의 최종 출력 방향이 고정된다. 사실상 제품 포지셔닝의 상위 결정이다. | `H-1`이 아직 OPEN이고, 실제 사내 시뮬레이터 API 계약이 문서화되지 않았다. | 사내 시뮬레이터 호출 규약, 런타임 제약, SystemC 허용 여부를 확보했을 때 |
| **CXXRTL 브리지 전략의 범위 확정** | 부분 실행 | UART 수준에서는 ROI가 보였고, GenModel 완성 전에도 단기 자동화를 얻을 수 있다. | UART/v2p PoC는 했지만, 어디까지를 성공 기준으로 삼을지 `H-2` 수준의 범위 합의가 없다. | 소형 모듈, UART급 IP, 칩 단위 중 어느 수준을 bridge success로 볼지 정했을 때 |
| **GenFuncModel을 `pure C core + wrapper` 구조로 재설계** | 부분 실행 | 한번 만든 코어를 DPI-C, standalone, QEMU, SystemC 등 여러 환경에 재사용할 수 있다. | `H-3`이 OPEN이고, 래퍼 우선순위와 실제 소비자 환경이 확정되지 않았다. | `H-1`이 정리되고, `read/write/tick` API를 표준화할 수 있을 때 |

### 4.2 차별화 가치 강화 계열

| 아이디어 | 현재 상태 | 왜 유망한가 | 왜 아직 안 했는가 | 다시 꺼낼 조건 |
|---|---|---|---|---|
| **레지스터 맵 + SMOKE 테스트 + Programmer's Guide + IP-XACT 자동 생성** | 부분 실행 | HIRCT가 Verilator와 다른 이유를 가장 잘 설명하는 차별화 축이다. RTL만으로 문서/검증/SW 연계를 만들 수 있다. | `GenRAL`, `GenDoc` 일부 기반은 있지만, 축 1 전체 패키지와 신규 SMOKE 생성기는 아직 전략 문서 수준이다. | 주소 디코딩/필드 추출 정확도에 대한 신뢰와 대표 IP smoke 패턴이 정리되었을 때 |
| **CPU interface 분석 기반 bare-metal driver 생성** | 부분 실행 | FW 팀 생산성에 직접 연결되는 제품형 산출물이다. 문서 생성보다 사업적 설명력이 크다. | 전략 문서에서 가능성은 언급됐지만, 실제 초기화 시퀀스/의미 정보는 RTL만으로 부족해 범위가 커졌다. | 최소 드라이버 스켈레톤과 추가 입력(YAML/IP-XACT) 허용 범위를 합의했을 때 |
| **테스트 인터페이스 축 강화** (`cocotb/UVM/DPI-C`) | 부분 실행 | 모델이 완벽하지 않아도, 테스트 인터페이스 자동 생성은 검증 생산성을 높일 수 있다. | 현재 우선순위가 cycle accuracy closure와 architecture stabilization에 더 쏠려 있었다. | 모델 정확도보다 인터페이스 자동화의 ROI를 먼저 얻고 싶을 때 |

### 4.3 품질 기반 강화 계열

| 아이디어 | 현재 상태 | 왜 유망한가 | 왜 아직 안 했는가 | 다시 꺼낼 조건 |
|---|---|---|---|---|
| **GenModel IR precondition/spec 공식화** | 부분 실행 | 실패를 "왜 안 되는가"가 아니라 "어디까지 지원하는가"로 바꿔 준다. emitter/debug/triage 모두 좋아진다. | 필요성은 분명했지만 `H-4`로 밀리며 전략 결정 이후로 이관되었다. | 상위 제품 방향이 정리되고, 지원 IR 경계를 문서화할 필요가 커졌을 때 |
| **pass별 IR 변화 비교와 공식 진입 조건 문서화** | 부분 실행 | lowering pipeline의 black box 성격을 줄여, 디버깅 비용과 논쟁을 줄인다. | 브레인스토밍 문서는 있었지만 실행/문서화 우선순위가 밀렸다. | 대표 모듈 몇 개에 대해 `--mlir-print-ir-after-all` 비교를 남길 여력이 생겼을 때 |
| **MLIR 기반 형식/등가성 검증 도입** (`Verification Dialects`, `HEC`, `circt-lec`) | 미실행 | HIRCT의 가장 약한 고리인 "자체 생성 모델 정확성 증명"을 장기적으로 강화할 수 있다. | 연구 가치는 크지만, 당장 제품 방향/커버리지/성능 문제보다 후순위다. | 대표 pipeline과 모듈 셋이 안정되고, pass 단위 검증 투자가 가능해졌을 때 |

### 4.4 운영 체계 강화 계열

| 아이디어 | 현재 상태 | 왜 유망한가 | 왜 아직 안 했는가 | 다시 꺼낼 조건 |
|---|---|---|---|---|
| **`.cursor` 운영 자산화** (`workflow-routing`, `verify-before-blocked`, report/docs rules 등) | 부분 실행 | 반복 blocker의 상당수가 기술보다 운영에서 나왔기 때문에, 이 영역은 ROI가 높다. | 가이드 문서와 초안은 있으나, 실제 rule/skill/command 파일로는 아직 승격되지 않았다. | 비슷한 blocker가 계속 반복되거나, 팀 차원의 운영 표준화가 필요해졌을 때 |
| **프롬프트/모델 A/B 테스트 체계화** | 미실행 | 지금까지는 정성 회고가 많았고, 실제로 어떤 프롬프트 구조가 성공률을 높였는지 계량화할 수 있다. | 보고서에서 필요성을 정리했지만, 아직 동일 과제를 통제 조건으로 반복 실행한 로그가 부족하다. | 동일 task 3~5개를 여러 프롬프트로 재실험할 수 있을 때 |
| **plan↔report↔handoff 자동 sync 규율 강제** | 부분 실행 | transcript상 가장 큰 낭비 요인을 줄일 수 있다. | 문서화는 되었지만, 실제 workflow에 강제되지 않았다. | 다음 phase에서 문서 불일치가 다시 cost를 만들기 시작할 때 |

---

## 5. 잠재 시나리오 분류

### 5.1 시나리오 A: Verilator 대체가 아니라 "분석 플랫폼"으로 간다

가장 현실적인 시나리오다. 현재 성능과 커버리지는 Verilator보다 열세이므로, 정면 대체 경쟁으로 가면 불리하다. 대신 HIRCT를 아래 가치로 재정의하는 방향이다.

- RTL 기반 레지스터 맵 역추출
- SMOKE 테스트 자동 생성
- Programmer's Guide / IP-XACT 산출
- 테스트 인터페이스 자동 생성
- 사내 시뮬레이터 맞춤형 모델/래퍼 생성

즉 **"빠른 시뮬레이터"가 아니라 "RTL 분석 기반 자동화 플랫폼"**으로 가는 시나리오다.

### 5.2 시나리오 B: CXXRTL을 단기 제품 카드로 쓴다

이 시나리오는 HIRCT가 직접 cycle model을 완성하기 전에, CXXRTL을 활용해 단기 ROI를 확보하는 길이다.

- 짧은 기간에 동작하는 bridge 확보
- 이름/계층 보존을 활용해 사내 연결성 검증
- GenModel은 장기 자산으로 남기되, 제품 시연/초기 자동화는 CXXRTL로 앞당김

이 경로는 현실적이지만, 장기적으로는 CXXRTL runtime 의존성과 커스텀 최적화 한계가 병목이 될 수 있다.

### 5.3 시나리오 C: GenModel을 유지하되 "정확도/명세" 우선으로 간다

이 시나리오는 feature를 더 늘리기보다, 지원 범위와 의미론을 먼저 닫는 쪽이다.

- IR precondition checklist
- pass별 IR 변화 비교
- reset/clock/memory/array 의미론 명세
- representative module scorecard

이 방향의 장점은 기술 부채를 줄이고 이후 emitter/verify를 더 안정적으로 만드는 것이다. 단점은 단기 데모나 사업적 화제성이 약하다는 점이다.

### 5.4 시나리오 D: 운영 최적화에 투자한다

이 시나리오는 코드보다 workflow에 먼저 투자하는 방식이다.

- plan-readiness 강제
- verify-before-blocked 강제
- handoff 템플릿 고정
- docs/report sync 규칙화
- prompt A/B testing

transcript 기준으로 보면 이 시나리오는 생각보다 가치가 크다. 현재 반복되는 낭비의 상당 부분이 기술 자체보다 운영 결함에서 나왔기 때문이다.

---

## 6. 기대 효과와 리스크

| 시나리오 | 기대 효과 | 가장 큰 리스크 |
|---|---|---|
| 분석 플랫폼 강화 | HIRCT만의 차별점이 선명해짐 | core model이 약한 상태로 주변 기능만 많아질 수 있음 |
| CXXRTL 브리지 확대 | 단기 ROI와 데모 가능성 상승 | 장기적으로 HIRCT 고유 가치가 흐려질 수 있음 |
| GenModel 명세 우선 | 디버깅 비용 감소, 정확도 개선 기반 확보 | 단기 외부 가시성이 낮음 |
| 운영 최적화 투자 | 동일 인력 대비 완료율 상승 | 기술 진척이 느려 보일 수 있음 |

---

## 7. 추천 우선순위

현재 상태와 transcript를 같이 놓고 보면, 우선순위는 아래처럼 잡는 것이 합리적이다.

### 7.1 1순위: 상위 방향을 고정하는 결정

1. `H-1` 사내 시뮬레이터 인터페이스 확정
2. `H-2` CXXRTL bridge 범위와 성공 기준 확정
3. `H-3` GenFuncModel wrapper 전략 확정

이 세 가지는 구현 전에 방향을 정하는 결정이다. 여기서 흔들리면 이후 작업이 다시 갈린다.

### 7.2 2순위: 차별화와 정확도의 균형 투자

1. 축 1(`레지스터 맵 + SMOKE + Guide + IP-XACT`) 중 최소 vertical slice 정의
2. `GenModel IR precondition/spec` 재개

이 둘은 각각 "왜 HIRCT를 해야 하는가"와 "어떻게 덜 틀리게 만들 것인가"를 담당한다.

### 7.3 3순위: 운영 자동화

1. `verify-before-blocked`
2. `workflow-routing`
3. `plan↔report↔handoff sync`

기술 과제처럼 보이진 않지만, 실제 세션 비용 절감 효과는 매우 클 가능성이 높다.

---

## 8. 지금 당장 가능한 작은 실험

큰 결정을 당장 못 하더라도, 아래 실험은 비교적 저비용으로 의미 있는 신호를 준다.

### 실험 1: 사내 시뮬레이터 인터페이스 decision memo

- 목표: `pure C API` / `SystemC TLM` / `custom API`를 1페이지 비교
- 성공 기준: H-1 의사결정을 위한 장단점 표 완성

### 실험 2: CXXRTL bridge success bar 정의

- 목표: UART / v2p / 1개 추가 모듈 기준으로 "성공"의 정의를 수치로 확정
- 성공 기준: `H-2`를 OPEN에서 PARTIAL로 내릴 수 있는 문서 작성

### 실험 3: IR precondition checklist 초안

- 목표: GenModel이 지원하는 IR/미지원 IR를 1차 checklist로 고정
- 성공 기준: skip/fail 이유가 문서와 코드에서 같은 언어로 설명됨

### 실험 4: 축 1 vertical slice

- 목표: APB 계열 1개 모듈에서 `register map + smoke test + guide`까지 연결
- 성공 기준: "HIRCT는 Verilator와 다른 무엇을 주는가"를 한 번에 보여 주는 데모 확보

### 실험 5: prompt A/B test

- 목표: 동일 task에 대해 `좋은 템플릿 프롬프트`와 `느슨한 프롬프트`를 비교
- 성공 기준: 완료율, 재지시 횟수, blocker 수 차이를 정량화

---

## 9. 아직 하지 말아야 할 것

다음 항목은 흥미롭지만, 지금 바로 큰 투자로 들어가면 분산이 커질 수 있다.

- 대규모 형식 검증 체계 도입
- 칩 단위 전체 bridge 제품화
- Verilator 정면 대체 전략
- 너무 이른 bare-metal full driver 자동 생성

이들은 장기적으로 의미는 있지만, 현재는 상위 방향과 support boundary가 더 먼저다.

---

## 10. 추적 규칙

이 문서의 항목은 계속 아이디어로만 남아 있으면 안 된다. 앞으로는 아래 규칙으로 관리하는 것이 좋다.

1. 아이디어가 실제 실행되면, 이 문서에서는 `부분 실행` 또는 `완료`로 상태를 변경한다.
2. 실측 결과가 생기면 해당 항목은 `현재 상태 보고서` 또는 별도 execution/report 문서로 이동한다.
3. 가치가 사라지거나 전략에서 제외되면 `보류/폐기` 이유를 남긴다.
4. `OPEN` 결정이 닫히면, 관련 시나리오도 재평가한다.

---

## 11. 최종 정리

좋은 아이디어는 많았지만, 모두를 동시에 밀 수는 없다. 지금까지의 문서와 session을 함께 보면, HIRCT의 잠재 시나리오는 크게 네 가지로 요약된다.

- **분석 플랫폼으로 차별화**
- **CXXRTL로 단기 브리지 확보**
- **GenModel의 정확도/명세를 먼저 닫기**
- **운영 자동화로 반복 비용 줄이기**

현재 기준에서 가장 중요한 것은 "무엇이 멋져 보이는가"보다 **무엇이 지금의 약점과 직접 맞닿아 있는가**다.

그래서 이 문서의 결론은 다음과 같다.

**HIRCT의 잠재 가치는 충분히 크지만, 다음 단계는 기능을 더 벌리는 것보다 상위 방향을 고정하고, 차별화 한 축과 정확도 한 축을 동시에 좁히는 방식으로 가는 것이 가장 유망하다.**
