#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fmt_error.h"
#include "parser.h"
#include "try.h"

// lots of these are just token versions of their lexer equivalents

// name is the same as in lexer_test but this prints it for humans
char *type_to_string (TokenType type) {
    switch (type) {
        case IDENT: return "identifier";
        case ARROW: return "arrow";
        case COMMA: return "comma";
        case BRACKET_OPEN: return "open bracket";
        case BRACKET_CLOSE: return "close bracket";
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
void expected_err (ParseState s, size_t offset, const char *expected) {
    char *got = offset >= s.tokens_len ? "EOF" : type_to_string(s.tokens[offset].type);
    char *message = malloc(sizeof("expected , got \n") + strlen(expected) + strlen(got));
    sprintf(message, "expected %s, got %s\n", expected, got);
    char *err = fmt_err(s.prog, s.tokens[offset].offset, message);
    fputs(err, stderr);
    free(err);
}

bool peek_tok (ParseState *s, Token *tok) {
    TRYBOOL(s->offset != s->tokens_len);
    *tok = s->tokens[s->offset];
    return true;
}

bool next_tok (ParseState *s, Token *tok) {
    TRYBOOL(peek_tok(s, tok));
    ++s->offset;
    return true;
}

bool take_token (ParseState *s, TokenType type, Token *tok) {
    TRYBOOL(next_tok(s, tok));
    if (tok->type != type) {
        --s->offset;
        return false;
    } else {
        return true;
    }
}

// identical to take_token except it doesn't return the result
bool take_token_ignore (ParseState *s, TokenType type) {
    Token tok;
    TRYBOOL(next_tok(s, &tok));
    if (tok.type != type) {
        --s->offset;
        return false;
    } else {
        return true;
    }
}

// nexts until semicolon
void synchronize (ParseState *s) {
    Token tok;
    while (next_tok(s, &tok) == true && tok.type != SEMICOLON);
}

bool parse_list (ParseState *s, ASTList *list) {
    ParseState s_save = *s;

    TRYBOOL(take_token_ignore(s, BRACKET_OPEN));

    // find elements
    Token next_token; // either close bracket or first element of list
    TRYBOOL_R(peek_tok(s, &next_token), expected_err(*s, s->offset, "list element or close bracket"));

    if (next_token.type == BRACKET_CLOSE) { // no length
        ++s->offset;
        list->length = 0;
        list->elems = malloc(0); // :troll:
        return true;
    }

    // reuses next_token for list elements
    Token comma = { 0 };
    do {
        TRYBOOL_R(
                take_token_ignore(s, IDENT) || take_token_ignore(s, STRING),
                expected_err(*s, s->offset, "list element"));
        ++list->length;

        TRYBOOL_R(next_tok(s, &next_token), expected_err(*s, s->offset, "list element"));

        TRYBOOL_R(
                take_token_ignore(s, COMMA) || take_token_ignore(s, BRACKET_CLOSE),
                expected_err(*s, s->offset, "comma or close bracket"));
    } while (comma.type != BRACKET_CLOSE);

    // start making list
    // first reset to start of list
    *s = s_save;
    ++s->offset;

    list->elems = malloc(list->length * sizeof(Token));
    for (size_t i = 0; i < list->length; ++i) {
        next_tok(s, list->elems + i);
        ++s->offset; // comma and close bracket
    }

    return true;
}

bool parse_action (ParseState *s, ASTAction *action) {
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

void push_action (ASTAction action, ASTAction **actions, size_t *len, size_t *cap) {
    if (*len >= *cap) {
        *actions = realloc(*actions , sizeof(ASTAction) * (*cap *= 2));
    }
    (*actions)[(*len)++] = action;
}

bool parse (Prog prog, const Token *tokens, size_t tokens_len, ASTAction **actions, size_t *actions_len) {
    ParseState state = (ParseState) {
        .prog = prog,
        .tokens = tokens,
        .tokens_len = tokens_len,
        .offset = 0
    };

    // we don't want to exit right away after parse_action fails because we'll miss all the other errors
    bool return_res = true;

    size_t actions_capacity = 2;
    while (state.offset < state.tokens_len) {
        ASTAction action = { 0 };
        if (!parse_action(&state, &action)) {
            return_res = false;
            synchronize(&state);
        }
        push_action(action, actions, actions_len, &actions_capacity);
    }

    return return_res;
}

// doesn't free name and val, because those are copied from tokens creating a double free after free_tokens
void free_actions (ASTAction *actions, size_t actions_len) {
    for (size_t i = 0; i < actions_len; ++i) {
        free(actions[i].reqs.elems);
        free(actions[i].commands.elems);
        free(actions[i].updates.elems);
    }
    free(actions);
}
