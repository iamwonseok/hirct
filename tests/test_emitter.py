from arc_to_cpp.emitter import emit_arc_body

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
