#include "../parser.h"
#include "../try.h"
#include "greatest/greatest.h"

static int astlist_equal (ASTList expd, ASTList got) {
    TRYBOOL(expd.length == got.length);
    for (size_t i = 0; i < expd.length; ++i) {
        TRYBOOL(expd.elems[i].type == got.elems[i].type);
        if (expd.elems[i].type == ASTLIST_IDENT) { // only need to check expd because got type is same
            TRYBOOL(strcmp(expd.elems[i].data.ident, got.elems[i].data.ident) == 0);
        } else if (expd.elems[i].type == ASTLIST_STRING) {
            TRYBOOL(expd.elems[i].data.string.length == got.elems[i].data.string.length);
            for (size_t j = 0; j < expd.elems[i].data.string.length; ++j) {
                switch (expd.elems[i].data.string.elems[j].type) {
                    case STRING_IDENT:
                        TRYBOOL(strcmp(
                            expd.elems[i].data.string.elems[j].data.ident,
                            expd.elems[i].data.string.elems[j].data.ident) == 0);
                        break;
                    case STRING_INTERPOL_STRING: {
                        InterpolString expd_interpol = expd.elems[i].data.string.elems[j].data.interpol_string;
                        InterpolString got_interpol = got.elems[i].data.string.elems[j].data.interpol_string;

                        TRYBOOL(expd_interpol.length == got_interpol.length);
                        TRYBOOL(expd_interpol.backticks == got_interpol.backticks);
                        for (size_t k = 0; k < expd_interpol.length; ++k) {
                            TRYBOOL(expd_interpol.elems[k].type == got_interpol.elems[k].type);
                            TRYBOOL(strcmp(expd_interpol.elems[k].data, got_interpol.elems[k].data) == 0);
                        }
                        break;
                    }
                }
            }
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
    TRYBOOL(strcmp(expd->name, got->name) == 0);
    TRYBOOL(astlist_equal(expd->updates, got->updates));

    return true;
}

static int astlist_printf (ASTList t) {
    putchar('[');

    for (size_t i = 0; i < t.length; ++i) {
        ASTListElem elem = t.elems[i];
        switch (elem.type) {
            case ASTLIST_IDENT:
                TRYBOOL(printf("%s", elem.data.ident) >= 0);
                break;
            case ASTLIST_STRING:
                for (size_t i = 0; i < elem.data.string.length; ++i) {
                    switch (elem.data.string.elems[i].type) {
                        case STRING_IDENT:
                            TRYBOOL(printf("%s", elem.data.string.elems[i].data.ident) >= 0);
                            break;
                        case STRING_INTERPOL_STRING:
                            if (i != 0) {
                                TRYBOOL(printf(" + ") >= 0);
                            }

                            InterpolString interpol_string = elem.data.string.elems[i].data.interpol_string;
                            for (size_t j = 0; j < interpol_string.backticks; ++j) {
                                TRYBOOL(putchar('`') >= 0);
                            }
                            TRYBOOL(putchar('\'') >= 0);

                            for (size_t j = 0; j < interpol_string.length; ++j) {
                                switch (interpol_string.elems[j].type) {
                                    case INTERPOL_IDENT:
                                        TRYBOOL(printf("$(%s)", interpol_string.elems[j].data) >= 0);
                                        break;
                                    case INTERPOL_STRING:
                                        TRYBOOL(printf("%s", interpol_string.elems[j].data) >= 0);
                                        break;
                                }
                            }

                            TRYBOOL(putchar('\'') >= 0);
                            for (size_t j = 0; j < interpol_string.backticks; ++j) {
                                TRYBOOL(putchar('`') >= 0);
                            }
                            break;
                    }
                }
                break;
            default:
                return -1; // i think this is how it works
        }
        // we don't want a trailing comma
        if (i < t.length - 1) {
            printf(", ");
        }
    }

    putchar(']');

    return 0;
}

static int astaction_printf_cb (const void *t_v, void *udata) {
    (void) udata;

    const ASTAction *t = (const ASTAction *) t_v;

    TRYBOOL(astlist_printf(t->reqs) >= 0);
    TRYBOOL(printf(" > %s ", t->name) >= 0);
    TRYBOOL(astlist_printf(t->commands) >= 0);
    TRYBOOL(fputs(" > ", stdout) >= 0);
    TRYBOOL(astlist_printf(t->updates) >= 0);

    return 0;
}


static greatest_type_info astaction_type_info = { .equal = astaction_equal_cb, .print = astaction_printf_cb };

TEST action_test (void) {
    Prog prog = (Prog) { .filename = "test", .text = "['foo' + 'bar'] > bar [] > [foo, bar, 'bar$(foo)bar'];" };

    Token *action_toks;
    size_t action_toks_len = 0;
    // shouldn't really fail because lexer tests should run first
    ASSERTm("lex should succeed on action", lex(prog, &action_toks, &action_toks_len));

    ASTAction *actions;
    size_t actions_len = 0;
    ASSERTm("parse should succeed on action", parse(prog, action_toks, action_toks_len, &actions, &actions_len));

    // make some kind of ast generator, this is not very poggers
    ASTAction correct_action = (ASTAction) {
       .reqs = (ASTList) {
           .length = 1,
           .elems = &(ASTListElem) {
               .type = ASTLIST_STRING,
               .data.string = (ASTConcatString) {
                   .length = 2,
                   .elems = (PossString[2]) {
                       (PossString) {
                           .type = STRING_INTERPOL_STRING,
                           .data.interpol_string = (InterpolString) {
                               .backticks = 0,
                               .length = 1,
                               .elems = &(InterpolStringElem) { .type = INTERPOL_STRING, .data = "foo" }
                            }
                       },
                       (PossString) {
                           .type = STRING_INTERPOL_STRING,
                           .data.interpol_string = (InterpolString) {
                               .backticks = 0,
                               .length = 1,
                               .elems = &(InterpolStringElem) { .type = INTERPOL_STRING, .data = "bar" }
                            }
                       }
                   }
               }
           }
       },
       .name = "bar",
       .commands = (ASTList) {
           .length = 0,
           .elems = NULL,
       },
       .updates = (ASTList) {
           .length = 3,
           .elems = (ASTListElem[3]) {
               (ASTListElem) {
                   .type = ASTLIST_IDENT,
                   .data.ident = "foo"
               },
               (ASTListElem) {
                   .type = ASTLIST_IDENT,
                   .data.ident = "bar"
               },
               (ASTListElem) {
                   .type = ASTLIST_STRING,
                   .data.string = (ASTConcatString) {
                       .length = 1,
                       .elems = &(PossString) {
                           .type = STRING_INTERPOL_STRING,
                           .data.interpol_string = (InterpolString) {
                               .backticks = 0,
                               .length = 3,
                               .elems = (InterpolStringElem[3]) {
                                   (InterpolStringElem) { .type = INTERPOL_STRING, .data = "bar" },
                                   (InterpolStringElem) { .type = INTERPOL_IDENT, .data = "foo" },
                                   (InterpolStringElem) { .type = INTERPOL_STRING, .data = "bar" },
                               }
                           }
                       }
                   }
               }
           }
       }
    };
    ASSERT_EQUAL_Tm("parse should parse action correctly", &correct_action, actions, &astaction_type_info, NULL);
    free_actions(actions, actions_len);
    free_tokens(action_toks, action_toks_len);

    PASS();
}

GREATEST_SUITE(parser_suite) {
    RUN_TEST(action_test);
}
