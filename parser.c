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
    TRYBOOL(take_token_ignore(s, IDENT) || take_token_ignore(s, STRING));

    concat_str->catee = list_new();
    Token concat;
    while (take_token(s, CONCAT, &concat)) {
        Token str;
        TRYBOOL_R(peek(s, &str), str.type = -1); // dirty hack
        list_push(&concat_str->catee, malloc(sizeof(ASTCatee)));
        switch(str.type) {
            case IDENT:
                *(ASTCatee *) concat_str->catee.last->data = (ASTCatee) {
                    .type = CATEE_IDENT,
                    .data.ident = str.data.ident
                };
                break;
            case STRING:
                *(ASTCatee *) concat_str->catee.last->data = (ASTCatee) {
                    .type = CATEE_INTERPOL_STRING,
                    .data.interpol_string = str.data.string 
                };
                break;
            default:
                list_free(concat_str->catee);
                expected_err(s, s->offset, "string or identifier");
                return false;
        }
        ++s->offset;
    }

    return true;
}

static bool parse_list (ParseState *s, ASTList *list) {
    TRYBOOL(take_token_ignore(s, BRACKET_OPEN));

    // find elements
    Token next_tok; // either close bracket or first element of list
    TRYBOOL_R(peek(s, &next_tok), expected_err(s, s->offset, "list element or close bracket"));

    list->elems = list_new();
    if (next_tok.type == BRACKET_CLOSE) { // no length
        ++s->offset;
        return true;
    }

    Token comma;
    do {
        list_push(&list->elems, malloc(sizeof(ASTConcat)));
        TRYBOOL_R(parse_concat_string(s, list->elems.last->data),
            list_free(list->elems),
            expected_err(s, s->offset, "list element"));

        TRYBOOL_R(take_token(s, COMMA, &comma) || take_token(s, BRACKET_CLOSE, &comma),
            list_free(list->elems),
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

bool parse (Prog prog, const Token *tokens, size_t tokens_len, LList *actions) {
    ParseState state = (ParseState) {
        .prog = prog,
        .tokens = tokens,
        .tokens_len = tokens_len,
        .offset = 0
    };

    // we don't want to exit right away after parse_action fails because we'll miss all the other errors
    bool return_res = true;

    *actions = list_new();
    while (state.offset < state.tokens_len) {
        ASTAction action;
        if (!parse_action(&state, &action)) {
            return_res = false;
            synchronize(&state);
        }
        list_push(actions, &action);
    }

    return return_res;
}

static void free_astlist (ASTList list) {
    list_free(list.elems);
}

// doesn't free some lexer stuff copied from tokens
void free_actions (LList actions) {
    FOREACH(ASTAction, action, actions) {
        free_astlist(action.reqs);
        free_astlist(action.commands);
        free_astlist(action.updates);
    }
    list_free(actions);
}
