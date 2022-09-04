#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fmt_error.h"
#include "lexer.h"
#include "try.h"

typedef struct {
    Prog prog; // program info
    size_t offset; // offset from beginning of file
} LexState;

// like peek but doesn't fail (just returns \0 on eof)
static char curr (const LexState *s) {
    return s->prog.text[s->offset];
}

// get next char without consuming input
static bool peek (const LexState *s, char *chr) {
    char c = curr(s);
    TRYBOOL(c != '\0');
    *chr = c;
    return true;
}

// get next char; fails on eof (\0)
// when peek fails it doesn' increment offset
static bool next (LexState *s, char *chr) {
    TRYBOOL(peek(s, chr));
    ++s->offset;
    return true;
}

static bool take_char (LexState *s, char chr) {
    char c;
    TRYBOOL(next(s, &c));
    if (c != chr) {
        --s->offset;
        return false;
    } else {
        return true;
    }
}

// parse misc symbols
static bool lex_symbol (LexState *s, Token *tok) {
    char sym;
    TRYBOOL(peek(s, &sym));
    switch (sym) {
        case '>':
            tok->type = ARROW;
            break;
        case '[':
            tok->type = BRACKET_OPEN;
            break;
        case ']':
            tok->type = BRACKET_CLOSE;
            break;
        case ',':
            tok->type = COMMA;
            break;
        case '+':
            tok->type = CONCAT;
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
static bool valid_ident_char (char c) {
    return ('0' <= c && c <= '9') ||
        ('A' <= c && c <= 'Z') ||
        ('a' <= c && c <= 'z') ||
        c == '-' || c == '_';
}

static bool lex_ident (LexState *s, Token *tok) {
    LexState s_save = *s; // save for resetting

    // first find number of chars
    size_t num_chars = 0;
    char ichar;
    TRYBOOL(next(s, &ichar));
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
        TRYBOOL_R(next(s, ident + i), *s = s_save);
    }
    ident[num_chars] = '\0';

    tok->type = IDENT;
    tok->data.ident = ident;
    tok->offset = s_save.offset;
    tok->length = num_chars;
    return true;
}

// if given "```'" for example writes 3 to backticks
static bool lex_quote_start (LexState *s, size_t *backticks) {
    LexState s_save = *s;

    while (take_char(s, '`')) {
        ++*backticks;
    }
    TRYBOOL_R(take_char(s, '\''), *s = s_save);
    return true;
}

// lexes "'<backticks `s>"
static bool lex_quote_end (LexState *s, size_t backticks) {
    LexState s_save = *s;

    TRYBOOL(take_char(s, '\''));
    for (size_t i = 0; i < backticks; ++i) {
        TRYBOOL_R(take_char(s, '`'), *s = s_save);
    }
    return true;
}

// basically only used one time (in this file) here
#include "list.h"
GENLIST_TYPE(char *, String, string)
GENLIST_TYPE(size_t, SizeT, size_t)

static bool lex_string (LexState *s, Token *tok) {
    LexState s_save = *s;

    size_t backticks = 0;
    TRYBOOL(lex_quote_start(s, &backticks));

    size_t first_in_str = s->offset;
    ListString idents = list_string_new();
    ListSizeT interpol_starts = list_size_t_new(); // start indexes of string interpolation (starts at $)
    ListSizeT interpol_ends = list_size_t_new(); // end indexes (after closing paren)
    size_t last_in_str = 0; // last character in string

    while (!lex_quote_end(s, backticks)) {
        // string interpolation
        if (take_char(s, '$')) {
            TRYBOOL(take_char(s, '('));
            list_size_t_push(&interpol_starts, s->offset - 2); // -2 because of $(

            Token ident;
            TRYBOOL_R(lex_ident(s, &ident), *s = s_save);
            list_string_push(&idents, ident.data.ident);

            TRYBOOL_R(take_char(s, ')'), *s = s_save);
            list_size_t_push(&interpol_ends, s->offset);
        } else {
            // only next if take_char hasn't nexted
            TRYBOOL_R(curr(s) != '\0', *s = s_save);
            ++s->offset;
        }
        last_in_str = s->offset - 1; // don't catch end quote
    }

    // reset to start storing
    *s = s_save;

    size_t sections = 2 * interpol_starts.len + 1;
    InterpolString str = (InterpolString) {
        .elems = malloc(sizeof(InterpolStringElem) * sections),
        .length = sections,
        .backticks = backticks
    };

    backticks = 0;
    lex_quote_start(s, &backticks); // should never be false because we checked already

    size_t curr_str_idx = 0; // current position in the InterpolString
    size_t last_end = first_in_str; // ending index of the last non-interpolated section
    for (size_t i = 0; i < idents.len; ++i){
        size_t dist = interpol_starts.elems[i] - last_end;
        last_end = interpol_ends.elems[i];

        // avoid empty strings
        if (dist != 0) {
            char *between = malloc(dist + 1);
            for (size_t j = 0; j < dist; ++j) {
                next(s, between + j);
            }
            between[dist] = '\0';

            str.elems[curr_str_idx++] = (InterpolStringElem) {
                .type = INTERPOL_STRING,
                .data = between
            };
        }

        str.elems[curr_str_idx++] = (InterpolStringElem) {
            .type = INTERPOL_IDENT,
            .data = idents.elems[i]
        };
        s->offset = interpol_ends.elems[i];
    }
    // last string section
    if (interpol_ends.len == 0 || s->prog.text[list_last(interpol_ends)] != '\0') {
        size_t last_len = interpol_ends.len == 0 ?
            last_in_str - first_in_str             + 1 :
            last_in_str - list_last(interpol_ends) + 1;
        char *last = malloc(last_len + 1);
        for (size_t j = 0; j < last_len; ++j) {
            next(s, last + j);
        }
        last[last_len] = '\0';

        str.elems[curr_str_idx++] = (InterpolStringElem) {
            .type = INTERPOL_STRING,
            .data = last
        };
    }

    free(idents.elems); // dont free char *s because they're copied to the interpol string
    free(interpol_starts.elems);
    free(interpol_ends.elems);

    lex_quote_end(s, backticks);

    tok->type = STRING;
    tok->data.string = str;
    tok->offset = s_save.offset;
    tok->length = s->offset - s_save.offset;
    return true;
}

static bool take_whitespace (LexState *s) {
    while (take_char(s, ' ') || take_char(s, '\t') || take_char(s, '\n') || take_char(s, '\r'));
    return true;
}

// push to tokens, given length and capacity
static void push_token (Token token, Token **tokens, size_t *len, size_t *cap) {
    // expand capacity when on the edge of overflowing
    if (*len >= *cap) {
        *tokens = realloc(*tokens, sizeof(Token) * (*cap *= 2));
    }
    (*tokens)[(*len)++] = token;
}

bool lex (Prog prog, Token **tokens, size_t *tokens_len) {
    // starting state
    LexState state = (LexState) {
        .prog = prog,
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
        for (size_t i = 0; i < sizeof(lexers) / sizeof(lexers[0]); ++i) {
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
void free_tokens (Token *tokens, size_t tokens_len) {
    for (size_t i = 0; i < tokens_len; ++i) {
        if (tokens[i].type == IDENT) {
            free(tokens[i].data.ident);
        } else if (tokens[i].type == STRING) {
            for (size_t j = 0; j < tokens[i].data.string.length; ++j) {
                free(tokens[i].data.string.elems[j].data);
            }
            free(tokens[i].data.string.elems);
        }
    }
    free(tokens);
}
