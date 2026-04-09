#!/usr/bin/env python3
"""Run the RTL→C++ conversion pipeline for a single DUT directory."""
import subprocess, sys, os, re
from pathlib import Path

PROJECT = Path(os.path.expanduser("~/work/fc6161/fc6161-trunk"))
RTL = PROJECT / "rtl"  # $rtl shorthand used in some filelists
CIRCT_BIN = Path("/Users/wonseok/work/iamwonseok/opensource/circt/build/bin")
ARC_TO_CPP = Path("/Users/wonseok/work/iamwonseok/agentic-ai/hirct")
OUT = Path("/tmp/dut_cmodels")
OUT.mkdir(exist_ok=True)

import importlib.util as _ilu

def _load_module(path: str):
    spec = _ilu.spec_from_file_location("_mod", path)
    if spec is None:
        raise FileNotFoundError(f"Cannot load module from {path!r}")
    m = _ilu.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

try:
    _stub_gen = _load_module('/tmp/generate_sram_stubs.py')
except Exception as _e:
    _stub_gen = None
    print(f"Warning: generate_sram_stubs.py not loaded: {_e}")

def strip_sim_modules(text: str) -> tuple[str, list[str]]:
    """Remove simulation-only hw.module blocks (sim_*, *_sim, *_assert*, *_mon*)
    and any hw.instance references to them.  Returns (stripped_text, [removed_names]).
    Uses brace-depth counting to delete the entire module body."""
    SIM_PAT = re.compile(
        r'^\s*hw\.module\s+(?:\w+\s+)*@('    # optional modifiers like 'private'
        r'sim_\w+|'              # sim_* prefix
        r'\w+_sim|'              # *_sim suffix
        r'\w+_assert\w*|'        # *_assert*
        r'\w+_mon\w*'            # *_mon*
        r')\s*[(\{]'
    )
    INST_PAT = re.compile(r'^\s*hw\.instance\s+"[^"]*"\s+@(\w+)\s*\(')

    lines = text.splitlines(keepends=True)
    removed: set[str] = set()
    kept: list[str] = []

    i = 0
    while i < len(lines):
        m = SIM_PAT.match(lines[i])
        if m:
            mod_name = m.group(1)
            removed.add(mod_name)
            # Skip until matching close brace (depth tracking)
            depth = 0
            while i < len(lines):
                depth += lines[i].count('{') - lines[i].count('}')
                i += 1
                if depth <= 0:
                    break
            continue
        kept.append(lines[i])
        i += 1

    if not removed:
        return text, []

    # Remove hw.instance lines referencing removed modules
    final: list[str] = []
    for ln in kept:
        m = INST_PAT.match(ln)
        if m and m.group(1) in removed:
            continue
        final.append(ln)

    return "".join(final), sorted(removed)


def strip_orphan_sig_cluster(mlir_text: str) -> tuple[str, int]:
    """Remove llhd.sig/sig.extract/prb clusters with no llhd.drv drivers.

    Pattern B+E residual: LLHD lowering removes the llhd.drv ops (process was
    lowered) but leaves the llhd.sig, llhd.sig.extract, and llhd.prb ops.  When
    the sig has zero drivers the probe always returns the initial value (0), so
    we can replace prb results with hw.constant 0 and delete the sig cluster.

    Returns (new_text, number_of_clusters_removed).
    """
    lines = mlir_text.splitlines(keepends=True)

    # Pass 1: collect all sig SSA names and their initial-value types
    # Pattern:  %NAME = llhd.sig %init_ssa : TYPE
    sig_ssa: dict[str, str] = {}  # ssa_name -> type string (e.g. "i44")
    sig_init: dict[str, str] = {}  # ssa_name -> initial value ssa (e.g. "%c0_i44")
    SIG_DECL = re.compile(r'\s*(%\w+)\s*=\s*llhd\.sig\s+(%\w+)\s*:\s*(\S+)')
    for line in lines:
        m = SIG_DECL.match(line)
        if m:
            sig_ssa[m.group(1)] = m.group(3).rstrip()
            sig_init[m.group(1)] = m.group(2)

    if not sig_ssa:
        return mlir_text, 0

    # Pass 2: find which sigs have at least one llhd.drv
    # Pattern:  llhd.drv %SIG_EXTRACT_SSA, ... (drives a sig.extract sub-signal)
    # OR:       llhd.drv %SIG_SSA, ...         (drives the sig directly)
    # We also need to find sig.extract SSA names for each parent sig.
    # sig.extract:  %EXTRACT_SSA = llhd.sig.extract %PARENT_SIG from ...
    sig_extracts: dict[str, set[str]] = {s: set() for s in sig_ssa}
    EXTRACT = re.compile(r'\s*(%\w+)\s*=\s*llhd\.sig\.extract\s+(%\w+)\s+from')
    extract_to_parent: dict[str, str] = {}
    for line in lines:
        m = EXTRACT.match(line)
        if m:
            extract_ssa, parent = m.group(1), m.group(2)
            if parent in sig_ssa:
                sig_extracts[parent].add(extract_ssa)
                extract_to_parent[extract_ssa] = parent

    # All sig-related SSAs (sig itself + its extracts)
    all_sig_related: dict[str, str] = {}  # ssa -> parent_sig_ssa
    for sig, typ in sig_ssa.items():
        all_sig_related[sig] = sig
    for ext, par in extract_to_parent.items():
        all_sig_related[ext] = par

    # Check if any llhd.drv references a sig-related SSA
    driven_sigs: set[str] = set()
    DRV = re.compile(r'\bllhd\.drv\s+(%\w+)')
    for line in lines:
        m = DRV.search(line)
        if m:
            target = m.group(1)
            if target in all_sig_related:
                driven_sigs.add(all_sig_related[target])

    orphan_sigs = set(sig_ssa.keys()) - driven_sigs
    if not orphan_sigs:
        return mlir_text, 0

    # Pass 3: collect llhd.prb results for orphan sigs
    # Pattern:  %PRB_SSA = llhd.prb %ORPHAN_SIG : TYPE
    # or:       %PRB_SSA = llhd.prb %EXTRACT_SSA : TYPE  (extract from orphan sig)
    prb_replacements: dict[str, str] = {}  # prb_ssa -> replacement constant expr
    PRB = re.compile(r'\s*(%\w+)\s*=\s*llhd\.prb\s+(%\w+)\s*:\s*(\S+)')

    # Build a set of all extract SSAs from orphan sigs
    orphan_extract_ssat: set[str] = set()
    for sig in orphan_sigs:
        orphan_extract_ssat.update(sig_extracts.get(sig, set()))

    for line in lines:
        m = PRB.match(line)
        if m:
            prb_ssa, probed_sig, typ = m.group(1), m.group(2), m.group(3).rstrip()
            if probed_sig in orphan_sigs or probed_sig in orphan_extract_ssat:
                # We'll insert a hw.constant 0 line before the first use
                prb_replacements[prb_ssa] = typ

    # Pass 4: collect all SSA names to delete (orphan sig + its extracts + its prbs)
    delete_ssa: set[str] = set()
    for sig in orphan_sigs:
        delete_ssa.add(sig)
        delete_ssa.update(sig_extracts.get(sig, set()))
    delete_prb_ssa: set[str] = set(prb_replacements.keys())

    # Pass 5: rewrite lines
    # Lines to delete: llhd.sig, llhd.sig.extract (for orphan), llhd.drv referencing them
    # Lines to replace: llhd.prb -> hw.constant 0
    USES_DELETED_SSA = re.compile(r'(%\w+)\s*=')  # general assignment

    def line_defines_deleted(line: str) -> bool:
        """True if this line defines one of the SSAs we want to delete."""
        m = re.match(r'\s*(%\w+)\s*=\s*(llhd\.sig(?:\.extract)?|llhd\.prb)\s', line)
        if m and m.group(1) in (delete_ssa | delete_prb_ssa):
            return True
        # Also delete llhd.sig without assignment (bare sig not SSA-assigned — rare)
        return False

    result = []

    for line in lines:
        # Check if this is a prb line to replace
        m_prb = PRB.match(line)
        if m_prb and m_prb.group(1) in prb_replacements:
            typ = prb_replacements[m_prb.group(1)]
            prb_ssa = m_prb.group(1)
            # Replace with hw.constant 0
            indent = len(line) - len(line.lstrip())
            result.append(' ' * indent + f'{prb_ssa} = hw.constant 0 : {typ}\n')
            continue

        # Check if this is an orphan sig/sig.extract declaration to delete
        if line_defines_deleted(line):
            continue

        result.append(line)

    return ''.join(result), len(orphan_sigs)


def break_comb_prefix_or_loops(mlir_text: str) -> tuple[str, int]:
    """Break combinational loops caused by prefix-OR shift patterns.

    The pattern (generated by CIRCT from round-robin arbiter RTL):
        %EXTRACT = comb.extract %OUT from 0 : (iN) -> i(N-1)
        %SHIFTED = comb.concat %EXTRACT, %false : i(N-1), i1
        %OUT = comb.or %SHIFTED, %IN : iN

    This computes %OUT[i] = OR(%IN[0..i]) (prefix-OR), but arcilator's loop
    detector sees %OUT → %EXTRACT → %SHIFTED → %OUT as a cycle.

    Fix: replace %OUT with an explicit prefix-OR tree that does not loop:
        %OUT_b0 = comb.extract %IN from 0 : (iN) -> i1
        %OUT_b1_sub = comb.extract %IN from 1 : (iN) -> i1
        %OUT_b1 = comb.or %OUT_b0, %OUT_b1_sub : i1
        ...
        %OUT = comb.concat %OUT_b(N-1), ..., %OUT_b1, %OUT_b0 : i1, ..., i1

    SSA names are local to each hw.module body, so detection is done per module.

    Returns (new_text, number_of_loops_broken).
    """
    EXTRACT_PAT = re.compile(
        r'^(\s*)(%\w+)\s*=\s*comb\.extract\s+(%\w+)\s+from\s+0\s*:\s*\(i(\d+)\)\s*->\s*i(\d+)')
    CONCAT_PAT = re.compile(
        r'^(\s*)(%\w+)\s*=\s*comb\.concat\s+(%\w+),\s*(%\w+)\s*:\s*i(\d+),\s*i1')
    OR_PAT = re.compile(
        r'^(\s*)(%\w+)\s*=\s*comb\.or\s+(%\w+),\s*(%\w+)\s*:\s*i(\d+)')
    MOD_START = re.compile(r'^\s*hw\.module\b')

    lines = mlir_text.splitlines(keepends=True)

    # Split file into module-body ranges for scoped SSA lookup
    # Each range is (start_idx, end_idx) inclusive
    module_ranges: list[tuple[int, int]] = []
    i = 0
    while i < len(lines):
        if MOD_START.match(lines[i]):
            # Find opening brace on same or next lines
            depth = 0
            start = i
            while i < len(lines):
                depth += lines[i].count('{') - lines[i].count('}')
                i += 1
                if depth <= 0:
                    module_ranges.append((start, i - 1))
                    break
        else:
            i += 1

    delete_lines: set[int] = set()
    insertions: dict[int, list[str]] = {}
    n_broken = 0

    for mod_start, mod_end in module_ranges:
        mod_lines = lines[mod_start:mod_end + 1]

        # Build per-module SSA → absolute line index map
        ssa_abs: dict[str, int] = {}
        for rel_idx, line in enumerate(mod_lines):
            m = re.match(r'\s*(%\w+)\s*=', line)
            if m:
                ssa_abs[m.group(1)] = mod_start + rel_idx

        # Scan for the loop pattern
        for rel_idx, line in enumerate(mod_lines):
            m_or = OR_PAT.match(line)
            if not m_or:
                continue
            indent, out_ssa, arg1, arg2, n_str = m_or.groups()
            N = int(n_str)
            abs_or_idx = mod_start + rel_idx

            for shifted_ssa, in_ssa in [(arg1, arg2), (arg2, arg1)]:
                if shifted_ssa not in ssa_abs:
                    continue
                shifted_line = lines[ssa_abs[shifted_ssa]]
                m_concat = CONCAT_PAT.match(shifted_line)
                if not m_concat:
                    continue
                _, _, concat_arg1, concat_arg2, _ = m_concat.groups()
                if concat_arg2 != '%false' and concat_arg1 != '%false':
                    continue
                extract_ssa = concat_arg1 if concat_arg2 == '%false' else concat_arg2
                if extract_ssa not in ssa_abs:
                    continue
                extract_line = lines[ssa_abs[extract_ssa]]
                m_extract = EXTRACT_PAT.match(extract_line)
                if not m_extract:
                    continue
                _, _, extracted_from, n_full_str, n_minus1_str = m_extract.groups()
                if extracted_from != out_ssa:
                    continue
                if int(n_full_str) != N or int(n_minus1_str) != N - 1:
                    continue
                # Found the loop!
                delete_lines.add(ssa_abs[extract_ssa])
                delete_lines.add(ssa_abs[shifted_ssa])
                delete_lines.add(abs_or_idx)

                # Generate explicit prefix-OR
                # SSA names must be alpha-prefixed (MLIR rule: numeric SSAs are positional)
                # Use pflp (prefix-or-loop) prefix + module offset + out_ssa digits
                out_digits = re.sub(r'\D', '', out_ssa)  # strip non-digits
                unique_id = f'pflp{mod_start}x{out_digits}'
                new_lines = []
                prev_bit = f'%{unique_id}pb0'
                new_lines.append(f'{indent}{prev_bit} = comb.extract {in_ssa} from 0 : (i{N}) -> i1\n')
                bit_ssas = [prev_bit]
                for bit_i in range(1, N):
                    cur_in = f'%{unique_id}ib{bit_i}'
                    cur_bit = f'%{unique_id}pb{bit_i}'
                    new_lines.append(f'{indent}{cur_in} = comb.extract {in_ssa} from {bit_i} : (i{N}) -> i1\n')
                    new_lines.append(f'{indent}{cur_bit} = comb.or {prev_bit}, {cur_in} : i1\n')
                    prev_bit = cur_bit
                    bit_ssas.append(cur_bit)
                concat_args = ', '.join(reversed(bit_ssas))
                concat_types = ', '.join(['i1'] * N)
                new_lines.append(f'{indent}{out_ssa} = comb.concat {concat_args} : {concat_types}\n')
                insertions.setdefault(abs_or_idx, []).extend(new_lines)
                n_broken += 1
                break  # only one match per or-line

    if n_broken == 0:
        return mlir_text, 0

    result = []
    for idx, line in enumerate(lines):
        if idx in insertions:
            result.extend(insertions[idx])
        if idx not in delete_lines:
            result.append(line)

    return ''.join(result), n_broken


def strip_naked_constant_time(mlir_text: str) -> str:
    """Remove llhd.constant_time ops and llhd.drv ops that use those SSA values.
    These appear in DUT assertion/timing code that doesn't lower through LLHD."""
    lines = mlir_text.splitlines(keepends=True)
    const_time_ssa: set[str] = set()
    for line in lines:
        m = re.match(r'\s*(%\w+)\s*=\s*llhd\.constant_time\s+', line)
        if m:
            const_time_ssa.add(m.group(1))
    result = []
    for line in lines:
        if re.search(r'llhd\.constant_time', line):
            continue
        if const_time_ssa and re.search(r'llhd\.drv\s+', line):
            m_time = re.search(r'\bafter\s+(%\w+)', line)
            if m_time and m_time.group(1) in const_time_ssa:
                continue
        result.append(line)
    return ''.join(result)


def fix_array_constant_zero(mlir_text: str) -> tuple[str, int]:
    """Fix `hw.constant 0 : !hw.array<NxTy>` which arcilator rejects.

    CIRCT sometimes emits `hw.constant 0 : !hw.array<4xi12>` for zero-init of
    an array register, but arcilator only accepts `hw.aggregate_constant`.
    We convert these to `hw.aggregate_constant [0:Ti, ..., 0:Ti] : !hw.array<NxTi>`.

    Returns (new_text, number_of_fixes).
    """
    # Match:  %SSA = hw.constant 0 : !hw.array<NxTy>
    PAT = re.compile(
        r'(%\w+)\s*=\s*hw\.constant\s+0\s*:\s*!hw\.array<(\d+)x(i\d+)>',
    )
    lines = mlir_text.splitlines(keepends=True)
    result = []
    count = 0
    for line in lines:
        m = PAT.search(line)
        if m:
            ssa, n_str, ty = m.group(1), m.group(2), m.group(3)
            n = int(n_str)
            elems = ', '.join([f'0 : {ty}'] * n)
            new_op = f'hw.aggregate_constant [{elems}] : !hw.array<{n}x{ty}>'
            line = line[:m.start()] + ssa + ' = ' + new_op + line[m.end():]
            count += 1
        result.append(line)

    # Warn on nested array constant zeros we can't handle yet
    for line in lines:
        if re.search(r'hw\.constant\s+0\s*:\s*!hw\.array<\d+x!hw\.array', line):
            print(f"  WARNING: unhandled nested array constant zero: {line.strip()[:80]}")

    return ''.join(result), count


def _sub_operand(old: str, new: str, line: str) -> str:
    """Replace SSA name only in the operand portion (before the last ':' type annotation).

    MLIR op format is `%result = op operands : types`. The operands appear before the `:`.
    This prevents corrupting debug locations or type annotations.
    """
    colon = line.rfind(' :')
    if colon == -1:
        # No type annotation — replace whole line
        return re.sub(re.escape(old) + r'(?=[\s,\)]|$)', new, line)
    operand_part = line[:colon]
    type_part = line[colon:]
    operand_part = re.sub(re.escape(old) + r'(?=[\s,\)]|$)', new, operand_part)
    return operand_part + type_part


def _simplify_taut_in_module_body(body_lines: list[str]) -> tuple[list[str], int]:
    """Apply `(A&B)|(A&~B)=A` simplifications to a single hw.module body.

    SSA names are scoped per module body, so we must process each body
    independently to avoid cross-module substitution errors.

    Returns (new_lines, substitution_count).
    """
    total_subs = 0
    converged = False
    for _pass in range(10):
        # Step 1: xor-true map (single-bit NOT)
        xor_true: dict[str, str] = {}
        for line in body_lines:
            m = re.match(r'\s*(%\w+)\s*=\s*comb\.xor\s+(%\w+),\s*(%true|true)\s*:', line)
            if m:
                xor_true[m.group(1)] = m.group(2)

        # Step 2: comb.and with exactly 2 operands
        and2: dict[str, tuple[str, str]] = {}
        for line in body_lines:
            m = re.match(r'\s*(%\w+)\s*=\s*comb\.and(?:\s+bin)?\s+(%\w+),\s*(%\w+)\s*:', line)
            if m:
                and2[m.group(1)] = (m.group(2), m.group(3))

        def is_not_of(x: str, y: str) -> bool:
            return (x in xor_true and xor_true[x] == y) or \
                   (y in xor_true and xor_true[y] == x)

        # Step 3: find comb.or(comb.and ...) tautologies
        replacements: dict[str, str] = {}
        for line in body_lines:
            m = re.match(r'\s*(%\w+)\s*=\s*comb\.or(?:\s+bin)?\s+(%\w+),\s*(%\w+)\s*:', line)
            if not m:
                continue
            or_ssa, p0, p1 = m.group(1), m.group(2), m.group(3)
            if p0 not in and2 or p1 not in and2:
                continue
            a0, b0 = and2[p0]
            a1, b1 = and2[p1]
            simplified = None
            if   a0 == a1 and is_not_of(b0, b1): simplified = a0
            elif b0 == b1 and is_not_of(a0, a1): simplified = b0
            elif a0 == b1 and is_not_of(b0, a1): simplified = a0
            elif b0 == a1 and is_not_of(a0, b1): simplified = b0
            if simplified:
                replacements[or_ssa] = simplified

        if not replacements:
            converged = True
            break

        # Step 4: transitive closure of replacements
        def resolve(ssa: str, seen: set) -> str:
            if ssa in seen:
                return ssa
            if ssa in replacements:
                seen.add(ssa)
                return resolve(replacements[ssa], seen)
            return ssa

        trans: dict[str, str] = {ssa: resolve(replacements[ssa], {ssa})
                                  for ssa in replacements}
        trans = {k: v for k, v in trans.items() if k != v}

        if not trans:
            break

        # Step 5: rewrite body lines
        del_ssas = set(trans.keys())
        new_body: list[str] = []
        subs_this_pass = 0
        for line in body_lines:
            dm = re.match(r'\s*(%\w+)\s*=\s*', line)
            if dm and dm.group(1) in del_ssas:
                continue
            new_line = line
            for old_ssa, new_ssa in trans.items():
                attempt = _sub_operand(old_ssa, new_ssa, new_line)
                if attempt != new_line:
                    subs_this_pass += 1
                    new_line = attempt
            new_body.append(new_line)

        body_lines = new_body
        total_subs += subs_this_pass

    if not converged:
        print(f"  WARNING: tautology simplification did not converge after 10 passes")

    return body_lines, total_subs


def simplify_and_or_tautologies(mlir_text: str) -> tuple[str, int]:
    """Simplify `comb.or (comb.and A, B), (comb.and A, ~B)` => A, per module.

    CIRCT's mux-to-combinational lowering emits tautologies that look like
    combinational loops to arcilator.  We simplify them away.

    SSA names are module-scoped: we split the text at hw.module boundaries,
    apply simplification within each module body, then reassemble.

    Returns (new_text, total_substitutions_made).
    """
    # Split text into hw.module blocks.
    # A module starts with a line matching `  hw.module` (2-space indent) and
    # ends with the matching `  }` (2-space indent closing brace).
    lines = mlir_text.splitlines(keepends=True)
    result_lines: list[str] = []
    total_subs = 0

    i = 0
    while i < len(lines):
        line = lines[i]
        # Detect hw.module header (top-level, 2-space indent)
        if re.match(r'  hw\.module\b', line):
            # Collect module header line(s) up to the opening brace
            module_header = [line]
            j = i + 1
            # The body starts on the next line after the opening `{` on the header
            # (In practice, the `{` is on the same header line)
            # Now collect lines until the matching closing `  }`
            depth = line.count('{') - line.count('}')
            body_lines: list[str] = []
            while j < len(lines) and depth > 0:
                bl = lines[j]
                depth += bl.count('{') - bl.count('}')
                if depth == 0:
                    # This is the closing `  }` line
                    body_lines.append(bl)
                else:
                    body_lines.append(bl)
                j += 1
            # Simplify within the module body (exclude last `  }` line)
            if body_lines:
                closing = body_lines[-1]
                inner = body_lines[:-1]
                inner_simplified, n = _simplify_taut_in_module_body(inner)
                total_subs += n
                result_lines.extend(module_header)
                result_lines.extend(inner_simplified)
                result_lines.append(closing)
            else:
                result_lines.extend(module_header)
            i = j
        else:
            result_lines.append(line)
            i += 1

    return ''.join(result_lines), total_subs


def expand_path(line: str, base: Path) -> str:
    """Expand $PROJECT, ${PROJECT}, $rtl, ${rtl} in a path string."""
    line = line.replace("${PROJECT}", str(PROJECT)).replace("$PROJECT", str(PROJECT))
    line = line.replace("${rtl}", str(RTL)).replace("$rtl", str(RTL))
    return line

def parse_flist(flist_path: Path) -> tuple[list[str], list[str]]:
    """Parse a .f filelist, returning (source_files, incdir_flags)."""
    files = []
    incdirs = []
    base = flist_path.parent
    for raw_line in flist_path.read_text(errors='replace').splitlines():
        # strip // comments (but be careful with paths)
        line = raw_line.strip()
        if not line:
            continue
        # Skip full-line // comments
        if line.startswith("//") or line.startswith("#"):
            continue
        # Strip inline // comment
        if " //" in line:
            line = line[:line.index(" //")].strip()
        if not line:
            continue

        # Handle +incdir+
        if line.startswith("+incdir+"):
            inc = line[len("+incdir+"):]
            inc = expand_path(inc, base)
            incdirs.append(inc)
            continue
        # Skip other + flags
        if line.startswith("+") or line.startswith("-"):
            continue

        # It's a file path
        line = expand_path(line, base)
        p = Path(line)
        if not p.is_absolute():
            p = base / line
        if p.exists():
            files.append(str(p))
        else:
            # Try to resolve relative to PROJECT
            pass  # silently skip missing files
    return files, incdirs

def find_sram_source_files(sram_names: set, all_sources: list) -> dict[str, str]:
    """For each SRAM module name, find the source .v file that defines it.
    Uses grep to match 'module <name>' in each source file.
    Returns {module_name: source_file_path}.
    """
    found = {}
    for src in all_sources:
        if not Path(src).exists():
            continue
        if not src.endswith(('.v', '.sv')):
            continue
        for name in list(sram_names):
            if name in found:
                continue
            r = subprocess.run(
                ['grep', '-l', f'module {name}', src],
                capture_output=True, text=True
            )
            if r.returncode == 0 and r.stdout.strip():
                found[name] = src
    return found

def find_flist(dut_dir: Path) -> list[Path]:
    """Find the primary filelist(s) for a DUT directory."""
    candidates = []
    # main.f is most common
    if (dut_dir / "main.f").exists():
        candidates.append(dut_dir / "main.f")
    # dut.f
    if (dut_dir / "dut.f").exists():
        candidates.append(dut_dir / "dut.f")
    # src/list/tb.f (cmd_sts_tbl_pli_new pattern)
    if (dut_dir / "src" / "list" / "tb.f").exists():
        candidates.append(dut_dir / "src" / "list" / "tb.f")
    # tb/dut.f (pt_ncs pattern)
    if (dut_dir / "tb" / "dut.f").exists():
        candidates.append(dut_dir / "tb" / "dut.f")
    return candidates

def run_dut(name: str, dut_dir: Path):
    dut_dir = Path(str(dut_dir))
    print(f"\n{'='*60}")
    print(f"DUT: {name}  ->  {dut_dir.name}")
    if not dut_dir.exists():
        print(f"  ERROR: directory not found")
        return {"name": name, "status": "DIR_NOT_FOUND"}

    # Step 1: List directory contents
    contents = sorted(dut_dir.iterdir())
    print(f"  Files: {[f.name for f in contents][:20]}")

    # Step 2: Find and read Makefile
    mk_path = dut_dir / "Makefile"
    if not mk_path.exists():
        for mk in sorted(dut_dir.glob("Makefile*")):
            mk_path = mk; break
    if not mk_path.exists():
        print(f"  ERROR: no Makefile found")
        return {"name": name, "status": "NO_MAKEFILE"}

    mk_text = mk_path.read_text(errors='replace')

    # Collect includes/defines from Makefile
    includes = set()
    defines = ["-DSIMULATION"]

    for line in mk_text.splitlines():
        m = re.search(r'\+incdir\+(\S+)', line)
        if m:
            inc = m.group(1)
            inc = expand_path(inc, dut_dir)
            if Path(inc).exists():
                includes.add(inc)
        m = re.search(r'-INCDIR\s+(\S+)', line)
        if m:
            inc = m.group(1)
            inc = expand_path(inc, dut_dir)
            if Path(inc).exists():
                includes.add(inc)
        m = re.search(r'\+define\+(\w+)', line)
        if m:
            defines.append(f"-D{m.group(1)}")
        m = re.search(r'-define\s+(\w+)', line)
        if m:
            defines.append(f"-D{m.group(1)}")

    # Add standard include paths
    for inc_candidate in [
        str(PROJECT / "rtl/pt_ncs/include"),
        str(PROJECT / "rtl/pt_ncs/src"),
        str(PROJECT / "rtl/pt_plat/src/ssd/include"),
        str(PROJECT / "rtl/pt_plat/include"),
    ]:
        if Path(inc_candidate).exists():
            includes.add(inc_candidate)

    # PT-NCS DUTs need prim.f and lib.f from shared filelists
    pt_ncs_prim_f = PROJECT / "rtl/pt_ncs/bin/lst/pt_ncs_prim.f"
    pt_ncs_lib_f = PROJECT / "rtl/pt_ncs/bin/lst/pt_ncs_lib.f"
    is_pt_ncs = "pt_ncs" in str(dut_dir)

    # Step 3: Find filelists
    flist_paths = find_flist(dut_dir)
    all_sources = []

    # For PT_NCS DUTs, prepend prim.f and lib.f
    if is_pt_ncs:
        for fpath in [pt_ncs_prim_f, pt_ncs_lib_f]:
            if fpath.exists():
                srcs, incs = parse_flist(fpath)
                all_sources.extend(srcs)
                includes.update(incs)
                print(f"  Shared filelist {fpath.name}: {len(srcs)} files")

    for fl in flist_paths:
        srcs, incs = parse_flist(fl)
        all_sources.extend(srcs)
        includes.update(incs)
        print(f"  Filelist {fl.relative_to(dut_dir)}: {len(srcs)} files")

    # Also look for .v/.sv files directly in the dir (fallback)
    direct_sv = list(dut_dir.glob("*.v")) + list(dut_dir.glob("*.sv"))
    if direct_sv and not all_sources:
        all_sources.extend([str(f) for f in direct_sv])
        print(f"  Direct RTL files: {[f.name for f in direct_sv]}")

    # Deduplicate while preserving order
    all_sources = list(dict.fromkeys(all_sources))

    # Exclude known-problematic files
    EXCLUDE = ["plusarg_reader.v", "sim_clk_gate.v"]
    # Exclude TB files with $vpli_put/$vpli_get (PLI DUTs) and tb.sv with type issues
    EXCLUDE_PATTERN_NAMES = ["tb.sv"]  # rob_ben tb.sv has type redefinition
    # For PLI DUTs, exclude tb.v (contains $vpli_put/$vpli_get)
    is_pli_dut = any(x in str(dut_dir) for x in [
        "wcmd_list_pli", "tdc_pli_billy", "meta_info_mgr_pli"
    ])
    if is_pli_dut:
        EXCLUDE_PATTERN_NAMES.append("tb.v")
        print(f"  PLI DUT: will exclude tb.v")

    excluded = [s for s in all_sources
                if Path(s).name in EXCLUDE or Path(s).name in EXCLUDE_PATTERN_NAMES]
    all_sources = [s for s in all_sources
                   if Path(s).name not in EXCLUDE and Path(s).name not in EXCLUDE_PATTERN_NAMES]
    if excluded:
        print(f"  Excluded: {[Path(e).name for e in excluded]}")

    # DUT-specific extra source injections
    # SSD7 (p2l_mgr): mpm_ppu_if_rdata_q.v needs rdata_q_rw.v and the full ecc+secded+prim chain
    MPM_RW = str(PROJECT / "rtl/pt_plat/src/ssd/ctrl/mpm_port_mult/ssd_ctrl_mpm_ppu_if_rdata_q_rw.v")
    ECC256 = str(PROJECT / "rtl/lib/ecc_mem/ecc_od1_ssd_ctrl_sp_256x512w1.v")
    SECDED_ENC512 = str(PROJECT / "rtl/lib/secded/secded_hamming_enc_d512_p11.v")
    SECDED_DEC512 = str(PROJECT / "rtl/lib/secded/secded_hamming_dec_d512_p11.v")
    PRIM256 = str(PROJECT / "rtl/prim/mem/ssd_ctrl_sp_256x523w1.sim.v")
    if "p2l_mgr" in str(dut_dir):
        # Inject rdata_q_rw before rdata_q
        if MPM_RW not in all_sources and Path(MPM_RW).exists():
            try:
                idx = next(i for i, s in enumerate(all_sources)
                          if "ssd_ctrl_mpm_ppu_if_rdata_q.v" in s and "_rw" not in s)
                all_sources.insert(idx, MPM_RW)
            except StopIteration:
                all_sources.append(MPM_RW)
            print(f"  Injected: {Path(MPM_RW).name}")
        # Inject ecc chain (secded + prim + ecc) before ecc_od1_ssd_ctrl_sp_256x512w1 would be used
        for extra_f in [SECDED_ENC512, SECDED_DEC512, PRIM256, ECC256]:
            if extra_f not in all_sources and Path(extra_f).exists():
                all_sources.append(extra_f)
                print(f"  Injected: {Path(extra_f).name}")

    # SSD4 (rob_ben): sync_fifo_reg.v uses sim_dbg_sync_fifo under `ifdef SIMULATION
    SIM_DBG = str(PROJECT / "rtl/lib/sim_util/sim_dbg_sync_fifo.v")
    if "rob_ben" in str(dut_dir) and SIM_DBG not in all_sources and Path(SIM_DBG).exists():
        all_sources.insert(0, SIM_DBG)
        print(f"  Injected: sim_dbg_sync_fifo.v")

    # SSD2 (prp_sgl_parser): needs ecc_od1_ssd_ctrl_sp_192x44w1 stub (module doesn't exist in repo)
    ECC192_STUB = "/tmp/stubs/ecc_od1_ssd_ctrl_sp_192x44w1.v"
    if "prp_sgl_parser" in str(dut_dir) and Path(ECC192_STUB).exists():
        all_sources.insert(0, ECC192_STUB)
        print(f"  Injected: ecc_od1_ssd_ctrl_sp_192x44w1.v (stub)")

    # Deduplicate defines
    seen_defines = set()
    deduped_defines = []
    for d in defines:
        if d not in seen_defines:
            seen_defines.add(d)
            deduped_defines.append(d)
    defines = deduped_defines

    print(f"  Total source files: {len(all_sources)}")
    print(f"  Include dirs: {len(includes)}")

    if not all_sources:
        print(f"  ERROR: no source files found")
        return {"name": name, "status": "NO_SOURCES"}

    # Step 4: Run circt-verilog
    hw_mlir = OUT / f"{name}_hw.mlir"
    inc_flags = [f"-I{i}" for i in sorted(includes) if Path(i).exists()]
    # DUTs with generate-loop out-of-bounds that slang evaluates conservatively
    range_oob_duts = ["cmd_sts_tbl_pli_new", "p2l_mgr"]
    extra_w_flags = ["-Wno-range-oob"] if any(x in str(dut_dir) for x in range_oob_duts) else []
    cmd = [str(CIRCT_BIN / "circt-verilog"),
           "--timescale=1ns/1ps",
           "--ir-hw",
           "-o", str(hw_mlir),
    ] + inc_flags + extra_w_flags + defines + all_sources

    print(f"  Running circt-verilog ({len(all_sources)} files)...")
    r = subprocess.run(cmd, capture_output=True, timeout=300)
    stdout_text = r.stdout.decode('utf-8', errors='replace')
    stderr_text = r.stderr.decode('utf-8', errors='replace')
    if r.returncode != 0:
        err_lines = stderr_text.strip().splitlines()
        print(f"  circt-verilog FAILED (exit {r.returncode})")
        for l in err_lines[-15:]:
            print(f"    {l}")
        return {"name": name, "status": "CIRCT_VERILOG_FAILED",
                "error": err_lines[-3:] if err_lines else []}
    print(f"  circt-verilog OK -> {hw_mlir.name} ({hw_mlir.stat().st_size//1024}KB)")

    # ── SRAM STUB INJECTION ─────────────────────────────────────────────────
    # Detect SRAM modules in hw_mlir, generate stubs, re-run circt-verilog.
    stubs_dir = OUT / "stubs"
    sram_stubs = _stub_gen.generate_all(hw_mlir, stubs_dir) if _stub_gen else {}

    if sram_stubs:
        print(f"  Found {len(sram_stubs)} SRAM modules: {sorted(sram_stubs.keys())[:5]}...")
        src_map = find_sram_source_files(set(sram_stubs.keys()), all_sources)
        src_to_exclude = set(src_map.values())
        if src_to_exclude:
            print(f"  Excluding {len(src_to_exclude)} SRAM source files")

        stub_files = [str(v) for v in sorted(sram_stubs.values())]
        clean_sources = [s for s in all_sources if s not in src_to_exclude]
        new_sources = stub_files + clean_sources

        hw_mlir_stubbed = OUT / f"{name}_hw_stubbed.mlir"
        cmd_stub = [str(CIRCT_BIN / "circt-verilog"),
                    "--timescale=1ns/1ps", "--ir-hw",
                    "-o", str(hw_mlir_stubbed),
                    ] + inc_flags + extra_w_flags + defines + new_sources
        print(f"  Re-running circt-verilog with {len(sram_stubs)} SRAM stubs...")
        r_stub = subprocess.run(cmd_stub, capture_output=True, timeout=300)
        if r_stub.returncode != 0:
            err = r_stub.stderr.decode('utf-8', errors='replace').strip().splitlines()
            print(f"  circt-verilog (stub) FAILED: {err[-3:]}")
            print(f"  Continuing with original hw_mlir (stubs failed)")
        else:
            hw_mlir = hw_mlir_stubbed
            print(f"  circt-verilog (stub) OK -> {hw_mlir_stubbed.name} ({hw_mlir_stubbed.stat().st_size//1024}KB)")
    # ─────────────────────────────────────────────────────────────────────────

    # Step 5: LLHD pipeline passes
    hw_out = OUT / f"{name}_hw_out.mlir"
    # llhd-strip-sim-processes crashes (SIGSEGV exit 139) on large files with many hw.modules
    # (tested: PT1/PT2 at 8MB). Skip it for files > 2MB.
    hw_mlir_size = hw_mlir.stat().st_size
    strip_pass = ["--llhd-strip-sim-processes"] if hw_mlir_size < 2 * 1024 * 1024 else []
    if not strip_pass:
        print(f"  (skipping llhd-strip-sim-processes: file {hw_mlir_size//1024}KB > 2MB)")
    cmd2 = [str(CIRCT_BIN / "circt-opt"),
            ] + strip_pass + [
            "--llhd-mem2reg", "--llhd-hoist-signals",
            "--llhd-unroll-loops", "--cse", "--canonicalize",
            "--llhd-unroll-loops", "--cse", "--canonicalize",
            "--llhd-deseq",
            "--llhd-lower-processes",
            str(hw_mlir), "-o", str(hw_out)]

    print(f"  Running LLHD pipeline...")
    r2 = subprocess.run(cmd2, capture_output=True, timeout=300)
    r2_stderr = r2.stderr.decode('utf-8', errors='replace')
    if r2.returncode != 0:
        err_lines = r2_stderr.strip().splitlines()
        print(f"  LLHD pipeline FAILED (exit {r2.returncode})")
        for l in err_lines[-10:]:
            print(f"    {l}")
        return {"name": name, "status": "LLHD_FAILED",
                "error": err_lines[-3:] if err_lines else []}

    hw_out_text = hw_out.read_text(errors='replace')

    # Strip simulation-only hw.modules (sim_*, *_assert*, *_mon*) that leave LLHD residuals.
    # Do this before counting residuals so the pipeline can proceed.
    stripped_text, stripped_mods = strip_sim_modules(hw_out_text)
    if stripped_mods:
        print(f"  Stripped sim modules: {stripped_mods}")
        hw_out_text = stripped_text

    # Strip naked llhd.constant_time ops (timing assertions in DUT code)
    ct_stripped = strip_naked_constant_time(hw_out_text)
    if ct_stripped != hw_out_text:
        print(f"  Stripped naked constant_time ops")
        hw_out_text = ct_stripped

    # Strip orphan llhd.sig/sig.extract/prb clusters with no llhd.drv drivers
    # (Pattern B+E: LLHD lower-processes removes drv ops but leaves sig infrastructure)
    oc_stripped, n_clusters = strip_orphan_sig_cluster(hw_out_text)
    if n_clusters > 0:
        print(f"  Stripped {n_clusters} orphan sig cluster(s)")
        hw_out_text = oc_stripped

    # Break combinational prefix-OR loops (round-robin arbiter pattern)
    # arcilator's arc-conv detects these as cycles even though they are bit-serial
    pl_broken, n_loops = break_comb_prefix_or_loops(hw_out_text)
    if n_loops > 0:
        print(f"  Broke {n_loops} comb prefix-OR loop(s)")
        hw_out_text = pl_broken

    # Fix hw.constant 0 : !hw.array<NxTy> — arcilator only accepts hw.aggregate_constant
    arr_fixed, n_arr_fixes = fix_array_constant_zero(hw_out_text)
    if n_arr_fixes > 0:
        print(f"  Fixed {n_arr_fixes} hw.array constant(s) -> hw.aggregate_constant")
        hw_out_text = arr_fixed

    # Simplify spurious combinational loops: comb.or(comb.and A,B, comb.and A,~B) = A
    # CIRCT's mux-lowering emits these tautologies; arcilator's loop detector trips on them.
    taut_fixed, n_taut = simplify_and_or_tautologies(hw_out_text)
    if n_taut > 0:
        print(f"  Simplified {n_taut} A&B|A&~B tautology/ies (loop breaking)")
        hw_out_text = taut_fixed

    hw_out.write_text(hw_out_text)

    # Count residuals
    residuals = sum(1 for l in hw_out_text.splitlines()
                   if re.search(r'llhd\.(process|sig|drv|prb|wait)', l))
    print(f"  LLHD pipeline OK -> residuals: {residuals}")

    if residuals > 0:
        return {"name": name, "status": "LLHD_RESIDUALS", "residuals": residuals}

    # Step 6: arcilator
    arc_mlir = OUT / f"{name}_arc.mlir"
    cmd3 = [str(CIRCT_BIN / "arcilator"),
            "--async-resets-as-sync",
            "--until-after=arc-opt",
            "--emit-mlir",
            str(hw_out), "-o", str(arc_mlir)]

    print(f"  Running arcilator...")
    r3 = subprocess.run(cmd3, capture_output=True, timeout=300)
    r3_stderr = r3.stderr.decode('utf-8', errors='replace')
    if r3.returncode != 0:
        err_lines = r3_stderr.strip().splitlines()
        print(f"  arcilator FAILED (exit {r3.returncode})")
        for l in err_lines[-10:]:
            print(f"    {l}")
        return {"name": name, "status": "ARC_FAILED",
                "error": err_lines[-3:] if err_lines else []}
    print(f"  arcilator OK -> {arc_mlir.name} ({arc_mlir.stat().st_size//1024}KB)")

    # Step 7: arc_to_cpp
    cpp_out = OUT / f"{name}/"
    cpp_out.mkdir(exist_ok=True)
    r4 = subprocess.run(
        ["python3", "-m", "arc_to_cpp", str(arc_mlir), "-o", str(cpp_out)],
        capture_output=True,
        cwd=str(ARC_TO_CPP), timeout=60)
    r4_stderr = r4.stderr.decode('utf-8', errors='replace')
    if r4.returncode != 0:
        print(f"  arc_to_cpp FAILED: {r4_stderr[:300]}")
        return {"name": name, "status": "ARC_TO_CPP_FAILED", "error": r4_stderr[:300]}

    headers = list(cpp_out.glob("*.h"))
    print(f"  arc_to_cpp OK -> {[h.name for h in headers]}")
    return {"name": name, "status": "SUCCESS", "headers": [h.name for h in headers]}

# Run all DUTs
DUTS = [
    ("PT1",  PROJECT / "rtl/pt_ncs/sim/module/ncs_ctrl_path/cmd_list"),
    ("PT2",  PROJECT / "rtl/pt_ncs/sim/module/ncs_ctrl_path/dmem_dma_top_ref_sim"),
    ("SSD1", PROJECT / "rtl/pt_plat/sim/ssd_ctrl/hil/wcmd_list_pli"),
    ("SSD2", PROJECT / "rtl/pt_plat/sim/ssd_ctrl/hil/prp_sgl_parser"),
    ("SSD3", PROJECT / "rtl/pt_plat/sim/ssd_ctrl/hil/cmd_sts_tbl_pli_new"),
    ("SSD4", PROJECT / "rtl/pt_plat/sim/ssd_ctrl/ftl/rob_ben"),
    ("SSD5", PROJECT / "rtl/pt_plat/sim/ssd_ctrl/ftl/tdc_pli_billy"),
    ("SSD6", PROJECT / "rtl/pt_plat/sim/ssd_ctrl/ftl/meta_info_mgr_pli"),
    ("SSD7", PROJECT / "rtl/pt_plat/sim/ssd_ctrl/ftl/p2l_mgr"),
    ("SSD8", PROJECT / "rtl/pt_plat/sim/ssd_ctrl/ftl/pfl_entry_sel"),
    ("SSD9", PROJECT / "rtl/pt_plat/sim/ssd_ctrl/ftl/mw_v2"),
]

results = []
for name, ddir in DUTS:
    try:
        result = run_dut(name, ddir)
        results.append(result)
    except Exception as e:
        import traceback
        print(f"  EXCEPTION: {e}")
        traceback.print_exc()
        results.append({"name": name, "status": "EXCEPTION", "error": str(e)})

print("\n" + "="*60)
print("SUMMARY")
print("="*60)
for r in results:
    extra = ""
    if r.get("headers"):
        extra = f"  headers={r['headers']}"
    elif r.get("residuals"):
        extra = f"  residuals={r['residuals']}"
    elif r.get("error"):
        err = r["error"]
        if isinstance(err, list):
            extra = f"  err={err[-1][:80]}" if err else ""
        else:
            extra = f"  err={str(err)[:80]}"
    print(f"  {r['name']:6s}: {r['status']}{extra}")
