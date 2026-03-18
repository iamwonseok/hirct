# Arc PoC Phase A 후속 작업 리포트

> **Date:** 2026-03-04
> **Status:** 구현 완료, 검증 통과
> **선행**: `docs/plans/2026-03-04-arc-poc-results.md` (Go/No-Go: CONDITIONAL GO)

## 1. HirctUnrollProcessLoops

| 항목 | 실측 |
|------|------|
| 구현 파일 | `hirct/lib/Transforms/HirctUnrollProcessLoops.cpp` (401줄) |
| 팩토리 | `hirct::create_unroll_process_loops_pass()` |
| 파이프라인 위치 | SimCleanup → **UnrollProcessLoops** → ProcessFlatten → SignalLowering |
| 지원 술어 | `slt`, `ult`, `eq` (fc6161 전수 조사에서 확인된 패턴 전부 커버) |
| 바운드 제한 | N < 1024 (fc6161 최대 N=73) |
| 대상 | `llhd.process` 내부 정적 바운드 for 루프 180/266건 |
| lit 테스트 | `hirct/test/Transforms/process-unroll-loops.mlir` — 3개 케이스 (LOOP+ARRAY slt, ult, LOOP+BITWISE slt) |
| pass-only 테스트 | `--run-pass unroll-process-loops` UNROLL check-prefix |
| 빌드 | ninja exit 0 |
| lit 결과 | 55/55 PASS (100%) |
| 커밋 | `be50882` feat: implement, `9c9083a` fix: code review, `e4ba61f` refactor: convention |

### 알고리즘 요약

CIRCT `UnrollLoops.cpp`의 `Loop` struct (match + unroll)를 `ProcessOp` 컨텍스트에 적응.
`CFGLoopInfo`로 루프 탐색 → 정적 바운드 match → body N회 clone → 유도변수 상수 치환 → dead block 제거 → trivial branch collapse.

## 2. verilator -E 전처리 통합

| 항목 | 실측 |
|------|------|
| 구현 파일 | `hirct/lib/Support/VerilatorPreprocessor.cpp` (85줄) + `hirct/include/hirct/Support/VerilatorPreprocessor.h` (31줄) |
| CLI 플래그 | `--preprocess <mode>` (none/verilator), `--verilator-path <path>` |
| filelist 확장 | `+define+`, `-y`, `-sverilog`, `-v`, `+incdir+` |
| API | `llvm::sys::findProgramByName`, `llvm::sys::ExecuteAndWait` (LLVM API 래핑) |
| CLI 테스트 | `hirct/test/Tools/hirct-gen/preprocess-flag.test` — help + invalid mode |
| 빌드 | ninja exit 0 |
| lit 결과 | 55/55 PASS |
| 커밋 | `8b5772a` feat: VerilatorPreprocessor, `b16c772` feat: CLI flags, `109e5e7` feat: parse_filelist, `c922e60` test |

### 해소된 문제

| 카테고리 | 문제 | verilator -E 효과 |
|----------|------|-------------------|
| L1-a | +define+ 미파싱 | PASS (mpm_bram exit 0) |
| L1-d | timescale | PASS (caliptra exit 0) |
| L2 | concat_ref legalize | PASS (i2cm exit 0, LLHD 잔존 96개는 별도 처리 필요) |

## 3. ProcessFlatten V2 (부수 작업)

| 항목 | 실측 |
|------|------|
| 구현 | `hirct/lib/Transforms/HirctProcessFlatten.cpp` — 재귀 flatten → 토폴로지 순서 알고리즘 |
| 핵심 수정 | (1) wait-arg 매핑 수정, (2) inverse_post_order O(n) 알고리즘, (3) SEQUENTIAL 패턴 감지 |
| 추가 테스트 | `process-flatten-diamond.mlir`, `process-flatten-dest-args.mlir` |
| 커밋 | `1f378fd` fix: wait-arg mapping, `d62e61c` refactor: topological-order, `f55932a` test: diamond+dest-args |

## 4. --run-pass CLI (부수 작업)

| 항목 | 실측 |
|------|------|
| 구현 | `tools/hirct-gen/main.cpp` — `create_pass_by_name()` 팩토리 |
| 지원 pass | sim-cleanup, unroll-process-loops, process-flatten, signal-lowering |
| 제약 | `--dump-ir` + `.mlir` 입력 필수 |
| pass-only lit | UnrollProcessLoops, ProcessFlatten 각 1건 |
| 커밋 | `dbb5051` feat: --run-pass, `2c280a7` test: UnrollProcessLoops, `5102e21` test: ProcessFlatten, `7641afa` docs: reference-commands |

## 5. 전체 빌드/테스트 증거

```
$ ninja -C hirct/build hirct-gen
[27/27] Linking CXX executable bin/hirct-gen   (exit 0)

$ ninja -C hirct/build check-hirct
Total Discovered Tests: 55
  Passed: 55 (100.00%)
```

## 6. 다음 단계 (Arc PoC 로드맵에서 도출)

| # | 작업 | 의존 | 상태 |
|---|------|------|------|
| 3 | Phase 2 재실행 — 전수 순회 테스트 (루프 언롤 전후 커버리지 비교) | #1 완료 | **완료** — GenModel failures 4→1, 시간 92.2%↓ |
| 4 | arcilator 종단 검증 — async reset 우회 후 arc-conv 통과 확인 | #1 완료 | 대기 |
| 5 | SKIP 53개 모듈 재분석 — verilator -E 경로 MLIR 생성 | #2 완료 | **완료** — 6/53 MLIR 성공, 4개 신규 (i2cm, smbus, axi_x2p1/2) |
| 6 | LLHD 잔존 op 처리 — fsl_wflow_fifo_reg llhd.prb/drv | #2 완료 | **완료** — ProcessDeseq V2: intermediate block clone + merge_args 수정. GenModel-specific failures 1→0 |
| 7 | Phase 2 전수 순회 (1,600 .v) | #1+#2 완료 | 대기 |
