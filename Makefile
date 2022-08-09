CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -O2
BUILD_DIR = build
OBJS = $(BUILD_DIR)/fmt_error.o $(BUILD_DIR)/lexer.o $(BUILD_DIR)/parser.o

.PHONY: run_tests
run_tests: build_tests
	$(BUILD_DIR)/tests

.PHONY: build_tests
build_tests: $(BUILD_DIR)/lexer_test.o $(BUILD_DIR)/parser_test.o $(BUILD_DIR)/tests.o
	cc $(CFLAGS) $(OBJS) $(BUILD_DIR)/lexer_test.o $(BUILD_DIR)/parser_test.o $(BUILD_DIR)/tests.o -o $(BUILD_DIR)/tests

$(BUILD_DIR)/lexer_test.o: test/lexer_test.c $(BUILD_DIR)/lexer.o try.h test/greatest/greatest.h
	cc $(CFLAGS) -c test/lexer_test.c -o $(BUILD_DIR)/lexer_test.o
$(BUILD_DIR)/parser_test.o: test/parser_test.c $(BUILD_DIR)/parser.o try.h test/greatest/greatest.h
	cc $(CFLAGS) -c test/parser_test.c -o $(BUILD_DIR)/parser_test.o
$(BUILD_DIR)/tests.o: test/tests.c test/tests.h $(BUILD_DIR)/lexer_test.o $(BUILD_DIR)/parser_test.o
	cc $(CFLAGS) -c test/tests.c -o $(BUILD_DIR)/tests.o

$(BUILD_DIR)/fmt_error.o: fmt_error.c fmt_error.h prog.h
	cc $(CFLAGS) -c fmt_error.c -o $(BUILD_DIR)/fmt_error.o
$(BUILD_DIR)/lexer.o: lexer.c $(BUILD_DIR)/fmt_error.o lexer.h try.h prog.h
	cc $(CFLAGS) -c lexer.c -o $(BUILD_DIR)/lexer.o
$(BUILD_DIR)/parser.o: parser.c $(BUILD_DIR)/fmt_error.o parser.h try.h ast.h $(BUILD_DIR)/lexer.o prog.h
	cc $(CFLAGS) -c parser.c -o $(BUILD_DIR)/parser.o

.PHONY: clean
clean:
	-rm $(BUILD_DIR)/*
