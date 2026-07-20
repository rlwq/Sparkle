CC := gcc
CFLAGS := -std=c11 -I./include -Wall -Wextra -Wswitch-enum -Wpedantic -Werror

DEBUG_FLAGS := -fsanitize=undefined,address -O1 -g
BUILD_FLAGS := -O3 -flto -funroll-loops -DNDEBUG

SRC := $(wildcard ./src/*.c) $(wildcard ./src/**/*.c)
HEADERS := $(wildcard ./include/*.h)
# libm is requested explicitly: it is only ever inlined away at higher
# optimisation levels, and the debug build does not get that.
LDLIBS := -lm

TARGET := ./build/sparkle

TESTER := ./tests/tester.py
TESTS_FOLDER := ./tests/
PYTHON := python3

debug: $(SRC) $(HEADERS)
	@mkdir -p ./build/
	$(CC) $(CFLAGS) $(SRC) $(DEBUG_FLAGS) -o $(TARGET) $(LDLIBS)

build: $(SRC) $(HEADERS)
	@mkdir -p ./build/
	$(CC) $(CFLAGS) $(SRC) $(BUILD_FLAGS) -o $(TARGET) $(LDLIBS)

clean:
	@rm -rf ./build *.plist

test:
	@$(PYTHON) $(TESTER) $(TARGET) $(TESTS_FOLDER)

rewrite_tests:
	@$(PYTHON) $(TESTER) $(TARGET) $(TESTS_FOLDER) --rewrite

format:
	clang-format -i $(SRC) $(HEADERS)

help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  build          Optimized build (-O3, LTO)"
	@echo "  debug          Debug build (ASan, UBSan, -O1)"
	@echo "  test           Run the test suite"
	@echo "  rewrite_tests  Rewrite expected test output"
	@echo "  format         Format source with clang-format"
	@echo "  clean          Remove build artifacts"
	@echo "  help           Show this message"

.PHONY: build debug clean test rewrite_tests format help

