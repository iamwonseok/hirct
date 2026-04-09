"""MLIR type → C++ type mapping."""
import re
from typing import Optional

def bits_of(mlir_type: str) -> int:
    t = mlir_type.strip()
    if "seq.clock" in t:
        return 1
    m = re.match(r'i(\d+)', t)
    return int(m.group(1)) if m else 32

def cpp_uint(bits: int) -> str:
    if bits == 1:  return "bool"
    if bits <= 8:  return "uint8_t"
    if bits <= 16: return "uint16_t"
    if bits <= 32: return "uint32_t"
    if bits <= 64: return "uint64_t"
    return f"uint64_t /*i{bits} truncated*/"

def cpp_type(mlir_type: str) -> str:
    t = mlir_type.strip().lstrip("!")
    if "seq.clock" in t:
        return "bool"
    # hw.array<N x iW> → std::array<elem_ctype, N>  (value type that supports copy)
    m = re.match(r'hw\.array<(\d+)\s*x\s*(i\d+)>', t)
    if m:
        n, elem_t = m.group(1), cpp_uint(int(m.group(2)[1:]))
        return f"std::array<{elem_t}, {n}>"
    m = re.match(r'i(\d+)', t)
    return cpp_uint(int(m.group(1))) if m else "uint32_t"

def cpp_array_type(mlir_type: str) -> tuple[str, int] | None:
    """Parse !hw.array<N x iW>, returns (elem_ctype, N) or None."""
    t = mlir_type.strip().lstrip("!")
    m = re.match(r'hw\.array<(\d+)\s*x\s*(i\d+)>', t)
    if m:
        return cpp_uint(int(m.group(2)[1:])), int(m.group(1))
    return None

def parse_memory_type(mlir_type: str) -> Optional[tuple[int, str, int]]:
    """Parse '<N x iW, iA>'. Returns (num_words, word_ctype, addr_bits) or None."""
    m = re.match(r'<(\d+)\s*x\s*(i\d+),\s*(i\d+)>', mlir_type.strip())
    if not m:
        return None
    return int(m.group(1)), cpp_uint(int(m.group(2)[1:])), int(m.group(3)[1:])
