CC = gcc
CFLAGS = -Wall -Wextra -I./include
SRC_DIR = src
BIN_DIR = bin
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean test

all: $(BIN_DIR)/hiasc

$(BIN_DIR)/hiasc: $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

test:
	$(CC) $(CFLAGS) tests/test_lexer.c src/lexer.c -o bin/test_lexer
	./bin/test_lexer

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)