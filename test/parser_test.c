#include "../parser.h"
#include "type_infos.h"
#include "greatest/greatest.h"

static greatest_type_info astaction_type_info = { .equal = astaction_equal_cb, .print = astaction_printf_cb };

TEST action_test (void) {
    Prog prog = (Prog) { .filename = "test", .text = "[foo, bar] > foobar [] > [];" };

    Token *action_toks;
    size_t action_toks_len = 0;
    // shouldn't really fail because lexer tests should run first
    ASSERTm("lex should succeed on action", lex(prog, &action_toks, &action_toks_len));

    LList actions;
    ASSERTm("parse should succeed on action", parse(prog, action_toks, action_toks_len, &actions));
    ASSERT_EQm("parse should only take one action", actions.head->next, NULL);
    ASTAction *action = actions.head->data;

    // make some kind of ast generator, this is not very poggers
    ASTAction correct_action = (ASTAction) {
       .reqs = (ASTList) {
           .elems = node_to_list(&(LLNode) {
                .data = &(ASTConcat) {
                    .catee = node_to_list(&(LLNode) {
                        .data = &(ASTCatee) {
                            .type = CATEE_IDENT,
                            .data.ident = str_to_slice_raw("foo")
                        },
                        .next = &(LLNode) {
                            .data = &(ASTCatee) {
                                .type = CATEE_IDENT,
                                .data.ident = str_to_slice_raw("bar")
                            },
                            .next = NULL
                        }
                    })
                }
           })
       },
       .name = str_to_slice_raw("bar"),
       .commands = (ASTList) {
           .elems = list_new(),
       },
       .updates = (ASTList) {
           .elems = list_new(),
       }
    };
    ASSERT_EQUAL_Tm("parse should parse action correctly", &correct_action, action, &astaction_type_info, NULL);
    free_actions(actions);
    free_tokens(action_toks, action_toks_len);

    PASS();
}

GREATEST_SUITE(parser_suite) {
    RUN_TEST(action_test);
}
