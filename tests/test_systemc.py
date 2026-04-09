import subprocess, tempfile, textwrap
from pathlib import Path
from arc_to_cpp.parser import parse
from arc_to_cpp.systemc import emit_systemc_wrapper
from arc_to_cpp.emitter import emit

_FIXTURES = Path(__file__).parent / "fixtures"
SIMPLE = (_FIXTURES / "simple_arc.mlir").read_text()

SC_STUB = textwrap.dedent("""
    #pragma once
    #include <cstdint>
    struct _SB { template<typename T> _SB& operator<<(const T&){return *this;} _SB pos(){return *this;} };
    inline _SB sensitive;
    template<typename T> struct sc_in  { T read() const { return T{}; } _SB pos(){return _SB{};} };
    template<typename T> struct sc_out { void write(T) {} };
    #define SC_MODULE(name) struct name
    #define SC_CTOR(name)   name(const char* = "")
    #define SC_METHOD(f)    (void)0
""")

def test_generates_sc_module():
    _, mods = parse(SIMPLE)
    code = emit_systemc_wrapper(mods[0])
    assert "SC_MODULE" in code and "sc_in<" in code and "sc_out<" in code

def test_has_all_ports():
    _, mods = parse(SIMPLE)
    mod = mods[0]
    code = emit_systemc_wrapper(mod)
    for p in mod.in_ports:
        if p.ctype != "bool":  # skip clock port
            assert p.name in code
    for p in mod.out_ports:
        assert p.name in code

def test_compiles_with_stub():
    defs, mods = parse(SIMPLE)
    cpp = emit(defs, mods)
    sc = emit_systemc_wrapper(mods[0])
    mod_name = mods[0].name
    with tempfile.TemporaryDirectory() as d:
        open(f"{d}/systemc.h", "w").write(SC_STUB)
        open(f"{d}/{mod_name}.h", "w").write(cpp)
        sc_patched = sc.replace("#include <systemc.h>", '#include "systemc.h"')
        open(f"{d}/{mod_name}SC.h", "w").write(sc_patched)
        r = subprocess.run(
            ["clang++", "-std=c++17", "-Wall", "-fsyntax-only",
             "-x", "c++-header", f"-I{d}", f"{d}/{mod_name}SC.h"],
            capture_output=True, text=True,
        )
        assert r.returncode == 0, r.stderr
