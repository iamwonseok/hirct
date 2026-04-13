# V1 Export Roadmap

**목적:** `hirct-gen`의 `export-cmodel`과 `export-systemc-wrapper`를 flatten-first incremental 방식으로 발전시켜, 최종적으로는 hierarchy-preserving C model export와 top-level만 wrapper를 제공하는 M4 상태까지 도달한 뒤 baseline으로 승격한다.

**현재 truthful contract:** [`docs/v1-scope.md`](../v1-scope.md)는 현재 구현의 계약 문서로 유지한다. 이 문서는 "지금 되는 것/안 되는 것"을 고정하고, 본 로드맵은 "다음에 무엇을 만들 것인가"를 정의한다.

**핵심 관점:**
- `C model`은 독립적인 simulator에 직접 연결 가능한 산출물이어야 한다.
- `SystemC wrapper`는 그 C model을 재사용하는 top-level integration layer여야 한다.
- wrapper가 항상 필수인 것이 아니라, 필요 시 top-level에만 얹는 선택적 계층으로 본다.

## 용어

- `current export top`: 각 중간 배치에서 export 대상으로 찍는 모듈
- `final integration top`: 최종적으로 wrapper를 얹는 사용자 진입 모듈

## 현재 상태

> **Updated:** 2026-04-13 (M4 closure)

- `M0` truthful baseline: **closed**
- `M1` flatten-first exporter 안정화: **closed**
- `M2` flatten-capable composition contract: **closed**
- `M3` hierarchical C model export: **closed**
- `M4` final integration top + top-level SystemC wrapper: **closed** — 서버 검증 완료 (144/144 unit, 91/91 lit)

즉, 지금까지의 작업은 헛돈 것이 아니라 `small flattened export top`을 안정적으로 export하는 기반을 단단히 만든 단계로 본다.

## 대략 추정

- `M0`: 0.5~1 day
- `M1`: 1~2 days
- `M2`: 2~4 days
- `M3`: 1~2 weeks
- `M4`: 3~5 days

## 리스크 메모

- 가장 큰 리스크는 `M3`다.
- `M3`는 instance DAG 추출, cross-module reference 해석, eval orchestration, cycle reject 정책을 모두 요구한다.
- `M2`는 문서 contract 중심 milestone로 유지하되, 필요 시 `M3` 위험을 줄이기 위한 spike/prototype을 별도로 허용한다.
- `M4`의 multi-clock / wide-I/O wrapper 정책은 `M3` 완료 시점에 명시적으로 결정한다.
- `M0`, `M1`의 lit 검증은 runner 존재만으로 충분하지 않다. generated `lit.site.cfg.py`가 있는 configured build tree 또는 동등한 substitution 환경이 필요하다.

## M0. Truthful Baseline 고정

**목적:** 현재 구현, 테스트, 문서(`v1-scope.md`)가 서로 어긋나지 않는 baseline을 확정한다.

**체크리스트**
- [ ] `v1-scope.md`의 supported/unsupported 항목을 현재 구현과 다시 대조한다.
- [ ] `known-limitations.md`와 SSOT 관계를 정리한다.
- [ ] export 관련 lit test와 문서 주장을 1:1로 연결한다.
- [ ] "현재 되는 것/안 되는 것"을 과장 없이 다시 고정한다.

**기대 결과**
- 현재 branch 상태를 설명하는 truthful contract가 생긴다.
- 이후 배치는 모두 이 baseline 대비 "새 capability"를 추가하는 방식으로 정의된다.

## M1. Flatten-First Exporter 안정화

**목적:** 작은 모듈을 `current export top`으로 잡았을 때 C model / wrapper 경로가 안정적으로 동작하도록 flatten 기반 exporter를 단단히 만든다.

**범위**
- small flattened module
- single-top export
- C model artifact correctness
- wrapper의 현재 v1 제약(single-clock, <=64-bit I/O) 유지

**체크리스트**
- [ ] M1에서 이미 닫힌 hardening 항목과 미완료 항목을 분리 정리한다.
- [ ] 작은 single-top flattened 모듈 기준 export artifact contract를 명문화한다.
- [ ] `export-cmodel` / `export-systemc-wrapper`의 현재 CLI contract를 문서와 테스트로 고정한다.
- [ ] 남은 bugfix/hardening이 실제로 M1 범위인지 확인하고, 범위 밖이면 backlog로 보낸다.

**기대 결과**
- flatten-first small module exporter가 신뢰 가능한 기반이 된다.
- 이후 M2에서 composition contract를 얹을 수 있다.

## M2. Flatten-Capable Composition Contract

**목적:** hierarchy-preserving으로 바로 뛰지 않고, flatten 가능한 작은 모듈 산출물을 조합 가능한 artifact contract로 정리한다.

**핵심 질문**
- 모듈별 산출물은 어떤 파일/심볼/인터페이스 contract를 가져야 하는가?
- top을 나중에 바꿔도 하위 산출물은 재사용 가능한가?
- linking/composition 시 어떤 naming / include / ownership 규칙을 가져야 하는가?

**체크리스트**
- [ ] per-module artifact layout을 정의한다.
- [ ] generated header/cpp naming 규칙과 include contract를 정의한다.
- [ ] top이 아닌 중간 모듈도 export 대상으로 찍을 수 있는지 contract를 정리한다.
- [ ] linking/composition 관점의 최소 integration fixture를 설계한다.
- [ ] M2 acceptance test를 정의한다.

**기대 결과**
- "작은 모듈을 먼저 성공시키고 나중에 조합한다"는 경로가 문서화된다.
- hierarchy를 직접 구현하지 않아도, 그 직전 단계까지의 연결 규칙이 고정된다.
- 필요하면 `M3` 리스크를 낮추기 위한 spike/prototype 범위가 별도로 식별된다.

## M3. Hierarchical C Model Export

**목적:** flatten하지 않고도 각 모듈을 개별 artifact로 export하고, 계층 구조를 유지한 채 조합 가능한 C model export를 지원한다.

**핵심 방향**
- 각 모듈은 독립 C model 산출물을 가진다.
- parent는 child artifact를 참조/조합한다.
- generated 결과는 독립 simulator에도 연결 가능해야 한다.

**체크리스트**
- [ ] module graph traversal 범위를 정의한다.
- [ ] instance DAG 추출 알고리즘과 cycle reject 정책을 정의한다.
- [ ] parent/child artifact 생성 규칙을 정한다.
- [ ] cross-module expression / output reference 해석 전략을 정의한다.
- [ ] submodule instance composition 방식(생성, 연결, step/eval orchestration)을 정의한다.
- [ ] hierarchical export acceptance fixture를 추가한다.
- [ ] flatten-first 경로와 hierarchical 경로의 계약 차이를 문서화한다.

**기대 결과**
- 각 파일(모듈)을 변환하고 연결해 하나의 실행 가능한 C model 집합으로 만드는 경로가 생긴다.
- hierarchical support가 단순 문구가 아니라 실제 artifact/link contract로 검증된다.

## M4. Final Integration Top + Top-Level SystemC Wrapper

**목적:** 최종 integration top에 대해서만 wrapper를 제공하고, 하위 모듈은 generated C/C++ artifact로 유지하는 최종 export 구조를 완성한다.

**핵심 방향**
- wrapper는 `final integration top`에만 제공한다.
- 하위 모듈들은 wrapper 내부 구성요소가 아니라 독립 산출물로 유지한다.
- top-level wrapper는 orchestration 계층이며, C model backend를 직접 재사용한다.

**체크리스트**
- [x] wrapper 생성 대상을 `final integration top`으로 한정한다.
- [x] 하위 모듈 artifact와 top wrapper 사이 interface contract를 고정한다.
- [x] `M3` 완료 후 multi-clock / wide I/O wrapper 정책을 명시적으로 결정한다. → **deferred 유지로 결정**
- [x] `M4` 범위에서 계속 reject할지, 지원으로 승격할지, next-scope로 미룰지 문서에 고정한다. → **reject 유지, next-scope deferred**
- [x] end-to-end integration fixture와 acceptance test를 만든다.
- [x] M4 완료 조건과 baseline 승격 조건을 문서화한다.

**기대 결과**
- hierarchy-preserving C model export + top-level wrapper 제공이라는 최종 목표가 달성된다.
- 이 시점 이후 현재 작업선을 baseline으로 승격할 수 있다.

**Closure:** M4 closed (2026-04-13). 상세는 `2026-04-12-m4-top-wrapper-integration-plan.md` § M4 Closure Record 참조.

## 운영 규칙

- 모든 새 작업은 `v1-scope.md` 또는 이 로드맵의 특정 milestone 항목과 연결돼야 한다.
- 새 gap을 발견해도, 먼저 `M0~M4 어디에 속하는가`를 판단한 뒤에만 배치로 승격한다.
- worker 배치는 항상 한 milestone의 한 하위 capability만 닫는다.
- manager는 보고서 문구 교정보다 milestone closure 여부만 판단한다.
- M4 완료 전에는 baseline 승격을 하지 않는다.

## 권장 실행 순서

1. M0 truthful baseline 재감사 및 문서 정합 고정
2. M1에서 이미 완료된 hardening 범위와 남은 항목 정리
3. M2 artifact/layout/composition contract 설계
4. M2 acceptance fixture 정의 후 구현 배치 분리
5. M3 hierarchical export 설계/구현
6. M4 top-level wrapper integration 설계/구현
7. M4 완료 후 baseline 승격 판단
