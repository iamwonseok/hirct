#!/usr/bin/env python3
"""Compare baseline and improved report.json files from HIRCT generate/report runs.

Produces summary tables, regression lists (PASS→FAIL), and new-pass lists
(FAIL→PASS). Outputs Markdown and/or JSON per CLI flags.

Usage:
    python3 utils/compare-mlir-report.py --baseline <path> --improved <path>
        [--output-md <path>] [--output-json <path>]

If neither --output-md nor --output-json is given, prints Markdown to stdout.
"""

import argparse
import json
import os
import sys


def err_exit(msg, code=1):
    """Print error message to stderr and exit with given code."""
    print(msg, file=sys.stderr)
    sys.exit(code)


def load_report(path):
    """Load and validate report JSON. Exit on error."""
    if not os.path.isfile(path):
        err_exit("Error: file not found: {}".format(path))
    try:
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        err_exit("Error: invalid JSON in {}: {}".format(path, e))

    required = ["total_files", "mlir_success", "mlir_fail", "files"]
    for key in required:
        if key not in data:
            err_exit("Error: missing key '{}' in {}".format(key, path))

    if not isinstance(data["files"], list):
        err_exit("Error: 'files' must be a list in {}".format(path))

    for i, entry in enumerate(data["files"]):
        if not isinstance(entry, dict):
            err_exit("Error: files[{}] must be an object in {}".format(i, path))
        if "path" not in entry:
            err_exit("Error: missing key 'path' in files[{}] in {}".format(i, path))
        if "mlir" not in entry:
            err_exit("Error: missing key 'mlir' in files[{}] in {}".format(i, path))

    return data


def path_to_mlir(data):
    """Return dict mapping path -> mlir status for each file entry."""
    return {f["path"]: f["mlir"] for f in data["files"]}


def compute_regressions_and_new_passes(baseline, improved):
    """Compute regressions (PASS→FAIL) and new passes (FAIL→PASS).

    Uses comm-style set logic:
    - regressions: paths PASS in baseline but not PASS in improved
    - new_passes: paths PASS in improved but not PASS in baseline
    """
    base_map = path_to_mlir(baseline)
    improved_map = path_to_mlir(improved)

    base_pass = {p for p, m in base_map.items() if m == "pass"}
    improved_pass = {p for p, m in improved_map.items() if m == "pass"}

    regressions = sorted(base_pass - improved_pass)
    new_passes = sorted(improved_pass - base_pass)

    return regressions, new_passes


def compute_delta(baseline, improved):
    """Compute delta dict for total_files, mlir_success, mlir_fail, success_rate."""
    base_total = baseline["total_files"]
    base_pass = baseline["mlir_success"]
    base_fail = baseline["mlir_fail"]
    base_rate = (base_pass / base_total * 100.0) if base_total else 0.0

    imp_total = improved["total_files"]
    imp_pass = improved["mlir_success"]
    imp_fail = improved["mlir_fail"]
    imp_rate = (imp_pass / imp_total * 100.0) if imp_total else 0.0

    delta = {
        "total_files": imp_total - base_total,
        "mlir_success": imp_pass - base_pass,
        "mlir_fail": imp_fail - base_fail,
        "success_rate_pct": imp_rate - base_rate,
    }
    return delta


def compute_per_emitter_delta(baseline, improved):
    """Compute per-emitter pass/fail deltas."""
    base_pe = baseline.get("per_emitter", {})
    imp_pe = improved.get("per_emitter", {})

    all_emitters = sorted(set(base_pe.keys()) | set(imp_pe.keys()))
    result = {}
    for name in all_emitters:
        base_vals = base_pe.get(name, {"pass": 0, "fail": 0})
        imp_vals = imp_pe.get(name, {"pass": 0, "fail": 0})
        result[name] = {
            "pass_delta": imp_vals.get("pass", 0) - base_vals.get("pass", 0),
            "fail_delta": imp_vals.get("fail", 0) - base_vals.get("fail", 0),
        }
    return result


def build_json_output(baseline, improved, regressions, new_passes):
    """Build machine-parseable JSON output dict."""
    base_total = baseline["total_files"]
    base_pass = baseline["mlir_success"]
    base_fail = baseline["mlir_fail"]
    base_rate = (base_pass / base_total * 100.0) if base_total else 0.0

    imp_total = improved["total_files"]
    imp_pass = improved["mlir_success"]
    imp_fail = improved["mlir_fail"]
    imp_rate = (imp_pass / imp_total * 100.0) if imp_total else 0.0

    delta = compute_delta(baseline, improved)

    return {
        "baseline": {
            "total_files": base_total,
            "mlir_success": base_pass,
            "mlir_fail": base_fail,
            "success_rate": base_rate,
        },
        "improved": {
            "total_files": imp_total,
            "mlir_success": imp_pass,
            "mlir_fail": imp_fail,
            "success_rate": imp_rate,
        },
        "delta": {
            "total_files": delta["total_files"],
            "mlir_success": delta["mlir_success"],
            "mlir_fail": delta["mlir_fail"],
            "success_rate_pct": round(delta["success_rate_pct"], 2),
        },
        "regressions": regressions,
        "new_passes": new_passes,
        "regression_count": len(regressions),
        "new_pass_count": len(new_passes),
    }


def format_md(baseline, improved, regressions, new_passes):
    """Produce human-readable Markdown summary."""
    base_total = baseline["total_files"]
    base_pass = baseline["mlir_success"]
    base_fail = baseline["mlir_fail"]
    base_rate = (base_pass / base_total * 100.0) if base_total else 0.0

    imp_total = improved["total_files"]
    imp_pass = improved["mlir_success"]
    imp_fail = improved["mlir_fail"]
    imp_rate = (imp_pass / imp_total * 100.0) if imp_total else 0.0

    delta = compute_delta(baseline, improved)
    emitter_delta = compute_per_emitter_delta(baseline, improved)

    lines = []
    lines.append("# MLIR Report Comparison (Baseline vs Improved)\n")

    # Summary table
    lines.append("## Summary")
    lines.append("")
    lines.append("| Metric | Baseline | Improved | Delta (abs) | Delta (%) |")
    lines.append("|--------|----------|----------|-------------|-----------|")

    def fmt_delta_int(x):
        if x == 0:
            return "0"
        return "{:+d}".format(int(x)) if isinstance(x, (int, float)) else str(x)

    def fmt_delta_pct(x):
        if x == 0:
            return "0"
        return "{:+.2f}pp".format(x)

    lines.append(
        "| total_files | {} | {} | {} | - |".format(
            base_total, imp_total, fmt_delta_int(delta["total_files"])
        )
    )
    lines.append(
        "| mlir_success | {} | {} | {} | - |".format(
            base_pass, imp_pass, fmt_delta_int(delta["mlir_success"])
        )
    )
    lines.append(
        "| mlir_fail | {} | {} | {} | - |".format(
            base_fail, imp_fail, fmt_delta_int(delta["mlir_fail"])
        )
    )
    lines.append(
        "| success_rate | {:.2f}% | {:.2f}% | {} | {} |".format(
            base_rate, imp_rate, fmt_delta_pct(delta["success_rate_pct"]), "-"
        )
    )
    lines.append("")

    # Per-emitter
    lines.append("## Per-Emitter Changes")
    lines.append("")
    lines.append("| Emitter | Pass Delta | Fail Delta |")
    lines.append("|---------|------------|------------|")
    for name in sorted(emitter_delta.keys()):
        d = emitter_delta[name]
        lines.append(
            "| {} | {} | {} |".format(
                name, fmt_delta_int(d["pass_delta"]), fmt_delta_int(d["fail_delta"])
            )
        )
    lines.append("")

    # Regressions
    lines.append("## Regressions (PASS → FAIL)")
    lines.append("")
    lines.append("Count: {}".format(len(regressions)))
    lines.append("")
    if regressions:
        for p in regressions:
            lines.append("- {}".format(p))
        lines.append("")
    else:
        lines.append("(none)")
        lines.append("")

    # Top new passes
    lines.append("## New Passes (FAIL → PASS)")
    lines.append("")
    lines.append("Count: {}".format(len(new_passes)))
    lines.append("")
    top_new = new_passes[:20] if len(new_passes) > 20 else new_passes
    if top_new:
        for p in top_new:
            lines.append("- {}".format(p))
        if len(new_passes) > 20:
            lines.append("")
            lines.append("_... and {} more_".format(len(new_passes) - 20))
        lines.append("")
    else:
        lines.append("(none)")
        lines.append("")

    # Conclusion
    lines.append("## Conclusion")
    lines.append("")
    if len(regressions) > 0 and len(new_passes) == 0 and delta["mlir_success"] <= 0:
        concl = (
            "Regression detected: {} module(s) no longer pass. No new passes.".format(
                len(regressions)
            )
        )
    elif len(regressions) == 0 and len(new_passes) > 0:
        concl = "Improvement: {} new module(s) pass, no regressions.".format(
            len(new_passes)
        )
    elif len(regressions) > 0 and len(new_passes) > 0:
        concl = "Mixed: {} regression(s), {} new pass(es).".format(
            len(regressions), len(new_passes)
        )
    elif len(regressions) == 0 and len(new_passes) == 0:
        if delta["mlir_success"] > 0:
            concl = (
                "Neutral: same module set, total success count increased by {}.".format(
                    delta["mlir_success"]
                )
            )
        elif delta["mlir_success"] < 0:
            concl = (
                "Neutral: same module set, total success count decreased by {}.".format(
                    abs(delta["mlir_success"])
                )
            )
        else:
            concl = "No change in pass/fail sets."
    else:
        concl = "Summary: {} regression(s), {} new pass(es).".format(
            len(regressions), len(new_passes)
        )
    lines.append(concl)
    lines.append("")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Compare baseline and improved report.json files.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""Examples:
  %(prog)s --baseline output/report.json --improved /tmp/new-report.json
  %(prog)s --baseline a.json --improved b.json --output-md diff.md --output-json diff.json
""",
    )
    parser.add_argument(
        "--baseline", required=True, help="Path to baseline report.json"
    )
    parser.add_argument(
        "--improved", required=True, help="Path to improved report.json"
    )
    parser.add_argument("--output-md", help="Write Markdown summary to this file")
    parser.add_argument("--output-json", help="Write JSON summary to this file")
    args = parser.parse_args()

    baseline = load_report(args.baseline)
    improved = load_report(args.improved)

    regressions, new_passes = compute_regressions_and_new_passes(baseline, improved)
    md_text = format_md(baseline, improved, regressions, new_passes)
    json_data = build_json_output(baseline, improved, regressions, new_passes)

    wrote_any = False
    if args.output_md:
        with open(args.output_md, "w", encoding="utf-8") as f:
            f.write(md_text)
        wrote_any = True
    if args.output_json:
        with open(args.output_json, "w", encoding="utf-8") as f:
            json.dump(json_data, f, indent=2)
        wrote_any = True

    if not wrote_any:
        print(md_text)


if __name__ == "__main__":
    main()
