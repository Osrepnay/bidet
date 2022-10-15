CFLAGS = -Wall -Wextra -Wpedantic -std=c99
BD = build
OBJS = $(BD)/list.o $(BD)/slice.o $(BD)/fmt_error.o $(BD)/lexer.o $(BD)/parser.o

.PHONY: run_tests
run_tests: build_tests
	$(BD)/tests

.PHONY: build_tests
build_tests: $(BD)/tests.o
	cc $(CFLAGS) -g -fsanitize=address $(OBJS) $(BD)/type_infos.o $(BD)/lexer_test.o $(BD)/parser_test.o $(BD)/tests.o -o build/tests

$(BD)/type_infos.o: test/type_infos.c test/type_infos.h $(BD)/parser.o try.h
	cc $(CFLAGS) -c test/type_infos.c -o $(BD)/type_infos.o
$(BD)/lexer_test.o: test/lexer_test.c $(BD)/lexer.o $(BD)/type_infos.o test/greatest/greatest.h
	cc $(CFLAGS) -c test/lexer_test.c -o $(BD)/lexer_test.o
$(BD)/parser_test.o: test/parser_test.c $(BD)/parser.o $(BD)/type_infos.o test/greatest/greatest.h
	cc $(CFLAGS) -c test/parser_test.c -o $(BD)/parser_test.o
$(BD)/tests.o: test/tests.c test/tests.h $(BD)/lexer_test.o $(BD)/parser_test.o
	cc $(CFLAGS) -c test/tests.c -o $(BD)/tests.o

$(BD)/list.o: list.c list.h
	cc $(CFLAGS) -c list.c -o $(BD)/list.o
$(BD)/slice.o: slice.c slice.h $(BD)/list.o prog.h
	cc $(CFLAGS) -c slice.c -o $(BD)/slice.o
$(BD)/fmt_error.o: fmt_error.c fmt_error.h prog.h
	cc $(CFLAGS) -c fmt_error.c -o $(BD)/fmt_error.o
$(BD)/lexer.o: lexer.c $(BD)/fmt_error.o lexer.h try.h $(BD)/list.o prog.h $(BD)/slice.o
	cc $(CFLAGS) -c lexer.c -o $(BD)/lexer.o
$(BD)/parser.o: parser.c $(BD)/fmt_error.o parser.h try.h ast.h $(BD)/lexer.o prog.h
	cc $(CFLAGS) -c parser.c -o $(BD)/parser.o

.PHONY: clean
clean:
	-rm $(BD)/*
