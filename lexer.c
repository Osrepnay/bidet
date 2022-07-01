#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "fmt_error.h"

// if v is false then return false
// else do nothing
#define TRYBOOL(v, rescue) \
    do { \
        if (!(v)) { rescue; return 0; } \
    } while (0)

typedef struct {
    Prog prog; // program info
    size_t offset; // offset from beginning of file
} LexState;

// get next char without consuming input
bool peek (LexState *s, char *chr) {
    char c = s->prog.text[s->offset];
    TRYBOOL(c != '\0', );
    *chr = c;
    return true;
}

// get next char; fails on eof (\0)
// when peek fails it doesn' increment offset
bool next (LexState *s, char *chr) {
    TRYBOOL(peek(s, chr), );
    ++s->offset;
    return true;
}

bool take_char (LexState *s, char chr) {
    char c;
    TRYBOOL(next(s, &c), );
    if (c != chr) {
        --s->offset;
        return false;
    } else {
        return true;
    }
}

// parse misc symbols
bool lex_symbol (LexState *s, Token *tok) {
    char sym;
    TRYBOOL(peek(s, &sym), );
    switch (sym) {
        case '>':
            tok->type = ARROW;
            break;
        case ',':
            tok->type = COMMA;
            break;
        case '[':
            tok->type = BRACKET_OPEN;
            break;
        case ']':
            tok->type = BRACKET_CLOSE;
            break;
        case ';':
            tok->type = SEMICOLON;
            break;
        default: // unknown symbol
            return false;
    }
    // default returns, so if func reaches here token succeeded
    tok->offset = s->offset++; // peeked, so now increment position
    tok->length = 1;
    return true;
}

// TODO unicode or something
bool valid_ident_char (char c) {
    return ('0' <= c && c <= '9') ||
        ('A' <= c && c <= 'Z') ||
        ('a' <= c && c <= 'z') ||
        c == '-' || c == '_';
}

bool lex_ident (LexState *s, Token *tok) {
    LexState s_save = *s; // save for resetting

    // first find number of chars
    size_t num_chars = 0;
    char ichar;
    TRYBOOL(next(s, &ichar), );
    while (valid_ident_char(ichar)) {
        ++num_chars;
        if (!next(s, &ichar)) {
            break;
        }
    }
    *s = s_save;

    // first character isn't valid
    if (num_chars == 0) {
        return false;
    }

    // then start writing to string
    char *ident = malloc(num_chars + 1);
    for (size_t i = 0; i < num_chars; ++i) {
        TRYBOOL(next(s, ident + i), *s = s_save);
    }
    ident[num_chars] = '\0';

    tok->type = IDENT;
    tok->val = ident;
    tok->offset = s_save.offset;
    tok->length = num_chars;
    return true;
}

// if given "``` " for example writes 3 to num_backticks
bool lex_quote_start (LexState *s, size_t *num_backticks) {
    LexState s_save = *s;

    while (take_char(s, '`')) {
        ++*num_backticks;
    }
    // if num_backticks is 0, that means that there was no quote
    TRYBOOL(*num_backticks != 0, *s = s_save);
    TRYBOOL(take_char(s, ' '), *s = s_save);
    return true;
}

// lexes " <num_backticks `s>"
bool lex_quote_end (LexState *s, size_t num_backticks) {
    LexState s_save = *s;

    TRYBOOL(take_char(s, ' '), );
    for (size_t i = 0; i < num_backticks; ++i) {
        TRYBOOL(take_char(s, '`'), *s = s_save);
    }
    return true;
}

bool lex_string (LexState *s, Token *tok) {
    LexState s_save = *s;

    size_t num_backticks = 0;
    TRYBOOL(lex_quote_start(s, &num_backticks), );

    size_t num_chars = 0;
    char curr_char;
    while (!lex_quote_end(s, num_backticks)) {
        TRYBOOL(next(s, &curr_char), *s = s_save);
        ++num_chars;
    }

    char *str = malloc(num_chars + 1);
    *s = s_save;

    num_backticks = 0;
    lex_quote_start(s, &num_backticks); // should never be false because we checked already
    for (size_t i = 0; i < num_chars; ++i) {
        TRYBOOL(next(s, str + i), *s = s_save);
    }
    lex_quote_end(s, num_backticks);
    str[num_chars] = '\0';

    tok->type = STRING;
    tok->val = str;
    tok->offset = s_save.offset;
    tok->length = s->offset - s_save.offset;
    return true;
}

bool take_whitespace (LexState *s) {
    while (take_char(s, ' ') || take_char(s, '\t') || take_char(s, '\n') || take_char(s, '\r'));
    return true;
}

// push to tokens, given length and capacity
void push_token (Token token, Token **tokens, size_t *len, size_t *cap) {
    // expand capacity when on the edge of overflowing
    if (*len >= *cap) {
        *tokens = realloc(*tokens, sizeof(Token) * (*cap *= 2));
    }
    (*tokens)[(*len)++] = token;
}

bool lex (const char *filename, const char *text, Token **tokens, size_t *tokens_len) {
    // starting state
    LexState state = (LexState) {
        .prog = (Prog) { .filename = filename, .text = text },
        .offset = 0
    };

    size_t tokens_capacity = 2;
    *tokens = malloc(sizeof(Token) * tokens_capacity);
    *tokens_len = 0;

    // array of possible lexers
    bool (*lexers[3]) (LexState *, Token *) = { lex_symbol, lex_string, lex_ident };

    // whether lexer failed while lexing
    bool failed = false;
    while (state.prog.text[state.offset] != '\0') {
        bool one_succeeded = false;
        for (int i = 0; i < 3; ++i) {
            Token tok;
            if (lexers[i](&state, &tok)) {
                one_succeeded = true;
                push_token(tok, tokens, tokens_len, &tokens_capacity);
                take_whitespace(&state);
                break;
            }
        }
        // no lexers worked
        if (!one_succeeded) {
            failed = true;

            // print error
            char *message = malloc(sizeof("unexpected character: x\n"));
            // state.prog.text[state.offset] can never be \0 because it would've been caught in the loop
            sprintf(message, "unexpected character: %c\n", state.prog.text[state.offset]); // fputs doesnt newline :((
            char *err = fmt_err(state.prog, state.offset, message);
            fputs(err, stderr);
            free(err);
            ++state.offset;
        }
    }
    return !failed;
}

// frees tokens from lex given tokens and length
// can't just free the list because of val
void free_tokens (Token *tokens, size_t tokens_len) {
    for (size_t i = 0; i < tokens_len; ++i) {
        if (tokens[i].type == IDENT || tokens[i].type == STRING) {
            free(tokens[i].val);
        }
    }
    free(tokens);
}
