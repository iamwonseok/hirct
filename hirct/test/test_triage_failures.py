#!/usr/bin/env python3
"""Tests for utils/triage-failures.py.

Creates temporary fixture directories with sample meta.json and
verify-report.json files, then verifies classification is correct
for every category defined in the triage spec.

Run:
    python3 test/test_triage_failures.py
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO_ROOT / "utils"))

import importlib

triage_mod = importlib.import_module("triage-failures")

classify_module = triage_mod.classify_module
build_triage_report = triage_mod.build_triage_report
find_meta_files = triage_mod.find_meta_files
load_json = triage_mod.load_json
_extract_origin_op = triage_mod._extract_origin_op
TIMEOUT_THRESHOLD_MS = triage_mod.TIMEOUT_THRESHOLD_MS


def _write_meta(base: Path, subdir: str, data: dict[str, Any]) -> Path:
    """Write a meta.json fixture under base/subdir/."""
    d = base / subdir
    d.mkdir(parents=True, exist_ok=True)
    p = d / "meta.json"
    p.write_text(json.dumps(data), encoding="utf-8")
    return p


class TestClassifyModule(unittest.TestCase):
    """Unit tests for classify_module()."""

    def test_parse_error(self) -> None:
        meta = {"mlir": "fail", "reason": "parse error: unknown module 'Bar'"}
        cat, reason = classify_module(meta, set())
        self.assertEqual(cat, "parse_error")
        self.assertIn("parse error:", reason)

    def test_unsupported_op_via_emitter(self) -> None:
        meta = {
            "mlir": "pass",
            "emitters": {
                "gen-model": {"result": "fail", "reason": "unsupported op: seq.firmem"}
            },
        }
        cat, reason = classify_module(meta, set())
        self.assertEqual(cat, "unsupported_op")
        self.assertIn("seq.firmem", reason)

    def test_unsupported_op_via_field(self) -> None:
        meta = {"mlir": "pass", "unsupported_ops": ["llhd.sig", "seq.firmem"]}
        cat, reason = classify_module(meta, set())
        self.assertEqual(cat, "unsupported_op")
        self.assertIn("llhd.sig", reason)

    def test_multi_module(self) -> None:
        meta = {"mlir": "fail", "reason": "multiple modules: A, B"}
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "multi_module")

    def test_flatten_error(self) -> None:
        meta = {"mlir": "fail", "reason": "flatten error: cannot flatten"}
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "flatten_error")

    def test_timeout_reason(self) -> None:
        meta = {"mlir": "fail", "reason": "timeout: 600s exceeded"}
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "timeout")

    def test_timeout_elapsed_ms(self) -> None:
        meta = {"mlir": "pass", "elapsed_ms": TIMEOUT_THRESHOLD_MS + 1}
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "timeout")

    def test_timeout_under_threshold(self) -> None:
        meta = {"mlir": "pass", "elapsed_ms": TIMEOUT_THRESHOLD_MS - 1}
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "")

    def test_combinational_loop(self) -> None:
        meta = {"mlir": "pass", "combinational_loop": True}
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "combinational_loop")

    def test_combinational_loop_false(self) -> None:
        meta = {"mlir": "pass", "combinational_loop": False}
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "")

    def test_verify_mismatch_by_path(self) -> None:
        meta = {"mlir": "pass", "path": "rtl/x/mod.v", "top": "mod"}
        cat, _ = classify_module(meta, {"rtl/x/mod.v"})
        self.assertEqual(cat, "verify_mismatch")

    def test_verify_mismatch_by_top(self) -> None:
        meta = {"mlir": "pass", "path": "rtl/x/mod.v", "top": "ModA"}
        cat, _ = classify_module(meta, {"ModA"})
        self.assertEqual(cat, "verify_mismatch")

    def test_inout_port(self) -> None:
        meta = {"mlir": "fail", "reason": "inout port: bidir_bus"}
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "inout_port")

    def test_multi_clock(self) -> None:
        meta = {"mlir": "fail", "reason": "multi clock: clk1, clk2"}
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "multi_clock")

    def test_wide_signal(self) -> None:
        meta = {"mlir": "fail", "reason": "wide signal: data[1023:0]"}
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "wide_signal")

    def test_success_no_category(self) -> None:
        meta = {
            "mlir": "pass",
            "emitters": {"gen-model": {"result": "pass"}},
        }
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "")

    def test_mlir_fail_unknown_reason_not_classified(self) -> None:
        meta = {"mlir": "fail", "reason": "something else entirely"}
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "")

    def test_combinational_loop_takes_priority(self) -> None:
        """combinational_loop flag should win over reason-based checks."""
        meta = {
            "mlir": "fail",
            "reason": "parse error: unknown",
            "combinational_loop": True,
        }
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "combinational_loop")

    def test_timeout_takes_priority_over_mlir(self) -> None:
        meta = {"mlir": "fail", "reason": "timeout: exceeded limit"}
        cat, _ = classify_module(meta, set())
        self.assertEqual(cat, "timeout")


class TestExtractOriginOp(unittest.TestCase):
    """Unit tests for _extract_origin_op()."""

    def test_from_unsupported_ops_list(self) -> None:
        meta = {"unsupported_ops": ["seq.firmem", "llhd.sig"]}
        self.assertEqual(_extract_origin_op(meta), "seq.firmem, llhd.sig")

    def test_from_emitter_reason(self) -> None:
        meta = {"emitters": {"gen-model": {"reason": "unsupported op: comb.mux2"}}}
        self.assertEqual(_extract_origin_op(meta), "comb.mux2")

    def test_empty(self) -> None:
        self.assertEqual(_extract_origin_op({}), "")


class TestBuildTriageReport(unittest.TestCase):
    """Integration tests for build_triage_report()."""

    def setUp(self) -> None:
        self.tmpdir = tempfile.mkdtemp(prefix="triage_test_")
        self.meta_dir = os.path.join(self.tmpdir, "output")
        os.makedirs(self.meta_dir)

    def tearDown(self) -> None:
        import shutil

        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def _write_fixture(
        self,
        subdir: str,
        data: dict[str, Any],
    ) -> None:
        _write_meta(Path(self.meta_dir), subdir, data)

    def _write_verify_report(
        self,
        modules: list[dict[str, Any]],
    ) -> str:
        vr = {
            "generated_at": "2026-01-01T00:00:00Z",
            "modules": modules,
        }
        path = os.path.join(self.meta_dir, "verify-report.json")
        with open(path, "w", encoding="utf-8") as f:
            json.dump(vr, f)
        return path

    def test_all_categories(self) -> None:
        self._write_fixture(
            "a/parse",
            {
                "mlir": "fail",
                "reason": "parse error: unknown module 'X'",
                "path": "rtl/a/parse.v",
                "top": "parse_mod",
            },
        )
        self._write_fixture(
            "b/unsup_emitter",
            {
                "mlir": "pass",
                "path": "rtl/b/unsup.v",
                "top": "unsup_mod",
                "emitters": {
                    "gen-model": {
                        "result": "fail",
                        "reason": "unsupported op: seq.firmem",
                    }
                },
            },
        )
        self._write_fixture(
            "c/unsup_field",
            {
                "mlir": "pass",
                "path": "rtl/c/unsup2.v",
                "top": "unsup2_mod",
                "unsupported_ops": ["llhd.sig"],
            },
        )
        self._write_fixture(
            "d/multi",
            {
                "mlir": "fail",
                "reason": "multiple modules: A, B",
                "path": "rtl/d/multi.v",
                "top": "multi_mod",
            },
        )
        self._write_fixture(
            "e/flat",
            {
                "mlir": "fail",
                "reason": "flatten error: cannot flatten",
                "path": "rtl/e/flat.v",
                "top": "flat_mod",
            },
        )
        self._write_fixture(
            "f/timeout",
            {
                "mlir": "fail",
                "reason": "timeout: 600s exceeded",
                "path": "rtl/f/slow.v",
                "top": "slow_mod",
            },
        )
        self._write_fixture(
            "g/timeout_ms",
            {
                "mlir": "pass",
                "path": "rtl/g/vslow.v",
                "top": "vslow_mod",
                "elapsed_ms": 700000,
            },
        )
        self._write_fixture(
            "h/combloop",
            {
                "mlir": "pass",
                "path": "rtl/h/loop.v",
                "top": "loop_mod",
                "combinational_loop": True,
            },
        )
        self._write_fixture(
            "i/inout",
            {
                "mlir": "fail",
                "reason": "inout port: bidir_bus",
                "path": "rtl/i/inout.v",
                "top": "inout_mod",
            },
        )
        self._write_fixture(
            "j/mclk",
            {
                "mlir": "fail",
                "reason": "multi clock: clk1, clk2",
                "path": "rtl/j/mclk.v",
                "top": "mclk_mod",
            },
        )
        self._write_fixture(
            "k/wide",
            {
                "mlir": "fail",
                "reason": "wide signal: data[1023:0]",
                "path": "rtl/k/wide.v",
                "top": "wide_mod",
            },
        )
        self._write_fixture(
            "l/good",
            {
                "mlir": "pass",
                "path": "rtl/l/good.v",
                "top": "good_mod",
                "emitters": {"gen-model": {"result": "pass"}},
            },
        )
        self._write_fixture(
            "m/vfail",
            {
                "mlir": "pass",
                "path": "rtl/m/vfail.v",
                "top": "vfail_mod",
                "emitters": {"gen-model": {"result": "pass"}},
            },
        )

        vr_path = self._write_verify_report(
            [
                {
                    "name": "vfail_mod",
                    "path": "rtl/m/vfail.v",
                    "status": "fail",
                    "seeds": [{"seed": 42, "result": "fail", "cycles": 347}],
                },
            ]
        )

        report = build_triage_report(self.meta_dir, None, vr_path)

        self.assertEqual(report["total_failures"], 12)

        cats = report["categories"]
        self.assertEqual(cats["parse_error"]["count"], 1)
        self.assertEqual(cats["unsupported_op"]["count"], 2)
        self.assertEqual(cats["multi_module"]["count"], 1)
        self.assertEqual(cats["flatten_error"]["count"], 1)
        self.assertEqual(cats["timeout"]["count"], 2)
        self.assertEqual(cats["combinational_loop"]["count"], 1)
        self.assertEqual(cats["verify_mismatch"]["count"], 1)
        self.assertEqual(cats["inout_port"]["count"], 1)
        self.assertEqual(cats["multi_clock"]["count"], 1)
        self.assertEqual(cats["wide_signal"]["count"], 1)

        self.assertNotIn("good_mod", str(cats))

        self.assertIn("vfail_mod", cats["verify_mismatch"]["modules"])

    def test_known_limitations_candidates(self) -> None:
        self._write_fixture(
            "a/parse",
            {
                "mlir": "fail",
                "reason": "parse error: syntax error",
                "path": "rtl/a.v",
                "top": "a_mod",
            },
        )
        self._write_fixture(
            "b/unsup",
            {
                "mlir": "pass",
                "path": "rtl/b.v",
                "top": "b_mod",
                "unsupported_ops": ["seq.firmem"],
            },
        )

        report = build_triage_report(self.meta_dir, None, None)

        kl = report["known_limitations_candidates"]
        self.assertEqual(len(kl), 2)
        cats = {c["category"] for c in kl}
        self.assertIn("parse_error", cats)
        self.assertIn("unsupported_op", cats)

        unsup_entry = next(c for c in kl if c["category"] == "unsupported_op")
        self.assertEqual(unsup_entry["origin_op"], "seq.firmem")

    def test_phase1_feedback(self) -> None:
        self._write_fixture(
            "a/unsup",
            {
                "mlir": "pass",
                "path": "rtl/a.v",
                "top": "a_mod",
                "unsupported_ops": ["seq.firmem"],
            },
        )
        self._write_fixture(
            "b/vfail",
            {
                "mlir": "pass",
                "path": "rtl/b.v",
                "top": "vfail_mod",
                "emitters": {"gen-model": {"result": "pass"}},
            },
        )
        vr_path = self._write_verify_report(
            [
                {
                    "name": "vfail_mod",
                    "path": "rtl/b.v",
                    "status": "fail",
                    "seeds": [{"seed": 7, "result": "fail", "cycles": 500}],
                },
            ]
        )

        report = build_triage_report(self.meta_dir, None, vr_path)

        fb = report["phase1_feedback"]
        self.assertEqual(len(fb), 2)

        unsup_fb = next(f for f in fb if f["category"] == "unsupported_op")
        self.assertEqual(unsup_fb["target_task"], "101-gen-model")

        vmm_fb = next(f for f in fb if f["category"] == "verify_mismatch")
        self.assertEqual(vmm_fb["seed"], 7)
        self.assertEqual(vmm_fb["cycle"], 500)

    def test_infra_error_on_bad_json(self) -> None:
        bad_dir = Path(self.meta_dir) / "bad_mod"
        bad_dir.mkdir(parents=True)
        (bad_dir / "meta.json").write_text("{invalid json", encoding="utf-8")

        report = build_triage_report(self.meta_dir, None, None)

        self.assertEqual(report["total_failures"], 1)
        self.assertIn("infra_error", report["categories"])
        self.assertEqual(report["categories"]["infra_error"]["count"], 1)

    def test_empty_meta_dir(self) -> None:
        report = build_triage_report(self.meta_dir, None, None)

        self.assertEqual(report["total_failures"], 0)
        self.assertIn("verilator_suspect", report["categories"])
        self.assertEqual(report["categories"]["verilator_suspect"]["count"], 0)

    def test_report_json_supplements_missing_meta(self) -> None:
        """Modules in report.json but without meta.json are still classified."""
        self._write_fixture(
            "a/has_meta",
            {
                "mlir": "fail",
                "reason": "parse error: test",
                "path": "rtl/a.v",
                "top": "a_mod",
            },
        )
        report_json = {
            "generated_at": "2026-01-01T00:00:00Z",
            "total_files": 2,
            "files": [
                {
                    "path": "rtl/a.v",
                    "top": "a_mod",
                    "mlir": "fail",
                    "reason": "parse error: test",
                },
                {
                    "path": "rtl/b.v",
                    "top": "b_mod",
                    "mlir": "fail",
                    "reason": "flatten error: cannot flatten",
                },
            ],
        }
        rp = os.path.join(self.meta_dir, "report.json")
        with open(rp, "w", encoding="utf-8") as f:
            json.dump(report_json, f)

        report = build_triage_report(self.meta_dir, rp, None)

        self.assertEqual(report["total_failures"], 2)
        self.assertIn("parse_error", report["categories"])
        self.assertIn("flatten_error", report["categories"])
        self.assertEqual(report["categories"]["flatten_error"]["count"], 1)

    def test_report_json_no_duplicate_when_meta_exists(self) -> None:
        """Modules already found via meta.json are not double-counted from report.json."""
        self._write_fixture(
            "a/mod",
            {
                "mlir": "fail",
                "reason": "parse error: x",
                "path": "rtl/a.v",
                "top": "a_mod",
            },
        )
        report_json = {
            "generated_at": "2026-01-01T00:00:00Z",
            "total_files": 1,
            "files": [
                {
                    "path": "rtl/a.v",
                    "top": "a_mod",
                    "mlir": "fail",
                    "reason": "parse error: x",
                },
            ],
        }
        rp = os.path.join(self.meta_dir, "report.json")
        with open(rp, "w", encoding="utf-8") as f:
            json.dump(report_json, f)

        report = build_triage_report(self.meta_dir, rp, None)

        self.assertEqual(report["total_failures"], 1)
        self.assertEqual(report["categories"]["parse_error"]["count"], 1)

    def test_verify_mismatch_not_classified_without_verify_report(self) -> None:
        self._write_fixture(
            "a/mod",
            {
                "mlir": "pass",
                "path": "rtl/a.v",
                "top": "mod_a",
                "emitters": {"gen-model": {"result": "pass"}},
            },
        )
        report = build_triage_report(self.meta_dir, None, None)
        self.assertEqual(report["total_failures"], 0)


class TestCLI(unittest.TestCase):
    """End-to-end CLI tests."""

    def setUp(self) -> None:
        self.tmpdir = tempfile.mkdtemp(prefix="triage_cli_")
        self.meta_dir = os.path.join(self.tmpdir, "output")
        os.makedirs(self.meta_dir)
        self.script = str(REPO_ROOT / "utils" / "triage-failures.py")

    def tearDown(self) -> None:
        import shutil

        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def test_missing_meta_dir_exits_1(self) -> None:
        result = subprocess.run(
            [sys.executable, self.script, "--meta-dir", "/nonexistent_dir_xyz"],
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 1)
        self.assertIn("ERROR", result.stderr)

    def test_missing_reports_partial_classification(self) -> None:
        mod_dir = os.path.join(self.meta_dir, "a", "mod")
        os.makedirs(mod_dir)
        with open(os.path.join(mod_dir, "meta.json"), "w") as f:
            json.dump(
                {
                    "mlir": "fail",
                    "reason": "parse error: test",
                    "path": "rtl/a.v",
                    "top": "mod",
                },
                f,
            )

        output_path = os.path.join(self.tmpdir, "triage-report.json")
        result = subprocess.run(
            [
                sys.executable,
                self.script,
                "--meta-dir",
                self.meta_dir,
                "--report",
                os.path.join(self.meta_dir, "nonexistent-report.json"),
                "--verify-report",
                os.path.join(self.meta_dir, "nonexistent-verify.json"),
                "--output",
                output_path,
            ],
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0)
        self.assertIn("WARN", result.stderr)

        with open(output_path, encoding="utf-8") as f:
            report = json.load(f)
        self.assertEqual(report["total_failures"], 1)
        self.assertIn("parse_error", report["categories"])

    def test_full_run_produces_valid_json(self) -> None:
        for name, data in [
            (
                "parse",
                {
                    "mlir": "fail",
                    "reason": "parse error: x",
                    "path": "rtl/p.v",
                    "top": "p",
                },
            ),
            (
                "loop",
                {
                    "mlir": "pass",
                    "path": "rtl/l.v",
                    "top": "l",
                    "combinational_loop": True,
                },
            ),
        ]:
            mod_dir = os.path.join(self.meta_dir, name)
            os.makedirs(mod_dir)
            with open(os.path.join(mod_dir, "meta.json"), "w") as f:
                json.dump(data, f)

        output_path = os.path.join(self.tmpdir, "triage-report.json")
        result = subprocess.run(
            [
                sys.executable,
                self.script,
                "--meta-dir",
                self.meta_dir,
                "--output",
                output_path,
            ],
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0)
        self.assertIn("Triage report written to:", result.stdout)
        self.assertIn("=== HIRCT Triage Summary ===", result.stdout)

        with open(output_path, encoding="utf-8") as f:
            report = json.load(f)
        self.assertIn("generated_at", report)
        self.assertIn("categories", report)
        self.assertEqual(report["total_failures"], 2)

    def test_tsv_output_columns(self) -> None:
        mod_dir = os.path.join(self.meta_dir, "x")
        os.makedirs(mod_dir)
        with open(os.path.join(mod_dir, "meta.json"), "w") as f:
            json.dump(
                {
                    "mlir": "fail",
                    "reason": "parse error: syntax",
                    "path": "rtl/x.v",
                    "top": "x",
                },
                f,
            )

        output_path = os.path.join(self.tmpdir, "triage-report.json")
        result = subprocess.run(
            [
                sys.executable,
                self.script,
                "--meta-dir",
                self.meta_dir,
                "--output",
                output_path,
            ],
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0)

        tsv_lines = [
            line
            for line in result.stdout.splitlines()
            if "\t" in line and not line.startswith("===")
        ]
        self.assertTrue(len(tsv_lines) >= 1, "Expected at least one TSV data line")

        header_line = None
        for line in result.stdout.splitlines():
            if line.startswith("Path\t"):
                header_line = line
                break
        self.assertIsNotNone(header_line)
        columns = header_line.split("\t")
        self.assertEqual(
            columns, ["Path", "Category", "Origin Op", "Reason", "Fix Phase", "Date"]
        )

        data_line = tsv_lines[0]
        fields = data_line.split("\t")
        self.assertEqual(
            len(fields), 6, f"Expected 6 TSV fields, got {len(fields)}: {fields}"
        )


if __name__ == "__main__":
    unittest.main()
