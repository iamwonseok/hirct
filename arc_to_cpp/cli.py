"""CLI entry point for arc-to-cpp emitter."""
import argparse
import sys
from pathlib import Path

from .parser import parse
from .emitter import emit


def main(argv=None):
    ap = argparse.ArgumentParser(
        prog="arc-to-cpp",
        description="Emit C++ from Arc dialect MLIR.",
    )
    ap.add_argument("input", help="Input .mlir file or - for stdin")
    ap.add_argument("-o", "--output-dir", default="-",
                    help="Output directory (default: stdout)")
    ap.add_argument("--systemc", action="store_true",
                    help="Also emit SystemC wrapper")
    args = ap.parse_args(argv)

    if args.input == "-":
        text = sys.stdin.read()
    else:
        p = Path(args.input)
        if not p.exists():
            print(f"arc-to-cpp: error: file not found: {args.input}", file=sys.stderr)
            sys.exit(1)
        text = p.read_text()

    arc_defs, hw_mods = parse(text)
    cpp_code = emit(arc_defs, hw_mods)

    if args.output_dir == "-":
        sys.stdout.write(cpp_code)
    else:
        out_dir = Path(args.output_dir)
        out_dir.mkdir(parents=True, exist_ok=True)
        fname = f"{hw_mods[0].name}.h" if hw_mods else "arc_output.h"
        (out_dir / fname).write_text(cpp_code)
        print(f"Written: {out_dir / fname}", file=sys.stderr)

    if args.systemc and hw_mods:
        from .systemc import emit_systemc_wrapper
        for mod in hw_mods:
            sc_code = emit_systemc_wrapper(mod)
            if args.output_dir == "-":
                sys.stdout.write(sc_code)
            else:
                sc_path = Path(args.output_dir) / f"{mod.name}SC.h"
                sc_path.write_text(sc_code)
                print(f"Written: {sc_path}", file=sys.stderr)


if __name__ == "__main__":
    main()
