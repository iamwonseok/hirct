import subprocess, sys, tempfile
from pathlib import Path

PYTHON = sys.executable
FIX = Path(__file__).parent / "fixtures"

def run(args, stdin=None):
    return subprocess.run([PYTHON, "-m", "arc_to_cpp"] + args,
                          capture_output=True, text=True, input=stdin)

def test_stdout():
    r = run([str(FIX / "simple_arc.mlir")])
    assert r.returncode == 0 and "#pragma once" in r.stdout

def test_output_dir():
    with tempfile.TemporaryDirectory() as d:
        r = run([str(FIX / "simple_arc.mlir"), "-o", d])
        assert r.returncode == 0
        assert len(list(Path(d).glob("*.h"))) == 1

def test_stdin():
    mlir = (FIX / "simple_arc.mlir").read_text()
    r = run(["-"], stdin=mlir)
    assert r.returncode == 0 and "struct TopState" in r.stdout

def test_missing_file():
    r = run(["nonexistent.mlir"])
    assert r.returncode != 0
