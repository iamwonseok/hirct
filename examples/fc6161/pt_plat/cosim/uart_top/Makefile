# ============================================================================
# examples/fc6161/pt_plat/cosim/uart_top/Makefile — DPI-C lock-step equiv
#
# Incisive151 bundles GCC 4.4 whose compiler AND linker are incompatible
# with the host Ubuntu 24.04 (C++17, .relr.dyn ELF sections).
# C++ sources are compiled and linked into a shared library with the host
# GCC, then loaded via irun -sv_lib to bypass the bundled toolchain.
# ============================================================================

###############
# INCLUDES    #
###############

include ../../config/env.mk

###############
# VARIABLES   #
###############

RTL_DIR          := ${PROJECT}/rtl/pt_plat/src/uart
FILELIST         := ${PROJECT}/rtl/pt_plat/bin/lst/uart_file_list.f
NCSIM_FILELIST   := filelist.ncsim.f
OUTPUT_DIR       := ../../output/uart_top
DPI_DIR          := ${OUTPUT_DIR}/dpi
CMODEL_DIR       := ${OUTPUT_DIR}/cmodel
STUBS_DIR        := ../../config/stubs

IRUN             := ${IUS_HOME}/tools/bin/irun

DPI_SV           := ${DPI_DIR}/uart_top_dpi.sv
DPI_CPP          := ${DPI_DIR}/uart_top_dpi.cpp
CMODEL_CPP       := ${CMODEL_DIR}/uart_top.cpp

# Host-GCC compilation → shared library (bypasses ncsc compiler + linker)
NCSIM_GXX        ?= ${HOST_GXX}
cmodel_module_dirs := ${sort ${patsubst %/cmodel,%,${wildcard ${OUTPUT_DIR}/*/cmodel}}}
NCSIM_INC_ARGS   := -I${CMODEL_DIR} -I${OUTPUT_DIR} \
                    ${foreach d,${cmodel_module_dirs},-I${d}}

OBJ_DIR          := obj
DPI_OBJ          := ${OBJ_DIR}/uart_top_dpi.o
# All sub-module CModel sources + top-level CModel
CMODEL_SRCS      := ${wildcard ${OUTPUT_DIR}/*/cmodel/*.cpp} ${CMODEL_CPP}
CMODEL_OBJS      := ${patsubst ${OUTPUT_DIR}/%.cpp,${OBJ_DIR}/%.o,${CMODEL_SRCS}}
DPI_SO           := ${OBJ_DIR}/libdpi.so
HOST_CXXFLAGS    := -std=gnu++17 -fPIC -Wall -DNCSC -DCADENCE -DLNX86 \
                    -I${IUS_HOME}/tools/include \
                    -I${IUS_HOME}/tools/inca/include \
                    ${NCSIM_INC_ARGS}

###############
# VCS         #
###############

VCS              := ${VCS_HOME}/bin/vcs
VCS_INC_ARGS     := ${foreach d,${cmodel_module_dirs},-I${d}/cmodel}
VCS_CPP_SRCS     := ${DPI_CPP} ${CMODEL_SRCS}
# g++ 13+ (Ubuntu 24.04) defaults to -std=gnu++17; -std=c++17 omitted here
# because VCS -CFLAGS applies to both C and C++ compilations.
VCS_CFLAGS       := -I${CMODEL_DIR} ${VCS_INC_ARGS}

# -kdb already loads verdi.tab; explicit -P novas.tab would duplicate PLI
# definitions (Warning-[UFTMD]).  Only link pli.a for FSDB runtime support.
VERDI_PLI_DIR    := ${VERDI_HOME}/share/PLI/VCS/LINUX64
VERDI_FLAGS      := ${VERDI_PLI_DIR}/pli.a

###############
# TARGETS     #
###############

# Ubuntu 24.04 libstdc++ is newer than Verdi's bundled copy; preload system lib
# to avoid GLIBCXX_3.4.30 not-found errors.
VERDI_PRELOAD    := LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6
VERDI            := ${VERDI_HOME}/bin/verdi

.PHONY: equiv-vcs run-equiv-vcs equiv-ncsim wave clean help

.DEFAULT_GOAL := help

help:
	@echo "cosim/uart_top — DPI-C lock-step verification targets:"
	@echo ""
	@echo "  make equiv-vcs      - Compile RTL vs CModel equiv with VCS"
	@echo "  make run-equiv-vcs  - Compile and run VCS equiv simulation"
	@echo "  make wave           - Open FSDB waveform in Verdi"
	@echo "  make equiv-ncsim    - Run DPI-C lock-step verification with ncsim"
	@echo "  make clean          - Remove build artifacts"

###############
# RULES       #
###############

${OBJ_DIR}:
	@mkdir -p $@

${DPI_OBJ}: ${DPI_CPP} | ${OBJ_DIR}
	@echo "[host-gcc] compiling ${DPI_CPP}"
	${NCSIM_GXX} ${HOST_CXXFLAGS} -c -o $@ $<

${OBJ_DIR}/%.o: ${OUTPUT_DIR}/%.cpp | ${OBJ_DIR}
	@mkdir -p ${dir $@}
	@echo "[host-gcc] compiling $<"
	${NCSIM_GXX} ${HOST_CXXFLAGS} -c -o $@ $<

${DPI_SO}: ${DPI_OBJ} ${CMODEL_OBJS}
	@echo "[host-gcc] linking ${DPI_SO} (${words $^} objects)"
	${NCSIM_GXX} -shared -static-libstdc++ -static-libgcc -o $@ $^

${NCSIM_FILELIST}: ${FILELIST}
	@echo "[equiv-ncsim] generating filtered filelist: ${NCSIM_FILELIST}"
	@awk '{ \
		line = $$0; \
		sub(/^[ \t]+/, "", line); \
		sub(/[ \t]+$$/, "", line); \
		if (line == "" || line ~ /^#/ || line ~ /^\/\//) next; \
		if (line ~ /^\+incdir\+/) { print line; next; } \
		if (line ~ /^-v[ \t]/) { print line; next; } \
		if (line ~ /\.vh([ \t]|$$)/) next; \
		print line; \
	}' ${FILELIST} > ${NCSIM_FILELIST}

EQUIV_TB         := equiv_tb.sv

# --- VCS ---
equiv-vcs: simv

simv: ${EQUIV_TB} ${DPI_SV} ${VCS_CPP_SRCS}
	@echo "=== uart_top RTL vs CModel Lock-step (VCS compile) ==="
	export PROJECT=${PROJECT} && \
	${VCS} -full64 -sverilog -timescale=1ns/10ps \
		+v2k -notice -q \
		-f ${FILELIST} \
		-y ${STUBS_DIR} +libext+.v \
		${DPI_SV} ${EQUIV_TB} \
		${VCS_CPP_SRCS} \
		-CFLAGS "${VCS_CFLAGS}" \
		-top equiv_tb \
		${VERDI_FLAGS} \
		-kdb -lca -debug_access+all \
		-o simv

run-equiv-vcs: simv
	./simv +fsdbfile+equiv_uart_top.fsdb | tee equiv-vcs.log
	@echo "=== VCS Verification Complete ==="
	@awk '/PASS|FAIL|MISMATCH|Results/ {print}' equiv-vcs.log || true

# --- Waveform viewer ---
wave: equiv_uart_top.fsdb
	${VERDI_PRELOAD} ${VERDI} -ssf equiv_uart_top.fsdb &

equiv_uart_top.fsdb: simv
	./simv +fsdbfile+equiv_uart_top.fsdb | tee equiv-vcs.log

# --- NCSim ---
equiv-ncsim: ${DPI_SO} ${NCSIM_FILELIST}
	@echo "=== uart_top RTL vs CModel Lock-step (ncsim) ==="
	export PROJECT=${PROJECT} && \
	${IRUN} -sv -64bit \
		-timescale 1ns/10ps \
		-f ${NCSIM_FILELIST} \
		-y ${STUBS_DIR} +libext+.v \
		${DPI_SV} ${EQUIV_TB} \
		-sv_lib ${DPI_SO} \
		-top equiv_tb \
		-access +rwc \
		-l equiv.log
	@echo "=== Verification Complete ==="
	@awk '/PASS|FAIL|MISMATCH|Results/ {print}' equiv.log || true

clean:
	rm -rf simv simv.daidir csrc *.fsdb *.key ucli.key AN.DB
	rm -rf INCA_libs ${OBJ_DIR} *.log ${NCSIM_FILELIST}
