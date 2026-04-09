from arc_to_cpp.parser import parse

SIMPLE = open("tests/fixtures/simple_arc.mlir").read()
ENABLE_RESET = open("tests/fixtures/enable_reset_arc.mlir").read()
MEMORY = open("tests/fixtures/memory_arc.mlir").read()

def test_parse_arc_define():
    defs, _ = parse(SIMPLE)
    assert len(defs) >= 1
    d = defs[0]
    assert d.name == "Top_arc"
    assert d.args[0].name == "arg0" and d.args[0].ctype == "uint8_t"
    assert d.ret_ctypes == ["uint8_t"]

def test_parse_hw_module_ports():
    _, mods = parse(SIMPLE)
    m = mods[0]
    assert m.name == "Top"
    assert any(p.name == "i0" for p in m.in_ports)
    assert any(p.name == "out" for p in m.out_ports)

def test_parse_state_names():
    _, mods = parse(SIMPLE)
    names = [s.reg_name for s in mods[0].states]
    assert "foo" in names and "bar" in names

def test_parse_state_enable_reset():
    _, mods = parse(ENABLE_RESET)
    m = mods[0]
    q0 = next(s for s in m.states if s.reg_name == "q0")
    q1 = next(s for s in m.states if s.reg_name == "q1")
    assert q0.enable_id is not None
    assert q0.reset_id is None
    assert q1.enable_id is not None
    assert q1.reset_id is not None

def test_parse_memory():
    _, mods = parse(MEMORY)
    m = mods[0]
    assert len(m.memories) == 1
    assert m.memories[0].num_words == 1024
    assert m.memories[0].word_ctype == "uint32_t"

def test_parse_memory_read_write():
    _, mods = parse(MEMORY)
    m = mods[0]
    assert len(m.mem_reads) == 1
    assert len(m.mem_writes) == 1
    assert m.mem_writes[0].has_enable is True
