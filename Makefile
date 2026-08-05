# Makefile for BetterThanCris
#
# Targets:
#   make            - same as `make build`
#   make build      - build the optimised engine (btc<version>.exe)
#   make debug      - build with -O0 -g (btc<version>_debug.exe)
#   make perft      - build the perft test driver (test_perft.exe)
#   make sprt       - build the SPRT match harness (sprt.exe)
#   make sprt-test  - build the harness and run its self test
#   make test       - run perft on a few positions and check the totals
#                     against the canonical node counts
#   make clean      - remove all build artefacts
#
# Binary name is derived from ENGINE_VERSION in src/version.h, so a bump
# there changes the build target without touching the Makefile. e.g.
# ENGINE_VERSION "2.6" produces btc26.exe.

CXX        := g++
CXXFLAGS   := -O2 -Wno-unused-result
DEBUGFLAGS := -O0 -g -Wno-unused-result

# Extract version string from version.h and strip the dot so 2.6 -> 26.
ENGINE_VERSION_STR := $(shell sed -n 's/.*ENGINE_VERSION[[:space:]]*"\([^"]*\)".*/\1/p' src/version.h)
ENGINE_VERSION_TAG := $(subst .,,$(ENGINE_VERSION_STR))

# On MSYS2 / Git Bash on Windows, g++ may fail to create a tempdir if TMP
# inherits an unwritable Windows path. Force a known-good location.
export TMP  ?= /tmp
export TEMP ?= /tmp

SRC_DIR    := src
TESTS_DIR  := tests

# Engine source files (everything in src/*.cpp)
SRCS := \
    $(SRC_DIR)/attacks.cpp \
    $(SRC_DIR)/bitboard.cpp \
    $(SRC_DIR)/eval_constants.cpp \
    $(SRC_DIR)/evaluation.cpp \
    $(SRC_DIR)/init.cpp \
    $(SRC_DIR)/magic.cpp \
    $(SRC_DIR)/main.cpp \
    $(SRC_DIR)/movegen.cpp \
    $(SRC_DIR)/perft.cpp \
    $(SRC_DIR)/position.cpp \
    $(SRC_DIR)/random.cpp \
    $(SRC_DIR)/search.cpp \
    $(SRC_DIR)/see.cpp \
    $(SRC_DIR)/timeman.cpp \
    $(SRC_DIR)/tt.cpp \
    $(SRC_DIR)/uci.cpp \
    $(SRC_DIR)/zobrist.cpp

# Same set minus main.cpp, used by the perft driver which has its own main
SRCS_NO_MAIN := $(filter-out $(SRC_DIR)/main.cpp,$(SRCS))

# Header files, so header edits trigger a rebuild
HDRS := $(wildcard $(SRC_DIR)/*.h)

HARNESS_DIR := harness

# SPRT harness sources. The harness is a standalone tool: it links none of
# src/, on purpose, so its arbiter cannot inherit an engine move generation bug.
HARNESS_SRCS :=     $(HARNESS_DIR)/board.cpp     $(HARNESS_DIR)/book.cpp     $(HARNESS_DIR)/engine.cpp     $(HARNESS_DIR)/game.cpp     $(HARNESS_DIR)/log.cpp     $(HARNESS_DIR)/main.cpp     $(HARNESS_DIR)/match.cpp     $(HARNESS_DIR)/options.cpp     $(HARNESS_DIR)/pgn.cpp     $(HARNESS_DIR)/platform.cpp     $(HARNESS_DIR)/process.cpp     $(HARNESS_DIR)/sprt.cpp

HARNESS_HDRS := $(wildcard $(HARNESS_DIR)/*.h)

# threads come from the system on Windows and from pthreads elsewhere
ifeq ($(OS),Windows_NT)
    HARNESS_LIBS :=
else
    HARNESS_LIBS := -lpthread
endif

ENGINE        := btc$(ENGINE_VERSION_TAG).exe
ENGINE_DEBUG  := btc$(ENGINE_VERSION_TAG)_debug.exe
PERFT         := test_perft.exe
TUNER         := texel.exe
HARNESS       := sprt.exe

.PHONY: all build debug perft test tuner sprt sprt-test clean

all: build

build: $(ENGINE)

$(ENGINE): $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

debug: $(ENGINE_DEBUG)

$(ENGINE_DEBUG): $(SRCS) $(HDRS)
	$(CXX) $(DEBUGFLAGS) -o $@ $(SRCS)

perft: $(PERFT)

$(PERFT): $(TESTS_DIR)/test_perft.cpp $(SRCS_NO_MAIN) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(TESTS_DIR)/test_perft.cpp $(SRCS_NO_MAIN)

# SPRT match harness: plays engine against engine and applies the sequential
# test to the results. Independent of the engine build.
sprt: $(HARNESS)

$(HARNESS): $(HARNESS_SRCS) $(HARNESS_HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(HARNESS_SRCS) $(HARNESS_LIBS)

# Arbiter perft, notation round trips, statistics and book parsing.
sprt-test: $(HARNESS)
	./$(HARNESS) -selftest

# Texel tuning driver: links the engine eval (no main.cpp) with its own main.
tuner: $(TUNER)

$(TUNER): tuner/texel.cpp $(SRCS_NO_MAIN) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ tuner/texel.cpp $(SRCS_NO_MAIN)

# Run perft on a handful of well-known positions at depth 4 and check
# the totals against the canonical node counts. Validates move
# generation, make/unmake, and zobrist hashing.
test: $(PERFT)
	@echo "=== Running perft (depth 4) ==="
	./$(PERFT) 4 > perft_out.txt 2>&1
	@echo "=== Checking canonical node counts ==="
	@grep -a "Nodes: 197281"   perft_out.txt > /dev/null && echo "  startpos        ok" || (echo "  startpos        FAIL"; exit 1)
	@grep -a "Nodes: 4085603"  perft_out.txt > /dev/null && echo "  tricky_position ok" || (echo "  tricky_position FAIL"; exit 1)
	@grep -a "Nodes: 1032012"  perft_out.txt > /dev/null && echo "  killer_position ok" || (echo "  killer_position FAIL"; exit 1)
	@grep -a "Nodes: 1679340"  perft_out.txt > /dev/null && echo "  cmk_position    ok" || (echo "  cmk_position    FAIL"; exit 1)
	@echo "PERFT TEST: PASS"
	@rm -f perft_out.txt

clean:
	-rm -f $(ENGINE) $(ENGINE_DEBUG) $(PERFT) $(HARNESS)
	-rm -f engine.exe engine_debug.exe
	-rm -f engine_current.exe engine_no_contempt.exe engine_old_time.exe engine_baseline.exe
	-rm -f engine43.exe engine52.exe
	-rm -f perft_out.txt
