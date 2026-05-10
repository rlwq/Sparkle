CC := gcc
CFLAGS := -std=c11 -I./include -Wall -Wextra -Wswitch-enum -Wpedantic

DEBUG_FLAGS := -fsanitize=undefined,address -O1 -g
BUILD_FLAGS := -O3 -flto -funroll-loops -DNDEBUG

SRC := $(wildcard ./src/*.c) $(wildcard ./src/**/*.c)
HEADERS := $(wildcard ./include/*.h)
TARGET := ./build/sparkle

TESTER := ./tests/tester.py
TESTS_FOLDER := ./tests/
PYTHON := python3

debug: $(SRC) $(HEADERS)
	@mkdir -p ./build/
	$(CC) $(CFLAGS) $(SRC) $(DEBUG_FLAGS) -o $(TARGET)

build: $(SRC) $(HEADERS)
	@mkdir -p ./build/
	$(CC) $(CFLAGS) $(SRC) $(BUILD_FLAGS) -o $(TARGET)

clean:
	@rm -rf ./build *.plist

test:
	@$(PYTHON) $(TESTER) $(TARGET) $(TESTS_FOLDER)

rewrite_tests:
	@$(PYTHON) $(TESTER) $(TARGET) $(TESTS_FOLDER) --rewrite

format:
	clang-format -i $(SRC) $(HEADERS)

.PHONY: build debug clean test rewrite_tests format

