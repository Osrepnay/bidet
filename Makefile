.PHONY: test

CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -O2
OBJS = fmt_error.o lexer.o parser.o

test: $(OBJS) lexer_test.o
	cc $(CFLAGS) test/lexer_test.c -c lexer_test.o
	cc $(OBJS) lexer_test.o -o lexer_test
	./lexer_test

fmt_error.o: fmt_error.c fmt_error.h prog.h
	cc $(CFLAGS) fmt_error.c -c fmt_error.o
lexer.o: lexer.c lexer.h fmt_error.h prog.h try.h
	cc $(CFLAGS) lexer.c -c lexer.o
parser.o: parser.c parser.h ast.h lexer.h prog.h try.h
	cc $(CFLAGS) parser.c -c parser.o
