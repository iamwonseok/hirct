# ============================================================================
# examples/fc6161/pt_plat/sim/uart_top/Makefile — RTL-only simulation
# ============================================================================

###############
# INCLUDES    #
###############

include ../../config/env.mk

###############
# VARIABLES   #
###############

RTL_DIR          := ${PROJECT}/rtl/pt_plat/src/uart
TB_FILE          := ../../output/uart_top/tb/uart_top_tb.sv
FILELIST         := ${PROJECT}/rtl/pt_plat/bin/lst/uart_file_list.f
STUBS_DIR        := ../../config/stubs

# VCS
VCS              := ${VCS_HOME}/bin/vcs
VCS_FLAGS        := -full64 -sverilog -timescale=1ns/1ps \
                    +v2k -notice -q \
                    -f ${FILELIST} \
                    -y ${STUBS_DIR} +libext+.v \
                    +define+PROJECT="${PROJECT}" \
                    -kdb -lca \
                    -debug_access+all
# -kdb already loads verdi.tab; explicit -P novas.tab would duplicate PLI
# definitions (Warning-[UFTMD]).  Only link pli.a for FSDB runtime support.
VERDI_FLAGS      := ${VERDI_HOME}/share/PLI/VCS/LINUX64/pli.a

# NCSim (Cadence irun)
IRUN             := ${IUS_HOME}/tools/bin/irun
FILELIST_NCSIM   := uart_file_list_ncsim.f
IRUN_FLAGS       := -sv -timescale 1ns/1ps \
                    -f ${FILELIST_NCSIM} \
                    -y ${STUBS_DIR} +libext+.v \
                    -access +rwc

###############
# TARGETS     #
###############

.PHONY: vcs run-vcs ncsim clean help

.DEFAULT_GOAL := help

help:
	@echo "sim/uart_top — RTL-only simulation targets:"
	@echo ""
	@echo "  make vcs      - Compile with VCS"
	@echo "  make run-vcs  - Compile and run VCS simulation"
	@echo "  make ncsim    - Compile and run with NCSim (irun)"
	@echo "  make clean    - Remove build artifacts"

###############
# RULES       #
###############

# --- VCS ---
vcs: simv

simv: ${TB_FILE}
	export PROJECT=${PROJECT} && \
	${VCS} ${VCS_FLAGS} ${VERDI_FLAGS} \
		${TB_FILE} \
		-o simv

run-vcs: simv
	./simv +fsdbfile+uart_top.fsdb | tee sim.log
	@echo "=== VCS Simulation Complete ==="
	@grep -E "PASS|FAIL|Results" sim.log || true

# --- NCSim ---
${FILELIST_NCSIM}: ${FILELIST}
	grep -v '\.vh$$' ${FILELIST} > $@

ncsim: ${FILELIST_NCSIM} ${TB_FILE}
	export PROJECT=${PROJECT} && \
	${IRUN} ${IRUN_FLAGS} ${TB_FILE} 2>&1 | tee sim.log
	@echo "=== NCSim Simulation Complete ==="
	@grep -E "PASS|FAIL|Results" sim.log || true

# --- Clean ---
clean:
	rm -rf simv simv.daidir csrc *.fsdb *.log *.key ucli.key AN.DB
	rm -rf INCA_libs xcelium.d *.shm waves.shm ${FILELIST_NCSIM}
