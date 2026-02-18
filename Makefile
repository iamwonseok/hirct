# ============================================================================
# HIRCT Root Makefile — Phase 0 (setup/build/lint/clean)
#
# Phase 0 targets only. Phase 1+ targets (check-hirct, generate, test-all,
# etc.) will be added as their respective tasks are implemented.
#
# Requirements: GNU Make >= 4.0
# ============================================================================

# ── Variables ──────────────────────────────────────────────────────────────

HAVE_VCS          ?= $(shell which vcs >/dev/null 2>&1 && echo 1 || echo 0)
HAVE_VERIBLE      ?= $(shell which verible-verilog-lint >/dev/null 2>&1 && echo 1 || echo 0)
HAVE_BLACK        ?= $(shell which black >/dev/null 2>&1 && echo 1 || echo 0)
HAVE_CLANG_FORMAT ?= $(shell which clang-format >/dev/null 2>&1 && echo 1 || echo 0)
HAVE_SHELLCHECK   ?= $(shell which shellcheck >/dev/null 2>&1 && echo 1 || echo 0)
HAVE_PRECOMMIT    ?= $(shell which pre-commit >/dev/null 2>&1 && echo 1 || echo 0)

# ── Phony declarations ────────────────────────────────────────────────────

.PHONY: setup build lint lint-precommit clean help

# ── Default target ─────────────────────────────────────────────────────────

.DEFAULT_GOAL := help

# ── help ───────────────────────────────────────────────────────────────────

help:
	@echo "HIRCT Makefile — available targets:"
	@echo ""
	@echo "  make setup   — Run environment setup (install/verify tools)"
	@echo "  make build   — Build hirct-gen/hirct-verify (requires CMakeLists.txt)"
	@echo "  make lint    — Run linters (clang-format, verible, black, shellcheck)"
	@echo "  make lint-precommit — Run all pre-commit hooks on all files"
	@echo "  make clean   — Remove build/, output/, site/"
	@echo ""

# ── setup ──────────────────────────────────────────────────────────────────
# Runs the environment bootstrap script.  Safe to run multiple times
# (idempotent).

setup:
	@bash utils/setup-env.sh
ifeq ($(HAVE_PRECOMMIT),1)
	@if [ -f .pre-commit-config.yaml ]; then \
		echo "[setup] Installing pre-commit hooks..."; \
		pre-commit install; \
	fi
endif

# ── build ──────────────────────────────────────────────────────────────────
# Phase 0: CMakeLists.txt and C++ sources do not exist yet.
# Phase 1+: cmake -B build -G Ninja … && ninja -C build

build:
	@if [ ! -f CMakeLists.txt ]; then \
		echo "CMakeLists.txt not found. C++ sources will be created in Phase 1 Bootstrap (Task 100)."; \
	else \
		cmake -B build -G Ninja \
			-DCMAKE_BUILD_TYPE=Release \
			-DMLIR_DIR=$${MLIR_DIR} \
			-DLLVM_DIR=$${LLVM_DIR} && \
		ninja -C build; \
	fi

# ── lint ───────────────────────────────────────────────────────────────────
# Runs available linters.  Each sub-target checks for source files and tool
# availability before proceeding, so it always exits 0 when nothing to lint.

lint: lint-cpp lint-sv lint-py lint-sh

.PHONY: lint-cpp lint-sv lint-py lint-sh

lint-cpp:
ifeq ($(HAVE_CLANG_FORMAT),1)
	@CPP_FILES=$$(find include/ lib/ tools/ -name '*.cpp' -o -name '*.h' 2>/dev/null); \
	if [ -n "$$CPP_FILES" ]; then \
		echo "[lint] clang-format checking C/C++ files..."; \
		echo "$$CPP_FILES" | xargs clang-format --dry-run --Werror; \
	else \
		echo "[lint] No C/C++ files found — skipping clang-format"; \
	fi
else
	@echo "[lint] clang-format not found — skipping C/C++ lint"
endif # HAVE_CLANG_FORMAT

lint-sv:
ifeq ($(HAVE_VERIBLE),1)
	@SV_FILES=$$(find test/ integration_test/ -name '*.sv' 2>/dev/null); \
	if [ -n "$$SV_FILES" ]; then \
		echo "[lint] verible checking SV files..."; \
		echo "$$SV_FILES" | xargs verible-verilog-lint; \
	else \
		echo "[lint] No SV files for verible — skipping"; \
	fi
else
	@echo "[lint] verible not found — skipping SV lint"
endif # HAVE_VERIBLE

lint-py:
ifeq ($(HAVE_BLACK),1)
	@PY_FILES=$$(find utils/ test/ integration_test/ -name '*.py' 2>/dev/null); \
	if [ -n "$$PY_FILES" ]; then \
		echo "[lint] black checking Python files..."; \
		black --check --quiet $$PY_FILES; \
	else \
		echo "[lint] No Python files found — skipping black"; \
	fi
else
	@echo "[lint] black not found — skipping Python lint"
endif # HAVE_BLACK

lint-sh:
ifeq ($(HAVE_SHELLCHECK),1)
	@SH_FILES=$$(find utils/ -name '*.sh' 2>/dev/null); \
	if [ -n "$$SH_FILES" ]; then \
		echo "[lint] shellcheck checking Shell files..."; \
		echo "$$SH_FILES" | xargs shellcheck; \
	else \
		echo "[lint] No Shell files found — skipping shellcheck"; \
	fi
else
	@echo "[lint] shellcheck not found — skipping Shell lint"
endif # HAVE_SHELLCHECK

# ── lint-precommit ─────────────────────────────────────────────────────────
# Runs all pre-commit hooks on all files (not just staged).

lint-precommit:
ifeq ($(HAVE_PRECOMMIT),1)
	@pre-commit run --all-files
else
	@echo "[lint] pre-commit not found — install with: pip install pre-commit"
endif

# ── clean ──────────────────────────────────────────────────────────────────

clean:
	rm -rf build/ output/ site/
	@echo "[clean] Removed build/, output/, site/"
