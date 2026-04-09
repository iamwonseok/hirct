import pytest
from arc_to_cpp.types import bits_of, cpp_uint, cpp_type, parse_memory_type

def test_bits_i1():    assert bits_of("i1") == 1
def test_bits_i8():    assert bits_of("i8") == 8
def test_bits_i32():   assert bits_of("i32") == 32
def test_bits_clock(): assert bits_of("!seq.clock") == 1
def test_bits_ws():    assert bits_of("  i16  ") == 16

def test_cpp_uint_bool():  assert cpp_uint(1) == "bool"
def test_cpp_uint_u8():    assert cpp_uint(8) == "uint8_t"
def test_cpp_uint_u16():   assert cpp_uint(16) == "uint16_t"
def test_cpp_uint_u32():   assert cpp_uint(32) == "uint32_t"
def test_cpp_uint_u64():   assert cpp_uint(64) == "uint64_t"
def test_cpp_uint_5bit():  assert cpp_uint(5) == "uint8_t"
def test_cpp_uint_42bit(): assert cpp_uint(42) == "uint64_t"

def test_cpp_type_clock():  assert cpp_type("!seq.clock") == "bool"
def test_cpp_type_i1():     assert cpp_type("i1") == "bool"
def test_cpp_type_i32():    assert cpp_type("i32") == "uint32_t"

def test_parse_mem_basic():
    assert parse_memory_type("<1024 x i32, i10>") == (1024, "uint32_t", 10)
def test_parse_mem_small():
    assert parse_memory_type("<16 x i8, i4>") == (16, "uint8_t", 4)
def test_parse_mem_invalid():
    assert parse_memory_type("i32") is None
