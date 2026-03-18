#!/usr/bin/env python3
"""Automatic failure triage for HIRCT Phase 2 traversal results.

Classifies per-module meta.json failures into categories (parse_error,
unsupported_op, multi_module, etc.) and produces a machine-readable
triage-report.json plus human-readable summaries on stdout.

Usage:
    python3 utils/triage-failures.py [options]

Options:
    --meta-dir DIR           Root directory with meta.json files (default: output)
    --report PATH            Input report.json path (default: output/report.json)
    --verify-report PATH     Input verify-report.json (default: output/verify-report.json)
    --output PATH            Output triage-report.json path (default: output/triage-report.json)
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

TIMEOUT_THRESHOLD_MS = 600_000

KNOWN_LIMITATIONS_CATEGORIES = frozenset(
    {
        "parse_error",
        "unsupported_op",
        "multi_module",
        "flatten_error",
        "timeout",
        "combinational_loop",
        "inout_port",
        "multi_clock",
        "wide_signal",
        "verilator_suspect",
    }
)

PHASE1_FEEDBACK_CATEGORIES = frozenset(
    {
        "unsupported_op",
        "verify_mismatch",
    }
)

FIX_PHASE_MAP: dict[str, str] = {
    "parse_error": "Phase 1",
    "unsupported_op": "Phase 1",
    "multi_module": "Phase 1",
    "flatten_error": "Phase 1",
    "timeout": "Phase 2",
    "combinational_loop": "Phase 2",
    "inout_port": "Phase 1",
    "multi_clock": "Phase 1",
    "wide_signal": "Phase 1",
    "verilator_suspect": "TBD",
}


def find_meta_files(meta_dir: str) -> list[Path]:
    """Recursively find all meta.json files under *meta_dir*."""
    result: list[Path] = []
    for root, _dirs, files in os.walk(meta_dir):
        if "meta.json" in files:
            result.append(Path(root) / "meta.json")
    result.sort()
    return result


def load_json(path: str | Path) -> dict[str, Any] | None:
    """Load a JSON file, returning None on error."""
    try:
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
        if not isinstance(data, dict):
            return None
        return data
    except (json.JSONDecodeError, OSError) as exc:
        print(f"WARN: cannot read {path}: {exc}", file=sys.stderr)
        return None


def classify_module(
    meta: dict[str, Any],
    verify_failures: set[str],
) -> tuple[str, str]:
    """Classify a single module and return (category, effective_reason).

    Returns ("", reason) when no failure category matches.
    """
    reason = meta.get("reason", "")
    mlir = meta.get("mlir", "")

    if meta.get("combinational_loop") is True:
        return "combinational_loop", reason or "combinational loop detected"

    if reason.startswith("timeout:"):
        return "timeout", reason
    elapsed_ms = meta.get("elapsed_ms")
    if isinstance(elapsed_ms, (int, float)) and elapsed_ms > TIMEOUT_THRESHOLD_MS:
        return "timeout", reason or f"timeout: elapsed_ms={elapsed_ms}"

    if reason.startswith("inout port:"):
        return "inout_port", reason
    if reason.startswith("multi clock:"):
        return "multi_clock", reason
    if reason.startswith("wide signal:"):
        return "wide_signal", reason

    if mlir == "fail":
        if reason.startswith("parse error:"):
            return "parse_error", reason
        if reason.startswith("multiple modules:"):
            return "multi_module", reason
        if reason.startswith("flatten error:"):
            return "flatten_error", reason

    if mlir == "pass":
        if meta.get("unsupported_ops"):
            ops = meta["unsupported_ops"]
            op_str = ", ".join(ops) if isinstance(ops, list) else str(ops)
            return "unsupported_op", f"unsupported op: {op_str}"
        emitters = meta.get("emitters", {})
        for _name, edata in emitters.items():
            if isinstance(edata, dict):
                ereason = edata.get("reason", "")
                if ereason.startswith("unsupported op:"):
                    return "unsupported_op", ereason

    module_path = meta.get("path", "")
    module_top = meta.get("top", "")
    if module_path in verify_failures or module_top in verify_failures:
        return "verify_mismatch", reason or "verification mismatch"

    return "", reason


def _extract_origin_op(meta: dict[str, Any]) -> str:
    """Extract the origin op name for unsupported_op classification."""
    if meta.get("unsupported_ops"):
        ops = meta["unsupported_ops"]
        return ", ".join(ops) if isinstance(ops, list) else str(ops)
    emitters = meta.get("emitters", {})
    for _name, edata in emitters.items():
        if isinstance(edata, dict):
            ereason = edata.get("reason", "")
            if ereason.startswith("unsupported op:"):
                return ereason.split(":", 1)[1].strip()
    return ""


def _find_verify_detail(
    verify_data: dict[str, Any] | None,
    module_path: str,
    module_top: str,
) -> dict[str, Any]:
    """Find seed/cycle detail for a verify_mismatch from verify-report."""
    if not verify_data:
        return {}
    for mod in verify_data.get("modules", []):
        if mod.get("path") == module_path or mod.get("name") == module_top:
            for seed_data in mod.get("seeds", []):
                if seed_data.get("result") == "fail":
                    result: dict[str, Any] = {}
                    if "seed" in seed_data:
                        result["seed"] = seed_data["seed"]
                    if "cycles" in seed_data:
                        result["cycle"] = seed_data["cycles"]
                    return result
    return {}


def _record_classified(
    meta: dict[str, Any],
    category: str,
    effective_reason: str,
    verify_data: dict[str, Any] | None,
    categorized: dict[str, list[dict[str, Any]]],
    reason_counters: dict[str, Counter[str]],
    known_limitations: list[dict[str, Any]],
    phase1_feedback: list[dict[str, Any]],
) -> None:
    """Record a classified module into all accumulator structures."""
    module_path = meta.get("path", "")
    module_top = meta.get("top", "")

    categorized.setdefault(category, []).append(
        {
            "path": module_path,
            "top": module_top,
            "category": category,
            "reason": effective_reason,
        }
    )

    short_reason = effective_reason
    if ":" in short_reason:
        short_reason = short_reason.split(":", 1)[1].strip()
    if not short_reason:
        short_reason = category
    reason_counters.setdefault(category, Counter())[short_reason] += 1

    if category in KNOWN_LIMITATIONS_CATEGORIES:
        origin_op = ""
        if category == "unsupported_op":
            origin_op = _extract_origin_op(meta)
        known_limitations.append(
            {
                "path": module_path,
                "category": category,
                "origin_op": origin_op,
                "reason": effective_reason,
            }
        )

    if category in PHASE1_FEEDBACK_CATEGORIES:
        fb: dict[str, Any] = {
            "module": module_top or module_path,
            "category": category,
            "target_task": "101-gen-model",
        }
        if category == "verify_mismatch":
            detail = _find_verify_detail(verify_data, module_path, module_top)
            if detail:
                fb.update(detail)
        phase1_feedback.append(fb)


def build_triage_report(
    meta_dir: str,
    report_path: str | None,
    verify_report_path: str | None,
) -> dict[str, Any]:
    """Build the triage report from all inputs."""
    verify_failures: set[str] = set()
    verify_data: dict[str, Any] | None = None
    if verify_report_path:
        verify_data = load_json(verify_report_path)
        if verify_data:
            for mod in verify_data.get("modules", []):
                if mod.get("status") == "fail":
                    if mod.get("path"):
                        verify_failures.add(mod["path"])
                    if mod.get("name"):
                        verify_failures.add(mod["name"])

    report_data: dict[str, Any] | None = None
    if report_path:
        report_data = load_json(report_path)

    meta_paths = find_meta_files(meta_dir)

    categorized: dict[str, list[dict[str, Any]]] = {}
    reason_counters: dict[str, Counter[str]] = {}
    known_limitations: list[dict[str, Any]] = []
    phase1_feedback: list[dict[str, Any]] = []
    seen_module_paths: set[str] = set()

    for mp in meta_paths:
        meta = load_json(mp)
        if meta is None:
            cat = "infra_error"
            categorized.setdefault(cat, []).append(
                {
                    "path": str(mp.parent),
                    "category": cat,
                    "reason": f"meta.json parse failed: {mp}",
                }
            )
            reason_counters.setdefault(cat, Counter())["parse failed"] += 1
            continue

        module_path = meta.get("path", str(mp.parent))
        seen_module_paths.add(module_path)

        category, effective_reason = classify_module(meta, verify_failures)
        if not category:
            continue

        _record_classified(
            meta,
            category,
            effective_reason,
            verify_data,
            categorized,
            reason_counters,
            known_limitations,
            phase1_feedback,
        )

    # Supplement from report.json: classify modules whose meta.json is missing
    if report_data:
        for entry in report_data.get("files", []):
            mod_path = entry.get("path", "")
            if not mod_path or mod_path in seen_module_paths:
                continue
            category, effective_reason = classify_module(entry, verify_failures)
            if not category:
                continue
            _record_classified(
                entry,
                category,
                effective_reason,
                verify_data,
                categorized,
                reason_counters,
                known_limitations,
                phase1_feedback,
            )

    categories_summary: dict[str, dict[str, Any]] = {}
    for cat, entries in sorted(categorized.items()):
        cat_info: dict[str, Any] = {"count": len(entries)}
        top = reason_counters.get(cat, Counter()).most_common(5)
        if top:
            cat_info["top_reasons"] = [f"{r} ({c})" for r, c in top]
        if cat == "verify_mismatch":
            cat_info["modules"] = [e.get("top") or e.get("path", "") for e in entries]
        categories_summary[cat] = cat_info

    # verilator_suspect: no auto-classification; populated by manual review only
    if "verilator_suspect" not in categories_summary:
        categories_summary["verilator_suspect"] = {"count": 0}

    total_failures = sum(len(v) for v in categorized.values())

    return {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "source": {
            "report": report_path or "",
            "verify_report": verify_report_path or "",
        },
        "total_failures": total_failures,
        "categories": categories_summary,
        "known_limitations_candidates": known_limitations,
        "phase1_feedback": phase1_feedback,
    }


def print_summary(report: dict[str, Any]) -> None:
    """Print human-readable triage summary to stdout."""
    print("=== HIRCT Triage Summary ===")
    print(f"Total failures: {report['total_failures']}")
    print()

    categories = report.get("categories", {})
    if not categories:
        print("No classified failures.")
        return

    print("Category breakdown:")
    for cat, data in sorted(categories.items(), key=lambda x: -x[1]["count"]):
        print(f"  {cat:25s}  {data['count']:5d}")
        for r in data.get("top_reasons", [])[:3]:
            print(f"    - {r}")
        for m in data.get("modules", [])[:5]:
            print(f"    * {m}")
    print()


def print_known_limitations_tsv(report: dict[str, Any]) -> None:
    """Print known-limitations.md candidates as TSV to stdout."""
    candidates = report.get("known_limitations_candidates", [])
    if not candidates:
        return

    today = datetime.now(timezone.utc).strftime("%Y-%m-%d")
    print("=== known-limitations.md candidates (TSV) ===")
    print("Path\tCategory\tOrigin Op\tReason\tFix Phase\tDate")
    for c in candidates:
        fix = FIX_PHASE_MAP.get(c["category"], "TBD")
        parts = [
            c["path"],
            c["category"],
            c.get("origin_op", ""),
            c["reason"],
            fix,
            today,
        ]
        print("\t".join(parts))
    print()


def print_pr_files(report: dict[str, Any]) -> None:
    """Print PR target files as JSON to stdout."""
    feedback = report.get("phase1_feedback", [])
    if not feedback:
        return

    files = sorted({f["module"] for f in feedback if f.get("module")})
    pr = {
        "files": files,
        "reason": f"Phase 1 feedback: {len(feedback)} modules need attention",
    }
    print("=== PR target files ===")
    print(json.dumps(pr, indent=2, ensure_ascii=False))
    print()


def main() -> int:
    """CLI entry point."""
    parser = argparse.ArgumentParser(
        description="Automatically triage HIRCT Phase 2 traversal failures"
    )
    parser.add_argument(
        "--meta-dir",
        default="output",
        help="Root directory with meta.json files (default: output)",
    )
    parser.add_argument(
        "--report",
        default=None,
        help="Input report.json path (default: <meta-dir>/report.json)",
    )
    parser.add_argument(
        "--verify-report",
        default=None,
        help="Input verify-report.json path (default: <meta-dir>/verify-report.json)",
    )
    parser.add_argument(
        "--output",
        default="output/triage-report.json",
        help="Output triage-report.json path (default: output/triage-report.json)",
    )
    args = parser.parse_args()

    if not os.path.isdir(args.meta_dir):
        print(f"ERROR: output directory not found: {args.meta_dir}", file=sys.stderr)
        return 1

    report_input = args.report or os.path.join(args.meta_dir, "report.json")
    report_path: str | None = report_input
    if not os.path.isfile(report_input):
        print(
            f"WARN: report not found: {report_input}; using meta.json only",
            file=sys.stderr,
        )
        report_path = None

    verify_input = args.verify_report or os.path.join(
        args.meta_dir, "verify-report.json"
    )
    verify_path: str | None = verify_input
    if not os.path.isfile(verify_input):
        print(
            f"WARN: verify-report not found: {verify_input}; "
            "skipping verify_mismatch classification",
            file=sys.stderr,
        )
        verify_path = None

    triage = build_triage_report(args.meta_dir, report_path, verify_path)

    os.makedirs(os.path.dirname(args.output) or ".", exist_ok=True)
    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(triage, f, indent=2, ensure_ascii=False)

    print_summary(triage)
    print_known_limitations_tsv(triage)
    print_pr_files(triage)
    print(f"Triage report written to: {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
