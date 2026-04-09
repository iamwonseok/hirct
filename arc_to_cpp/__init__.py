"""Arc dialect MLIR → C++ emitter."""
from .parser import parse
from .emitter import emit
from .systemc import emit_systemc_wrapper
__all__ = ["parse", "emit", "emit_systemc_wrapper"]
