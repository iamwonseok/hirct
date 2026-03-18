# cxxrtl-poc/Makefile — GenModel vs CXXRTL 비교 테스트 자동화
#
# 사전 요건:
#   - uart_cxxrtl.cc / uart_cxxrtl.h : yowasp-yosys로 생성 (README 참조)
#   - ../output/uart_top/           : hirct-gen으로 생성 (make -C .. generate-uart)
#
# 사용법:
#   make compare          — compare_test 빌드
#   make test-compare     — 5시드 비교 (기준: PRDATA+INTR 0 mismatch)
#   make test-all         — basic_test + extended_test + compare_test 전부

CXXRTL_INC ?= $(shell python3 -c 'import yowasp_yosys, pathlib; \
    print(pathlib.Path(yowasp_yosys.__file__).parent / "share/include/backends/cxxrtl/runtime")')

GM_BASE     ?= ../output
CXXFLAGS    := -std=c++17 -O2
GM_SRCS     := $(wildcard $(GM_BASE)/cmodel/*.cpp) $(wildcard $(GM_BASE)/DW_*/cmodel/*.cpp)
INCS        := -I "$(CXXRTL_INC)" -I . -I $(GM_BASE) -I ..

CYCLES      ?= 1000
SEEDS       := 42 1 7 100 999

.PHONY: all compare uart_txd_probe test-compare clean help

all: compare

compare: compare_test

uart_txd_probe: uart_txd_probe.cc $(GM_SRCS) uart_cxxrtl.cc
	$(CXX) $(CXXFLAGS) $(INCS) -o $@ $< $(GM_SRCS)

compare_test: compare_test.cc $(GM_SRCS) uart_cxxrtl.cc
	$(CXX) $(CXXFLAGS) $(INCS) -o $@ $< $(GM_SRCS)

test-compare: compare_test
	@PASS=0; FAIL=0; SEEDS_FAILED=""; \
	for seed in $(SEEDS); do \
	  echo "--- seed=$$seed ---"; \
	  ./compare_test $(CYCLES) $$seed 2>/dev/null | tail -8; \
	  if ./compare_test $(CYCLES) $$seed 2>/dev/null | grep -q "COMPARE PASS"; then \
	    PASS=$$((PASS+1)); \
	  else \
	    FAIL=$$((FAIL+1)); SEEDS_FAILED="$$SEEDS_FAILED $$seed"; \
	  fi; \
	done; \
	echo ""; \
	echo "=== Summary: $$PASS/$(words $(SEEDS)) PASS ==="; \
	if [ $$FAIL -gt 0 ]; then \
	  echo "FAILED seeds:$$SEEDS_FAILED"; exit 1; \
	fi

clean:
	rm -f compare_test basic_test extended_test test_counter gm_txd_trace cx_txd_trace txd_debug uart_txd_probe

help:
	@echo "make compare      — compare_test 빌드"
	@echo "make uart_txd_probe — uart_txd_probe 빌드"
	@echo "make test-compare — 5시드 비교 테스트 (SEEDS=$(SEEDS))"
	@echo "make clean        — 바이너리 정리"
	@echo ""
	@echo "Override: make test-compare CYCLES=2000 SEEDS='42 1 7'"
