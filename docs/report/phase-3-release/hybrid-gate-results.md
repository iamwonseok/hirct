# Hybrid Gate Results

## Commands and Exit Codes

| command | exit code | note |
|---|---:|---|
| `make hybrid-module-gate` (before implementation) | 2 | `No rule to make target 'hybrid-module-gate'` |
| `make hybrid-module-gate` (after implementation) | 0 | hybrid smoke 실행, XFAIL 정책 유지 |
| `make hybrid-top-gate` | 0 | `module -> subsystem -> top` 의존 순서 통과 |

## Key Logs

- module gate run:
  - `Passed: 1`
  - `Expectedly Failed: 1`
  - summary: `[hybrid-module-gate] PASS (including expected XFAIL)`
- top gate run:
  - `[hybrid-subsystem-gate] Placeholder gate: module gate passed`
  - `[hybrid-top-gate] Placeholder gate: subsystem gate passed`

## Current KPI Snapshot

| KPI | value |
|---|---|
| module gate command availability | READY |
| module gate result | PASS (with expected XFAIL) |
| subsystem gate chaining | READY |
| top gate chaining | READY |
