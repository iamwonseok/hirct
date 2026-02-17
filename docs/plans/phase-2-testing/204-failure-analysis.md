# Task 204: 실패 분석 + Phase 1 되돌림

> **목표**: 201~203에서 수집한 모든 실패를 분류하고 수정 가능한 항목 해결
> **예상 시간**: 2일
> **산출물**: `failure-classification.md`, `known-limitations.md`, Phase 1 수정
> **피드백 루프**: MLIR 실패→문서화, 미지원 IR→101, emitter 버그→해당 1xx, 검증 mismatch→101/109, DPI-C→103

---

> **Agent 정의**: 이 문서에서 "Agent"는 두 가지를 포함한다:
> 1. **Cursor 서브에이전트**: `subagent-driven-development` 스킬로 코드 수정 실행 (Phase 1 되돌림)
> 2. **Rule-based triage 도구**: `utils/triage-failures.py` — 실패 자동 분류 (`206-agent-triage.md` 참조)
>
> Agent의 자동 수정 범위, 권한, 금지사항은 `206-agent-triage.md`에 정의된다.

## 목표

report.json, verify-report.json의 모든 실패를 분류하고, 각 카테고리별 조치를 수행한다. 수정 가능한 항목은 Phase 1 태스크로 되돌리고, 수정 불가 항목은 제한 사항으로 문서화한다.

## 주요 작업

- 실패 데이터 수집 (MLIR, emitter별, 검증 mismatch)
- 분류 체크리스트 작성: MLIR 생성 실패, 미지원 IR 연산, emitter 버그, 검증 mismatch
- **Verilator 버그 의심**: Phase 3 VCS 교차 검증 대상으로 등록 (verify-report.json의 FAIL 모듈 중 hirct-gen 수정으로 해소되지 않는 항목)
- MLIR 실패 샘플 분석 → known-limitations.md 문서화
- 미지원 IR op → 101-gen-model 되돌림
- emitter 버그 → 해당 1xx 태스크 되돌림
- 검증 mismatch → 101 또는 109 되돌림
- TDD로 되돌림 이슈 수정
- 영향받은 테스트 재실행
- failure-classification.md, known-limitations.md 최종 갱신

## Mismatch 디버깅 절차 가이드

Agent가 실패를 발견했을 때 따르는 구체적 절차:

### GenModel 실패 (meta.json: gen-model = "fail")

1. 해당 모듈의 정규화된 `.mlir` 파일 확인 (`output/<path>/<module>/<module>.mlir`)
2. `meta.json`에서 `unsupported_ops` 필드 확인 → 미지원 op 목록 추출
3. `lib/Target/GenModel.cpp`의 op 매핑 테이블에 해당 op 추가
4. `hirct-gen <module>.v --only model` → `g++ -c` → 컴파일 성공 확인
5. `test/Target/GenModel/` lit 테스트에 해당 op 테스트 추가

### Verify mismatch (verify-report.json: result = "fail")

1. 실패 시드와 사이클 번호 확인 (`seed=X, cycle=Y`)
2. `hirct-gen <module>.v --only model` → 생성된 C++ step() 로직 검토
3. MLIR에서 해당 사이클에 영향을 주는 op 추적
4. GenModel의 op 번역이 올바른지 확인 (비트폭 truncation, 부호 확장 등)
5. 수정 후 `make test-verify SEED=X`로 해당 시드만 재검증

### infra-error (meta.json 누락)

1. hirct-gen 실행 시 stderr 출력 확인
2. `circt-verilog` → MLIR 변환 단계 실패 여부 확인
3. 실패 시 `known-limitations.md`에 `parse_error` 카테고리로 등록

---

## 게이트 (완료 기준)

- [ ] 수정 가능한 실패 전부 해결
- [ ] 수정 불가 항목은 known-limitations.md에 문서화
- [ ] 피드백 루프가 Phase 1 해당 태스크에 명확히 매핑됨
- [ ] Verilator 버그 의심 모듈 목록 → Phase 3 (301) VCS 교차 검증 대상으로 전달
- [ ] XFAIL 리스트 확정, CI Green 상태 유지 확인
