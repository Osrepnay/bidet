#include "../lexer.h"
#include "../try.h"
#include "greatest/greatest.h"

static int token_equal_cb (const void *expd_v, const void *got_v, void *udata) {
    (void) udata;

    const Token *expd = (const Token *) expd_v;
    const Token *got = (const Token *) got_v;

    TRYBOOL(expd->type == got->type)
    if (expd->type == IDENT) {
        TRYBOOL(strcmp(expd->data.ident, got->data.ident) == 0);
    } else if (expd->type == STRING) {
        TRYBOOL(expd->data.string.length == got->data.string.length);
        for (size_t i = 0; i < expd->data.string.length; ++i) {
            TRYBOOL(expd->data.string.elems[i].type == got->data.string.elems[i].type);
            TRYBOOL(strcmp(expd->data.string.elems[i].data, got->data.string.elems[i].data) == 0);
        }
        TRYBOOL(expd->data.string.backticks == got->data.string.backticks);
    }
    TRYBOOL(expd->offset == got->offset && expd->length == got->length);

    return true;
}

static char *type_to_string (TokenType type) {
    switch (type) {
        case IDENT:
            return "IDENT";
        case ARROW:
            return "ARROW";
        case BRACKET_OPEN:
            return "BRACKET_OPEN";
        case BRACKET_CLOSE:
            return "BRACKET_CLOSE";
        case COMMA:
            return "COMMA";
        case CONCAT:
            return "CONCAT";
        case SEMICOLON:
            return "SEMICOLON";
        case STRING:
            return "STRING";
        default:
            return "not a type";
    }
}

static int token_printf_cb (const void *t_v, void *udata) {
    (void) udata;

    const Token *t = (const Token *) t_v;

    // has value, print that too
    if (t->type == IDENT) {
        return printf(
            "Token { .type = %s, .data.ident = %s, .offset = %zu, .length = %zu }",
            type_to_string(t->type), t->data.ident, t->offset, t->length);
    } else if (t->type == STRING) {
        char *backticks = malloc(t->data.string.backticks + 1);
        for (size_t i = 0; i < t->data.string.backticks; ++i) {
            backticks[i] = '`';
        }
        backticks[t->data.string.backticks] = '\0';;

        TRYBOOL(printf(
            "Token { "
                ".type = %s, "
                ".data.string = InterpolString { .text = %s'",
            type_to_string(t->type), backticks) >= 0);
        for (size_t i = 0; i < t->data.string.length; ++i) {
            switch (t->data.string.elems[i].type) {
                case INTERPOL_IDENT:
                    TRYBOOL(printf("$(%s)", t->data.string.elems[i].data) >= 0);
                    break;
                case INTERPOL_STRING:
                    TRYBOOL(printf("#[%s]", t->data.string.elems[i].data) >= 0);
                    break;
            }
        }
        return printf(
            "'%s, .backticks = %zu },"
            ".offset = %zu, .length = %zu",
            backticks, t->data.string.backticks, t->offset, t->length);
    } else {
        return printf(
            "Token { .type = %s, .offset = %zu, .length = %zu }",
            type_to_string(t->type), t->offset, t->length
        );
    }
}

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
        .data.ident = "a-b_0",
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
            .elems = (InterpolStringElem[3]) {
                (InterpolStringElem) { .type = INTERPOL_STRING, .data = "bar 'bar`'" },
                (InterpolStringElem) { .type = INTERPOL_IDENT,  .data = "bar" },
                (InterpolStringElem) { .type = INTERPOL_STRING, .data = " bar" }
            },
            .length = 3,
            .backticks = 2
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
