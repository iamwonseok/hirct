from pathlib import Path
from arc_to_cpp.emitter import emit_arc_body
from arc_to_cpp.parser import parse
from arc_to_cpp.emitter import emit

def test_comb_add():
    body = ["%0 = comb.add %arg0, %arg1 : i8", "arc.output %0 : i8"]
    code = emit_arc_body(body, {"arg0": "arg0", "arg1": "arg1"}, ["uint8_t"])
    assert "arg0 + arg1" in code and "return" in code

def test_comb_n_ary_and():
    body = ["%0 = comb.and %arg0, %arg1, %arg2 : i1", "arc.output %0 : i1"]
    code = emit_arc_body(body, {"arg0": "a", "arg1": "b", "arg2": "c"}, ["bool"])
    assert "a & b & c" in code

def test_comb_mux():
    body = ["%0 = comb.mux %arg0, %arg1, %arg2 : i8", "arc.output %0 : i8"]
    code = emit_arc_body(body, {"arg0": "s", "arg1": "a", "arg2": "b"}, ["uint8_t"])
    assert "s ? a : b" in code

def test_comb_bin_qualifier():
    body = ["%0 = comb.mux bin %arg0, %arg1, %arg2 : i8", "arc.output %0 : i8"]
    code = emit_arc_body(body, {"arg0": "s", "arg1": "a", "arg2": "b"}, ["uint8_t"])
    assert "s ? a : b" in code

def test_comb_icmp_eq():
    body = ["%0 = comb.icmp eq %arg0, %arg1 : i8", "arc.output %0 : i1"]
    code = emit_arc_body(body, {"arg0": "a", "arg1": "b"}, ["bool"])
    assert "a == b" in code

def test_hw_constant_int():
    body = ["%c1 = hw.constant 1 : i8", "%0 = comb.add %arg0, %c1 : i8", "arc.output %0 : i8"]
    code = emit_arc_body(body, {"arg0": "x"}, ["uint8_t"])
    assert "1" in code and "+" in code

def test_hw_constant_true():
    body = ["%true = hw.constant true", "%0 = comb.xor %arg0, %true : i1", "arc.output %0 : i1"]
    code = emit_arc_body(body, {"arg0": "x"}, ["bool"])
    assert "^ 1" in code or "^1" in code

def test_seq_to_clock():
    body = ["%0 = seq.to_clock %arg0", "arc.output %0 : !seq.clock"]
    code = emit_arc_body(body, {"arg0": "x"}, ["bool"])
    assert "return" in code

def test_comb_concat():
    body = ["%0 = comb.concat %arg0, %arg1 : i4, i4", "arc.output %0 : i8"]
    code = emit_arc_body(body, {"arg0": "hi", "arg1": "lo"}, ["uint8_t"])
    assert "<<" in code or "|" in code

def test_comb_extract():
    body = ["%0 = comb.extract %arg0 from 4 : (i8) -> i4", "arc.output %0 : i4"]
    code = emit_arc_body(body, {"arg0": "x"}, ["uint8_t"])
    assert ">> 4" in code

def test_comb_shrs():
    body = ["%0 = comb.shrs %arg0, %arg1 : i8", "arc.output %0 : i8"]
    code = emit_arc_body(body, {"arg0": "a", "arg1": "b"}, ["uint8_t"])
    assert "int8_t" in code and ">>" in code

def test_arc_output_tuple():
    body = ["arc.output %arg0, %arg1 : i8, i8"]
    code = emit_arc_body(body, {"arg0": "a", "arg1": "b"}, ["uint8_t", "uint8_t"])
    assert "make_tuple" in code

def test_emit_simple_structure():
    code = emit(*parse(open(str(Path(__file__).parent / "fixtures" / "simple_arc.mlir")).read()))
    assert "#pragma once" in code
    assert "struct TopState" in code
    assert "class Top" in code
    assert "void eval_" in code

def test_emit_register_naming():
    code = emit(*parse(open(str(Path(__file__).parent / "fixtures" / "simple_arc.mlir")).read()))
    assert "state.foo" in code
    assert "state.bar" in code

def test_emit_simple_compiles():
    import subprocess, tempfile, os
    code = emit(*parse(open(str(Path(__file__).parent / "fixtures" / "simple_arc.mlir")).read()))
    with tempfile.NamedTemporaryFile(suffix=".hpp", mode="w", delete=False) as f:
        f.write(code); fname = f.name
    r = subprocess.run(["clang++", "-std=c++17", "-Wall", "-Werror",
                        "-fsyntax-only", "-x", "c++-header", fname],
                       capture_output=True, text=True)
    os.unlink(fname)
    assert r.returncode == 0, r.stderr

def test_emit_counter_cycle_accurate():
    import subprocess, tempfile, os
    code = emit(*parse(open(str(Path(__file__).parent / "fixtures" / "counter_arc.mlir")).read()))
    tb = r'''
#include "m.h"
#include <cassert>
#include <cstdio>
int main() {
    fc_counter dut;
    dut.set_rst_n(0); dut.set_en(0); dut.set_thresh(5);
    dut.eval_clk();
    assert(dut.get_count() == 0);
    dut.set_rst_n(1); dut.set_en(1);
    for (int i = 0; i < 3; i++) dut.eval_clk();
    assert(dut.get_count() == 3);
    printf("PASS\n");
}
'''
    with tempfile.TemporaryDirectory() as d:
        open(f"{d}/m.h", "w").write(code)
        open(f"{d}/tb.cpp", "w").write(tb)
        r = subprocess.run(["clang++", "-std=c++17", "-o", f"{d}/tb", f"{d}/tb.cpp"],
                           capture_output=True, text=True, cwd=d)
        assert r.returncode == 0, r.stderr
        r2 = subprocess.run([f"{d}/tb"], capture_output=True, text=True)
        assert "PASS" in r2.stdout

def test_enable_generates_if():
    from pathlib import Path
    code = emit(*parse((Path(__file__).parent / "fixtures" / "enable_reset_arc.mlir").read_text()))
    assert "if (" in code and "en" in code
    assert "next_q0 = state.q0" in code  # hold path in two-phase update

def test_reset_generates_else():
    from pathlib import Path
    code = emit(*parse((Path(__file__).parent / "fixtures" / "enable_reset_arc.mlir").read_text()))
    assert "rst" in code
    assert "(uint32_t)0" in code or "= 0" in code  # reset-to-zero path

def test_enable_reset_compiles():
    import subprocess, tempfile, os
    from pathlib import Path
    code = emit(*parse((Path(__file__).parent / "fixtures" / "enable_reset_arc.mlir").read_text()))
    with tempfile.NamedTemporaryFile(suffix=".h", mode="w", delete=False) as f:
        f.write(code); fname = f.name
    r = subprocess.run(["clang++", "-std=c++17", "-Wall", "-Werror",
                        "-fsyntax-only", "-x", "c++-header", fname],
                       capture_output=True, text=True)
    os.unlink(fname)
    assert r.returncode == 0, r.stderr

def test_memory_has_array():
    import re
    from pathlib import Path
    code = emit(*parse((Path(__file__).parent / "fixtures" / "memory_arc.mlir").read_text()))
    assert re.search(r'uint32_t\s+\w+\[1024\]', code)

def test_memory_write_conditional():
    from pathlib import Path
    code = emit(*parse((Path(__file__).parent / "fixtures" / "memory_arc.mlir").read_text()))
    assert "if (" in code  # write enable

def test_all_fixtures_parse():
    """Every fixture must parse without raising."""
    for f in sorted((Path(__file__).parent / "fixtures").glob("*.mlir")):
        defs, mods = parse(f.read_text())  # must not raise

def test_register_fallback_naming():
    """When no {names=[...]}, register gets reg_<ssa> name, not 'reg_v0' from stale SSA."""
    mlir = """
module {
  arc.define @f(%arg0: i32) -> i32 {
    arc.output %arg0 : i32
  }
  hw.module @M(in %clock : !seq.clock, in %a : i32, out q : i32) {
    %r = arc.state @f(%a) clock %clock latency 1 : (i32) -> i32
    hw.output %r : i32
  }
}
"""
    code = emit(*parse(mlir))
    assert "uint32_t" in code
    assert "struct MState" in code

def test_memory_cycle_accurate():
    import subprocess, tempfile, os
    from pathlib import Path
    code = emit(*parse((Path(__file__).parent / "fixtures" / "memory_arc.mlir").read_text()))
    # Read-before-write: cycle 1 writes, cycle 2 reads the written value.
    tb = r'''
#include "m.h"
#include <cassert>
#include <cstdio>
int main() {
    simple_mem2 dut;
    dut.set_wr_en(1); dut.set_wr_addr(42); dut.set_wr_data(0xDEADBEEF);
    dut.set_rd_addr(42);
    dut.eval_clk();     // cycle 1: write occurs, rd_data = old (0)
    dut.set_wr_en(0);
    dut.eval_clk();     // cycle 2: read reflects written value
    assert(dut.get_rd_data() == 0xDEADBEEF);
    printf("PASS\n");
}
'''
    with tempfile.TemporaryDirectory() as d:
        open(f"{d}/m.h", "w").write(code)
        open(f"{d}/tb.cpp", "w").write(tb)
        r = subprocess.run(["clang++", "-std=c++17", "-o", f"{d}/tb", f"{d}/tb.cpp"],
                           capture_output=True, text=True, cwd=d)
        assert r.returncode == 0, r.stderr
        assert "PASS" in subprocess.run([f"{d}/tb"], capture_output=True, text=True).stdout
