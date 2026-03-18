# HIRCT 진행 조사 리포트: 모델 비교와 프롬프트 운영 팁

> **작성일**: 2026-03-12
> **범위**: Phase 0 ~ Phase 4, Phase 3 감사, fc6161 CXXRTL PoC, 최근 커밋 흐름, parent transcript 회고
> **목적**: 지금까지의 프로젝트 진행 내용을 바탕으로, (1) 모델/도구별 장단점과 성능을 비교하고, (2) 실제로 효과가 있었던 프롬프트/에이전트 운영 패턴을 정리한다.

---

## 1. 핵심 결론

### 1.1 한 줄 요약

- **AI 모델(GPT/OPUS/Sonnet)**: 이 저장소 근거상 "개별 모델 성능 서열"보다 **교차 리뷰 세트**로서의 가치가 확인되었다.
- **실행/변환 도구(Verilator/CXXRTL/GenModel/arcilator)**: **Verilator는 기준선**, **CXXRTL은 단기 브릿지**, **HIRCT GenModel은 통합 최적화 대상**, **arcilator 단독 경로는 범용 NO-GO**로 정리된다.
- **운영 방식**: 좋은 결과는 "좋은 모델" 하나보다 **좋은 게이트 설계와 프롬프트 구조**에서 더 많이 나왔다.

### 1.2 이번 조사에서 가장 중요한 판단

1. **AI 모델 비교는 정성 평가 중심으로 써야 한다.**
   저장소 내부에는 GPT/OPUS/Sonnet의 속도, 비용, 성공률을 같은 조건에서 비교한 실측 표가 없다. 대신 Phase 3 감사에서 **3-모델 교차 리뷰로 blind spot을 찾는 데 유효했다**는 정성 근거가 있다.

2. **실행 모델/변환 도구 비교는 실측 수치 중심으로 쓸 수 있다.**
   Verilator, HIRCT GenModel, CXXRTL, Phase 4 CIRCT 내장 경로에 대해서는 통과율, mismatch, 실행 시간, 커버리지 변화 같은 숫자가 존재한다.

3. **프로젝트 생산성은 프롬프트의 길이보다 구조가 좌우했다.**
   실제 회고 문서에서 반복적으로 확인된 패턴은 `세션 목표 1개`, `minimum pass bar 1개`, `cwd/branch 명시`, `scope-out`, `재현 명령`, `artifact 확인`, `stretch 분리`였다.

---

## 2. 조사 방법과 근거 수준

### 2.1 조사 대상

- `docs/plans/summary.md`
- `docs/plans/draft-hirct-project-retrospective.md`
- `docs/plans/2026-03-08-hirct-product-strategy.md`
- `docs/plans/2026-03-10-agent-workflow-guidance.md`
- `docs/plans/2026-03-10-blocker-retrospective.md`
- `docs/plans/2026-03-10-blocker-root-cause-analysis.md`
- `docs/report/phase-3-release/*.md`
- `docs/report/phase-4-circt-embedding/phase2-rerun-and-skip-analysis.md`
- `examples/fc6161/pt_plat/cxxrtl-poc/README.md`
- 최근 커밋 (`git log --oneline -15`)
- parent transcript: [TXD Readiness Recovery](c02b7990-2ae4-4489-a6ee-9c00ddb9f2f8), [Array Inject Recovery](ffd27fc3-3939-42c1-95cc-407b64e661cd), [UART Compare Retry](22a129a2-498d-4cfe-a980-c19c12ccac2d), [CXXRTL DPI Bringup](a54603ae-5d9a-4a9b-adc6-d91c370ba422), [IR Clock Domain Fix](c7c8b341-7a15-49fd-a088-fee84b71744a)

### 2.2 근거 수준 분류

| 등급 | 의미 | 예시 |
|------|------|------|
| **A. 저장소 실측** | 실행 로그/리포트/수치가 있음 | `10 seeds x 1000 cycles`, `357.8s -> 27.9s`, `5/5 compare pass` |
| **B. 저장소 관찰** | 회고/전략/감사 문서에 반복 등장 | "3-모델 교차 리뷰로 blind spot 발견", "stale plan cascade" |
| **C. 외부 조사 인용** | 저장소 안에 적혀 있으나 외부 출처 기반 | `NVIDIA GEM 5-64x vs Verilator` |

이 문서에서는 **A와 B를 주근거**로 쓰고, C는 참고로만 사용한다.

---

## 3. AI 모델 비교: 무엇을 잘했고, 무엇은 아직 말할 수 없는가

### 3.1 중요한 전제

이번 조사에서 GPT/OPUS/Sonnet은 **코드 생성기 자체의 직접 비교 대상**이 아니라, 주로 **감사/리뷰/의사결정 보조 모델**로 등장한다. 따라서 이 셋을 Verilator/CXXRTL/GenModel과 같은 표에서 직접 비교하면 축이 섞인다.

### 3.2 AI 모델 비교 표

| 모델 | 저장소에서 확인된 역할 | 장점 | 한계 | 근거 수준 | 실무 권고 |
|------|----------------------|------|------|----------|----------|
| GPT | Phase closeout 교차 리뷰 참여 | blind spot 탐지에 기여 | 개별 우위/속도/비용 수치 없음 | B | 단독 판정보다 교차 리뷰 세트로 사용 |
| OPUS | Phase closeout 교차 리뷰 참여 | 서로 다른 관점 보강 | 개별 우위/속도/비용 수치 없음 | B | 설계/범위 누락 점검용 |
| Sonnet | Phase closeout 교차 리뷰 참여 | 누락/과잉 구현 포착 보강 | 개별 우위/속도/비용 수치 없음 | B | 최종 감사/리뷰 보조에 적합 |

### 3.3 AI 모델에 대한 실제 결론

- **확실히 말할 수 있는 것**
  - Phase 3 감사에서 **3-모델 교차 리뷰(GPT/OPUS/Sonnet)**가 수행되었고, 회고 문서에서 이를 **blind spot 발견**에 유효했다고 평가한다.
  - 이 패턴은 상시 구현보다 **phase closeout 감사**에 더 적합했다.

- **이번 조사로는 말할 수 없는 것**
  - 어느 모델이 가장 빠른지
  - 어느 모델이 가장 정확한지
  - 어느 모델이 가장 싼지
  - 동일 프롬프트 조건에서 모델별 성공률이 어떤지

- **따라서 리포트 문구는 이렇게 쓰는 것이 정확하다**
  - "GPT/OPUS/Sonnet은 개별 성능 서열보다는 교차 리뷰 체계로서 의미가 있었다."
  - "개별 모델의 정량 우열은 현재 저장소 근거만으로는 판정 불가하다."

---

## 4. 실행 모델/변환 도구 비교: 프로젝트에서 실제로 검증된 축

### 4.1 비교 대상

- **Verilator**: 정확성 검증용 기준선, reference simulator
- **HIRCT GenModel**: 사내 시뮬레이터 통합을 위한 cycle-accurate C++ 모델
- **Yosys CXXRTL**: 단기 자동화 브릿지 후보
- **CIRCT arcilator**: Phase 4 검토 대상이었으나 범용 경로로는 실패
- **HIRCT Phase 4 CIRCT 내장 경로**: 외부 프로세스 + 텍스트 파싱을 대체한 현재 핵심 아키텍처

### 4.2 구조/통합 관점 비교

| 대상 | 모듈 계층 | 이름 보존 | 런타임 의존 | 프로젝트에서 확인된 강점 | 프로젝트에서 확인된 한계 | 현재 판단 |
|------|----------|----------|------------|-------------------------|-------------------------|----------|
| Verilator | flatten | 낮음 | `libverilated` 필요 | 성숙한 정확성 기준선, 검증 reference로 강함 | 사내 시뮬레이터용 모듈 단위 통합이 불편 | **기준선 유지** |
| HIRCT GenModel | 보존 | 높음 | 없음 (`cstdint` 중심) | 계층/이름 보존, 사내 API 적응 쉬움, 투명한 코드 | 자체 검증 비용 필요, known limitations 존재 | **핵심 투자 대상** |
| Yosys CXXRTL | 보존 | 높음 | CXXRTL runtime header 필요 | standalone C++, 빠른 PoC, 이름 접근 용이 | runtime header 의존, 커스텀 최적화 한계 | **단기 브릿지** |
| CIRCT arcilator | flatten 성격 | 제한적 | LLVM/JIT 성격 | CIRCT 내장 경로라는 매력 | `cf.br/cf.cond_br` 잔존 시 범용 실패 | **범용 NO-GO** |
| HIRCT Phase 4 내장 경로 | 보존 가능 | 높음 | CIRCT/MLIR 링크 | 파싱 성공률/속도/유지보수성 개선, analyzer 분기 제거 | LLHD lowering 미완전성은 자체 pass 필요 | **game changer** |

### 4.3 정량 비교 표

| 항목 | Verilator | HIRCT GenModel | Yosys CXXRTL | 비고 |
|------|-----------|----------------|-------------|------|
| LevelGateway 교차 검증 | VCS 10/10 PASS, ncsim 10/10 PASS | 동일 테스트에서 PASS | 해당 항목 직접 비교 근거 없음 | 기준선/정확성 검증에 적합 |
| Queue_11 | reference | VCS 1/10 PASS, ncsim 0/10 PASS | 직접 비교 수치 없음 | GenModel FIFO 로직 버그 확정 |
| CLINT | reference | VCS 0/10 PASS, mismatch 182~219/1000cyc | 직접 비교 수치 없음 | IP-top 확장 한계 노출 |
| UART compare | reference 역할 | PRDATA/INTR 5/5 시드 0 mismatch, TXD 일부 timing diff | UART 변환 PASS, 컴파일 ~25s, 실행 <1s | CXXRTL이 단기 기준선으로 유효 |
| v2p 확대 검증 | reference 역할 | 생성 BLOCKED | 변환 PASS, 컴파일 ~13s, 실행 <1s, 1165cyc crash 없음 | CXXRTL 단독 Conditional Go |

### 4.4 가장 의미 있는 수치

#### A. Phase 4 전환 효과

| 지표 | Baseline | 개선 후 | 변화 |
|------|---------:|--------:|------|
| GenModel-specific failures | 4 | 0 | 전부 해소 |
| 총 실행 시간 | 357.8s | 27.9s | **92.2% 감소** |
| uart_top GenModel 생성 | - | 37/39 | residual LLHD 2개 제외 |

#### B. CXXRTL UART PoC

| 항목 | 결과 |
|------|------|
| 입력 규모 | preprocessed.v 21,041줄, 22개 모듈 |
| 변환 | PASS |
| 컴파일 | PASS, 약 25초 |
| 실행 | 1000cyc 기준 <1초 |
| 초기 비교 | PRDATA 55/65 일치, INTR 251 mismatch, TXD 0 mismatch |
| 개선 후 비교 | PRDATA 5/5 시드 0 mismatch, INTR 5/5 시드 0 mismatch, TXD 일부 시드 timing diff |

#### C. 대표 교차 검증 결과

| 모듈 | 결과 | 해석 |
|------|------|------|
| LevelGateway | VCS 10/10 PASS, ncsim 10/10 PASS | GenModel 정확성 확인 |
| Queue_11 | VCS 1/10 PASS, ncsim 0/10 PASS | `seq.firmem.read_port` 계열 버그 확인 |
| DW_apb_uart_bcm99 | VCS 10/10 PASS | UART 계열 pilot 성공 |
| CLINT | 0/10 PASS, 182~219 mismatch | TileLink timer logic mismatch |

### 4.5 실행 도구별 장단점 요약

#### Verilator

- 장점
  - 프로젝트에서 가장 믿을 수 있는 reference simulator 역할
  - `hirct-verify`와 cross-validation의 기준선
- 단점
  - flatten, 이름 변형, `libverilated` 의존
  - 사내 시뮬레이터에 모듈 단위로 바로 끼우기 불편

#### HIRCT GenModel

- 장점
  - 계층/이름 보존
  - 사내 API 적응성이 가장 높음
  - 런타임 의존성이 거의 없음
- 단점
  - correctness를 직접 관리해야 함
  - KL-3, KL-5, KL-10, KL-14 등 known limitations가 아직 남아 있음

#### Yosys CXXRTL

- 장점
  - 단기간에 실제 동작하는 standalone C++ 모델 확보 가능
  - UART/v2p에서 빠른 bridge로 검증 가치가 확인됨
- 단점
  - CXXRTL runtime header 의존
  - 장기적으로 사내 최적화/커스텀 분석을 넣기 어렵다

#### CIRCT arcilator

- 장점
  - CIRCT 생태계 내부 해법이라는 방향성
- 단점
  - 이 저장소의 일반적인 FSM/branch-heavy 케이스에서는 범용 경로로 막힘

---

## 5. 성능 비교를 어떻게 읽어야 하는가

### 5.1 같은 표에 넣어도 되는 것

- `357.8s -> 27.9s`
- `10 seeds x 1000 cycles`
- `5/5 compare pass`
- `37/39 생성`
- `182~219 mismatch`

이 숫자들은 모두 **저장소 실측(A)** 에 가깝다.

### 5.2 같은 표에 넣으면 안 되는 것

- `GPT vs OPUS vs Sonnet`의 속도/정확도 서열
- `NVIDIA GEM 5-64x vs Verilator`

이 둘은 각각 **저장소 근거 부재** 또는 **외부 조사 인용(C)** 이므로, 본문에서는 별도 표시가 필요하다.

### 5.3 권장 서술 방식

- 좋은 표현:
  - "Phase 3에서는 GPT/OPUS/Sonnet의 3-모델 교차 리뷰가 blind spot 탐지에 유효했다."
  - "Phase 4 이후 HIRCT 경로의 전수 시간은 357.8초에서 27.9초로 감소했다."
  - "CXXRTL은 UART와 v2p에서 단기 브릿지로 유효함이 확인되었다."

- 피해야 할 표현:
  - "Sonnet이 GPT보다 더 정확했다."
  - "CXXRTL이 GenModel보다 우수하다."
  - "arcilator는 쓸모없다."

프로젝트 근거는 **용도별 적합성**을 말해 주지, 보편적 우열을 확정해 주지는 않는다.

---

## 6. 프롬프트 운영 팁: 실제로 효과가 있었던 패턴

### 6.1 가장 효과적인 프롬프트 구조

| 항목 | 왜 중요한가 | 권장 형태 |
|------|-------------|----------|
| 세션 목표 1개 | 범위 폭주 방지 | "이번 세션 목표: UART seed=42 mismatch 0" |
| minimum pass bar 1개 | 완료 기준 고정 | "`make test-compare SEED=42 CYCLES=1000` exit 0" |
| branch/cwd 명시 | 환경 drift 방지 | "`feat/uart-genmodel`, cwd 절대 경로 명시" |
| 수정 범위 + 비범위 | 과잉 구현 방지 | "`lib/Target/GenModel/`만, GenDPIC 제외" |
| 재현 명령 | 가설 선행 방지 | "이 명령으로 실패를 먼저 재현" |
| 확인 artifact | 증거 기반 디버깅 | "IR dump + 생성 C++ 코드 확인" |
| stretch 분리 | 검증 기준선 유지 | "minimum 통과 후 5seed 10000cyc" |
| handoff 형식 | 다음 세션 연결 | "다음 첫 명령 + 남은 blocker + 재현 명령" |

### 6.2 잘 먹힌 운영 패턴

1. **plan-readiness-check 선행**
   - stale plan 상태에서 바로 구현하면 세션 시간이 급격히 새나갔다.
   - 실제 blocker 분류에서 Pattern A의 핵심 원인이었다.

2. **증거 2종 확보 후 root cause 주장**
   - IR/생성 코드/로그 중 최소 2개를 보고 수정할 때 실패율이 줄었다.
   - "가설부터 세우는 디버깅"이 가장 비효율적인 패턴으로 기록되었다.

3. **minimum 통과 후 stretch**
   - `seed=42 1000cyc` 같은 최소 기준을 먼저 고정해야 했다.
   - 검증 범위가 중간에 확장되면 완료 판정이 흔들렸다.

4. **fresh subagent + spec review -> quality review**
   - 구현보다 먼저 요구사항 누락을 잡고, 그 다음 코드 품질을 보는 순서가 안정적이었다.
   - 상시 병렬 구현보다 task 단위 순차 실행이 컨텍스트 오염을 줄였다.

5. **phase closeout에서 3-모델 교차 리뷰**
   - 구현 중 상시 사용보다, 문서/범위/리스크 누락 점검용 감사 단계에서 유효했다.

### 6.3 실패를 많이 부른 프롬프트 패턴

| 나쁜 패턴 | 실제로 생긴 문제 | 개선 방법 |
|----------|------------------|----------|
| "이 버그 고쳐줘" | 가설 선행, 반복 시도 | 재현 명령 + 증상 + seed/cycle 지정 |
| "plan 따라서 진행해" | stale plan cascade | "현재 상태와 맞는지 먼저 확인" |
| "전부 테스트해" | 기준선 흔들림 | minimum과 stretch 분리 |
| cwd/절대 경로 없음 | build/bin 혼동, setup-env 오판 | 절대 경로 또는 working_directory 명시 |
| 추측성 BLOCKED | 실행 증거 없는 중단 | 먼저 실행 후 실패 로그 확보 |
| 목표 여러 개 | 어느 것도 닫히지 않음 | 세션 목표 1개 고정 |

---

## 7. 재사용 가능한 프롬프트 템플릿

### 7.1 기능/행동 변경

```text
@docs/plans/<관련 plan 파일> 을 기반으로 <기능명>을 구현해줘.

## 세션 목표
<1줄 목표>

## 완료 기준
- minimum pass bar: <명령 + 기대 결과>

## 실행 컨텍스트
- 브랜치: <branch명>
- cwd: <절대 경로>

## 수정 범위
- 대상: <파일/디렉토리>
- 비범위: <제외 항목>

## 실행 원칙
- plan-readiness-check 통과 후 시작
- 완료 전 verification-before-completion 실행
- 종료 시 다음 첫 명령이 포함된 handoff 작성
```

### 7.2 버그 디버깅

```text
<증상 1줄 설명>을 근본원인부터 분석해줘.

## 세션 목표
<버그 해결 1줄>

## 재현 정보
- 재현 명령: <command>
- 증상: <무엇이 / 어디서 / 어떻게 틀렸는지>
- 대상 signal/seed/cycle: <범위>

## 완료 기준
- minimum pass bar: <명령 + 기대 결과>

## 실행 컨텍스트
- 브랜치: <branch명>
- cwd: <절대 경로>

## 확인할 artifact
- IR dump / 생성 코드 / 비교 로그 중: <무엇을 볼지>

## 실행 원칙
- 가설 수립 전 재현 + 2종 증거 확보
- minimum 통과 후에만 stretch 검증
- 종료 시 남은 blocker + 재현 명령 포함
```

### 7.3 문서/감사 작업

```text
docs/plans와 docs/report의 정합성을 점검하고 동기화해줘.

## 세션 목표
plan↔report 불일치 0건 달성

## 대상 범위
- docs/plans/...
- docs/report/...

## 실행 원칙
- 실측 수치와 근거 링크 기준으로만 수정
- 주관적 완료 선언 금지
- 종료 시 다음 세션이 바로 시작 가능한 handoff 작성
```

---

## 8. 추천 운영안

### 8.1 AI 모델 사용 전략

- **구현 중**: 모델 하나에 과하게 기대하지 말고, 프롬프트 구조와 검증 게이트를 먼저 강화한다.
- **감사 시점**: GPT/OPUS/Sonnet 같은 복수 모델 교차 리뷰를 사용해 blind spot을 줄인다.
- **문서화**: AI 모델 결과는 정량 표보다 "어떤 역할에 유효했는가"를 중심으로 적는다.

### 8.2 실행 도구 사용 전략

- **정확성 기준선**: Verilator
- **사내 통합 대상**: HIRCT GenModel
- **단기 ROI/브릿지**: CXXRTL
- **범용 주 경로 제외**: arcilator 단독
- **현재 핵심 투자 방향**: CIRCT in-process + HIRCT custom pass

### 8.3 다음 리포트에서 추가로 채우면 좋은 것

1. GPT/OPUS/Sonnet 동일 프롬프트 A/B 테스트
2. CXXRTL vs GenModel build/run 시간의 동일 입력 직접 비교
3. 대표 모듈 5개에 대한 공통 scorecard
4. 프롬프트 템플릿별 세션 완료율/재지시율 측정

---

## 9. 부록: 인용 가능한 세션

- [TXD Readiness Recovery](c02b7990-2ae4-4489-a6ee-9c00ddb9f2f8): stale plan, readiness No-Go, dirty worktree 정리
- [Array Inject Recovery](ffd27fc3-3939-42c1-95cc-407b64e661cd): `hw.array_inject` 대응, GenModel vs CXXRTL 비교 맥락
- [UART Compare Retry](22a129a2-498d-4cfe-a980-c19c12ccac2d): UART compare 기준선과 다음 세션 프롬프트 설계
- [CXXRTL DPI Bringup](a54603ae-5d9a-4a9b-adc6-d91c370ba422): CXXRTL을 DPI/VCS/NC 인프라에 연결한 실무 기록
- [IR Clock Domain Fix](c7c8b341-7a15-49fd-a088-fee84b71744a): 이름 휴리스틱 대신 IR-derived SSOT로 전환한 설계 교훈

---

## 10. 최종 요약

이 프로젝트에서 가장 큰 성과는 "어떤 모델이 최고인가"를 찾은 것이 아니라, **어떤 문제에 어떤 모델/도구를 배치해야 하는지**를 배운 데 있다.

- AI 모델은 **교차 리뷰와 감사**에서 가치가 컸다.
- Verilator는 **기준선**이었다.
- CXXRTL은 **단기 브릿지**로 유효했다.
- HIRCT GenModel은 **통합 중심 자산**으로 남았다.
- Phase 4의 CIRCT 내장 전환은 **속도, 커버리지, 유지보수성** 모두에서 가장 큰 전환점이었다.

그리고 반복 blocker를 가장 많이 줄인 것은 모델 교체보다도,
**`목표 1개 + minimum pass bar 1개 + cwd/branch + evidence-first + stretch 분리`**
라는 프롬프트/운영 규율이었다.
