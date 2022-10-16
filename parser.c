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
    LList tokens;
    LLNode *pos;
} ParseState;

// reports error at s's offset
static void expected_err (const ParseState *s, const char *expected) {
    Token *pos_data = s->pos->data;
    char *got = s->pos == NULL ? "EOF" : type_to_string(pos_data->type);
    char *message = malloc(sizeof("expected , got \n") + strlen(expected) + strlen(got));
    sprintf(message, "expected %s, got %s\n", expected, got);
    char *err = fmt_err(s->prog, pos_data->offset, message);
    fputs(err, stderr);
    free(err);
}

static bool peek (ParseState *s, Token *tok) {
    TRYBOOL(s->pos != NULL);
    *tok = *(Token *) s->pos->data;
    return true;
}

static bool next (ParseState *s, Token *tok) {
    TRYBOOL(peek(s, tok));
    s->pos = s->pos->next;
    return true;
}

static bool take_token (ParseState *s, TokenType type, Token *tok) {
    TRYBOOL(peek(s, tok));
    if (tok->type != type) {
        return false;
    } else {
        s->pos = s->pos->next;
        return true;
    }
}

// identical to take_token except it doesn't return the result
static bool take_token_ignore (ParseState *s, TokenType type) {
    Token tok;
    TRYBOOL(peek(s, &tok));
    if (tok.type != type) {
        return false;
    } else {
        s->pos = s->pos->next;
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
    Token first;
    TRYBOOL(peek(s, &first) && (first.type == IDENT || first.type == STRING));

    concat_str->catee = list_new();
    Token concat;
    do {
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
                expected_err(s, "string or identifier");
                return false;
        }
        s->pos = s->pos->next;
    } while (take_token(s, CONCAT, &concat));

    return true;
}

static bool parse_list (ParseState *s, ASTList *list) {
    TRYBOOL(take_token_ignore(s, BRACKET_OPEN));

    // find elements
    Token next_tok; // either close bracket or first element of list
    TRYBOOL_R(peek(s, &next_tok), expected_err(s, "list element or close bracket"));

    list->elems = list_new();
    if (next_tok.type == BRACKET_CLOSE) { // no length
        s->pos = s->pos->next;
        return true;
    }

    Token comma;
    do {
        list_push(&list->elems, malloc(sizeof(ASTConcat)));
        TRYBOOL_R(parse_concat_string(s, list->elems.last->data),
            list_free(list->elems),
            expected_err(s, "list element"));

        TRYBOOL_R(take_token(s, COMMA, &comma) || take_token(s, BRACKET_CLOSE, &comma),
            list_free(list->elems),
            expected_err(s, "comma or close bracket"));
    } while (comma.type != BRACKET_CLOSE);

    return true;
}

static bool parse_action (ParseState *s, ASTAction *action) {
    TRYBOOL(parse_list(s, &action->reqs));
    TRYBOOL_R(take_token_ignore(s, ARROW), expected_err(s, "arrow"));

    Token name_tok;
    TRYBOOL_R(take_token(s, IDENT, &name_tok), expected_err(s, "identifier"));
    action->name = name_tok.data.ident;
    TRYBOOL(parse_list(s, &action->commands));

    TRYBOOL(take_token_ignore(s, ARROW));
    TRYBOOL(parse_list(s, &action->updates));

    TRYBOOL_R(take_token_ignore(s, SEMICOLON), expected_err(s, "semicolon"));

    return true;
}

bool parse (Prog prog, LList tokens, LList *actions) {
    ParseState state = (ParseState) {
        .prog = prog,
        .tokens = tokens,
        .pos = tokens.head
    };

    // we don't want to exit right away after parse_action fails because we'll miss all the other errors
    bool return_res = true;

    *actions = list_new();
    while (state.pos != NULL) {
        list_push(actions, malloc(sizeof(ASTAction)));
        if (!parse_action(&state, actions->last->data)) {
            return_res = false;
            synchronize(&state);
        }
    }

    return return_res;
}

// doesn't free some lexer stuff copied from tokens
// free_tokens does that
void free_actions (LList actions) {
    FOREACH(ASTAction, action, actions) {
        list_free(action.reqs.elems);
        list_free(action.commands.elems);
        list_free(action.updates.elems);
    }
    list_free(actions);
}
