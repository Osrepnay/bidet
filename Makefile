CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -O2
BUILD_DIR = build
OBJS = $(BUILD_DIR)/fmt_error.o $(BUILD_DIR)/lexer.o $(BUILD_DIR)/parser.o

.PHONY: test
test: $(OBJS)
	cc $(CFLAGS) -c test/lexer_test.c -o $(BUILD_DIR)/lexer_test.o
	cc $(OBJS) $(BUILD_DIR)/lexer_test.o -o $(BUILD_DIR)/lexer_test
	$(BUILD_DIR)/lexer_test

$(BUILD_DIR)/fmt_error.o: fmt_error.c fmt_error.h prog.h
	cc $(CFLAGS) -c fmt_error.c -o $(BUILD_DIR)/fmt_error.o
$(BUILD_DIR)/lexer.o: lexer.c lexer.h fmt_error.h prog.h try.h
	cc $(CFLAGS) -c lexer.c -o $(BUILD_DIR)/lexer.o
$(BUILD_DIR)/parser.o: parser.c parser.h ast.h lexer.h prog.h try.h
	cc $(CFLAGS) -c parser.c -o $(BUILD_DIR)/parser.o

.PHONY: clean
clean:
	-rm $(BUILD_DIR)/*
