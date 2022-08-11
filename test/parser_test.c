#include "../parser.h"
#include "../try.h"
#include "greatest/greatest.h"

static int astlist_equal (ASTList expd, ASTList got) {
    TRYBOOL(expd.length == got.length);
    for (size_t i = 0; i < expd.length; ++i) {
        TRYBOOL(expd.elems[i].type == got.elems[i].type);
        if (expd.elems[i].type == IDENT) { // only need to check expd because got type is same
            TRYBOOL(strcmp(expd.elems[i].data.ident.name, got.elems[i].data.ident.name) == 0);
        } else if (expd.elems[i].type == STRING) {
            TRYBOOL(expd.elems[i].data.string.backticks == got.elems[i].data.string.backticks);
            TRYBOOL(strcmp(expd.elems[i].data.string.text, got.elems[i].data.string.text) == 0);
        }
    }
    return true;
}

static int astaction_equal_cb (const void *expd_v, const void *got_v, void *udata) {
    (void) udata;

    const ASTAction *expd = (const ASTAction *) expd_v;
    const ASTAction *got = (const ASTAction *) got_v;

    TRYBOOL(astlist_equal(expd->reqs, got->reqs));
    TRYBOOL(astlist_equal(expd->commands, got->commands));
    TRYBOOL(strcmp(expd->name.name, got->name.name) == 0);
    TRYBOOL(astlist_equal(expd->updates, got->updates));

    return true;
}

static int astlist_printf (ASTList t) {
    putchar('[');

    for (size_t i = 0; i < t.length; ++i) {
        Token elem = t.elems[i];
        switch (elem.type) {
            case IDENT:
                TRYBOOL(fputs(elem.data.ident.name, stdout) >= 0);
                break;
            case STRING: {
                char *backticks = malloc(elem.data.string.backticks + 1);
                for (size_t i = 0; i < elem.data.string.backticks; ++i) {
                    backticks[i] = '`';
                }
                backticks[elem.data.string.backticks] = '\0';;

                TRYBOOL(printf("%s'%s'%s", backticks, elem.data.string.text, backticks) >= 0);
                break;
            }
            default:
                return -1; // i think this is how it works
        }
        // we don't want a trailing comma
        if (i < t.length - 1) {
            fputs(", ", stdout);
        }
    }

    putchar(']');

    return 0;
}

static int astaction_printf_cb (const void *t_v, void *udata) {
    (void) udata;

    const ASTAction *t = (const ASTAction *) t_v;

    TRYBOOL(astlist_printf(t->reqs) >= 0);
    TRYBOOL(printf(" > %s ", t->name.name) >= 0);
    TRYBOOL(astlist_printf(t->commands) >= 0);
    TRYBOOL(fputs(" > ", stdout) >= 0);
    TRYBOOL(astlist_printf(t->updates) >= 0);

    return 0;
}


static greatest_type_info astaction_type_info = { .equal = astaction_equal_cb, .print = astaction_printf_cb };

TEST action_test (void) {
    Prog prog = (Prog) { .filename = "test", .text = "['foo'] > bar [] > ['foo', bar];" };

    Token *action_toks;
    size_t action_toks_len = 0;
    // shouldn't really fail because lexer tests should run first
    ASSERTm("lex should succeed on action", lex(prog, &action_toks, &action_toks_len));

    ASTAction *actions;
    size_t actions_len = 0;
    ASSERTm("parse should succeed on action", parse(prog, action_toks, action_toks_len, &actions, &actions_len));

    ASTAction correct_action = (ASTAction) {
       .reqs = (ASTList) {
           .length = 1,
           .elems = &(Token) {
               .type = STRING,
               .data.string = (TokenString) { .text = "foo", .backticks = 0 },
               .offset = 1,
               .length = 5
           },
       } ,
       .name = (TokenIdent) { .name = "bar" },
       .commands = (ASTList) {
           .length = 0,
           .elems = NULL,
       },
       .updates = (ASTList) {
           .length = 2,
           .elems = (Token[2]) {
               (Token) {
                   .type = STRING,
                   .data.string = (TokenString) { .text = "foo", .backticks = 0 },
                   .offset = 20,
                   .length = 5
               },
               (Token) {
                   .type = IDENT,
                   .data.ident = (TokenIdent) { .name = "bar" },
                   .offset = 27,
                   .length = 3
               }
           }
       }
    };
    ASSERT_EQUAL_Tm("parse should parse action correctly", &correct_action, actions, &astaction_type_info, NULL);

    PASS();
}

GREATEST_SUITE(parser_suite) {
    RUN_TEST(action_test);
}
