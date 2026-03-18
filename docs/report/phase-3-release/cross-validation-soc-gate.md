# Cross-Validation SoC Gate — Task 304

**Date**: 2026-02-24
**Status**: SKIP (CModel 미생성)

---

## 1. SoC Gate 결과

| Module | CModel | 상태 | 사유 |
|--------|--------|------|------|
| Fadu_K2_S5_RocketTile | N | SKIP | hw.instance 깊이 → CModel 미생성 |
| Fadu_K2_S5_CoreIPSubsystem | N | SKIP | hw.instance 깊이 → CModel 미생성 |

## 2. 실행 증거

```
$ ls output/plat/src/s5/design/Fadu_K2_S5_RocketTile/Fadu_K2_S5_RocketTile/cmodel/
ls: cannot access '...': No such file or directory

$ ls output/plat/src/s5/design/Fadu_K2_S5_CoreIPSubsystem/Fadu_K2_S5_CoreIPSubsystem/cmodel/
ls: cannot access '...': No such file or directory
```

## 3. 미생성 원인

- `Fadu_K2_S5_RocketTile`과 `CoreIPSubsystem`은 다수의 `hw.instance`를 포함
- 현재 GenModel은 CIRCT `--hw-flatten-modules` pass 후 flat IR만 처리
- 깊은 계층 구조의 모듈은 flatten 실패 → CModel 생성 불가
- `known-limitations.md`에 해당 제한사항 기록 완료

## 4. SoC Gate 확대 조건

1. GenModel의 계층적 hw.instance 직접 지원 (compositional CModel)
2. 또는 대상 SoC RTL의 flat 버전 확보 (synthesis flatten 결과)
3. SRAM 매크로 stub 파일 완성 (`known-limitations.md`의 SRAM macro 항목)

## 5. TB 경로

- `vcs-cosim/tb/soc/` — 디렉토리 생성 완료, TB 미작성 (CModel 부재)

## 6. 결론

- SoC gate는 현재 **SKIP** — CModel 생성 불가로 실행 불가능
- GenModel 계층 지원 확장 후 재시도 대상
- 인프라(디렉토리 구조, Makefile 타겟)는 구축 완료
