# Task 110 Output Structure 게이트 검증 리포트

> **검증 일시**: 2026-02-18 ~ 2026-02-19
> **환경**: Ubuntu 24.04 (x86_64), kernel 6.14.0-27-generic
>
> **역할**: 이 파일은 게이트 실측 데이터(근거)만 기록한다.
> 체크리스트 진행 상태는 `docs/plans/phase-1-pipeline/110-output-structure.md`를 참조.

---

## 종합 판정: [V] ALL PASS (4/4)

| # | 게이트 | 결과 | 실측 |
|---|--------|------|------|
| G01 | hirct-gen LevelGateway.mlir → output/Fadu_K2_S5_LevelGateway/ 생성 | [V] PASS | 2026-02-18 9종 서브디렉토리 + 16파일 |
| G02 | 8종 서브디렉토리 존재 | [V] PASS | 2026-02-18 cmodel/tb/dpi/wrapper/rtl/doc/cocotb/ral/verify 확인 |
| G03 | Makefile 자동 생성 확인 | [V] PASS | 2026-02-18 test-compile/test-verify/test-artifacts/test 타겟 포함 |
| G04 | hirct-gen -f filelist.f --top → 산출물 생성 | [V] PASS | 2026-02-19 multi-file + --top → 16파일 정상 생성 |
