.PHONY: test

CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -O2
OBJS = fmt_error.o lexer.o parser.o

test: $(OBJS) lexer_test.o
	cc $(CFLAGS) -c test/lexer_test.c -o lexer_test.o
	cc $(OBJS) lexer_test.o -o lexer_test
	./lexer_test

fmt_error.o: fmt_error.c fmt_error.h prog.h
	cc $(CFLAGS) -c fmt_error.c -o fmt_error.o
lexer.o: lexer.c lexer.h fmt_error.h prog.h try.h
	cc $(CFLAGS) -c lexer.c -o lexer.o
parser.o: parser.c parser.h ast.h lexer.h prog.h try.h
	cc $(CFLAGS) -c parser.c -o parser.o
