###############
# PATHS       #
###############

# fc6161 RTL (SSOT)
PROJECT       ?= /user/wonseok/fc6161-trunk-rom

# HIRCT tools
HIRCT_HOME    ?= ${realpath ${dir ${lastword ${MAKEFILE_LIST}}}/../../../../hirct}
HIRCT_GEN     ?= ${HIRCT_HOME}/build/bin/hirct-gen
HIRCT_VERIFY  ?= ${HIRCT_HOME}/build/bin/hirct-verify

# CIRCT tools
CIRCT_HOME    ?= /user/wonseok/circt
CIRCT_BIN     ?= ${CIRCT_HOME}/build/bin

# EDA tools
VCS_HOME      ?= /tools/synopsys/vcs/V-2023.12-SP2-7
IUS_HOME      ?= /tools/cadence/INCISIVE151
VERDI_HOME    ?= /tools/synopsys/verdi/V-2023.12-SP2-7

# Host C++ compiler — bypasses Incisive151 bundled GCC 4.4
HOST_GXX      ?= /usr/bin/g++

###############
# LICENSE     #
###############

SNPSLMD_LICENSE_FILE ?= 27020@fdn21:27000@fdn99:27020@fdn03:27030@fdn02:27020@fdn05

###############
# EXPORTS     #
###############

export PROJECT
export VCS_HOME
export VERDI_HOME
export SNPSLMD_LICENSE_FILE
export PATH   := ${CIRCT_BIN}:${HIRCT_HOME}/build/bin:${VCS_HOME}/bin:${IUS_HOME}/tools/bin:${PATH}
