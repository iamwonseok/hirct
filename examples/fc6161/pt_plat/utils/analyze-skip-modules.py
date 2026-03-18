#!/usr/bin/env python3
"""Analyze SKIP modules with verilator -E preprocessing → circt-verilog → MLIR.

Reads the baseline error-taxonomy.json, identifies non-pass targets, and tests
whether verilator -E preprocessing enables successful MLIR generation.

Usage:
    python3 utils/analyze-skip-modules.py
    python3 utils/analyze-skip-modules.py --limit 3  # quick test
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import time
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path


@dataclass
class AnalysisResult:
    name: str
    filelist: str
    top_module: str
    baseline_category: str
    verilator_exit: int
    verilator_lines: int
    circt_exit: int
    mlir_lines: int
    mlir_modules: int
    llhd_process_count: int
    cf_br_count: int
    cf_cond_br_count: int
    llhd_sig_count: int
    llhd_drv_count: int
    llhd_prb_count: int
    remaining_patterns: list[str] = field(default_factory=list)
    error_preview: str = ""
    elapsed_sec: float = 0.0


def parse_targets(config_path: str, project: str) -> list[dict]:
    targets = []
    with open(config_path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) < 2 or parts[0] != "rtl":
                continue
            targets.append({
                "filelist": parts[1],
                "top": parts[2] if len(parts) > 2 else "",
                "name": Path(parts[1]).stem.replace("_file_list", "").replace("_filelist", ""),
            })
    return targets


def get_skip_targets(taxonomy_path: str, all_targets: list[dict]) -> list[dict]:
    """Identify targets that failed in the baseline survey."""
    with open(taxonomy_path, encoding="utf-8") as f:
        taxonomy = json.load(f)

    pass_names = set()
    for cat_name in ("pass", "pass_with_warnings"):
        cat = taxonomy.get("categories", {}).get(cat_name, {})
        for t in cat.get("targets", []):
            pass_names.add(t["name"])

    baseline_map = {}
    for cat_name, cat_data in taxonomy.get("categories", {}).items():
        for t in cat_data.get("targets", []):
            baseline_map[t["name"]] = cat_name

    skip = []
    for t in all_targets:
        if t["name"] not in pass_names:
            t["baseline_category"] = baseline_map.get(t["name"], "unknown")
            skip.append(t)
    return skip


def run_verilator_preprocess(
    filelist_abs: str,
    project: str,
    top_module: str,
    verilator_path: str,
    output_dir: str,
    timeout_sec: int,
) -> tuple[int, str, int]:
    """Run verilator -E on a filelist, return (exit_code, output_path, line_count)."""
    os.makedirs(output_dir, exist_ok=True)

    with open(filelist_abs, encoding="utf-8", errors="replace") as f:
        content = f.read()

    args = [verilator_path, "-E", "--pp-comments"]
    input_files = []
    for line in content.splitlines():
        line = line.strip()
        if not line or line.startswith("//"):
            continue
        expanded = line.replace("$PROJECT", project).replace("${PROJECT}", project)
        if expanded.startswith("+define+"):
            args.append(expanded)
        elif expanded.startswith("+incdir+"):
            args.append(expanded)
        elif expanded.startswith("-y"):
            if " " in expanded:
                parts = expanded.split(None, 1)
                args.extend(parts)
            else:
                args.append(expanded)
        elif expanded.startswith("-v "):
            parts = expanded.split(None, 1)
            args.extend(parts)
        elif expanded.startswith("-sverilog"):
            continue
        elif expanded.startswith("-") or expanded.startswith("+"):
            args.append(expanded)
        else:
            input_files.append(expanded)

    args.extend(input_files)

    out_path = os.path.join(output_dir, "preprocessed.v")
    env = os.environ.copy()
    env["PROJECT"] = project

    try:
        result = subprocess.run(
            args, capture_output=True, timeout=timeout_sec, env=env,
        )
        with open(out_path, "wb") as f:
            f.write(result.stdout)
        lines = result.stdout.count(b"\n") if result.stdout else 0
        if result.returncode != 0:
            err_path = os.path.join(output_dir, "verilator-stderr.log")
            with open(err_path, "wb") as f:
                f.write(result.stderr)
        return result.returncode, out_path, lines
    except subprocess.TimeoutExpired:
        return 137, "", 0
    except OSError as e:
        return -1, "", 0


def run_circt_verilog(
    input_path: str,
    circt_verilog: str,
    output_dir: str,
    timeout_sec: int,
) -> tuple[int, str, int]:
    """Run circt-verilog --ir-hw on preprocessed input."""
    mlir_path = os.path.join(output_dir, "output.mlir")

    cmd = [circt_verilog, "--ir-hw", input_path]
    try:
        result = subprocess.run(
            cmd, capture_output=True, timeout=timeout_sec,
        )
        with open(mlir_path, "wb") as f:
            f.write(result.stdout)
        lines = result.stdout.count(b"\n") if result.stdout else 0
        if result.returncode != 0:
            err_path = os.path.join(output_dir, "circt-stderr.log")
            with open(err_path, "wb") as f:
                f.write(result.stderr)
            return result.returncode, mlir_path, lines
        return 0, mlir_path, lines
    except subprocess.TimeoutExpired:
        return 137, "", 0
    except OSError as e:
        return -1, "", 0


def analyze_mlir(mlir_path: str) -> dict:
    """Count LLHD/CF patterns in MLIR output."""
    counts = {
        "hw_module": 0,
        "llhd_process": 0,
        "cf_br": 0,
        "cf_cond_br": 0,
        "llhd_sig": 0,
        "llhd_drv": 0,
        "llhd_prb": 0,
        "llhd_wait": 0,
        "llhd_halt": 0,
        "sim_proc_print": 0,
        "seq_firreg": 0,
        "moore_concat_ref": 0,
    }
    patterns = {
        "hw_module": re.compile(r"\bhw\.module\b"),
        "llhd_process": re.compile(r"\bllhd\.process\b"),
        "cf_br": re.compile(r"\bcf\.br\b"),
        "cf_cond_br": re.compile(r"\bcf\.cond_br\b"),
        "llhd_sig": re.compile(r"\bllhd\.sig\b"),
        "llhd_drv": re.compile(r"\bllhd\.drv\b"),
        "llhd_prb": re.compile(r"\bllhd\.prb\b"),
        "llhd_wait": re.compile(r"\bllhd\.wait\b"),
        "llhd_halt": re.compile(r"\bllhd\.halt\b"),
        "sim_proc_print": re.compile(r"\bsim\.proc\.print\b"),
        "seq_firreg": re.compile(r"\bseq\.firreg\b"),
        "moore_concat_ref": re.compile(r"\bmoore\.concat_ref\b"),
    }

    if not os.path.isfile(mlir_path):
        return counts

    try:
        with open(mlir_path, encoding="utf-8", errors="replace") as f:
            for line in f:
                for key, pat in patterns.items():
                    counts[key] += len(pat.findall(line))
    except Exception:
        pass
    return counts


def analyze_target(
    target: dict,
    project: str,
    verilator_path: str,
    circt_verilog: str,
    output_base: str,
    timeout_sec: int,
) -> AnalysisResult:
    name = target["name"]
    filelist_abs = os.path.join(project, target["filelist"])
    out_dir = os.path.join(output_base, name)

    t0 = time.monotonic()

    v_exit, v_out, v_lines = run_verilator_preprocess(
        filelist_abs, project, target["top"],
        verilator_path, out_dir, timeout_sec,
    )

    c_exit, mlir_path, mlir_lines = 0, "", 0
    if v_exit == 0 and v_out:
        c_exit, mlir_path, mlir_lines = run_circt_verilog(
            v_out, circt_verilog, out_dir, timeout_sec,
        )

    elapsed = time.monotonic() - t0

    counts = analyze_mlir(mlir_path) if mlir_lines > 0 else {}

    remaining = []
    if counts.get("llhd_process", 0) > 0:
        remaining.append(f"llhd.process({counts['llhd_process']})")
    if counts.get("cf_br", 0) > 0:
        remaining.append(f"cf.br({counts['cf_br']})")
    if counts.get("moore_concat_ref", 0) > 0:
        remaining.append(f"moore.concat_ref({counts['moore_concat_ref']})")

    error = ""
    if v_exit != 0:
        err_file = os.path.join(out_dir, "verilator-stderr.log")
        if os.path.isfile(err_file):
            with open(err_file, encoding="utf-8", errors="replace") as f:
                error = f.read(500).strip()
    elif c_exit != 0:
        err_file = os.path.join(out_dir, "circt-stderr.log")
        if os.path.isfile(err_file):
            with open(err_file, encoding="utf-8", errors="replace") as f:
                error = f.read(500).strip()

    return AnalysisResult(
        name=name,
        filelist=target["filelist"],
        top_module=target["top"],
        baseline_category=target.get("baseline_category", ""),
        verilator_exit=v_exit,
        verilator_lines=v_lines,
        circt_exit=c_exit,
        mlir_lines=mlir_lines,
        mlir_modules=counts.get("hw_module", 0),
        llhd_process_count=counts.get("llhd_process", 0),
        cf_br_count=counts.get("cf_br", 0),
        cf_cond_br_count=counts.get("cf_cond_br", 0),
        llhd_sig_count=counts.get("llhd_sig", 0),
        llhd_drv_count=counts.get("llhd_drv", 0),
        llhd_prb_count=counts.get("llhd_prb", 0),
        remaining_patterns=remaining,
        error_preview=error,
        elapsed_sec=round(elapsed, 1),
    )


def generate_report(results: list[AnalysisResult], output_base: str) -> dict:
    total = len(results)
    v_pass = sum(1 for r in results if r.verilator_exit == 0)
    c_pass = sum(1 for r in results if r.verilator_exit == 0 and r.circt_exit == 0)
    mlir_with_hw = sum(1 for r in results if r.mlir_modules > 0)
    total_modules = sum(r.mlir_modules for r in results)
    total_llhd = sum(r.llhd_process_count for r in results)

    by_baseline = {}
    for r in results:
        cat = r.baseline_category
        if cat not in by_baseline:
            by_baseline[cat] = {"total": 0, "v_pass": 0, "c_pass": 0, "targets": []}
        by_baseline[cat]["total"] += 1
        if r.verilator_exit == 0:
            by_baseline[cat]["v_pass"] += 1
        if r.verilator_exit == 0 and r.circt_exit == 0:
            by_baseline[cat]["c_pass"] += 1
        by_baseline[cat]["targets"].append({
            "name": r.name,
            "v_exit": r.verilator_exit,
            "v_lines": r.verilator_lines,
            "c_exit": r.circt_exit,
            "mlir_lines": r.mlir_lines,
            "hw_modules": r.mlir_modules,
            "llhd_process": r.llhd_process_count,
            "cf_br": r.cf_br_count,
            "remaining": r.remaining_patterns,
            "error": r.error_preview[:200],
            "elapsed": r.elapsed_sec,
        })

    report = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "total_skip_targets": total,
        "verilator_pass": v_pass,
        "circt_pass": c_pass,
        "mlir_with_modules": mlir_with_hw,
        "total_hw_modules": total_modules,
        "total_llhd_process": total_llhd,
        "by_baseline_category": dict(sorted(
            by_baseline.items(), key=lambda x: -x[1]["total"],
        )),
        "timings": {
            "total_sec": round(sum(r.elapsed_sec for r in results), 1),
            "avg_sec": round(sum(r.elapsed_sec for r in results) / max(total, 1), 1),
        },
    }

    out_dir = Path(output_base)
    out_dir.mkdir(parents=True, exist_ok=True)

    with open(out_dir / "skip-analysis.json", "w", encoding="utf-8") as f:
        json.dump(report, f, indent=2, ensure_ascii=False)

    summary = format_summary(report, results)
    with open(out_dir / "skip-analysis-summary.txt", "w", encoding="utf-8") as f:
        f.write(summary)

    return report


def format_summary(report: dict, results: list[AnalysisResult]) -> str:
    lines = []
    lines.append("=" * 90)
    lines.append("SKIP Module Re-analysis: verilator -E → circt-verilog → MLIR")
    lines.append("=" * 90)
    lines.append(f"Generated: {report['generated_at']}")
    lines.append(f"Total SKIP targets: {report['total_skip_targets']}")
    lines.append(f"verilator -E pass: {report['verilator_pass']}")
    lines.append(f"circt-verilog pass: {report['circt_pass']}")
    lines.append(f"MLIR with hw.module: {report['mlir_with_modules']}")
    lines.append(f"Total hw.module: {report['total_hw_modules']}")
    lines.append(f"Total llhd.process: {report['total_llhd_process']}")
    lines.append(f"Time: {report['timings']['total_sec']}s")
    lines.append("")

    lines.append("-" * 90)
    lines.append(f"{'Baseline Category':<25} {'Total':>6} {'V-Pass':>7} {'C-Pass':>7}")
    lines.append("-" * 90)
    for cat, data in report["by_baseline_category"].items():
        lines.append(
            f"{cat:<25} {data['total']:>6} {data['v_pass']:>7} {data['c_pass']:>7}"
        )
    lines.append("-" * 90)
    lines.append("")

    lines.append("=" * 90)
    lines.append("Per-Target Detail")
    lines.append("=" * 90)
    lines.append(
        f"{'Name':<25} {'Baseline':<20} {'V-E':>4} {'CV':>4} {'MLIR':>6} "
        f"{'Mods':>5} {'Proc':>5} {'Time':>6}"
    )
    lines.append("-" * 90)
    for r in sorted(results, key=lambda x: x.name):
        v_status = "OK" if r.verilator_exit == 0 else "FAIL"
        c_status = "OK" if r.circt_exit == 0 else ("FAIL" if r.verilator_exit == 0 else "-")
        lines.append(
            f"{r.name:<25} {r.baseline_category:<20} {v_status:>4} {c_status:>4} "
            f"{r.mlir_lines:>6} {r.mlir_modules:>5} "
            f"{r.llhd_process_count:>5} {r.elapsed_sec:>5.1f}s"
        )
    lines.append("-" * 90)
    lines.append("")

    with_llhd = [r for r in results if r.llhd_process_count > 0]
    if with_llhd:
        lines.append("Targets with remaining llhd.process:")
        for r in sorted(with_llhd, key=lambda x: -x.llhd_process_count):
            lines.append(
                f"  {r.name}: {r.llhd_process_count} process, "
                f"{r.cf_br_count} cf.br, {r.cf_cond_br_count} cf.cond_br"
            )
        lines.append("")

    lines.append("=" * 90)
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Analyze SKIP modules with verilator -E → circt-verilog"
    )
    parser.add_argument(
        "--project",
        default=os.environ.get("PROJECT", "/user/wonseok/fc6161-trunk-rom"),
    )
    parser.add_argument("--targets", default="config/survey-targets.txt")
    parser.add_argument(
        "--baseline", default="survey-results/error-taxonomy.json",
        help="Baseline taxonomy for SKIP identification",
    )
    parser.add_argument("--verilator", default=None)
    parser.add_argument(
        "--circt-verilog", default=None, dest="circt_verilog",
    )
    parser.add_argument("--output", default="skip-analysis-results")
    parser.add_argument("--timeout", type=int, default=300)
    parser.add_argument("--limit", type=int, default=None)
    args = parser.parse_args()

    verilator = args.verilator or "verilator"
    circt_verilog = args.circt_verilog
    if not circt_verilog:
        candidates = [
            "/user/wonseok/circt/build/bin/circt-verilog",
            os.environ.get("CIRCT_BIN", "") + "/circt-verilog",
        ]
        for c in candidates:
            if os.path.isfile(c):
                circt_verilog = c
                break
    if not circt_verilog:
        print("ERROR: circt-verilog not found", file=sys.stderr)
        return 1

    all_targets = parse_targets(args.targets, args.project)
    skip_targets = get_skip_targets(args.baseline, all_targets)

    if args.limit:
        skip_targets = skip_targets[:args.limit]

    print(f"[skip-analysis] Project:        {args.project}", file=sys.stderr)
    print(f"[skip-analysis] verilator:      {verilator}", file=sys.stderr)
    print(f"[skip-analysis] circt-verilog:  {circt_verilog}", file=sys.stderr)
    print(f"[skip-analysis] SKIP targets:   {len(skip_targets)}", file=sys.stderr)
    print(f"[skip-analysis] Output:         {args.output}", file=sys.stderr)
    print("", file=sys.stderr)

    results: list[AnalysisResult] = []
    t_start = time.monotonic()

    for i, target in enumerate(skip_targets):
        print(
            f"[skip-analysis] [{i+1}/{len(skip_targets)}] {target['name']} "
            f"(was: {target['baseline_category']}) ...",
            file=sys.stderr, end="", flush=True,
        )
        r = analyze_target(
            target, args.project, verilator, circt_verilog,
            args.output, args.timeout,
        )
        results.append(r)

        v_s = "V-OK" if r.verilator_exit == 0 else "V-FAIL"
        c_s = "C-OK" if r.circt_exit == 0 else "C-FAIL"
        print(
            f" {v_s} {c_s} ({r.mlir_lines}L, {r.mlir_modules}mod, "
            f"{r.llhd_process_count}proc, {r.elapsed_sec:.1f}s)",
            file=sys.stderr,
        )

    report = generate_report(results, args.output)
    total_elapsed = time.monotonic() - t_start
    print(f"\n[skip-analysis] Done in {total_elapsed:.0f}s", file=sys.stderr)

    summary_path = os.path.join(args.output, "skip-analysis-summary.txt")
    if os.path.isfile(summary_path):
        with open(summary_path, encoding="utf-8") as f:
            print(f.read())

    return 0


if __name__ == "__main__":
    sys.exit(main())
