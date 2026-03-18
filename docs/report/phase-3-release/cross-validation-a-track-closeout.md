# Task 304 A-Track Closeout (Status Audit)

**Date**: 2026-02-24  
**Scope**: Task 304 실행 인프라 + 실행 증거 체인 (A트랙)  
**Method**: plan↔report 동기화 감사

---

## 1) A-Track 종결 판정

- **확인됨**: A트랙 핵심 산출물(인프라/리포트/문서 링크)은 생성 완료
- **확인됨**: module/ip-top/soc 결과가 report 문서에 기록됨
- **확인됨**: `make test-all` exit 0로 회귀 없음 기록 존재
- **미확인**: G2/G3를 "완료"로 닫을 조건(대표 IP PASS, SoC smoke PASS)은 충족되지 않음

**판정**: A트랙은 "**증거 체인 구축 완료 + 게이트 일부 미달 상태로 종결**"이 적절함.

---

## 2) 완료 항목 (확인됨)

### 인프라
- `Makefile`에 `hybrid-ip-gate`, `hybrid-soc-gate`, `ncsim-cosim`, `cross-validation-report` 추가
- `vcs-cosim/tb/module/`, `vcs-cosim/tb/ip-top/`, `vcs-cosim/tb/soc/` 경로 분리
- VCS/ncsim 실행 환경 변수 및 IUS ABI workaround(`-static-libstdc++`) 반영

### 실행 증거 문서
- `docs/report/phase-3-release/cross-validation-module-gate.md`
- `docs/report/phase-3-release/cross-validation-ip-gate.md`
- `docs/report/phase-3-release/cross-validation-soc-gate.md`
- `docs/report/phase-3-release/cross-validation-diff-vcs-vs-ncsim.md`

### 계획 동기화
- `docs/plans/phase-3-release/304-vcs-ncsim-multi-level-cross-validation.md`에 실행 결과 반영
- `docs/plans/phase-3-release/README.md`, `docs/plans/summary.md` 링크 반영

---

## 3) 미진 항목 TODO LIST (A-Track 후속)

| ID | TODO | 상태 | 근거 |
|---|---|---|---|
| A-1 | `hybrid-ip-gate`에서 CLINT 외 TLPLIC/PeripheryBus VCS cosim 실행 증거 확보 | 미완료 | 현재 문서는 SKIP/미실행 |
| A-2 | `hybrid-soc-gate`에서 SoC smoke "실행" 증거 확보 (현재는 SKIP 사유 문서화만 존재) | 미완료 | CModel 미생성으로 실행 불가 |
| A-3 | ncsim 측 IP-top(CLINT) 실행 로그 추가 확보 | 미완료 | VCS 결과만 존재 |
| A-4 | `cross-validation-report` 타겟을 실제 diff 테이블 자동 집계형으로 고도화 | 부분완료 | 현재는 헤더/골격 생성 중심 |
| A-5 | PRNG 차이로 per-seed direct compare 불가 규칙을 `hirct-convention`/304 실행 섹션에 명문화 | 부분완료 | report에는 기술됨, 규약 문서 반영은 별도 |

---

## 4) 운영 메모

- **확인됨**: 기존 사용자 변경 파일(`lib/Target/GenModel.cpp`, `test/Target/GenModel/*`, `known-limitations.md`, `docs/plans/open-decisions.md`)은 되돌리지 않음
- **확인됨**: BLOCKED 선언 없이 실제 실행 로그로 상태 판정
- **확인됨**: waveform 증거 경로 존재  
  - `vcs-cosim/results/vcs/Queue_11_s1.vcd`

