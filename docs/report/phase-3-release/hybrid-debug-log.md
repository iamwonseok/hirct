# Hybrid Queue/FIFO Systematic Debug Log

## Logging Template (Required Fields)

각 디버그 사이클은 아래 필드를 반드시 포함한다.

- command
- seed / cycle setting
- exit code
- representative mismatch port(s)
- single hypothesis
- single change
- rerun result

---

## Entry 2026-02-23-01

- command: `build/bin/hirct-verify rtl/plat/src/s5/design/Fadu_K2_S5_Queue_11.v --lib-dir rtl/lib/stubs --seeds 3 --cycles 100`
- seed / cycle: `seeds=3`, `cycles=100`
- exit code: `2`
- representative mismatch port(s): `io_deq_bits` (cycle 0~9, seed 1)
- observed excerpt:
  - `FAIL cycle=0 seed=1 port=io_deq_bits rtl=202 model=0`
  - `FAIL cycle=3 seed=1 port=io_deq_bits rtl=0 model=436`
  - `FAIL cycle=9 seed=1 port=io_deq_bits rtl=1669 model=324`

### Single-Hypothesis Loop

- hypothesis: mismatch is not seed-distribution artifact; deterministic Queue/FIFO semantic mismatch appears immediately from early cycles.
- single change: reduce run scope to one fixed seed while keeping cycle depth (`--seeds 1 --cycles 100`) to isolate randomness factor.
- rerun command: `build/bin/hirct-verify rtl/plat/src/s5/design/Fadu_K2_S5_Queue_11.v --lib-dir rtl/lib/stubs --seeds 1 --cycles 100`
- rerun result:
  - exit code `2` (still failing)
  - same representative mismatch (`io_deq_bits`, cycle 0~9)
- conclusion: failure is reproducible and not dependent on multi-seed scheduling; next loop should target queue read/dequeue temporal semantics in generated cmodel.
