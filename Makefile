CC := gcc
CFLAGS := -I./include -Wall -Wextra -Wswitch-enum 
SFLAGS := -fsanitize=address,undefined
SRC := $(wildcard ./src/*.c) 
HEADERS := $(wildcard ./include/*.h)
TARGET := ./build/sparkle

debug: $(SRC) $(HEADERS)
	@mkdir -p ./build/
	$(CC) $(CFLAGS) $(SRC) $(SFLAGS) -g -o $(TARGET)

build: $(SRC) $(HEADERS)
	@mkdir -p ./build/
	$(CC) $(CFLAGS) $(SRC) -O3 -DNDEBUG -o $(TARGET)

run: $(TARGET)
	$(TARGET)

clean:
	rm -rf ./build *.plist

.PHONY: build debug run clean
