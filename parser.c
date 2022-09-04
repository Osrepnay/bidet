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
        case IDENT: return "identifier";
        case ARROW: return "arrow";
        case BRACKET_OPEN: return "open bracket";
        case BRACKET_CLOSE: return "close bracket";
        case COMMA: return "comma";
        case CONCAT: return "concat";
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
static void expected_err (ParseState s, size_t offset, const char *expected) {
    char *got = offset >= s.tokens_len ? "EOF" : type_to_string(s.tokens[offset].type);
    char *message = malloc(sizeof("expected , got \n") + strlen(expected) + strlen(got));
    sprintf(message, "expected %s, got %s\n", expected, got);
    char *err = fmt_err(s.prog, s.tokens[offset].offset, message);
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
static bool parse_concat_string (ParseState *s, ASTConcatString *concat_str) {
    ParseState s_save = *s;

    concat_str->length = 0;
    Token concat;
    do {
        TRYBOOL_R(
                take_token_ignore(s, IDENT) || take_token_ignore(s, STRING),
                expected_err(*s, s->offset, "string"));
        ++concat_str->length;
    } while (take_token(s, CONCAT, &concat));

    *s = s_save;

    concat_str->elems = malloc(sizeof(PossString) * concat_str->length);
    for (size_t i = 0; i < concat_str->length; ++i) {
        Token str;
        next(s, &str);
        if (str.type == IDENT) {
            concat_str->elems[i].type = STRING_IDENT;
            concat_str->elems[i].data.ident = str.data.ident;
        } else {
            concat_str->elems[i].type = STRING_INTERPOL_STRING;
            concat_str->elems[i].data.interpol_string = str.data.string;
        }
        ++s->offset;
    }
    --s->offset; // there is no trailing concat so we have to backtrack

    return true;
}

static bool parse_list (ParseState *s, ASTList *list) {
    ParseState s_save = *s;

    TRYBOOL(take_token_ignore(s, BRACKET_OPEN));

    // find elements
    Token next_tok; // either close bracket or first element of list
    TRYBOOL_R(peek(s, &next_tok), expected_err(*s, s->offset, "list element or close bracket"));

    list->length = 0;
    if (next_tok.type == BRACKET_CLOSE) { // no length
        ++s->offset;
        list->elems = malloc(0); // :troll:
        return true;
    }

    Token comma;
    do {
        if (!take_token_ignore(s, IDENT)) {
            // TODO if we're storing it anyway to free then maybe we should just use a list instead...
            // why don't we just use a list everywhere?? benchmark????
            ASTConcatString concat_str;
            TRYBOOL_R(parse_concat_string(s, &concat_str), expected_err(*s, s->offset, "list element"));
            free(concat_str.elems);
        }
        ++list->length;

        TRYBOOL_R(
                take_token(s, COMMA, &comma) || take_token(s, BRACKET_CLOSE, &comma),
                expected_err(*s, s->offset, "comma or close bracket"));
    } while (comma.type != BRACKET_CLOSE);

    // start making list
    // first reset to start of list
    *s = s_save;
    ++s->offset;

    list->elems = malloc(sizeof(ASTListElem) * list->length);
    for (size_t i = 0; i < list->length; ++i) {
        ASTListElem elem;
        Token ident;
        if (take_token(s, IDENT, &ident)) {
            elem.type = ASTLIST_IDENT;
            elem.data.ident = ident.data.ident;
        } else {
            elem.type = ASTLIST_STRING;
            ASTConcatString concat_str;
            TRYBOOL_R(parse_concat_string(s, &concat_str), expected_err(*s, s->offset, "list element"));
            elem.data.string = concat_str;
        }
        list->elems[i] = elem;

        ++s->offset; // comma and close bracket
    }

    return true;
}

static bool parse_action (ParseState *s, ASTAction *action) {
    TRYBOOL(parse_list(s, &action->reqs));
    TRYBOOL_R(take_token_ignore(s, ARROW), expected_err(*s, s->offset, "arrow"));

    Token name_tok;
    TRYBOOL_R(take_token(s, IDENT, &name_tok), expected_err(*s, s->offset, "identifier"));
    action->name = name_tok.data.ident;
    TRYBOOL(parse_list(s, &action->commands));

    TRYBOOL(take_token_ignore(s, ARROW));
    TRYBOOL(parse_list(s, &action->updates));

    TRYBOOL_R(take_token_ignore(s, SEMICOLON), expected_err(*s, s->offset, "semicolon"));

    return true;
}

#include "list.h"
GENLIST_TYPE(ASTAction, Action, action)

bool parse (Prog prog, const Token *tokens, size_t tokens_len, ASTAction **actions, size_t *actions_len) {
    ParseState state = (ParseState) {
        .prog = prog,
        .tokens = tokens,
        .tokens_len = tokens_len,
        .offset = 0
    };

    // we don't want to exit right away after parse_action fails because we'll miss all the other errors
    bool return_res = true;

    ListAction actions_list = list_action_new();
    while (state.offset < state.tokens_len) {
        ASTAction action;
        if (!parse_action(&state, &action)) {
            return_res = false;
            synchronize(&state);
        }
        list_action_push(&actions_list, action);
    }
    *actions = actions_list.elems;
    *actions_len = actions_list.len;

    return return_res;
}

static void free_astlist (ASTList list) {
    for (size_t i = 0; i < list.length; ++i) {
        // ignore ident, that should be freed by token freer
        if (list.elems[i].type == ASTLIST_STRING) {
            free(list.elems[i].data.string.elems);
        }
    }
    free(list.elems);
}

// doesn't free some lexer stuff copied from tokens
void free_actions (ASTAction *actions, size_t actions_len) {
    for (size_t i = 0; i < actions_len; ++i) {
        free_astlist(actions[i].reqs);
        free_astlist(actions[i].commands);
        free_astlist(actions[i].updates);
    }
    free(actions);
}
