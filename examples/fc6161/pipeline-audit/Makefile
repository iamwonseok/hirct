# fc6161 pt_plat_only pipeline audit
# Usage:
#   make audit              — run all IPs x all stages
#   make audit IP=uart      — run single IP through all stages
#   make stage-0            — Stage 0 for all IPs
#   make stage-0 IP=uart    — Stage 0 for uart only
#   make report             — generate report.json + summary.json
#   make clean              — remove output directory
#   make help               — this message

###############################################################################
# Paths (must be set before env.mk alters MAKEFILE_LIST)
###############################################################################
SCRIPT_DIR  := ${abspath ${dir ${lastword ${MAKEFILE_LIST}}}}
OUTPUT_BASE ?= ${SCRIPT_DIR}/../../output/fc6161

###############################################################################
# Environment (SSOT)
###############################################################################
include ${SCRIPT_DIR}/../pt_plat/config/env.mk
PARSE_FL    := ${SCRIPT_DIR}/parse-filelist.py
GEN_REPORT  := ${SCRIPT_DIR}/gen-report.py

###############################################################################
# IP Registry
###############################################################################
IP_LIST := \
  lib s5 s5mc ahb axi_2x3 axi_x2p1 axi_x2p2 axi_x2h \
  clk_rst_gen fsl_wflow edma fadu_cse pka trng secreg \
  gpio i2cm smbus i3c func_test_io_mux ptimer sys_ctrl \
  uart wdt msgd_router caliptra top chip \
  model prim ext_model

IP_COUNT := ${words ${IP_LIST}}

# Filelist paths (relative to $(PROJECT))
FILELIST_lib              := rtl/chip/bin/lst/lib_file_list.f
FILELIST_s5               := rtl/pt_plat/bin/lst/s5_file_list.f
FILELIST_s5mc             := rtl/pt_plat/bin/lst/s5mc_file_list.f
FILELIST_ahb              := rtl/pt_plat/bin/lst/ahb_file_list.f
FILELIST_axi_2x3          := rtl/pt_plat/bin/lst/axi_2x3_file_list.f
FILELIST_axi_x2p1         := rtl/pt_plat/bin/lst/axi_x2p1_file_list.f
FILELIST_axi_x2p2         := rtl/pt_plat/bin/lst/axi_x2p2_file_list.f
FILELIST_axi_x2h          := rtl/pt_plat/bin/lst/axi_x2h_file_list.f
FILELIST_clk_rst_gen      := rtl/pt_plat/bin/lst/clk_rst_gen_file_list.f
FILELIST_fsl_wflow        := rtl/pt_plat/bin/lst/fsl_wflow_file_list.f
FILELIST_edma             := rtl/pt_plat/bin/lst/edma_file_list.f
FILELIST_fadu_cse         := rtl/pt_plat/bin/lst/fadu_cse_file_list.f
FILELIST_pka              := rtl/pt_plat/bin/lst/pka_file_list.f
FILELIST_trng             := rtl/pt_plat/bin/lst/trng_file_list.f
FILELIST_secreg           := rtl/pt_plat/bin/lst/secreg_file_list.f
FILELIST_gpio             := rtl/pt_plat/bin/lst/gpio_file_list.f
FILELIST_i2cm             := rtl/pt_plat/bin/lst/i2cm_file_list.f
FILELIST_smbus            := rtl/pt_plat/bin/lst/smbus_file_list.f
FILELIST_i3c              := rtl/pt_plat/bin/lst/i3c_file_list.f
FILELIST_func_test_io_mux := rtl/pt_plat/bin/lst/func_test_io_mux_file_list.f
FILELIST_ptimer           := rtl/pt_plat/bin/lst/ptimer_file_list.f
FILELIST_sys_ctrl         := rtl/pt_plat/bin/lst/sys_ctrl_file_list.f
FILELIST_uart             := rtl/pt_plat/bin/lst/uart_file_list.f
FILELIST_wdt              := rtl/pt_plat/bin/lst/wdt_file_list.f
FILELIST_msgd_router      := rtl/pt_plat/bin/lst/msgd_router_file_list.f
FILELIST_caliptra         := rtl/pt_plat/bin/lst/caliptra_file_list.f
FILELIST_top              := rtl/pt_plat/bin/lst/top_file_list.f
FILELIST_chip             := rtl/chip/bin/lst/chip_file_list.f
FILELIST_model            := rtl/pt_plat/bin/lst/model_file_list.f
# prim: prefer prim_file_list.chip.f; fall back to prim_file_list.f
FILELIST_prim             := ${shell \
  if [ -f ${PROJECT}/rtl/chip/bin/lst/prim_file_list.chip.f ]; then \
    echo rtl/chip/bin/lst/prim_file_list.chip.f; \
  else \
    echo rtl/chip/bin/lst/prim_file_list.f; \
  fi}
FILELIST_ext_model        := rtl/pt_plat/bin/lst/ext_model_file_list.f

# Top modules (- means no explicit top; used with --top flag)
TOP_lib              := -
TOP_s5               := -
TOP_s5mc             := -
TOP_ahb              := DW_ahb
TOP_axi_2x3          := -
TOP_axi_x2p1         := -
TOP_axi_x2p2         := -
TOP_axi_x2h          := -
TOP_clk_rst_gen      := -
TOP_fsl_wflow        := -
TOP_edma             := -
TOP_fadu_cse         := -
TOP_pka              := -
TOP_trng             := -
TOP_secreg           := -
TOP_gpio             := DW_apb_gpio
TOP_i2cm             := i2cm_top
TOP_smbus            := -
TOP_i3c              := -
TOP_func_test_io_mux := -
TOP_ptimer           := ptimer_top
TOP_sys_ctrl         := -
TOP_uart             := uart_top
TOP_wdt              := wdt_top
TOP_msgd_router      := -
TOP_caliptra         := -
TOP_top              := pt_plat
TOP_chip             := -
TOP_model            := -
TOP_prim             := -
TOP_ext_model        := -

###############################################################################
# circt-opt pass chains (stage-0, stage-1, stage-2 use circt-opt directly)
###############################################################################
PASSES_STAGE1 := \
  --moore-create-vtables \
  --moore-lower-concatref \
  --moore-simplify-procedures \
  --convert-moore-to-core

PASSES_STAGE2 := \
  --llhd-deseq \
  --llhd-lower-processes \
  --llhd-sig2reg
# NOTE: --llhd-mem2reg excluded — triggers segfault (Mem2Reg.cpp:resolveDefinitionValue)
# in CIRCT upstream for hw.module inputs; tracked in known-limitations.md

# Stage 3 uses hirct-gen --dump-ir --pipeline-checkpoint-dir (full lowering pipeline)
HIRCT_GEN     ?= ${HIRCT_HOME}/build/bin/hirct-gen

###############################################################################
# IP filtering (IP= on CLI restricts to single IP)
###############################################################################
ifeq (${strip ${IP}},)
  ACTIVE_IPS := ${IP_LIST}
else
  ACTIVE_IPS := ${IP}
endif # IP

###############################################################################
# Rule generator macros
###############################################################################

# ── Stage 0 ────────────────────────────────────────────────────────────────
define STAGE0_RULE
${OUTPUT_BASE}/$(1)/stage-0-import/$(1).mlir:
	@mkdir -p $$(@D)
	@echo "[$(1)] Stage 0: importing Verilog -> Moore MLIR..."
	@ARGS=$$$$(python3 ${PARSE_FL} ${PROJECT}/${FILELIST_$(1)} 2>$$(@D)/stage-0-parse.log); \
	 TOP_FLAG=""; \
	 if [ "${TOP_$(1)}" != "-" ]; then TOP_FLAG="--top ${TOP_$(1)}"; fi; \
	 start=$$$$(date +%s); \
	 ${CIRCT_BIN}/circt-verilog --ir-moore --format=sv $$$$TOP_FLAG $$$$ARGS -o $$@ \
	   </dev/null >$$(@D)/stage-0.log 2>&1; \
	 rc=$$$$?; end=$$$$(date +%s); \
	 if [ $$$$rc -eq 0 ]; then \
	   echo "pass:$$$$((end-start))" > $$(@D)/.status; \
	   echo "[$(1)] Stage 0: OK ($$$$((end-start))s)"; \
	 else \
	   echo "fail:$$$$((end-start))" > $$(@D)/.status; \
	   echo "[$(1)] Stage 0: FAILED ($$$$((end-start))s) -- see $$(@D)/stage-0.log"; \
	 fi; \
	 exit 0
endef

# ── Stage 1 ────────────────────────────────────────────────────────────────
define STAGE1_RULE
${OUTPUT_BASE}/$(1)/stage-1-moore-to-core/$(1).mlir: \
    ${OUTPUT_BASE}/$(1)/stage-0-import/$(1).mlir
	@if [ -f $$(<D)/.status ] && grep -qE "^(fail|skip)" $$(<D)/.status 2>/dev/null; then \
	  mkdir -p $$(@D); \
	  echo "skip:prev-stage-0-failed" > $$(@D)/.status; \
	  echo "[$(1)] SKIP stage-1: stage-0 failed/skipped"; \
	  exit 0; \
	fi
	@mkdir -p $$(@D)
	@echo "[$(1)] Stage 1: Moore -> Core..."
	@start=$$$$(date +%s); \
	 ${CIRCT_BIN}/circt-opt ${PASSES_STAGE1} $$< -o $$@ \
	   >$$(@D)/stage-1.log 2>&1; \
	 rc=$$$$?; end=$$$$(date +%s); \
	 if [ $$$$rc -eq 0 ]; then \
	   echo "pass:$$$$((end-start))" > $$(@D)/.status; \
	   echo "[$(1)] Stage 1: OK ($$$$((end-start))s)"; \
	 else \
	   echo "fail:$$$$((end-start))" > $$(@D)/.status; \
	   echo "[$(1)] Stage 1: FAILED ($$$$((end-start))s) -- see $$(@D)/stage-1.log"; \
	 fi; \
	 exit 0
endef

# ── Stage 2 ────────────────────────────────────────────────────────────────
define STAGE2_RULE
${OUTPUT_BASE}/$(1)/stage-2-llhd-to-core/$(1).mlir: \
    ${OUTPUT_BASE}/$(1)/stage-1-moore-to-core/$(1).mlir
	@if [ -f $$(<D)/.status ] && grep -qE "^(fail|skip)" $$(<D)/.status 2>/dev/null; then \
	  mkdir -p $$(@D); \
	  echo "skip:prev-stage-1-failed" > $$(@D)/.status; \
	  echo "[$(1)] SKIP stage-2: stage-1 failed/skipped"; \
	  exit 0; \
	fi
	@mkdir -p $$(@D)
	@echo "[$(1)] Stage 2: LLHD -> Core..."
	@start=$$$$(date +%s); \
	 ${CIRCT_BIN}/circt-opt ${PASSES_STAGE2} $$< -o $$@ \
	   >$$(@D)/stage-2.log 2>&1; \
	 rc=$$$$?; end=$$$$(date +%s); \
	 if [ $$$$rc -eq 0 ]; then \
	   echo "pass:$$$$((end-start))" > $$(@D)/.status; \
	   echo "[$(1)] Stage 2: OK ($$$$((end-start))s)"; \
	 else \
	   echo "fail:$$$$((end-start))" > $$(@D)/.status; \
	   echo "[$(1)] Stage 2: FAILED ($$$$((end-start))s) -- see $$(@D)/stage-2.log"; \
	 fi; \
	 exit 0
endef

# ── Stage 3 (hirct-gen full lowering pipeline with checkpoints) ────────────
define STAGE3_RULE
${OUTPUT_BASE}/$(1)/stage-3-lowering/$(1).mlir: \
    ${OUTPUT_BASE}/$(1)/stage-2-llhd-to-core/$(1).mlir
	@if [ -f $$(<D)/.status ] && grep -qE "^(fail|skip)" $$(<D)/.status 2>/dev/null; then \
	  mkdir -p $$(@D); \
	  echo "skip:prev-stage-2-failed" > $$(@D)/.status; \
	  echo "[$(1)] SKIP stage-3: stage-2 failed/skipped"; \
	  exit 0; \
	fi
	@mkdir -p $$(@D)/checkpoints
	@echo "[$(1)] Stage 3: HIRCT lowering pipeline..."
	@start=$$$$(date +%s); \
	 ${HIRCT_GEN} --dump-ir \
	   --pipeline-checkpoint-dir $$(@D)/checkpoints \
	   $$< > $$@ 2>$$(@D)/stage-3.log; \
	 rc=$$$$?; end=$$$$(date +%s); \
	 if [ $$$$rc -eq 0 ]; then \
	   echo "pass:$$$$((end-start))" > $$(@D)/.status; \
	   echo "[$(1)] Stage 3: OK ($$$$((end-start))s)"; \
	 else \
	   echo "fail:$$$$((end-start))" > $$(@D)/.status; \
	   rm -f $$@; \
	   echo "[$(1)] Stage 3: FAILED ($$$$((end-start))s) -- see $$(@D)/stage-3.log"; \
	 fi; \
	 exit 0
endef

# ── Stage 4 (genmodel: hirct-gen full C++ model generation) ────────────────
define STAGE4_RULE
${OUTPUT_BASE}/$(1)/stage-4-genmodel/.done: \
    ${OUTPUT_BASE}/$(1)/stage-3-lowering/$(1).mlir
	@if [ -f $$(<D)/.status ] && grep -qE "^(fail|skip)" $$(<D)/.status 2>/dev/null; then \
	  mkdir -p $$(@D); \
	  echo "skip:prev-stage-3-failed" > $$(@D)/.status; \
	  echo "[$(1)] SKIP stage-4: stage-3 failed/skipped"; \
	  exit 0; \
	fi
	@mkdir -p $$(@D)
	@echo "[$(1)] Stage 4: genmodel (hirct-gen -f filelist -> C++ model)..."
	@TOP_FLAG=""; \
	 if [ "${TOP_$(1)}" != "-" ]; then TOP_FLAG="--top ${TOP_$(1)}"; fi; \
	 start=$$$$(date +%s); \
	 ${HIRCT_GEN} -f ${PROJECT}/${FILELIST_$(1)} $$$$TOP_FLAG \
	   -o $$(@D) \
	   >$$(@D)/stage-4.log 2>&1; \
	 rc=$$$$?; end=$$$$(date +%s); \
	 if [ $$$$rc -eq 0 ]; then \
	   echo "pass:$$$$((end-start))" > $$(@D)/.status; \
	   touch $$@; \
	   echo "[$(1)] Stage 4: OK ($$$$((end-start))s)"; \
	 else \
	   echo "fail:$$$$((end-start))" > $$(@D)/.status; \
	   echo "[$(1)] Stage 4: FAILED ($$$$((end-start))s) -- see $$(@D)/stage-4.log"; \
	 fi; \
	 exit 0
endef

# ── Stage 4-compile (g++ compile of generated C++ sources) ─────────────────
define STAGE4COMPILE_RULE
${OUTPUT_BASE}/$(1)/stage-4-compile/.done: \
    ${OUTPUT_BASE}/$(1)/stage-4-genmodel/.done
	@if [ ! -f ${OUTPUT_BASE}/$(1)/stage-4-genmodel/.status ] || \
	    grep -qE "^(fail|skip)" ${OUTPUT_BASE}/$(1)/stage-4-genmodel/.status 2>/dev/null; then \
	  mkdir -p $$(@D); \
	  echo "skip:prev-stage-4-failed" > $$(@D)/.status; \
	  echo "[$(1)] SKIP stage-4-compile: stage-4 failed/skipped"; \
	  exit 0; \
	fi
	@mkdir -p $$(@D)
	@echo "[$(1)] Stage 4-compile: g++ -std=c++17 compile..."
	@STAGE4_DIR="${OUTPUT_BASE}/$(1)/stage-4-genmodel"; \
	 HOST_GXX="$$$$(command -v g++ || echo g++)"; \
	 if ! find "$$STAGE4_DIR" -name '*.cpp' -print -quit 2>/dev/null | grep -q .; then \
	   echo "skip:no-cpp-files" > $$(@D)/.status; \
	   echo "[$(1)] Stage 4-compile: SKIP (no .cpp files generated)"; \
	   exit 0; \
	 fi; \
	 start=$$$$(date +%s); \
	 find "$$STAGE4_DIR" -name '*.cpp' -print0 | \
	   xargs -0 -r "$$HOST_GXX" -std=c++17 -c \
	   -I"$$STAGE4_DIR" \
	   -o /dev/null \
	   >$$(@D)/stage-4-compile.log 2>&1; \
	 rc=$$$$?; end=$$$$(date +%s); \
	 if [ $$$$rc -eq 0 ]; then \
	   echo "pass:$$$$((end-start))" > $$(@D)/.status; \
	   touch $$@; \
	   echo "[$(1)] Stage 4-compile: OK ($$$$((end-start))s)"; \
	 else \
	   echo "fail:$$$$((end-start))" > $$(@D)/.status; \
	   echo "[$(1)] Stage 4-compile: FAILED ($$$$((end-start))s) -- see $$(@D)/stage-4-compile.log"; \
	 fi; \
	 exit 0
endef

# ── Stage V (hirct-gen --preprocess verilator, independent) ────────────────
define STAGEV_RULE
${OUTPUT_BASE}/$(1)/stage-v-verilator-pp/.done:
	@mkdir -p $$(@D)
	@echo "[$(1)] Stage V: Verilator preprocess via hirct-gen..."
	@if ! command -v verilator >/dev/null 2>&1; then \
	  echo "skip:verilator-not-found" > $$(@D)/.status; \
	  echo "[$(1)] Stage V: SKIP (verilator not found)"; \
	  exit 0; \
	fi; \
	TOP_FLAG=""; \
	if [ "${TOP_$(1)}" != "-" ]; then TOP_FLAG="--top ${TOP_$(1)}"; fi; \
	start=$$$$(date +%s); \
	${HIRCT_GEN} -f ${PROJECT}/${FILELIST_$(1)} $$$$TOP_FLAG \
	  --preprocess verilator --dump-ir \
	  -o $$(@D) \
	  >$$(@D)/stage-v.log 2>&1; \
	rc=$$$$?; end=$$$$(date +%s); \
	if [ $$$$rc -eq 0 ]; then \
	  echo "pass:$$$$((end-start))" > $$(@D)/.status; \
	  touch $$@; \
	  echo "[$(1)] Stage V: OK ($$$$((end-start))s)"; \
	else \
	  echo "fail:$$$$((end-start))" > $$(@D)/.status; \
	  echo "[$(1)] Stage V: FAILED ($$$$((end-start))s) -- see $$(@D)/stage-v.log"; \
	fi; \
	exit 0
endef

# ── Per-IP report.json ─────────────────────────────────────────────────────
define REPORT_RULE
${OUTPUT_BASE}/$(1)/report.json:
	@python3 ${GEN_REPORT} --ip $(1) --base ${OUTPUT_BASE} \
	  --filelist ${FILELIST_$(1)} --top ${TOP_$(1)}
endef

###############################################################################
# Instantiate rules for all IPs
###############################################################################
${foreach ip,${IP_LIST},${eval ${call STAGE0_RULE,${ip}}}}
${foreach ip,${IP_LIST},${eval ${call STAGE1_RULE,${ip}}}}
${foreach ip,${IP_LIST},${eval ${call STAGE2_RULE,${ip}}}}
${foreach ip,${IP_LIST},${eval ${call STAGE3_RULE,${ip}}}}
${foreach ip,${IP_LIST},${eval ${call STAGE4_RULE,${ip}}}}
${foreach ip,${IP_LIST},${eval ${call STAGE4COMPILE_RULE,${ip}}}}
${foreach ip,${IP_LIST},${eval ${call STAGEV_RULE,${ip}}}}
${foreach ip,${IP_LIST},${eval ${call REPORT_RULE,${ip}}}}

###############################################################################
# Phony targets
###############################################################################
.PHONY: audit stage-0 stage-1 stage-2 stage-3 \
        stage-4 stage-4-compile stage-v report clean help

stage-0:         ${foreach ip,${ACTIVE_IPS},${OUTPUT_BASE}/${ip}/stage-0-import/${ip}.mlir}
stage-1:         ${foreach ip,${ACTIVE_IPS},${OUTPUT_BASE}/${ip}/stage-1-moore-to-core/${ip}.mlir}
stage-2:         ${foreach ip,${ACTIVE_IPS},${OUTPUT_BASE}/${ip}/stage-2-llhd-to-core/${ip}.mlir}
stage-3:         ${foreach ip,${ACTIVE_IPS},${OUTPUT_BASE}/${ip}/stage-3-lowering/${ip}.mlir}
stage-4:         ${foreach ip,${ACTIVE_IPS},${OUTPUT_BASE}/${ip}/stage-4-genmodel/.done}
stage-4-compile: ${foreach ip,${ACTIVE_IPS},${OUTPUT_BASE}/${ip}/stage-4-compile/.done}
stage-v:         ${foreach ip,${ACTIVE_IPS},${OUTPUT_BASE}/${ip}/stage-v-verilator-pp/.done}

ifdef STAGE
audit: ${STAGE}
else
audit: stage-0 stage-1 stage-2 stage-3 stage-4 stage-4-compile stage-v report
endif # STAGE

report: ${foreach ip,${ACTIVE_IPS},${OUTPUT_BASE}/${ip}/report.json}
	@python3 ${GEN_REPORT} --summary --base ${OUTPUT_BASE}


clean:
	@echo "Removing ${OUTPUT_BASE}..."
	@rm -rf ${OUTPUT_BASE}

help:
	@echo ""
	@echo "fc6161 pt_plat_only Pipeline Audit"
	@echo "==================================="
	@echo ""
	@echo "Targets:"
	@echo "  audit                    Run all IPs x all stages, then generate reports"
	@echo "  audit IP=<name>          Run single IP through all stages"
	@echo "  audit STAGE=<stage>      Run only the specified stage for all IPs"
	@echo "  audit IP=<n> STAGE=<s>   Run only the specified stage for single IP"
	@echo "  stage-0                  Import Verilog -> Moore MLIR  (all IPs)"
	@echo "  stage-0 IP=<name>        Import single IP"
	@echo "  stage-1                  Moore -> Core  (depends on stage-0)"
	@echo "  stage-2                  LLHD lowering  (depends on stage-1)"
	@echo "  stage-3                  HIRCT lowering pipeline (full, with checkpoints)  (depends on stage-2)"
	@echo "  stage-4                  C++ model generation via hirct-gen  (depends on stage-3)"
	@echo "  stage-4-compile          g++ compile of generated C++ sources  (depends on stage-4)"
	@echo "  stage-v                  hirct-gen --preprocess verilator  (independent)"
	@echo "  report                   Generate per-IP report.json + summary.json"
	@echo "  clean                    Remove \$${OUTPUT_BASE}"
	@echo "  help                     This message"
	@echo ""
	@echo "Variables:"
	@echo "  IP=<name>          Restrict to single IP (default: all ${IP_COUNT} IPs)"
	@echo "  STAGE=<name>       Restrict audit to single stage (e.g. stage-0)"
	@echo "  PROJECT=<path>     RTL root  (default: ${PROJECT})"
	@echo "  OUTPUT_BASE=<path> Output root  (default: ${OUTPUT_BASE})"
	@echo "  CIRCT_BIN=<path>   CIRCT binary dir  (default: ${CIRCT_BIN})"
	@echo "  HIRCT_GEN=<path>   hirct-gen binary  (default: ${HIRCT_GEN})"
	@echo ""
	@echo "Stage dependency chain:"
	@echo "  stage-0 -> stage-1 -> stage-2 -> stage-3 -> stage-4 -> stage-4-compile"
	@echo "  stage-v is independent"
	@echo ""
	@echo "Error handling:"
	@echo "  Each stage recipe always exits 0 — make never stops on a stage failure."
	@echo "  Results are recorded in <output>/<ip>/<stage>/.status (pass/fail/skip:elapsed)."
	@echo "  A failed stage causes all downstream stages to write skip:.status and do nothing."
	@echo "  Use 'make report' to aggregate .status files into report.json / summary.json."
	@echo ""
	@echo "Available IPs (${IP_COUNT} total):"
	@printf "  %s\n" ${IP_LIST}
	@echo ""
