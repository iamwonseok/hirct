#!/usr/bin/env python3
"""Survey RTL IP blocks with hirct-gen using filelists, classify errors.

Reads config/survey-targets.txt, runs hirct-gen per filelist (like the
UART build), captures stderr, classifies failures into GenModel-specific
categories, and produces a taxonomy report.

Usage:
    # Quick test: UART only
    python3 utils/survey-errors.py --limit 1

    # Full survey: all RTL IP blocks from config
    python3 utils/survey-errors.py

    # Custom config
    python3 utils/survey-errors.py --targets config/survey-targets.txt
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path


# ---------------------------------------------------------------------------
# Error classification (order matters: first match wins)
# ---------------------------------------------------------------------------

CATEGORIES = [
    # GenModel-specific
    ("unresolved_process", re.compile(r"(\d+)\s+LLHD\s+process\(es\)\s+unresolved")),
    ("unresolved_drive", re.compile(r"(\d+)\s+LLHD\s+drive\(s\)\s+unresolved")),
    ("flatten_cycle", re.compile(r"flatten.*cycle|back.?edge|depth\s+limit", re.I)),
    ("val_map_gap", re.compile(r"body.?level result not in val|missing from val", re.I)),
    ("emit_fail", re.compile(r"failed to emit|emit.*failed|cannot emit", re.I)),

    # Clock/reset
    ("clock_domain_error", re.compile(r"clock\s+domain|multi.?clock|clk.*mismatch", re.I)),

    # CIRCT lowering
    ("lowering_fail", re.compile(r"moore/llhd-to-core lowering failed")),

    # Verilog import
    ("unknown_module", re.compile(r"error:\s*unknown\s+module|unknown\s+definition", re.I)),
    ("unsupported_construct", re.compile(
        r"error:\s*unsupported|not yet supported|cannot be|unhandled", re.I)),
    ("no_top_found", re.compile(r"no top-level module|no module.*found", re.I)),

    # Resource/system
    ("encoding_error", re.compile(r"encoding|non-UTF-8|invalid byte", re.I)),
    ("port_mismatch", re.compile(r"port.*does not exist|port.*has no connection", re.I)),
    ("max_width", re.compile(r"maximum vector width", re.I)),
    ("timescale_error", re.compile(r"time\s*scale|timescale", re.I)),
]


def classify_stderr(stderr: str, exit_code: int) -> tuple[str, dict]:
    """Return (category, details)."""
    if exit_code == 139:
        return "segfault", {}
    if exit_code == 137 or "timeout" in stderr.lower():
        return "timeout", {}

    all_matches: list[tuple[str, dict]] = []
    for cat, pat in CATEGORIES:
        m = pat.search(stderr)
        if m:
            details: dict = {}
            if cat in ("unresolved_process", "unresolved_drive") and m.group(1):
                details["count"] = int(m.group(1))
            all_matches.append((cat, details))

    if exit_code == 0 and not all_matches:
        return ("pass_with_warnings", {}) if stderr.strip() else ("pass", {})

    if all_matches:
        return all_matches[0][0], all_matches[0][1]

    if exit_code != 0:
        return "other", {}
    return "pass", {}


def collect_all_categories(stderr: str, exit_code: int) -> list[str]:
    """Return ALL matching categories (for multi-issue detection)."""
    cats = []
    for cat, pat in CATEGORIES:
        if pat.search(stderr):
            cats.append(cat)
    if exit_code == 139:
        cats.append("segfault")
    if exit_code == 137 or "timeout" in stderr.lower():
        cats.append("timeout")
    return cats


# ---------------------------------------------------------------------------
# Config parsing
# ---------------------------------------------------------------------------

@dataclass
class SurveyTarget:
    type: str           # rtl, lib, skip
    filelist: str       # relative to PROJECT
    top_module: str     # empty = auto-detect


def parse_targets(config_path: str, project: str) -> list[SurveyTarget]:
    """Parse survey-targets.txt."""
    targets = []
    with open(config_path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) < 2:
                continue
            t = SurveyTarget(
                type=parts[0],
                filelist=parts[1],
                top_module=parts[2] if len(parts) > 2 else "",
            )
            abs_path = os.path.join(project, t.filelist)
            if not os.path.isfile(abs_path):
                print(f"  WARN: filelist not found: {abs_path}", file=sys.stderr)
            targets.append(t)
    return targets


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------

@dataclass
class RunResult:
    name: str
    filelist: str
    top_module: str
    exit_code: int
    primary_category: str
    all_categories: list
    details: dict
    elapsed_sec: float
    stderr_preview: str
    num_modules_generated: int


def run_filelist(
    target: SurveyTarget,
    hirct_gen: str,
    project: str,
    output_base: str,
    stubs_dir: str,
    timeout_sec: int,
) -> RunResult:
    """Run hirct-gen on a filelist and return classification."""
    filelist_abs = os.path.join(project, target.filelist)
    name = Path(target.filelist).stem.replace("_file_list", "").replace("_filelist", "")

    out_dir = os.path.join(output_base, "gen", name)
    os.makedirs(out_dir, exist_ok=True)

    cmd = [hirct_gen, "-f", filelist_abs, "-o", out_dir, "--timescale", "1ns/10ps"]
    if target.top_module:
        cmd += ["--top", target.top_module]
    if stubs_dir and os.path.isdir(stubs_dir):
        cmd += ["--lib-dir", stubs_dir]

    env = os.environ.copy()
    env["PROJECT"] = project

    t0 = time.monotonic()
    try:
        result = subprocess.run(
            cmd, capture_output=True, timeout=timeout_sec, env=env,
        )
        exit_code = result.returncode
        stderr = result.stderr.decode("utf-8", errors="replace") if result.stderr else ""
        stdout = result.stdout.decode("utf-8", errors="replace") if result.stdout else ""
    except subprocess.TimeoutExpired:
        exit_code = 137
        stderr = f"timeout after {timeout_sec}s"
        stdout = ""
    except OSError as e:
        exit_code = -1
        stderr = str(e)
        stdout = ""

    elapsed = time.monotonic() - t0

    raw_dir = os.path.join(output_base, "raw")
    os.makedirs(raw_dir, exist_ok=True)
    log_path = os.path.join(raw_dir, f"{name}.log")
    with open(log_path, "w", encoding="utf-8") as f:
        f.write(f"# name: {name}\n")
        f.write(f"# filelist: {target.filelist}\n")
        f.write(f"# top: {target.top_module or '(auto)'}\n")
        f.write(f"# cmd: {' '.join(cmd)}\n")
        f.write(f"# exit_code: {exit_code}\n")
        f.write(f"# elapsed: {elapsed:.1f}s\n")
        f.write(f"# --- stdout ---\n{stdout}\n")
        f.write(f"# --- stderr ---\n{stderr}\n")

    primary, details = classify_stderr(stderr, exit_code)
    all_cats = collect_all_categories(stderr, exit_code)

    num_gen = 0
    if os.path.isdir(out_dir):
        for d in os.listdir(out_dir):
            meta = os.path.join(out_dir, d, "meta.json")
            if os.path.isfile(meta):
                num_gen += 1

    return RunResult(
        name=name,
        filelist=target.filelist,
        top_module=target.top_module,
        exit_code=exit_code,
        primary_category=primary,
        all_categories=all_cats,
        details=details,
        elapsed_sec=round(elapsed, 1),
        stderr_preview=stderr[:800].strip(),
        num_modules_generated=num_gen,
    )


# ---------------------------------------------------------------------------
# Report
# ---------------------------------------------------------------------------

def generate_report(results: list[RunResult], output_base: str) -> dict:
    cats: dict[str, dict] = {}
    for r in results:
        c = r.primary_category
        if c not in cats:
            cats[c] = {"count": 0, "percentage": 0.0, "targets": [], "multi_issue": []}
        cats[c]["count"] += 1
        cats[c]["targets"].append({
            "name": r.name,
            "filelist": r.filelist,
            "top": r.top_module,
            "exit_code": r.exit_code,
            "elapsed": r.elapsed_sec,
            "modules_generated": r.num_modules_generated,
            "all_categories": r.all_categories,
            "stderr_preview": r.stderr_preview[:300],
        })
        if len(r.all_categories) > 1:
            cats[c]["multi_issue"].append({
                "name": r.name,
                "categories": r.all_categories,
            })

    total = len(results)
    for data in cats.values():
        data["percentage"] = round(data["count"] / total * 100, 1) if total else 0

    pass_count = cats.get("pass", {}).get("count", 0)
    pass_warn = cats.get("pass_with_warnings", {}).get("count", 0)
    total_gen = sum(r.num_modules_generated for r in results)

    report = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "total_targets": total,
        "pass": pass_count,
        "pass_with_warnings": pass_warn,
        "fail": total - pass_count - pass_warn,
        "total_modules_generated": total_gen,
        "categories": dict(sorted(cats.items(), key=lambda x: -x[1]["count"])),
        "timings": {
            "total_sec": round(sum(r.elapsed_sec for r in results), 1),
            "avg_sec": round(sum(r.elapsed_sec for r in results) / max(total, 1), 1),
            "max_sec": round(max((r.elapsed_sec for r in results), default=0), 1),
        },
    }

    out_dir = Path(output_base)
    out_dir.mkdir(parents=True, exist_ok=True)

    with open(out_dir / "error-taxonomy.json", "w", encoding="utf-8") as f:
        json.dump(report, f, indent=2, ensure_ascii=False)

    summary = format_summary(report, results)
    with open(out_dir / "error-taxonomy-summary.txt", "w", encoding="utf-8") as f:
        f.write(summary)

    return report


def format_summary(report: dict, results: list[RunResult]) -> str:
    lines = []
    lines.append("=" * 78)
    lines.append("HIRCT GenModel Error Taxonomy — Filelist-Based Survey")
    lines.append("=" * 78)
    lines.append(f"Generated: {report['generated_at']}")
    lines.append(f"Total IP targets: {report['total_targets']}")
    lines.append(f"Pass: {report['pass']}  |  Pass+Warn: {report['pass_with_warnings']}  |  Fail: {report['fail']}")
    lines.append(f"Total modules generated: {report['total_modules_generated']}")
    lines.append(f"Time: {report['timings']['total_sec']}s total, "
                 f"{report['timings']['avg_sec']}s avg, "
                 f"{report['timings']['max_sec']}s max")
    lines.append("")

    lines.append("-" * 78)
    lines.append(f"{'Category':<30} {'Count':>6} {'%':>7}  Targets")
    lines.append("-" * 78)
    for cat, data in report["categories"].items():
        names = [t["name"] for t in data["targets"][:5]]
        ex = ", ".join(names)
        if len(ex) > 35:
            ex = ex[:32] + "..."
        lines.append(f"{cat:<30} {data['count']:>6} {data['percentage']:>6.1f}%  {ex}")
    lines.append("-" * 78)
    lines.append("")

    genmodel_cats = [
        "unresolved_process", "unresolved_drive", "flatten_cycle",
        "val_map_gap", "clock_domain_error", "emit_fail",
    ]
    genmodel_total = sum(
        report["categories"].get(c, {}).get("count", 0) for c in genmodel_cats
    )
    lines.append(f"GenModel-specific failures: {genmodel_total}")
    lines.append(f"CIRCT lowering failures:    {report['categories'].get('lowering_fail', {}).get('count', 0)}")
    verilog_cats = ["unknown_module", "unsupported_construct", "no_top_found"]
    lines.append(f"Verilog import failures:    {sum(report['categories'].get(c, {}).get('count', 0) for c in verilog_cats)}")
    lines.append(f"Other failures:             {report['categories'].get('other', {}).get('count', 0)}")
    lines.append("")

    # Per-target detail table
    lines.append("=" * 78)
    lines.append("Per-Target Detail")
    lines.append("=" * 78)
    lines.append(f"{'Name':<25} {'Category':<25} {'Exit':>5} {'Time':>6} {'Mods':>5}")
    lines.append("-" * 78)
    for r in sorted(results, key=lambda x: x.name):
        lines.append(
            f"{r.name:<25} {r.primary_category:<25} {r.exit_code:>5} "
            f"{r.elapsed_sec:>5.1f}s {r.num_modules_generated:>5}"
        )
    lines.append("-" * 78)
    lines.append("")

    # Multi-issue targets
    multi = [r for r in results if len(r.all_categories) > 1]
    if multi:
        lines.append("Targets with multiple issue categories:")
        for r in multi:
            lines.append(f"  {r.name}: {', '.join(r.all_categories)}")
        lines.append("")

    lines.append("=" * 78)
    lines.append("Next step: share this summary with the agent to decide")
    lines.append("MLIR analysis scope and script architecture.")
    lines.append("=" * 78)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Survey RTL IP blocks with hirct-gen, classify errors"
    )
    parser.add_argument(
        "--project",
        default=os.environ.get("PROJECT", "/user/wonseok/fc6161-trunk-rom"),
        help="RTL project root (default: $PROJECT)",
    )
    parser.add_argument(
        "--targets",
        default="config/survey-targets.txt",
        help="Survey targets config (default: config/survey-targets.txt)",
    )
    parser.add_argument(
        "--hirct-gen", default=None,
        help="hirct-gen binary path (default: auto-detect)",
    )
    parser.add_argument(
        "--stubs-dir", default="config/stubs",
        help="Stubs directory for --lib-dir (default: config/stubs)",
    )
    parser.add_argument(
        "--output", default="survey-results",
        help="Output directory (default: survey-results)",
    )
    parser.add_argument(
        "--timeout", type=int, default=300,
        help="Per-target timeout in seconds (default: 300)",
    )
    parser.add_argument(
        "--limit", type=int, default=None,
        help="Max targets to process (default: all)",
    )
    args = parser.parse_args()

    if not os.path.isfile(args.targets):
        print(f"ERROR: targets config not found: {args.targets}", file=sys.stderr)
        return 1

    hirct_gen = args.hirct_gen
    if hirct_gen is None:
        candidates = [
            os.path.join(os.path.dirname(__file__), "../../../../hirct/build/bin/hirct-gen"),
            os.environ.get("HIRCT_GEN", ""),
        ]
        for c in candidates:
            if c and os.path.isfile(c):
                hirct_gen = os.path.abspath(c)
                break
    if not hirct_gen or not os.path.isfile(hirct_gen):
        print(f"ERROR: hirct-gen not found. Use --hirct-gen PATH", file=sys.stderr)
        return 1

    stubs_dir = os.path.abspath(args.stubs_dir) if os.path.isdir(args.stubs_dir) else ""

    targets = parse_targets(args.targets, args.project)
    rtl_targets = [t for t in targets if t.type == "rtl"]

    if args.limit:
        rtl_targets = rtl_targets[:args.limit]

    print(f"[survey] Project:   {args.project}", file=sys.stderr)
    print(f"[survey] hirct-gen: {hirct_gen}", file=sys.stderr)
    print(f"[survey] Stubs:     {stubs_dir or '(none)'}", file=sys.stderr)
    print(f"[survey] Targets:   {len(rtl_targets)} RTL IP blocks", file=sys.stderr)
    print(f"[survey] Output:    {args.output}", file=sys.stderr)
    print(f"[survey] Timeout:   {args.timeout}s per target", file=sys.stderr)
    print("", file=sys.stderr)

    results: list[RunResult] = []
    t_start = time.monotonic()

    for i, target in enumerate(rtl_targets):
        name = Path(target.filelist).stem.replace("_file_list", "").replace("_filelist", "")
        print(
            f"[survey] [{i+1}/{len(rtl_targets)}] {name} ...",
            file=sys.stderr, end="", flush=True,
        )

        r = run_filelist(target, hirct_gen, args.project, args.output, stubs_dir, args.timeout)
        results.append(r)

        status = "PASS" if r.primary_category == "pass" else r.primary_category.upper()
        print(f" {status} ({r.elapsed_sec:.1f}s, {r.num_modules_generated} mods)", file=sys.stderr)

    report = generate_report(results, args.output)

    total_elapsed = time.monotonic() - t_start
    print(f"\n[survey] Done in {total_elapsed:.0f}s", file=sys.stderr)
    print(f"[survey] Results: {args.output}/error-taxonomy.json", file=sys.stderr)
    print(f"[survey] Summary: {args.output}/error-taxonomy-summary.txt", file=sys.stderr)

    summary_path = os.path.join(args.output, "error-taxonomy-summary.txt")
    if os.path.isfile(summary_path):
        with open(summary_path, encoding="utf-8") as f:
            print(f.read())

    return 0


if __name__ == "__main__":
    sys.exit(main())
