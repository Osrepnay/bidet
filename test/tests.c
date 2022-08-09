#include "tests.h"
#include "greatest/greatest.h"

GREATEST_MAIN_DEFS();
int main (int argc, char *argv[]) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(lexer_suite);
    RUN_SUITE(parser_suite);

    GREATEST_MAIN_END();
}
