CC := gcc
CFLAGS := -std=c11 -I./include -Wall -Wextra -Wswitch-enum -Wpedantic

DEBUG_FLAGS := -fsanitize=undefined,address -O1 -g
BUILD_FLAGS := -O3 -DNDEBUG

SRC := $(wildcard ./src/*.c) 
HEADERS := $(wildcard ./include/*.h)
TARGET := ./build/sparkle

debug: $(SRC) $(HEADERS)
	@mkdir -p ./build/
	$(CC) $(CFLAGS) $(SRC) $(DEBUG_FLAGS) -o $(TARGET)

build: $(SRC) $(HEADERS)
	@mkdir -p ./build/
	$(CC) $(CFLAGS) $(SRC) $(BUILD_FLAGS) -o $(TARGET)

run: $(TARGET)
	$(TARGET)

clean:
	rm -rf ./build *.plist

.PHONY: build debug run clean
