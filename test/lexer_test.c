#include "../lexer.h"
#include "greatest/greatest.h"

static int token_equal_cb (const void *expd_v, const void *got_v, void *udata) {
    (void) udata;

    const Token *expd = (const Token *) expd_v;
    const Token *got = (const Token *) got_v;
    if (expd->type != got->type) { return false; }
    if (expd->type == IDENT || expd->type == STRING) {
        if (strcmp(expd->val, got->val) != 0) { return false; }
    }
    return expd->offset == got->offset && expd->length == got->length;
}

static char *type_to_string (TokenType type) {
    switch (type) {
        case IDENT:
            return "IDENT";
        case ARROW:
            return "ARROW";
        case COMMA:
            return "COMMA";
        case S_BRACE_OPEN:
            return "S_BRACE_OPEN";
        case S_BRACE_CLOSE:
            return "S_BRACE_CLOSE";
        case SEMICOLON:
            return "SEMICOLON";
        case STRING:
            return "STRING";
    }
    return "not a type";
}

static int token_printf_cb (const void *t_v, void *udata) {
    (void) udata;

    const Token *t = (const Token *) t_v;
    // has value, print that too
    if (t->type == IDENT || t->type == STRING) {
        return printf(
            "Token { .type = %s, .val = %s, .offset = %zu, .length = %zu }",
            type_to_string(t->type), t->val, t->offset, t->length
        );
    } else {
        return printf(
            "Token { .type = %s, .offset = %zu, .length = %zu }",
            type_to_string(t->type), t->offset, t->length
        );
    }
}

static greatest_type_info token_type_info = (greatest_type_info) { .equal = token_equal_cb, .print = token_printf_cb };

TEST symbol_test (void) {
    Token answers[5] = {
        { .type = ARROW        , .offset = 0, .length = 1 },
        { .type = COMMA        , .offset = 1, .length = 1 },
        { .type = S_BRACE_OPEN , .offset = 2, .length = 1 },
        { .type = S_BRACE_CLOSE, .offset = 3, .length = 1 },
        { .type = SEMICOLON    , .offset = 4, .length = 1 }
    };
    Token *sym;
    size_t len = 0;
    ASSERTm("lex should succeed on symbols", lex("test", ">,[];", &sym, &len));
    ASSERT_EQm("lex should lex the right number of symbols", 5, len);

    char *message = malloc(sizeof("lex should lex x correctly"));
    char characters[5] = { '>', ',', '[', ']', ';' };
    for (int i = 0; i < 5; ++i) {
        sprintf(message, "lex should lex %c correctly", characters[i]);
        ASSERT_EQUAL_Tm(message, answers + i, sym + i, &token_type_info, NULL);
    }

    PASS();
}

TEST ident_test (void) {
    // lex returns an array of tokens, but we only use the first element
    Token *ident;
    size_t len = 0;
    ASSERTm("lex should succeed on identifier", lex("test", "a-b_0", &ident, &len));

    Token correct_ident = (Token) {
        .type = IDENT,
        .val = "a-b_0",
        .offset = 0,
        .length = strlen("a-b_0")
    };
    ASSERT_EQUAL_Tm("lex should lex identifier correctly", &correct_ident, ident, &token_type_info, NULL);

    PASS();
}

TEST string_test (void) {
    Token* str;
    size_t len = 0;
    ASSERTm("lex should succeed on string", lex("test", "`` bar `bar``bar ``", &str, &len));

    Token correct_str = (Token) {
        .type = STRING,
        .val = "bar `bar``bar",
        .offset = 0,
        .length = strlen("`` bar `bar``bar ``")
    };
    ASSERT_EQUAL_Tm("lex should lex string correctly", &correct_str, str, &token_type_info, NULL);

    PASS();
}

GREATEST_MAIN_DEFS();

int main (int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    RUN_TEST(symbol_test);
    RUN_TEST(ident_test);
    RUN_TEST(string_test);

    GREATEST_MAIN_END();
}
