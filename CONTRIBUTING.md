# Contributing to HIRCT

> Phase 0 stub — Phase 3에서 완성 예정.

## Quick Start

```bash
make setup        # 도구 설치 + 환경 검증
make build        # hirct-gen / hirct-verify 빌드
make lint         # 코드 스타일 검사
make check-hirct  # lit 단위 테스트
```

## Code Style

- C/C++: `.clang-format` (LLVM 기반), `.clang-tidy` (정적 분석)
- Verilog/SV: `verible-verilog-lint`
- Python: `black` + `flake8` + `mypy`
- Shell: `shellcheck` (setup-env.sh only)
- Makefile: 프로젝트 컨벤션 (`.cursor/convention/make.md`)

## Tool Versions

검증된 도구 버전은 `tool-versions.env`에 기록되어 있습니다.
`make setup --strict` 로 핀 버전과의 일치를 강제할 수 있습니다.

## Branch Strategy

| Phase | 브랜치 |
|-------|--------|
| Phase 0 | `feature/hirct-phase0` |
| Phase 1A | `feature/hirct-phase1a` (또는 Phase 0 연장) |
| Phase 1B~3 | Phase별 분기 또는 연장 |

main에는 Phase 3 완료 후에만 머지합니다.

## License

Apache License 2.0 with LLVM Exceptions. `LICENSE` 파일을 참조하세요.
