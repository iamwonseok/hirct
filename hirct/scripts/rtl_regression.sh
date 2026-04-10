#!/usr/bin/env bash
set -euo pipefail

# =============================================================================
# hirct RTL Regression Pipeline
# =============================================================================
# Converts real Verilog RTL through CIRCT to Arc MLIR, then runs hirct
# semantic model build and CModel emitter.
#
# Prerequisites:
#   - CIRCT build at $CIRCT_BIN
#   - hirct build at $HIRCT_DIR/build
#   - RTL project at $PROJECT
#
# Usage:
#   ./rtl_regression.sh <module_name> <verilog_source> [include_dirs...]
#
# Example:
#   ./rtl_regression.sh ncs_hp_first_sig_gen \
#     /path/to/ncs_hp_first_sig_gen.v \
#     /path/to/pt_ncs/include
# =============================================================================

CIRCT_BIN="${CIRCT_BIN:-/Users/wonseok/work/iamwonseok/opensource/circt/build/bin}"
HIRCT_DIR="${HIRCT_DIR:-/Users/wonseok/.config/superpowers/worktrees/hirct/semantic-model-cpp-first/hirct}"
WORK_DIR="${WORK_DIR:-/tmp/hirct_rtl_regression}"

if [ $# -lt 2 ]; then
  echo "Usage: $0 <module_name> <verilog_source> [include_dirs...]"
  exit 1
fi

MOD="$1"
SRC="$2"
shift 2

INC_FLAGS=""
for inc in "$@"; do
  INC_FLAGS="$INC_FLAGS -I$inc"
done

mkdir -p "$WORK_DIR"

STATUS_OK="\033[32mOK\033[0m"
STATUS_FAIL="\033[31mFAIL\033[0m"
STATUS_MANUAL="\033[33mMANUAL\033[0m"

echo "================================================================="
echo "MODULE: $MOD"
echo "SOURCE: $SRC ($(wc -l < "$SRC") lines)"
echo "================================================================="

# --- Stage 1: Verilog -> HW MLIR ---
echo -n "  S1  circt-verilog --ir-hw:  "
if $CIRCT_BIN/circt-verilog --ir-hw $INC_FLAGS "$SRC" \
    > "$WORK_DIR/${MOD}_hw.mlir" 2>"$WORK_DIR/${MOD}_s1_err.txt"; then
  echo -e "$STATUS_OK ($(wc -l < "$WORK_DIR/${MOD}_hw.mlir") lines)"
else
  echo -e "$STATUS_FAIL"
  cat "$WORK_DIR/${MOD}_s1_err.txt"
  exit 1
fi

# --- Stage 2: arc-strip-sv (async reset -> sync) ---
echo -n "  S2  arc-strip-sv async->sync:  "
if $CIRCT_BIN/circt-opt --arc-strip-sv='async-resets-as-sync=true' \
    "$WORK_DIR/${MOD}_hw.mlir" \
    > "$WORK_DIR/${MOD}_stripped.mlir" 2>"$WORK_DIR/${MOD}_s2_err.txt"; then
  echo -e "$STATUS_OK"
else
  echo -e "$STATUS_FAIL"
  cat "$WORK_DIR/${MOD}_s2_err.txt"
  exit 1
fi

# --- Stage 2b: llhd.delay removal (automatic) ---
LLHD_COUNT=$(grep -c "llhd.delay" "$WORK_DIR/${MOD}_stripped.mlir" || true)
echo -n "  S2b llhd.delay removal:  "
if [ "$LLHD_COUNT" -gt 0 ]; then
  python3 << PYEOF
import re
content = open('$WORK_DIR/${MOD}_stripped.mlir').read()
pattern = r'^\s+(%\S+) = llhd\.delay (%\S+) by .*\$'
repls = {}
clean = []
for line in content.split('\n'):
    m = re.match(pattern, line)
    if m:
        repls[m.group(1)] = m.group(2)
    else:
        clean.append(line)
result = '\n'.join(clean)
for old, new in repls.items():
    for suffix in [' ', ',', ')', ']']:
        result = result.replace(old + suffix, new + suffix)
open('$WORK_DIR/${MOD}_clean.mlir', 'w').write(result)
print(f'{len(repls)} removed')
PYEOF
  echo -e "$STATUS_MANUAL ($LLHD_COUNT occurrences, auto-patched)"
else
  cp "$WORK_DIR/${MOD}_stripped.mlir" "$WORK_DIR/${MOD}_clean.mlir"
  echo -e "$STATUS_OK (none)"
fi

# --- Stage 3: convert-to-arcs ---
echo -n "  S3  convert-to-arcs:  "
if $CIRCT_BIN/circt-opt \
    --convert-to-arcs \
    --arc-infer-state-properties='resets=true enables=true' \
    "$WORK_DIR/${MOD}_clean.mlir" \
    > "$WORK_DIR/${MOD}_arc.mlir" 2>"$WORK_DIR/${MOD}_s3_err.txt"; then
  echo -e "$STATUS_OK ($(wc -l < "$WORK_DIR/${MOD}_arc.mlir") lines)"
else
  echo -e "$STATUS_FAIL"
  head -5 "$WORK_DIR/${MOD}_s3_err.txt"
  exit 1
fi

# --- Stage 4: hirct-semantic ---
echo -n "  S4  hirct-semantic:  "
if $HIRCT_DIR/build/bin/hirct-semantic --module="$MOD" \
    "$WORK_DIR/${MOD}_arc.mlir" > "$WORK_DIR/${MOD}_sem.txt" 2>"$WORK_DIR/${MOD}_s4_err.txt"; then
  echo -e "$STATUS_OK"
  cat "$WORK_DIR/${MOD}_sem.txt"
else
  echo -e "$STATUS_FAIL"
  cat "$WORK_DIR/${MOD}_s4_err.txt"
  exit 1
fi

echo ""
echo "Arc MLIR saved: $WORK_DIR/${MOD}_arc.mlir"
echo "Semantic model: $WORK_DIR/${MOD}_sem.txt"
echo "================================================================="
echo "Pipeline complete. Next: run CModel emitter gtest with this fixture."
