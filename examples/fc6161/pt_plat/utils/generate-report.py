#!/usr/bin/env python3
"""Collect per-module meta.json files and produce output/report.json.

Usage:
    python3 utils/generate-report.py [--meta-dir DIR] [--output PATH]

Defaults:
    --meta-dir  output
    --output    output/report.json
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

EMITTER_NAMES = [
    "gen-model",
    "gen-tb",
    "gen-makefile",
    "gen-verify",
    "gen-dpic",
    "gen-wrapper",
    "gen-format",
    "gen-ral",
    "gen-doc",
    "gen-cocotb",
]


def find_meta_files(meta_dir: str) -> list[Path]:
    """Recursively find all meta.json files under *meta_dir*."""
    result: list[Path] = []
    for root, _dirs, files in os.walk(meta_dir):
        if "meta.json" in files:
            result.append(Path(root) / "meta.json")
    result.sort()
    return result


def load_meta(path: Path) -> dict[str, Any] | None:
    """Load and validate a single meta.json, returning None on parse error."""
    try:
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
        if not isinstance(data, dict):
            return None
        return data
    except (json.JSONDecodeError, OSError) as exc:
        print(f"WARN: cannot read {path}: {exc}", file=sys.stderr)
        return None


def build_report(meta_dir: str) -> dict[str, Any]:
    """Walk *meta_dir*, collect meta.json files, and build the report dict."""
    meta_paths = find_meta_files(meta_dir)

    files_list: list[dict[str, Any]] = []
    mlir_success = 0
    mlir_fail = 0
    infra_error = 0

    per_emitter: dict[str, dict[str, int]] = {}
    for name in EMITTER_NAMES:
        per_emitter[name] = {"pass": 0, "fail": 0, "skipped": 0}

    for mp in meta_paths:
        data = load_meta(mp)
        if data is None:
            infra_error += 1
            files_list.append(
                {
                    "path": str(mp.parent),
                    "mlir": "infra-error",
                    "reason": f"meta.json parse failed: {mp}",
                    "emitters": {},
                }
            )
            continue

        mlir_status = data.get("mlir", "fail")
        if mlir_status == "pass":
            mlir_success += 1
        else:
            mlir_fail += 1

        emitters_data = data.get("emitters", {})
        emitters_summary: dict[str, str] = {}
        for name in EMITTER_NAMES:
            entry = emitters_data.get(name, {})
            result_val = entry.get("result", "missing")
            emitters_summary[name] = result_val
            if result_val in per_emitter.get(name, {}):
                per_emitter[name][result_val] += 1

        entry_record: dict[str, Any] = {
            "path": data.get("path", str(mp.parent)),
            "top": data.get("top", ""),
            "mlir": mlir_status,
            "emitters": emitters_summary,
        }
        reason = data.get("reason", "")
        if reason:
            entry_record["reason"] = reason

        files_list.append(entry_record)

    per_emitter_clean: dict[str, dict[str, int]] = {}
    for name in EMITTER_NAMES:
        per_emitter_clean[name] = {
            "pass": per_emitter[name]["pass"],
            "fail": per_emitter[name]["fail"],
        }

    report: dict[str, Any] = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "meta_dir": meta_dir,
        "total_files": len(meta_paths),
        "mlir_success": mlir_success,
        "mlir_fail": mlir_fail,
        "infra_error": infra_error,
        "per_emitter": per_emitter_clean,
        "files": files_list,
    }
    return report


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Collect meta.json files and produce report.json"
    )
    parser.add_argument(
        "--meta-dir",
        default="output",
        help="Root directory containing meta.json files (default: output)",
    )
    parser.add_argument(
        "--output",
        default="output/report.json",
        help="Output report path (default: output/report.json)",
    )
    args = parser.parse_args()

    if not os.path.isdir(args.meta_dir):
        print(f"ERROR: meta directory not found: {args.meta_dir}", file=sys.stderr)
        return 1

    report = build_report(args.meta_dir)

    os.makedirs(os.path.dirname(args.output) or ".", exist_ok=True)
    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(report, f, indent=2, ensure_ascii=False)

    total = report["total_files"]
    mlir_ok = report["mlir_success"]
    mlir_ng = report["mlir_fail"]
    infra = report["infra_error"]
    pct = (mlir_ok / total * 100) if total > 0 else 0.0

    print(f"=== HIRCT Report Summary ===")
    print(f"Total modules:   {total}")
    print(f"MLIR success:    {mlir_ok} ({pct:.1f}%)")
    print(f"MLIR fail:       {mlir_ng}")
    print(f"Infra error:     {infra}")
    print()
    print("Per-emitter (pass / fail):")
    for name in EMITTER_NAMES:
        p = report["per_emitter"][name]["pass"]
        fl = report["per_emitter"][name]["fail"]
        print(f"  {name:16s}  {p:5d} / {fl:5d}")
    print()
    print(f"Report written to: {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
