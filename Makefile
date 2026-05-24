# Makefile for BetterThanCris
#
# Targets:
#   make            - same as `make build`
#   make build      - build the optimised engine (engine.exe)
#   make debug      - build with -O0 -g (engine_debug.exe)
#   make perft      - build the perft test driver (test_perft.exe)
#   make test       - run perft on a few positions and check the totals
#                     against the canonical node counts
#   make clean      - remove all build artefacts

CXX        := g++
CXXFLAGS   := -O2 -Wno-unused-result
DEBUGFLAGS := -O0 -g -Wno-unused-result

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
    $(SRC_DIR)/timeman.cpp \
    $(SRC_DIR)/tt.cpp \
    $(SRC_DIR)/uci.cpp \
    $(SRC_DIR)/zobrist.cpp

# Same set minus main.cpp, used by the perft driver which has its own main
SRCS_NO_MAIN := $(filter-out $(SRC_DIR)/main.cpp,$(SRCS))

# Header files, so header edits trigger a rebuild
HDRS := $(wildcard $(SRC_DIR)/*.h)

ENGINE        := engine.exe
ENGINE_DEBUG  := engine_debug.exe
PERFT         := test_perft.exe

.PHONY: all build debug perft test clean

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
	-rm -f $(ENGINE) $(ENGINE_DEBUG) $(PERFT)
	-rm -f perft_out.txt
