#include "../lexer.h"
#include "type_infos.h"
#include "greatest/greatest.h"

static greatest_type_info token_type_info = { .equal = token_equal_cb, .print = token_printf_cb };

TEST symbol_test (void) {
    Token answers[6] = {
        { .type = ARROW        , .offset = 0, .length = 1 },
        { .type = BRACKET_OPEN , .offset = 1, .length = 1 },
        { .type = BRACKET_CLOSE, .offset = 2, .length = 1 },
        { .type = COMMA        , .offset = 3, .length = 1 },
        { .type = CONCAT       , .offset = 4, .length = 1 },
        { .type = SEMICOLON    , .offset = 5, .length = 1 }
    };
    Prog prog = (Prog) { .filename = "test", .text = ">[],+;" };
    // lex returns an array of tokens, but we only use the first element
    Token *sym;
    size_t len = 0;
    ASSERTm("lex should succeed on symbols", lex(prog, &sym, &len));
    ASSERT_EQm("lex should lex the right number of symbols", 6, len);

    char *message = malloc(sizeof("lex should lex x correctly"));
    char characters[6] = { '>', '[', ']', ',', '+', ';' };
    for (int i = 0; i < 6; ++i) {
        sprintf(message, "lex should lex %c correctly", characters[i]);
        ASSERT_EQUAL_Tm(message, answers + i, sym + i, &token_type_info, NULL);
    }
    free(message);

    free_tokens(sym, len);
    PASS();
}

TEST ident_test (void) {
    Prog prog = (Prog) { .filename = "test", .text = "a-b_0" };
    Token *ident;
    size_t len = 0;
    ASSERTm("lex should succeed on identifier", lex(prog, &ident, &len));

    Token correct_ident = (Token) {
        .type = IDENT,
        .data.ident = str_to_slice_raw("a-b_0"),
        .offset = 0,
        .length = strlen("a-b_0")
    };
    ASSERT_EQUAL_Tm("lex should lex identifier correctly", &correct_ident, ident, &token_type_info, NULL);

    free_tokens(ident, len);
    PASS();
}

TEST string_test (void) {
    Prog prog = (Prog) { .filename = "test", .text = "``'bar 'bar`'$(bar) bar'``" };
    Token* str;
    size_t len = 0;
    ASSERTm("lex should succeed on string", lex(prog, &str, &len));

    Token correct_str = (Token) {
        .type = STRING,
        .data.string = (InterpolString) {
            .backticks = 2,
            .parts = node_to_list(&(LLNode) {
                .data = &(InterpolPart) {
                    .type = INTERPOL_STRING,
                    .data = str_to_slice_raw("bar 'bar`'")
                },
                .next = &(LLNode) {
                    .data = &(InterpolPart) {
                        .type = INTERPOL_IDENT,
                        .data = str_to_slice_raw("bar")
                    },
                    .next = &(LLNode) {
                        .data = &(InterpolPart) {
                            .type = INTERPOL_STRING,
                            .data = str_to_slice_raw(" bar")
                        },
                        .next = NULL
                    }
                }
            })
        },
        .offset = 0,
        .length = strlen("``'bar 'bar`'$(bar) bar'``")
    };
    ASSERT_EQUAL_Tm("lex should lex string correctly", &correct_str, str, &token_type_info, NULL);

    free_tokens(str, len);
    PASS();
}

GREATEST_SUITE(lexer_suite) {
    RUN_TEST(symbol_test);
    RUN_TEST(ident_test);
    RUN_TEST(string_test);
}
