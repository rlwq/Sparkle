CC := gcc
CFLAGS := -std=c11 -I./include -Wall -Wextra -Wswitch-enum -Wpedantic

DEBUG_FLAGS := -fsanitize=undefined,address -O1 -g
BUILD_FLAGS := -O3 -DNDEBUG

SRC := $(wildcard ./src/*.c) 
HEADERS := $(wildcard ./include/*.h)
TARGET := ./build/sparkle

TESTER := ./tests/tester.py
TESTS_FOLDER := ./tests/cases/
PYTHON := python3

debug: $(SRC) $(HEADERS)
	@mkdir -p ./build/
	$(CC) $(CFLAGS) $(SRC) $(DEBUG_FLAGS) -o $(TARGET)

build: $(SRC) $(HEADERS)
	@mkdir -p ./build/
	$(CC) $(CFLAGS) $(SRC) $(BUILD_FLAGS) -o $(TARGET)

run:
	@$(TARGET)

clean:
	@rm -rf ./build *.plist

test:
	@python $(TESTER) $(TARGET) $(TESTS_FOLDER)

rewrite_tests:
	@$(PYTHON) $(TESTER) $(TARGET) $(TESTS_FOLDER) --rewrite


.PHONY: build debug run clean test rewrite_tests
