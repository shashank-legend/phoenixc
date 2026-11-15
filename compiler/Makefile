CC = gcc
CFLAGS = -Wall -g -Isrc
FLEX = flex
BISON = bison

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = .

SOURCES = $(SRC_DIR)/ast/ast.c \
          $(SRC_DIR)/semantic/semantic.c \
          $(SRC_DIR)/icg/tac.c \
          $(SRC_DIR)/icg/icg.c \
          $(SRC_DIR)/icg/optimize.c \
          $(SRC_DIR)/error/error.c \
          $(SRC_DIR)/main.c

OBJECTS = $(BUILD_DIR)/ast.o \
          $(BUILD_DIR)/semantic.o \
          $(BUILD_DIR)/tac.o \
          $(BUILD_DIR)/icg.o \
          $(BUILD_DIR)/optimize.o \
          $(BUILD_DIR)/error.o \
          $(BUILD_DIR)/main.o \
          $(BUILD_DIR)/lex.yy.o \
          $(BUILD_DIR)/parser.tab.o

TARGET = $(BIN_DIR)/phoenixc

all: $(TARGET)

$(TARGET): $(BUILD_DIR) $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/lex.yy.c: $(SRC_DIR)/lexer/lexer.l
	$(FLEX) -o $(BUILD_DIR)/lex.yy.c $(SRC_DIR)/lexer/lexer.l

$(BUILD_DIR)/parser.tab.c $(BUILD_DIR)/parser.tab.h: $(SRC_DIR)/parser/parser.y
	$(BISON) -d -o $(BUILD_DIR)/parser.tab.c $(SRC_DIR)/parser/parser.y

$(BUILD_DIR)/lex.yy.o: $(BUILD_DIR)/lex.yy.c $(BUILD_DIR)/parser.tab.h
	$(CC) $(CFLAGS) -c $(BUILD_DIR)/lex.yy.c -o $(BUILD_DIR)/lex.yy.o

$(BUILD_DIR)/parser.tab.o: $(BUILD_DIR)/parser.tab.c
	$(CC) $(CFLAGS) -c $(BUILD_DIR)/parser.tab.c -o $(BUILD_DIR)/parser.tab.o

$(BUILD_DIR)/ast.o: $(SRC_DIR)/ast/ast.c $(SRC_DIR)/ast/ast.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/ast/ast.c -o $(BUILD_DIR)/ast.o

$(BUILD_DIR)/semantic.o: $(SRC_DIR)/semantic/semantic.c $(SRC_DIR)/semantic/semantic.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/semantic/semantic.c -o $(BUILD_DIR)/semantic.o

$(BUILD_DIR)/tac.o: $(SRC_DIR)/icg/tac.c $(SRC_DIR)/icg/tac.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/icg/tac.c -o $(BUILD_DIR)/tac.o

$(BUILD_DIR)/icg.o: $(SRC_DIR)/icg/icg.c $(SRC_DIR)/icg/icg.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/icg/icg.c -o $(BUILD_DIR)/icg.o

$(BUILD_DIR)/optimize.o: $(SRC_DIR)/icg/optimize.c $(SRC_DIR)/icg/optimize.h $(SRC_DIR)/icg/tac.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/icg/optimize.c -o $(BUILD_DIR)/optimize.o

$(BUILD_DIR)/error.o: $(SRC_DIR)/error/error.c $(SRC_DIR)/error/error.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/error/error.c -o $(BUILD_DIR)/error.o

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(BUILD_DIR)/parser.tab.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/main.c -o $(BUILD_DIR)/main.o

test: $(TARGET)
	cd .. && ./run_all_tests.sh

clean:
	rm -rf $(BUILD_DIR) $(TARGET) minic

.PHONY: all clean test

