#!/usr/bin/env python3
"""Run verification traversal on all gen-model=pass modules.

Usage:
    python3 utils/run-verify-traversal.py [options]

Options:
    --report PATH     Input report.json path (default: output/report.json)
    --output PATH     Output verify-report.json path (default: output/verify-report.json)
    --seeds N         Number of seeds (default: 10)
    --cycles N        Cycles per seed (default: 1000)
    --jobs N          Parallel jobs (default: 1)
    --timeout N       Timeout per seed in seconds (default: 300)
    --dry-run         Show which modules would be verified, don't run

Reads output/report.json, filters modules where gen-model=pass, runs
``make test-verify`` for each seed, and produces output/verify-report.json.
"""

from __future__ import annotations

import argparse
import json
import os
import signal
import subprocess
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

_print_lock = threading.Lock()

MAX_STDERR_CHARS = 4096


def _tprint(*args: Any, **kwargs: Any) -> None:
    """Thread-safe print (always flushes)."""
    kwargs.setdefault("flush", True)
    with _print_lock:
        print(*args, **kwargs)


def load_report(path: str) -> dict[str, Any]:
    """Load the input report.json."""
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def extract_qualifying_modules(report: dict[str, Any]) -> list[dict[str, Any]]:
    """Return file entries where gen-model=pass."""
    return [
        entry
        for entry in report.get("files", [])
        if entry.get("emitters", {}).get("gen-model") == "pass"
    ]


def module_output_dir(meta_path: str) -> str:
    """Derive the module output directory from a meta.json path.

    Example:
        "output/.../mod/meta.json" -> "output/.../mod"
    """
    if meta_path.endswith("/meta.json"):
        return meta_path[: -len("/meta.json")]
    return str(Path(meta_path).parent)


def _kill_pgroup(pid: int) -> None:
    """Best-effort kill of the process group rooted at *pid*."""
    try:
        os.killpg(os.getpgid(pid), signal.SIGKILL)
    except (ProcessLookupError, PermissionError, OSError):
        pass


def run_seed(
    module_dir: str,
    seed: int,
    cycles: int,
    timeout: int,
) -> dict[str, Any]:
    """Run ``make test-verify`` for one seed and return a result dict."""
    cmd = [
        "make",
        "-C",
        module_dir,
        "test-verify",
        f"SEED={seed}",
        f"CYCLES={cycles}",
    ]
    start = time.monotonic()
    try:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            start_new_session=True,
        )
        try:
            _, stderr = proc.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            _kill_pgroup(proc.pid)
            try:
                proc.wait(timeout=30)
            except subprocess.TimeoutExpired:
                pass
            elapsed = round(time.monotonic() - start, 2)
            return {
                "seed": seed,
                "result": "fail",
                "cycles": cycles,
                "elapsed_s": elapsed,
                "error": f"timeout after {timeout}s",
            }

        elapsed = round(time.monotonic() - start, 2)
        result: dict[str, Any] = {
            "seed": seed,
            "result": "pass" if proc.returncode == 0 else "fail",
            "cycles": cycles,
            "elapsed_s": elapsed,
        }
        if proc.returncode != 0:
            trimmed = stderr.strip()[:MAX_STDERR_CHARS]
            if trimmed:
                result["error"] = trimmed
        return result
    except OSError as exc:
        elapsed = round(time.monotonic() - start, 2)
        return {
            "seed": seed,
            "result": "fail",
            "cycles": cycles,
            "elapsed_s": elapsed,
            "error": str(exc),
        }


def verify_module(
    entry: dict[str, Any],
    seeds: int,
    cycles: int,
    timeout: int,
    index: int,
    total: int,
) -> dict[str, Any]:
    """Run verification for one module across all seeds.

    Returns a module-level result dict suitable for verify-report.json.
    """
    name = entry.get("top") or "unknown"
    meta_path = entry["path"]
    mod_dir = module_output_dir(meta_path)

    module_result: dict[str, Any] = {
        "name": name,
        "path": meta_path,
        "status": "pass",
        "seeds": [],
    }

    if not os.path.isdir(mod_dir):
        module_result["status"] = "skip"
        _tprint(f"[verify] {index}/{total} {name}: SKIP (dir not found)")
        return module_result

    for s in range(1, seeds + 1):
        seed_result = run_seed(mod_dir, s, cycles, timeout)
        module_result["seeds"].append(seed_result)
        _tprint(
            f"[verify] {index}/{total} {name}: "
            f"seed {s}/{seeds} {seed_result['result']} "
            f"({seed_result['elapsed_s']}s)"
        )

        if s == 1 and seed_result["result"] == "fail":
            verify_bin = os.path.join(mod_dir, f"verify_{name}")
            if not os.path.isfile(verify_bin):
                module_result["status"] = "skip"
                _tprint(
                    f"[verify] {index}/{total} {name}: "
                    f"SKIP (compilation failed, skipping remaining seeds)"
                )
                break

    if module_result["status"] != "skip":
        has_fail = any(sr["result"] == "fail" for sr in module_result["seeds"])
        module_result["status"] = "fail" if has_fail else "pass"

    return module_result


def _verify_module_star(
    args: tuple[dict[str, Any], int, int, int, int, int],
) -> dict[str, Any]:
    """Unpack positional args for verify_module (ThreadPoolExecutor)."""
    return verify_module(*args)


def run_traversal(
    report_path: str,
    output_path: str,
    seeds: int,
    cycles: int,
    jobs: int,
    timeout: int,
    dry_run: bool,
) -> int:
    """Orchestrate the full verification traversal."""
    if not os.path.isfile(report_path):
        print(f"ERROR: report not found: {report_path}", file=sys.stderr)
        return 1

    report = load_report(report_path)
    qualifying = extract_qualifying_modules(report)
    total = len(qualifying)

    print(f"[verify] Found {total} modules with gen-model=pass")
    print(
        f"[verify] Config: seeds={seeds}, cycles={cycles}, "
        f"jobs={jobs}, timeout={timeout}s"
    )

    if dry_run:
        print(f"\n[dry-run] Would verify {total} modules:")
        for i, entry in enumerate(qualifying, 1):
            name = entry.get("top") or "unknown"
            mod_dir = module_output_dir(entry["path"])
            print(f"  {i:4d}. {name}")
            print(f"        dir: {mod_dir}")
        est_runs = total * seeds
        print(f"\n[dry-run] Total: {total} modules x {seeds} seeds = {est_runs} runs")
        print(f"[dry-run] Each run: {cycles} cycles, {timeout}s timeout")
        return 0

    wall_start = time.monotonic()
    results: list[dict[str, Any]] = []

    if jobs <= 1:
        for i, entry in enumerate(qualifying, 1):
            results.append(verify_module(entry, seeds, cycles, timeout, i, total))
    else:
        task_args: list[tuple[dict[str, Any], int, int, int, int, int]] = [
            (entry, seeds, cycles, timeout, i, total)
            for i, entry in enumerate(qualifying, 1)
        ]
        with ThreadPoolExecutor(max_workers=jobs) as executor:
            futures = [executor.submit(_verify_module_star, ta) for ta in task_args]
            for future in as_completed(futures):
                results.append(future.result())
        results.sort(key=lambda r: r["path"])

    wall_elapsed = time.monotonic() - wall_start

    pass_count = sum(1 for r in results if r["status"] == "pass")
    fail_count = sum(1 for r in results if r["status"] == "fail")
    skip_count = sum(1 for r in results if r["status"] == "skip")

    verify_report: dict[str, Any] = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "seeds": seeds,
        "cycles": cycles,
        "total_modules": total,
        "pass": pass_count,
        "fail": fail_count,
        "skip": skip_count,
        "modules": results,
    }

    os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(verify_report, f, indent=2, ensure_ascii=False)

    pct = (pass_count / total * 100) if total > 0 else 0.0
    print(f"\n{'=' * 50}")
    print(f"[verify] Traversal complete in {wall_elapsed:.1f}s")
    print(f"[verify]   Total:  {total}")
    print(f"[verify]   Pass:   {pass_count} ({pct:.1f}%)")
    print(f"[verify]   Fail:   {fail_count}")
    print(f"[verify]   Skip:   {skip_count}")
    print(f"[verify] Report written to: {output_path}")
    return 0


def main() -> int:
    """CLI entry point."""
    parser = argparse.ArgumentParser(
        description="Run verification traversal on gen-model=pass modules"
    )
    parser.add_argument(
        "--report",
        default="output/report.json",
        help="Input report.json path (default: output/report.json)",
    )
    parser.add_argument(
        "--output",
        default="output/verify-report.json",
        help="Output verify-report.json path (default: output/verify-report.json)",
    )
    parser.add_argument(
        "--seeds",
        type=int,
        default=10,
        help="Number of seeds (default: 10)",
    )
    parser.add_argument(
        "--cycles",
        type=int,
        default=1000,
        help="Cycles per seed (default: 1000)",
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=1,
        help="Parallel jobs (default: 1)",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=300,
        help="Timeout per seed in seconds (default: 300)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show which modules would be verified, don't run",
    )
    args = parser.parse_args()

    if args.seeds < 1:
        print("ERROR: --seeds must be >= 1", file=sys.stderr)
        return 1
    if args.cycles < 1:
        print("ERROR: --cycles must be >= 1", file=sys.stderr)
        return 1
    if args.jobs < 1:
        print("ERROR: --jobs must be >= 1", file=sys.stderr)
        return 1
    if args.timeout < 1:
        print("ERROR: --timeout must be >= 1", file=sys.stderr)
        return 1

    return run_traversal(
        report_path=args.report,
        output_path=args.output,
        seeds=args.seeds,
        cycles=args.cycles,
        jobs=args.jobs,
        timeout=args.timeout,
        dry_run=args.dry_run,
    )


if __name__ == "__main__":
    sys.exit(main())
