# ============================================================================
# examples/fc6161/pt_plat/Makefile — fc6161 uart_top generation & simulation
#
# Requirements: GNU Make >= 4.0, hirct-gen/hirct-verify built, circt-verilog
# ============================================================================

###############
# INCLUDES    #
###############

include config/env.mk

###############
# VARIABLES   #
###############

UART_FILELIST     ?= ${PROJECT}/rtl/pt_plat/bin/lst/uart_file_list.f
MPM_BRAM_FILELIST ?= ${PROJECT}/rtl/pt_mpm/bin/lst/mpm_bram_file_list.f
OUTPUT_DIR        ?= output

###############
# TARGETS     #
###############

.PHONY: generate-uart verify-uart \
        generate-mpm-bram \
        rtl-sim-uart-vcs rtl-sim-uart-ncsim \
        equiv-uart-vcs equiv-uart-ncsim \
        run-equiv-uart-vcs run-equiv-uart-ncsim \
        cosim-uart-chip-vcs \
        survey-errors \
        clean help

.DEFAULT_GOAL := help

help:
	@echo "examples/fc6161/pt_plat/Makefile — fc6161 project targets:"
	@echo ""
	@echo "  make generate-uart        — Generate all artifacts for uart_top"
	@echo "  make verify-uart          — Run hirct-verify on uart_top model"
	@echo "  make rtl-sim-uart-vcs     — VCS RTL simulation of uart_top"
	@echo "  make rtl-sim-uart-ncsim   — NCSim RTL simulation of uart_top"
	@echo "  make equiv-uart-vcs       — DPI-C lock-step compile (VCS)"
	@echo "  make run-equiv-uart-vcs   — DPI-C lock-step compile + run (VCS)"
	@echo "  make equiv-uart-ncsim     — DPI-C lock-step compile (ncsim)"
	@echo "  make run-equiv-uart-ncsim — DPI-C lock-step compile + run (ncsim)"
	@echo "  make cosim-uart-chip-vcs  — Chip-level co-sim (requires VCS)"
	@echo "  make generate-mpm-bram    — Generate artifacts for mpm_bram_top (verilator preprocess)"
	@echo "  make survey-errors        — Survey all .v files, classify errors"
	@echo "  make clean                — Remove output/"
	@echo ""
	@echo "Survey options:"
	@echo "  SURVEY_LIMIT=3            — Limit targets for testing"
	@echo ""
	@echo "NOTE: Build hirct tools first: make -C ../../../hirct build"

###############
# RULES       #
###############

generate-uart:
	export PROJECT=${PROJECT} && \
	${HIRCT_GEN} -f ${UART_FILELIST} \
		--top uart_top \
		--timescale 1ns/10ps \
		--verbose \
		-o ${OUTPUT_DIR}/uart_top

verify-uart: generate-uart
	${HIRCT_VERIFY} ${OUTPUT_DIR}/uart_top/meta.json --verbose

rtl-sim-uart-vcs:
	${MAKE} -C sim/uart_top vcs

rtl-sim-uart-ncsim:
	${MAKE} -C sim/uart_top ncsim

equiv-uart-vcs:
	${MAKE} -C cosim/uart_top equiv-vcs

run-equiv-uart-vcs:
	${MAKE} -C cosim/uart_top run-equiv-vcs

equiv-uart-ncsim:
	${MAKE} -C cosim/uart_top equiv-ncsim

run-equiv-uart-ncsim:
	${MAKE} -C cosim/uart_top run-equiv-ncsim

generate-mpm-bram:
	export PROJECT=${PROJECT} && \
	${HIRCT_GEN} -f ${MPM_BRAM_FILELIST} \
		--top mpm_bram_top \
		--preprocess verilator \
		-o ${OUTPUT_DIR}/mpm_bram_top

cosim-uart-chip-vcs:
	@echo "ERROR: chip-level co-sim requires VCS license + complete CModel"
	@echo "See cosim/uart_top/chip_cosim.md for details"
	@exit 1

###############
# SURVEY      #
###############

SURVEY_LIMIT  ?=
SURVEY_OUTPUT ?= survey-results

survey-errors:
	python3 utils/survey-errors.py \
		--project "${PROJECT}" \
		--hirct-gen "${HIRCT_GEN}" \
		--targets config/survey-targets.txt \
		--stubs-dir config/stubs \
		--output "${SURVEY_OUTPUT}" \
		$(if ${SURVEY_LIMIT},--limit ${SURVEY_LIMIT},)

###############
# CLEAN       #
###############

clean:
	@if [ -n "${OUTPUT_DIR}" ]; then rm -rf "${OUTPUT_DIR}/"; fi
	@echo "[clean] Removed ${OUTPUT_DIR}/"
