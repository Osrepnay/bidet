#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fmt_error.h"
#include "parser.h"
#include "try.h"

// lots of these are just token versions of their lexer equivalents

// name is the same as in lexer_test but this prints it for humans
static char *type_to_string (TokenType type) {
    switch (type) {
        case ARROW: return "arrow";
        case BRACKET_CLOSE: return "close bracket";
        case BRACKET_OPEN: return "open bracket";
        case COMMA: return "comma";
        case CONCAT: return "concat";
        case IDENT: return "identifier";
        case SEMICOLON: return "semicolon";
        case STRING: return "string";
        default: return "unimplemented type"; // dumb!
    }
}

typedef struct {
    Prog prog;
    const Token *tokens;
    size_t tokens_len;
    size_t offset;
} ParseState;

// reports error at s's offset
static void expected_err (const ParseState *s, size_t offset, const char *expected) {
    char *got = offset >= s->tokens_len ? "EOF" : type_to_string(s->tokens[offset].type);
    char *message = malloc(sizeof("expected , got \n") + strlen(expected) + strlen(got));
    sprintf(message, "expected %s, got %s\n", expected, got);
    char *err = fmt_err(s->prog, s->tokens[offset].offset, message);
    fputs(err, stderr);
    free(err);
}

static bool peek (ParseState *s, Token *tok) {
    TRYBOOL(s->offset != s->tokens_len);
    *tok = s->tokens[s->offset];
    return true;
}

static bool next (ParseState *s, Token *tok) {
    TRYBOOL(peek(s, tok));
    ++s->offset;
    return true;
}

static bool take_token (ParseState *s, TokenType type, Token *tok) {
    TRYBOOL(next(s, tok));
    if (tok->type != type) {
        --s->offset;
        return false;
    } else {
        return true;
    }
}

// identical to take_token except it doesn't return the result
static bool take_token_ignore (ParseState *s, TokenType type) {
    Token tok;
    TRYBOOL(next(s, &tok));
    if (tok.type != type) {
        --s->offset;
        return false;
    } else {
        return true;
    }
}

// nexts until semicolon
static void synchronize (ParseState *s) {
    Token tok;
    while (next(s, &tok) && tok.type != SEMICOLON);
}

// parses concatenated strings concatenated with the concatenation operator, + (concatenation operator)
static bool parse_concat_string (ParseState *s, ASTConcat *concat_str) {
    // first check first element
    Token first;
    TRYBOOL(peek(s, &first));
    TRYBOOL(first.type == IDENT || first.type == STRING);

    concat_str->catee = list_astcatee_new();
    Token concat;
    do {
        Token str;
        TRYBOOL_R(next(s, &str),
            list_astcatee_free(concat_str->catee),
            expected_err(s, s->offset, "string or identifier"));
        switch(str.type) {
            case IDENT:
                list_astcatee_push(&concat_str->catee, (ASTCatee) {
                    .type = CATEE_IDENT,
                    .data.ident = str.data.ident
                });
                break;
            case STRING:
                list_astcatee_push(&concat_str->catee, (ASTCatee) {
                    .type = CATEE_INTERPOL_STRING,
                    .data.interpol_string = str.data.string 
                });
                break;
            default:
                list_astcatee_free(concat_str->catee);
                expected_err(s, s->offset - 1, "string or identifier");
                return false;
        }
    } while (take_token(s, CONCAT, &concat));

    return true;
}

static bool parse_list (ParseState *s, ASTList *list) {
    TRYBOOL(take_token_ignore(s, BRACKET_OPEN));

    // find elements
    Token next_tok; // either close bracket or first element of list
    TRYBOOL_R(peek(s, &next_tok),
        expected_err(s, s->offset, "list element or close bracket"));

    list->elems = list_astconcat_new();
    if (next_tok.type == BRACKET_CLOSE) { // no length
        ++s->offset;
        return true;
    }

    Token comma;
    do {
        ASTConcat concat_str;
        TRYBOOL_R(parse_concat_string(s, &concat_str),
            list_astconcat_free(list->elems),
            expected_err(s, s->offset, "list element"));
        list_astconcat_push(&list->elems, concat_str);

        TRYBOOL_R(take_token(s, COMMA, &comma) || take_token(s, BRACKET_CLOSE, &comma),
            list_astconcat_new(list->elems),
            expected_err(s, s->offset, "comma or close bracket"));
    } while (comma.type != BRACKET_CLOSE);

    return true;
}

static bool parse_action (ParseState *s, ASTAction *action) {
    TRYBOOL(parse_list(s, &action->reqs));
    TRYBOOL_R(take_token_ignore(s, ARROW), expected_err(s, s->offset, "arrow"));

    Token name_tok;
    TRYBOOL_R(take_token(s, IDENT, &name_tok), expected_err(s, s->offset, "identifier"));
    action->name = name_tok.data.ident;
    TRYBOOL(parse_list(s, &action->commands));

    TRYBOOL(take_token_ignore(s, ARROW));
    TRYBOOL(parse_list(s, &action->updates));

    TRYBOOL_R(take_token_ignore(s, SEMICOLON), expected_err(s, s->offset, "semicolon"));

    return true;
}

bool parse (Prog prog, const Token *tokens, size_t tokens_len, LLAction *actions) {
    ParseState state = (ParseState) {
        .prog = prog,
        .tokens = tokens,
        .tokens_len = tokens_len,
        .offset = 0
    };

    // we don't want to exit right away after parse_action fails because we'll miss all the other errors
    bool return_res = true;

    *actions = list_action_new();
    while (state.offset < state.tokens_len) {
        ASTAction action;
        if (!parse_action(&state, &action)) {
            return_res = false;
            synchronize(&state);
        }
        list_action_push(actions, action);
    }

    return return_res;
}

static void free_astlist (ASTList list) {
    list_astconcat_free(list.elems);
}

// doesn't free some lexer stuff copied from tokens
void free_actions (LLAction actions) {
    FOREACH (LLActionNode, action, actions) {
        free_astlist(action->data.reqs);
        free_astlist(action->data.commands);
        free_astlist(action->data.updates);
    }
    list_action_free(actions);
}
