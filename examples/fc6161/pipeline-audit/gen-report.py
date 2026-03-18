#!/usr/bin/env python3
"""gen-report.py — Generate report.json for one IP and summary.json for all IPs.

Usage:
    # per-IP report
    python3 gen-report.py --ip <name> --base <output_base> [--filelist <path>] [--top <module>]

    # summary across all IPs
    python3 gen-report.py --summary --base <output_base>
"""

import argparse
import glob
import json
import os
import re
import sys


STAGES = [
    "stage-0-import",
    "stage-1-moore-to-core",
    "stage-2-llhd-to-core",
    "stage-3-lowering",
    "stage-4-genmodel",
    "stage-4-compile",
    "stage-v-verilator-pp",
]


def read_status(status_file: str) -> dict:
    if not os.path.isfile(status_file):
        return {"status": "missing", "time_sec": None}
    with open(status_file) as f:
        raw = f.read().strip()
    m = re.match(r"^(pass|fail|skip):(.+)$", raw)
    if m:
        st, detail = m.group(1), m.group(2)
        try:
            elapsed = int(detail)
            return {"status": st, "time_sec": elapsed}
        except ValueError:
            return {"status": st, "time_sec": None, "reason": detail}
    return {"status": "other", "time_sec": None, "reason": raw}


def build_ip_report(ip: str, base: str, filelist: str | None = None, top: str | None = None) -> dict:
    ip_dir = os.path.join(base, ip)
    result: dict = {"ip": ip}
    if filelist is not None:
        result["filelist"] = filelist
    if top is not None:
        result["top"] = top
    result["stages"] = {}
    for s in STAGES:
        status_file = os.path.join(ip_dir, s, ".status")
        result["stages"][s] = read_status(status_file)
    return result


def write_ip_report(ip: str, base: str, filelist: str | None = None, top: str | None = None) -> None:
    report = build_ip_report(ip, base, filelist=filelist, top=top)
    out_path = os.path.join(base, ip, "report.json")
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w") as f:
        json.dump(report, f, indent=2)
    print(f"[{ip}] report.json written.")


def write_summary(base: str) -> None:
    summary: dict = {"ips": {}, "totals": {}}

    for report_file in sorted(glob.glob(os.path.join(base, "*/report.json"))):
        with open(report_file) as f:
            data = json.load(f)
        summary["ips"][data["ip"]] = data["stages"]

    for s in STAGES:
        counts: dict = {"pass": 0, "fail": 0, "skip": 0, "missing": 0, "other": 0}
        for ip_data in summary["ips"].values():
            st = ip_data.get(s, {}).get("status", "missing")
            counts[st] = counts.get(st, 0) + 1
        summary["totals"][s] = counts

    out_path = os.path.join(base, "summary.json")
    with open(out_path, "w") as f:
        json.dump(summary, f, indent=2)

    print("\n=== Pipeline Audit Summary ===")
    for s in STAGES:
        c = summary["totals"].get(s, {})
        print(f"  {s:<35} pass={c.get('pass',0):3d}  "
              f"fail={c.get('fail',0):3d}  "
              f"skip={c.get('skip',0):3d}  "
              f"missing={c.get('missing',0):3d}  "
              f"other={c.get('other',0):3d}")
    print(f"\nsummary.json written to {out_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate pipeline audit reports")
    parser.add_argument("--ip", help="IP name for per-IP report")
    parser.add_argument("--summary", action="store_true", help="Generate summary.json")
    parser.add_argument("--base", required=True, help="Output base directory")
    parser.add_argument("--filelist", default=None, help="Filelist path for this IP")
    parser.add_argument("--top", default=None, help="Top module name for this IP")
    args = parser.parse_args()

    if args.ip:
        top = args.top if args.top and args.top != "-" else None
        write_ip_report(args.ip, args.base, filelist=args.filelist, top=top)
    if args.summary:
        write_summary(args.base)
    if not args.ip and not args.summary:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
