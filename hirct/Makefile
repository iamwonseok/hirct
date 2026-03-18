# ============================================================================
# hirct/Makefile — HIRCT tool build, test, and lint
#
# Standalone: cd hirct && make build
# From root:  make -C hirct build
#
# Requirements: GNU Make >= 4.0, CMake, Ninja
# ============================================================================

# ── Variables ──────────────────────────────────────────────────────────────

HIRCT_GEN     ?= build/bin/hirct-gen
HIRCT_VERIFY  ?= build/bin/hirct-verify

HAVE_VERIBLE      ?= $(shell which verible-verilog-lint >/dev/null 2>&1 && echo 1 || echo 0)
HAVE_BLACK        ?= $(shell which black >/dev/null 2>&1 && echo 1 || echo 0)
HAVE_CLANG_FORMAT ?= $(shell which clang-format >/dev/null 2>&1 && echo 1 || echo 0)
HAVE_SHELLCHECK   ?= $(shell which shellcheck >/dev/null 2>&1 && echo 1 || echo 0)

# ── Phony declarations ────────────────────────────────────────────────────

.PHONY: build check-hirct check-hirct-unit check-hirct-integration \
       lint lint-cpp lint-sv lint-py lint-sh clean help

# ── Default target ─────────────────────────────────────────────────────────

.DEFAULT_GOAL := help

# ── help ───────────────────────────────────────────────────────────────────

help:
	@echo "hirct/Makefile — HIRCT tool targets:"
	@echo ""
	@echo "  make build                   — cmake + ninja"
	@echo "  make check-hirct             — lit tests (requires build)"
	@echo "  make check-hirct-unit        — gtest unit tests"
	@echo "  make check-hirct-integration — integration smoke tests"
	@echo "  make lint                    — C++/SV/Py/Shell linters"
	@echo "  make clean                   — rm -rf build/"
	@echo ""

# ── build ──────────────────────────────────────────────────────────────────

build:
	@if [ ! -f CMakeLists.txt ]; then \
		echo "CMakeLists.txt not found."; exit 1; \
	elif [ -f build/build.ninja ]; then \
		ninja -C build; \
	else \
		cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && \
		ninja -C build; \
	fi

# ── check-hirct ────────────────────────────────────────────────────────────

check-hirct: build
	@ninja -C build check-hirct

# ── check-hirct-unit ──────────────────────────────────────────────────────

check-hirct-unit:
	@if ninja -C build -t targets all 2>/dev/null | grep -q check-hirct-unit; then \
		ninja -C build check-hirct-unit; \
	else \
		echo "[check-hirct-unit] Not configured. Re-run: cmake -B build -G Ninja -DHIRCT_ENABLE_UNITTESTS=ON"; \
	fi

# ── check-hirct-integration ───────────────────────────────────────────────

check-hirct-integration: build
	@if [ ! -d integration_test/smoke ]; then \
		echo "[check-hirct-integration] integration_test/smoke/ not found — skipping"; \
	elif [ -z "$$(find integration_test/smoke -name '*.mlir' -o -name '*.test' -o -name '*.sv' 2>/dev/null)" ]; then \
		echo "[check-hirct-integration] No test files — skipping"; \
	else \
		lit -sv integration_test/smoke/; \
	fi

# ── lint ───────────────────────────────────────────────────────────────────

lint: lint-cpp lint-sv lint-py lint-sh

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

# ── clean ──────────────────────────────────────────────────────────────────

clean:
	rm -rf build/
	@echo "[clean] Removed build/"
