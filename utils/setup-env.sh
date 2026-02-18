#!/usr/bin/env bash
# ============================================================================
# HIRCT Environment Setup — Phase 0, Task 001
#
# Purpose : Install/verify all external tools required for Phase 0-3,
#           set up CIRCT/LLVM environment variables, and install Python
#           packages from requirements.txt.
#
# Idempotency : Safe to run multiple times. Already-installed packages and
#               already-set variables are detected and skipped.
#
# Usage   : source utils/setup-env.sh   (to export env vars into shell)
#         : bash   utils/setup-env.sh   (verify only, env vars lost on exit)
#         : bash   utils/setup-env.sh --strict  (fail on pinned version mismatch)
# ============================================================================
set -euo pipefail

# ── Colour helpers (disabled when not a terminal) ──────────────────────────
if [[ -t 1 ]]; then
  RED=$'\033[0;31m'; GREEN=$'\033[0;32m'; YELLOW=$'\033[0;33m'
  CYAN=$'\033[0;36m'; BOLD=$'\033[1m'; RESET=$'\033[0m'
else
  RED=''; GREEN=''; YELLOW=''; CYAN=''; BOLD=''; RESET=''
fi

# ── Script-level state ─────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
MANDATORY_FAIL=0
WARN_COUNT=0
STRICT_MODE=0

# Parse --strict flag
for arg in "$@"; do
  case "${arg}" in
    --strict) STRICT_MODE=1 ;;
  esac
done

declare -a SUMMARY_ROWS=()

add_summary() {
  local tool="$1" version="$2" method="${3:--}" path="${4:--}" status="$5"
  SUMMARY_ROWS+=("${tool}|${version}|${method}|${path}|${status}")
}

log_pass() { echo "  ${GREEN}[V]${RESET} $1"; }
log_fail() { echo "  ${RED}[X]${RESET} $1" >&2; MANDATORY_FAIL=1; }
log_warn() { echo "  ${YELLOW}[!]${RESET} $1"; WARN_COUNT=$((WARN_COUNT + 1)); }
log_info() { echo "  ${CYAN}[i]${RESET} $1"; }

# ── Version comparison helpers ─────────────────────────────────────────────
# version_ge A B  →  returns 0 if A >= B
version_ge() {
  local IFS=.
  # shellcheck disable=SC2206
  local -a a=($1) b=($2)
  local i
  for ((i = 0; i < ${#b[@]}; i++)); do
    local av="${a[i]:-0}"
    local bv="${b[i]:-0}"
    if ((av > bv)); then return 0; fi
    if ((av < bv)); then return 1; fi
  done
  return 0
}

# major_version "13.3.0" → "13"
major_version() {
  echo "$1" | cut -d. -f1
}

# Extract version number from a string
extract_version() {
  echo "$1" | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -1
}

detect_method() {
  local p="$1"
  case "${p}" in
    */circt/build/*)              echo "source" ;;
    */.venv/bin/*|*/.local/bin/*) echo "pip" ;;
    /snap/*)                      echo "snap" ;;
    /usr/bin/*|/usr/sbin/*)       echo "apt" ;;
    /usr/local/bin/*)             echo "apt" ;;
    /tools/*)                     echo "vendor" ;;
    *)                            echo "-" ;;
  esac
}

shorten_path() {
  local p="$1"
  p="${p/#${PROJECT_ROOT}\//./}"
  p="${p/#${HOME}/\~}"
  echo "${p}"
}

# ============================================================================
# 0. Load tool-versions.env (SSOT)
# ============================================================================
TOOL_VERSIONS_FILE="${PROJECT_ROOT}/tool-versions.env"
if [[ -f "${TOOL_VERSIONS_FILE}" ]]; then
  # shellcheck disable=SC1090
  source "${TOOL_VERSIONS_FILE}"
else
  echo "${RED}[FATAL] tool-versions.env not found at ${TOOL_VERSIONS_FILE}${RESET}" >&2
  exit 1
fi

# ============================================================================
# 1. CIRCT_BUILD auto-detection
# ============================================================================
echo "${BOLD}=== CIRCT/LLVM Build Detection ===${RESET}"

if [[ -n "${CIRCT_BUILD:-}" ]] && [[ -d "${CIRCT_BUILD}" ]]; then
  log_info "Using CIRCT_BUILD from environment: ${CIRCT_BUILD}"
elif [[ -d "${HOME}/circt/build" ]]; then
  export CIRCT_BUILD="${HOME}/circt/build"
  log_info "Auto-detected CIRCT_BUILD: ${CIRCT_BUILD}"
else
  log_fail "CIRCT build directory not found."
  echo "       Set CIRCT_BUILD environment variable or build CIRCT at \$HOME/circt/build"
  echo "       See: https://github.com/llvm/circt#setting-this-up"
  MANDATORY_FAIL=1
fi

if [[ -n "${CIRCT_BUILD:-}" ]]; then
  export PATH="${CIRCT_BUILD}/bin:${PATH}"
  export MLIR_DIR="${CIRCT_BUILD}/lib/cmake/mlir"
  export LLVM_DIR="${CIRCT_BUILD}/lib/cmake/llvm"
  circt_bin_short="$(shorten_path "${CIRCT_BUILD}/bin")"

  # Verify key binaries + check pinned commit
  if [[ -x "${CIRCT_BUILD}/bin/circt-verilog" ]]; then
    local_cv_ver=$(circt-verilog --version 2>&1 | grep '^CIRCT' | awk '{print $2}' || true)
    local_cv_ver="${local_cv_ver:-unknown}"
    log_pass "circt-verilog ${local_cv_ver}"
    add_summary "circt-verilog" "${local_cv_ver}" "source" "${circt_bin_short}/circt-verilog" "PASS"
  else
    log_fail "circt-verilog not found in ${CIRCT_BUILD}/bin/"
    add_summary "circt-verilog" "N/A" "source" "-" "FAIL"
  fi

  if [[ -x "${CIRCT_BUILD}/bin/circt-opt" ]]; then
    local_co_ver=$(circt-opt --version 2>&1 | grep '^CIRCT' | awk '{print $2}' || true)
    local_co_ver="${local_co_ver:-unknown}"
    log_pass "circt-opt ${local_co_ver}"
    add_summary "circt-opt" "${local_co_ver}" "source" "${circt_bin_short}/circt-opt" "PASS"
  else
    log_fail "circt-opt not found in ${CIRCT_BUILD}/bin/"
    add_summary "circt-opt" "N/A" "source" "-" "FAIL"
  fi

  # CIRCT commit verification against pinned version
  CIRCT_PARENT="$(cd "${CIRCT_BUILD}/.." && pwd)"
  if [[ -d "${CIRCT_PARENT}/.git" ]]; then
    ACTUAL_CIRCT_COMMIT="$(cd "${CIRCT_PARENT}" && git rev-parse HEAD)"
    if [[ "${ACTUAL_CIRCT_COMMIT}" == "${CIRCT_COMMIT}" ]]; then
      log_pass "CIRCT commit matches pinned: ${CIRCT_COMMIT:0:9}"
    else
      log_warn "CIRCT commit mismatch: pinned=${CIRCT_COMMIT:0:9} actual=${ACTUAL_CIRCT_COMMIT:0:9}"
      log_info "  Update tool-versions.env if this version is verified."
      if ((STRICT_MODE)); then
        MANDATORY_FAIL=1
      fi
    fi
  fi
fi

# ============================================================================
# 2. Mandatory tool version checks (min + pinned)
# ============================================================================
echo ""
echo "${BOLD}=== Mandatory Tool Checks ===${RESET}"

# check_tool NAME CMD MIN_VAR PINNED_VAR VER_CMD
#   MIN_VAR    = variable name holding minimum version (from tool-versions.env)
#   PINNED_VAR = variable name holding pinned version  (from tool-versions.env)
check_tool() {
  local name="$1" cmd="$2" min_ver="$3" pinned_ver="$4" ver_cmd="$5"
  local bin_path method short_path

  bin_path=$(command -v "${cmd}" 2>/dev/null || true)
  if [[ -n "${bin_path}" ]]; then
    method=$(detect_method "${bin_path}")
    short_path=$(shorten_path "${bin_path}")
  else
    method="-"; short_path="-"
  fi

  if [[ -z "${bin_path}" ]]; then
    log_fail "${name}: not found (need >= ${min_ver})"
    add_summary "${name}" "N/A" "-" "-" "FAIL"
    return
  fi

  local raw_ver
  raw_ver=$(eval "${ver_cmd}" 2>&1 || true)
  local ver
  ver=$(extract_version "${raw_ver}")

  if [[ -z "${ver}" ]]; then
    log_warn "${name}: installed but could not parse version from: ${raw_ver}"
    add_summary "${name}" "unknown" "${method}" "${short_path}" "WARN"
    return
  fi

  # Check minimum version
  if ! version_ge "${ver}" "${min_ver}"; then
    log_fail "${name} ${ver} (need >= ${min_ver})"
    add_summary "${name}" "${ver}" "${method}" "${short_path}" "FAIL"
    return
  fi

  # Check pinned version (major version drift)
  local status="PASS"
  if [[ -n "${pinned_ver}" ]]; then
    local actual_major pinned_major
    actual_major=$(major_version "${ver}")
    pinned_major=$(major_version "${pinned_ver}")
    if [[ "${actual_major}" != "${pinned_major}" ]]; then
      log_warn "${name} ${ver} — major version differs from pinned ${pinned_ver}"
      log_info "  Verify compatibility, then update tool-versions.env."
      if ((STRICT_MODE)); then
        MANDATORY_FAIL=1
        status="FAIL"
      else
        status="WARN"
      fi
    else
      log_pass "${name} ${ver} (pinned: ${pinned_ver}, min: ${min_ver})"
    fi
  else
    log_pass "${name} ${ver} (>= ${min_ver})"
  fi

  add_summary "${name}" "${ver}" "${method}" "${short_path}" "${status}"
}

check_tool "g++"       "g++"       "${GPP_MIN}"       "${GPP_PINNED}"       "g++ --version | head -1"
check_tool "cmake"     "cmake"     "${CMAKE_MIN}"     "${CMAKE_PINNED}"     "cmake --version | head -1"
check_tool "ninja"     "ninja"     "${NINJA_MIN}"     "${NINJA_PINNED}"     "ninja --version"
check_tool "python3"   "python3"   "${PYTHON_MIN}"    "${PYTHON_PINNED}"    "python3 --version"
check_tool "make"      "make"      "${MAKE_MIN}"      "${MAKE_PINNED}"      "make --version | head -1"
check_tool "git"       "git"       "${GIT_MIN}"       "${GIT_PINNED}"       "git --version"
check_tool "verilator" "verilator" "${VERILATOR_MIN}" "${VERILATOR_PINNED}" "verilator --version 2>&1 | head -1"

# ── Linters (pinned version check, no minimum) ───────────────────────────
echo ""
echo "${BOLD}=== Linter Checks ===${RESET}"

check_linter() {
  local name="$1" cmd="$2" pinned_ver="$3" ver_cmd="$4"
  local bin_path method short_path

  bin_path=$(command -v "${cmd}" 2>/dev/null || true)
  if [[ -n "${bin_path}" ]]; then
    method=$(detect_method "${bin_path}")
    short_path=$(shorten_path "${bin_path}")
  else
    method="-"; short_path="-"
  fi

  if [[ -z "${bin_path}" ]]; then
    log_warn "${name}: not found (linting will be skipped for this tool)"
    add_summary "${name}" "N/A" "-" "-" "WARN"
    return
  fi

  local raw_ver
  raw_ver=$(eval "${ver_cmd}" 2>&1 || true)
  local ver
  ver=$(extract_version "${raw_ver}")
  ver="${ver:-unknown}"

  local status="PASS"
  if [[ -n "${pinned_ver}" ]] && [[ "${ver}" != "unknown" ]]; then
    local actual_major pinned_major
    actual_major=$(major_version "${ver}")
    pinned_major=$(major_version "${pinned_ver}")
    if [[ "${actual_major}" != "${pinned_major}" ]]; then
      log_warn "${name} ${ver} — major version differs from pinned ${pinned_ver}"
      status="WARN"
    else
      log_pass "${name} ${ver} (pinned: ${pinned_ver})"
    fi
  else
    log_pass "${name} ${ver}"
  fi

  add_summary "${name}" "${ver}" "${method}" "${short_path}" "${status}"
}

check_linter "clang-format" "clang-format" "${CLANG_FORMAT_PINNED}" "clang-format --version"
check_linter "clang-tidy"   "clang-tidy"   "${CLANG_TIDY_PINNED}"  "clang-tidy --version | head -1"
check_linter "jq"           "jq"           "${JQ_PINNED}"          "jq --version"
check_linter "lit"          "lit"          "${LIT_PINNED}"         "lit --version | head -1"

# ── Optional system tools ────────────────────────────────────────────────
echo ""
echo "${BOLD}=== Optional System Tools ===${RESET}"

check_optional() {
  local name="$1" cmd="$2" pinned_ver="$3" ver_cmd="$4"
  local bin_path method short_path

  bin_path=$(command -v "${cmd}" 2>/dev/null || true)
  if [[ -n "${bin_path}" ]]; then
    method=$(detect_method "${bin_path}")
    short_path=$(shorten_path "${bin_path}")
  else
    method="-"; short_path="-"
  fi

  if [[ -n "${bin_path}" ]]; then
    local raw_ver
    raw_ver=$(eval "${ver_cmd}" 2>&1 || true)
    local ver
    ver=$(extract_version "${raw_ver}")
    ver="${ver:-unknown}"
    if [[ -n "${pinned_ver}" ]]; then
      log_pass "${name} ${ver} (pinned: ${pinned_ver})"
    else
      log_pass "${name} ${ver}"
    fi
    add_summary "${name}" "${ver}" "${method}" "${short_path}" "PASS"
  else
    log_warn "${name}: not found (optional)"
    if ! sudo -n true 2>/dev/null; then
      log_info "  sudo requires password — skipping auto-install."
    fi
    add_summary "${name}" "N/A" "-" "-" "SKIP"
  fi
}

check_optional "verible"    "verible-verilog-lint" "${VERIBLE_PINNED}"    "verible-verilog-lint --version 2>&1 | head -1"
check_optional "shellcheck" "shellcheck"           "${SHELLCHECK_PINNED}" "shellcheck --version | grep '^version:' | head -1"

# ── EDA Tools: VCS / ncsim (Synopsys / Cadence) ──────────────────────────
echo ""
echo "${BOLD}=== EDA Tool Detection (VCS / ncsim) ===${RESET}"

# --- VCS ---
VCS_HOME="${SYNOPSYS_ROOT}/vcs/${VCS_VER}"
if [[ -d "${VCS_HOME}" ]]; then
  export VCS_HOME
  export SYNOPSYS_ROOT
  export SNPSLMD_LICENSE_FILE="${SNPSLMD_LICENSE_FILE:-${SNPS_LICENSE_SERVER}}"
  export LM_LICENSE_FILE="${SNPSLMD_LICENSE_FILE}${LM_LICENSE_FILE:+:${LM_LICENSE_FILE}}"
  export VCS_CC="/usr/bin/gcc"
  export PATH="${VCS_HOME}/bin:${VCS_HOME}/amd64/bin:${PATH}"

  # Verdi (같은 버전)
  VERDI_HOME="${SYNOPSYS_ROOT}/verdi/${VERDI_VER}"
  if [[ -d "${VERDI_HOME}" ]]; then
    export VERDI_HOME
    export PATH="${VERDI_HOME}/bin:${PATH}"
  fi

  log_pass "VCS ${VCS_VER} (requires -full64 on this kernel)"
  log_info "  VCS_HOME=${VCS_HOME}"
  add_summary "vcs" "${VCS_VER}" "vendor" "$(shorten_path "${VCS_HOME}")" "PASS"
else
  log_warn "VCS: not found at ${VCS_HOME}"
  log_info "  Phase 3 VCS co-sim will be skipped."
  add_summary "vcs" "N/A" "vendor" "-" "SKIP"
fi

# --- ncsim (Cadence Incisive) ---
if [[ -d "${IUS_HOME}" ]]; then
  export IUS_HOME
  export LDV_HOME="${IUS_HOME}"
  export PATH="${IUS_HOME}/tools/bin:${PATH}"

  local_ncsim_ver=$(ncsim -version 2>&1 | head -1 || true)
  local_ncsim_extracted=$(extract_version "${local_ncsim_ver}")
  log_pass "ncsim ${local_ncsim_extracted} (pinned: ${NCSIM_PINNED})"
  log_info "  IUS_HOME=${IUS_HOME}"
  add_summary "ncsim" "${local_ncsim_extracted}" "vendor" "$(shorten_path "${IUS_HOME}")" "PASS"
else
  log_warn "ncsim: not found at ${IUS_HOME}"
  add_summary "ncsim" "N/A" "vendor" "-" "SKIP"
fi

# ============================================================================
# 3. Python packages (pip)
# ============================================================================
echo ""
echo "${BOLD}=== Python Package Installation ===${RESET}"

PIP_INSTALL_FLAGS=()

if [[ -n "${VIRTUAL_ENV:-}" ]]; then
  log_info "Using active virtualenv: ${VIRTUAL_ENV}"
elif [[ -d "${PROJECT_ROOT}/.venv" ]]; then
  log_info "Activating existing virtualenv: ${PROJECT_ROOT}/.venv"
  # shellcheck disable=SC1091
  source "${PROJECT_ROOT}/.venv/bin/activate"
else
  log_info "No virtualenv detected — creating .venv (PEP 668 compliance)"
  if python3 -m venv "${PROJECT_ROOT}/.venv" 2>/dev/null; then
    # shellcheck disable=SC1091
    source "${PROJECT_ROOT}/.venv/bin/activate"
    log_pass "Created and activated virtualenv: ${PROJECT_ROOT}/.venv"
  else
    log_warn "Could not create virtualenv — trying --user install"
    PIP_INSTALL_FLAGS+=(--user)
  fi
fi

# Install required packages
if [[ -f "${PROJECT_ROOT}/requirements.txt" ]]; then
  log_info "Installing required Python packages..."
  if python3 -m pip install ${PIP_INSTALL_FLAGS[@]+"${PIP_INSTALL_FLAGS[@]}"} -r "${PROJECT_ROOT}/requirements.txt" --quiet 2>&1; then
    log_pass "requirements.txt packages installed"
  else
    log_warn "Some packages from requirements.txt failed to install"
  fi
else
  log_warn "requirements.txt not found at ${PROJECT_ROOT}/"
fi

# Install optional packages (failure is non-critical)
if [[ -f "${PROJECT_ROOT}/requirements-optional.txt" ]]; then
  log_info "Installing optional Python packages..."
  if python3 -m pip install ${PIP_INSTALL_FLAGS[@]+"${PIP_INSTALL_FLAGS[@]}"} -r "${PROJECT_ROOT}/requirements-optional.txt" --quiet 2>&1; then
    log_pass "requirements-optional.txt packages installed"
  else
    log_warn "Some optional packages failed to install (non-critical)"
  fi
fi

# Verify pip-installed tools
echo ""
echo "${BOLD}=== Python Tool Verification ===${RESET}"

check_linter "black"  "black"  "" "black --version"
check_linter "flake8" "flake8" "" "flake8 --version | head -1"
check_linter "mypy"   "mypy"   "" "mypy --version"

echo ""
echo "${BOLD}=== Optional Python Tool Verification ===${RESET}"

check_optional "cocotb" "cocotb-config" "" "cocotb-config --version"
check_optional "mkdocs" "mkdocs"        "" "mkdocs --version"

mkdocs_material_ver=$(python3 -c "from importlib.metadata import version; print(version('mkdocs-material'))" 2>/dev/null || true)
if [[ -n "${mkdocs_material_ver}" ]]; then
  pip_site=$(python3 -c "import site; print(site.getsitepackages()[0])" 2>/dev/null || echo "-")
  log_pass "mkdocs-material ${mkdocs_material_ver}"
  add_summary "mkdocs-material" "${mkdocs_material_ver}" "pip" "$(shorten_path "${pip_site}")" "PASS"
else
  log_warn "mkdocs-material: not found (optional)"
  add_summary "mkdocs-material" "N/A" "-" "-" "SKIP"
fi

# ============================================================================
# 4. Summary table
# ============================================================================
echo ""
echo "${BOLD}================================================================${RESET}"
echo "${BOLD}  HIRCT Environment Summary${RESET}"
echo "${BOLD}================================================================${RESET}"
printf "  ${BOLD}    %-18s %-16s %-7s %s${RESET}\n" "Tool" "Version" "Via" "Path"
printf "  --- %-18s %-16s %-7s %s\n" "----" "-------" "---" "----"

for row in "${SUMMARY_ROWS[@]}"; do
  IFS='|' read -r tool version method path status <<< "${row}"
  local_color="${RESET}"
  local_marker="${status}"
  case "${status}" in
    PASS) local_color="${GREEN}"; local_marker="[V]" ;;
    FAIL) local_color="${RED}"; local_marker="[X]" ;;
    WARN) local_color="${YELLOW}"; local_marker="[!]" ;;
    SKIP) local_color="${YELLOW}"; local_marker="[-]" ;;
  esac
  printf "  ${local_color}%-3s${RESET} %-18s %-16s %-7s %s\n" "${local_marker}" "${tool}" "${version}" "${method}" "${path}"
done

echo "${BOLD}================================================================${RESET}"

if ((STRICT_MODE)); then
  echo "  ${CYAN}(strict mode: pinned version mismatch = FAIL)${RESET}"
fi

if ((MANDATORY_FAIL)); then
  echo ""
  echo "${RED}${BOLD}ERROR: One or more mandatory tools failed validation.${RESET}"
  echo "       Fix the issues above and re-run: bash utils/setup-env.sh"
  exit 1
else
  echo ""
  if ((WARN_COUNT > 0)); then
    echo "${YELLOW}${BOLD}OK with ${WARN_COUNT} warning(s).${RESET} All mandatory tools passed."
  else
    echo "${GREEN}${BOLD}All checks passed.${RESET} Environment is ready for HIRCT development."
  fi
  exit 0
fi
