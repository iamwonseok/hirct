#!/usr/bin/env python3
"""MLIR failure triage analysis based on meta.json and re-run stderr capture.

Re-runs failed modules with hirct-gen to capture stderr, classifies errors,
and produces a triage report with known-limitations candidates.

Usage:
    python3 utils/triage-mlir-failures.py [options]

Options:
    --report PATH     Input report.json (default: output/report.json)
    --baseline PATH   Baseline report for pre-207 comparison (default: output/report-baseline.json)
    --output PATH     Output JSON path (default: output/triage-report-post-207.json)
    --hirct-gen PATH  hirct-gen binary (default: build/bin/hirct-gen)
    --limit N         Max failures to re-analyze (default: all)
    --generate-f PATH config/generate.f for source path mapping (default: config/generate.f)
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from datetime import datetime, timezone

# Pre-207 baseline numbers from 207-mlir-parse-improvement.md
PRE_207 = {
    "total_fail": 1011,
    "unknown_module": 993,
    "unsupported_construct": 12,
    "no_top_found": 2,
    "max_width": 1,
    "encoding_error": 1,
    "other": 2,
}

# Error classification patterns (order matters for first match)
CATEGORY_PATTERNS = [
    ("unknown_module", re.compile(r"error:\s*unknown\s+module", re.I)),
    (
        "unsupported_construct",
        re.compile(r"error:\s*unsupported|not yet supported|cannot be", re.I),
    ),
    ("no_top_found", re.compile(r"no top-level module|no module.*found", re.I)),
    ("encoding_error", re.compile(r"encoding|non-UTF-8|invalid byte", re.I)),
    (
        "port_mismatch",
        re.compile(r"port.*does not exist|port.*has no connection", re.I),
    ),
    ("time_scale", re.compile(r"time scale|timescale", re.I)),
    ("max_width", re.compile(r"maximum vector width", re.I)),
]


def load_source_paths(generate_f_path: str) -> set[str]:
    """Load source paths from config/generate.f (path without rtl/ and .v)."""
    paths = set()
    if not os.path.isfile(generate_f_path):
        return paths
    with open(generate_f_path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if line.startswith("rtl/"):
                rel = line[4:]  # strip rtl/
            else:
                rel = line
            if rel.endswith(".v"):
                rel = rel[:-2]  # strip .v
            paths.add(rel)
    return paths


def meta_path_to_source(meta_path: str, source_paths: set[str]) -> str | None:
    """Derive RTL source path from meta.json path.

    Pattern:
    - output/<rel>/meta.json (fail, single module) -> rtl/<rel>.v
    - output/<rel>/<module>/meta.json (pass) -> rtl/<dir>.v where rel=dir/module

    Uses config/generate.f to resolve the correct split.
    """
    if not meta_path.startswith("output/") or not meta_path.endswith("/meta.json"):
        return None
    rel = meta_path[7:-10]  # strip "output/" and "/meta.json"
    if not rel:
        return None

    # Try full path as dir first (fail case: output/dir/meta.json)
    if rel in source_paths:
        return f"rtl/{rel}.v"

    # Try dir/module: find longest source path that is a prefix
    best_dir = None
    for src in source_paths:
        prefix = src + "/"
        if rel.startswith(prefix):
            if best_dir is None or len(src) > len(best_dir):
                best_dir = src

    if best_dir is not None:
        return f"rtl/{best_dir}.v"

    return None


def classify_stderr(stderr: str, exit_code: int) -> str:
    """Classify error category from stderr and exit code."""
    if exit_code == 139:
        return "segfault"
    if exit_code == 137:
        return "timeout"
    stderr_lower = stderr.lower()
    if "timeout" in stderr_lower:
        return "timeout"

    for cat, pat in CATEGORY_PATTERNS:
        if pat.search(stderr):
            return cat

    return "other"


def run_hirct_gen(
    hirct_gen: str, source_path: str, tmp_dir: str
) -> tuple[int, str, str]:
    """Run hirct-gen on source file, return (exit_code, stdout, stderr)."""
    abs_src = os.path.abspath(source_path)
    if not os.path.isfile(abs_src):
        return -1, "", f"source file not found: {abs_src}"

    cmd = [hirct_gen, abs_src, "-o", tmp_dir]
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            timeout=120,
        )
        stdout = (
            result.stdout.decode("utf-8", errors="replace") if result.stdout else ""
        )
        stderr = (
            result.stderr.decode("utf-8", errors="replace") if result.stderr else ""
        )
        return result.returncode, stdout, stderr
    except subprocess.TimeoutExpired:
        return 137, "", "timeout"
    except OSError as e:
        return -1, "", str(e)


def project_root() -> str | None:
    """Return project root (parent of config/)."""
    cwd = os.getcwd()
    for _ in range(5):
        if os.path.isdir(os.path.join(cwd, "config")):
            return cwd
        parent = os.path.dirname(cwd)
        if parent == cwd:
            break
        cwd = parent
    return None


def main() -> int:
    parser = argparse.ArgumentParser(
        description="MLIR failure triage: re-run failed modules, classify, report"
    )
    parser.add_argument(
        "--report",
        default="output/report.json",
        help="Input report.json (default: output/report.json)",
    )
    parser.add_argument(
        "--baseline",
        default="output/report-baseline.json",
        help="Baseline report for pre-207 (default: output/report-baseline.json)",
    )
    parser.add_argument(
        "--output",
        default="output/triage-report-post-207.json",
        help="Output JSON path (default: output/triage-report-post-207.json)",
    )
    parser.add_argument(
        "--hirct-gen",
        default="build/bin/hirct-gen",
        help="hirct-gen binary path (default: build/bin/hirct-gen)",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=None,
        help="Max failures to re-analyze (default: all)",
    )
    parser.add_argument(
        "--generate-f",
        default="config/generate.f",
        help="generate.f for source path mapping (default: config/generate.f)",
    )
    args = parser.parse_args()

    root = project_root()
    if root:
        os.chdir(root)

    # Resolve hirct-gen path relative to project root
    hirct_gen = args.hirct_gen
    if not os.path.isabs(hirct_gen) and root:
        hirct_gen = os.path.join(root, hirct_gen)

    if not os.path.isfile(args.report):
        print(f"ERROR: report not found: {args.report}", file=sys.stderr)
        return 1

    with open(args.report, encoding="utf-8") as f:
        report = json.load(f)

    source_paths = load_source_paths(args.generate_f)
    failed_entries = [e for e in report.get("files", []) if e.get("mlir") == "fail"]
    total_fail = report.get("mlir_fail", len(failed_entries))
    total_files = report.get("total_files", 0)
    mlir_pass = report.get("mlir_success", 0)

    if args.limit is not None:
        failed_entries = failed_entries[: args.limit]

    categories: dict[str, dict] = {
        "unknown_module": {"count": 0, "percentage": 0.0, "examples": []},
        "unsupported_construct": {"count": 0, "percentage": 0.0, "examples": []},
        "no_top_found": {"count": 0, "percentage": 0.0, "examples": []},
        "encoding_error": {"count": 0, "percentage": 0.0, "examples": []},
        "segfault": {"count": 0, "percentage": 0.0, "examples": []},
        "timeout": {"count": 0, "percentage": 0.0, "examples": []},
        "port_mismatch": {"count": 0, "percentage": 0.0, "examples": []},
        "time_scale": {"count": 0, "percentage": 0.0, "examples": []},
        "max_width": {"count": 0, "percentage": 0.0, "examples": []},
        "other": {"count": 0, "percentage": 0.0, "examples": [], "details": []},
    }

    skipped = 0
    with tempfile.TemporaryDirectory(prefix="triage_tmp_") as tmpdir:
        for i, entry in enumerate(failed_entries):
            meta_path = entry.get("path", "")
            source = meta_path_to_source(meta_path, source_paths)
            if source is None or not os.path.isfile(source):
                skipped += 1
                continue

            exit_code, out, err = run_hirct_gen(hirct_gen, source, tmpdir)
            cat = classify_stderr(err, exit_code)

            categories[cat]["count"] += 1
            if len(categories[cat]["examples"]) < 5:
                categories[cat]["examples"].append(meta_path)
            if cat == "other" and err.strip():
                details = categories[cat].get("details", [])
                if len(details) < 10:
                    details.append(
                        {"path": meta_path, "stderr_preview": err[:500].strip()}
                    )
                categories[cat]["details"] = details

            if (i + 1) % 50 == 0:
                print(
                    f"  [triage] analyzed {i + 1} / {len(failed_entries)}",
                    file=sys.stderr,
                )

    analyzed = sum(c["count"] for c in categories.values())
    for cat, data in categories.items():
        if analyzed > 0:
            data["percentage"] = round(data["count"] / analyzed * 100, 1)

    # Build output report
    out_report = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "report_source": args.report,
        "total_files": total_files,
        "mlir_pass": mlir_pass,
        "mlir_fail": total_fail,
        "categories": categories,
        "pre_207": PRE_207,
        "post_207": {"total_fail": total_fail},
        "re_analyzed_count": analyzed,
        "improvement": {
            "unknown_module_reduction": (
                f"{PRE_207['unknown_module']} → {categories['unknown_module']['count']}"
                + (f" (sample of {analyzed})" if analyzed < total_fail else "")
            ),
            "overall": f"586→{mlir_pass} MLIR pass (+{mlir_pass - 586})",
        },
        "known_limitations_candidates": [],
    }

    # Known limitations candidates
    for cat in [
        "unknown_module",
        "unsupported_construct",
        "no_top_found",
        "encoding_error",
        "port_mismatch",
        "time_scale",
        "max_width",
    ]:
        c = categories[cat]["count"]
        if c == 0:
            continue
        if cat == "unknown_module" and c > 100:
            priority = "high"
        elif c > 10:
            priority = "medium"
        else:
            priority = "low"
        out_report["known_limitations_candidates"].append(
            {
                "category": cat,
                "count": c,
                "priority": priority,
                "rationale": f"{c} failures; add representative modules to known-limitations.md",
            }
        )

    if categories["other"]["count"] > 0:
        out_report["known_limitations_candidates"].append(
            {
                "category": "other",
                "count": categories["other"]["count"],
                "priority": "low",
                "rationale": "Requires manual review; check 'details' in categories.other",
            }
        )

    os.makedirs(os.path.dirname(args.output) or ".", exist_ok=True)
    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(out_report, f, indent=2, ensure_ascii=False)

    # Markdown summary to stdout
    print()
    print("## MLIR Failure Triage Report (Post-207)")
    print()
    print(f"**Report source**: {args.report}")
    print(f"**Generated**: {out_report['generated_at']}")
    print()
    print("### Summary")
    print(f"- Total files: {total_files}")
    print(f"- MLIR pass: {mlir_pass}")
    print(f"- MLIR fail: {total_fail}")
    print(f"- Re-analyzed: {analyzed} (skipped {skipped} no-source)")
    print()
    print("### Failure Categories")
    print("| Category | Count | % | Examples |")
    print("|----------|------|---|----------|")
    for cat, data in categories.items():
        ex = ", ".join(data["examples"][:3]) if data["examples"] else "-"
        if len(ex) > 60:
            ex = ex[:57] + "..."
        print(f"| {cat} | {data['count']} | {data['percentage']}% | {ex} |")
    print()
    print("### Pre vs Post 207")
    if analyzed < total_fail:
        print(f"  (Results from sample of {analyzed} re-analyzed failures)")
    print(
        f"- unknown_module: {PRE_207['unknown_module']} → {categories['unknown_module']['count']}"
    )
    print(f"- MLIR pass: 586 → {mlir_pass} (+{mlir_pass - 586})")
    print()
    print("### Known-Limitations Candidates")
    for c in out_report["known_limitations_candidates"]:
        print(
            f"- **{c['category']}** ({c['count']}): {c['rationale']} [{c['priority']}]"
        )
    print()
    print(f"**JSON report**: {args.output}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
