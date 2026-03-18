# ============================================================================
# HIRCT Root Makefile — Project-level orchestration
#
# Delegates tool targets (build, check, lint) to hirct/Makefile.
# Example-project targets (generate, verify, cosim) live in examples/.
#
# Requirements: GNU Make >= 4.0
# ============================================================================

HAVE_PRECOMMIT ?= $(shell which pre-commit >/dev/null 2>&1 && echo 1 || echo 0)

# ── Phony declarations ────────────────────────────────────────────────────

.PHONY: setup build check-hirct check-hirct-unit check-hirct-integration \
       lint lint-precommit docs clean help

# ── Default target ─────────────────────────────────────────────────────────

.DEFAULT_GOAL := help

# ── help ───────────────────────────────────────────────────────────────────

help:
	@echo "HIRCT Makefile — available targets:"
	@echo ""
	@echo "  Tool (delegated to hirct/):"
	@echo "    make setup              — Run environment setup (install/verify tools)"
	@echo "    make build              — Build hirct-gen/hirct-verify via cmake+ninja"
	@echo "    make check-hirct        — Run lit tests (requires build)"
	@echo "    make check-hirct-unit   — Run gtest unit tests"
	@echo "    make check-hirct-integration — Run integration smoke tests"
	@echo "    make lint               — Run linters (clang-format, verible, black, shellcheck)"
	@echo "    make lint-precommit     — Run all pre-commit hooks on all files"
	@echo "    make docs               — Build mkdocs documentation site"
	@echo "    make clean              — Remove hirct/build/ and site/"
	@echo ""
	@echo "  Example projects (run inside each project directory):"
	@echo "    cd examples/fc6161/pt_plat && make help"
	@echo ""

# ── setup ──────────────────────────────────────────────────────────────────

setup:
	@bash hirct/utils/setup-env.sh
ifeq ($(HAVE_PRECOMMIT),1)
	@if [ -f .pre-commit-config.yaml ]; then \
		echo "[setup] Installing pre-commit hooks..."; \
		pre-commit install; \
	fi
endif

# ── build ──────────────────────────────────────────────────────────────────

build:
	$(MAKE) -C hirct build

# ── check-hirct ────────────────────────────────────────────────────────────

check-hirct:
	$(MAKE) -C hirct check-hirct

check-hirct-unit:
	$(MAKE) -C hirct check-hirct-unit

check-hirct-integration:
	$(MAKE) -C hirct check-hirct-integration

# ── lint ───────────────────────────────────────────────────────────────────

lint:
	$(MAKE) -C hirct lint

lint-precommit:
ifeq ($(HAVE_PRECOMMIT),1)
	@pre-commit run --all-files
else
	@echo "[lint] pre-commit not found — install with: pip install pre-commit"
endif

# ── docs ──────────────────────────────────────────────────────────────────

HAVE_MKDOCS ?= $(shell which mkdocs >/dev/null 2>&1 && echo 1 || echo 0)

docs:
ifeq ($(HAVE_MKDOCS),1)
	@echo "[docs] Building documentation site..."
	@mkdocs build --strict 2>&1
	@test -f site/index.html && echo "[docs] Success: site/index.html exists" || (echo "[docs] FAIL: site/index.html not found"; exit 1)
else
	@echo "[docs] mkdocs not found — install with: pip install -r requirements.txt"
	@exit 1
endif

# ── clean ──────────────────────────────────────────────────────────────────

clean:
	$(MAKE) -C hirct clean
	rm -rf site/
	@echo "[clean] Removed hirct/build/ and site/"
