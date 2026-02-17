# Task 201: 개별 파일 순회 + 리포트

> **목표**: rtl/**/*.v 전체를 hirct-gen으로 순회하여 per-file·per-emitter 결과 리포트 생성
> **예상 시간**: 3일
> **산출물**: `lit integration_test/traversal/`, `output/report.json`

---

## 목표

모든 .v 파일에 대해 `hirct-gen <file>.v`를 실행하고, 산출물은 소스 트리 구조를 반영한 `output/<path>/<file>/`에 생성한다. MLIR 생성·emitter별(GenModel, GenRAL 등) 성공/실패를 기록하여 `output/report.json`에 저장한다.

> **실측 참고** (Phase 0 Pre-test, CIRCT `5e760efa9` 기준):
> - 단일 파일 모드 파싱 성공: **590/1,597 (36%)**
> - 실패 원인: 대부분 `unknown module` (외부 모듈 의존성) — filelist 모드(`-f`)에서 해소 예상
> - 성공한 590파일에서 24종 IR op 확인 (상세: `risk-validation-results.md` §2)

## 주요 작업

- rtl/**/*.v 전체 파일 목록 수집
- 각 파일에 hirct-gen 실행 (최종 CLI: `hirct-gen input.v`)
- 출력: rtl/<path>/<file>.v → output/<path>/<file>/
- circt-verilog → MLIR 성공/실패 분기 및 기록
- emitter별(GenModel, GenRAL 등) 성공/실패 기록
- output/report.json 스키마: total_files, mlir_success, mlir_fail, per_emitter, files[]
- 순회 완료 후 요약 통계 출력
- 리포트 생성 경로: lit xunit XML + meta.json → `utils/generate-report.py` → report.json

## config/generate.f 생성

전체 순회 대상 파일 목록을 config/generate.f에 기록:

```bash
find rtl/ -name "*.v" -type f | sort > config/generate.f
```

이 파일은 `make generate`의 입력이 되며, 백업/임시 파일 포함을 방지하기 위해 와일드카드 대신 명시적 목록을 사용한다.

## 게이트 (완료 기준)

- [ ] `make test-traversal` (또는 `lit integration_test/traversal/`) 실행 시 `rtl/**/*.v` 전체 처리 (미처리 파일 0개)
- [ ] `output/report.json` 생성, 필수 키 포함:
  ```json
  {
    "total_files": N,
    "mlir_success": N,
    "mlir_fail": N,
    "per_emitter": {
      "gen-model": {"pass": N, "fail": N},
      "gen-tb": {"pass": N, "fail": N},
      ...
    },
    "files": [{"path": "...", "mlir": "pass|fail", "emitters": {...}}]
  }
  ```
- [ ] 산출물이 `output/<path>/<file>/` 구조로 생성됨
- [ ] `jq '.total_files' output/report.json` → 0보다 큰 수
