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

static bool take_chars (LexState *s, char* chrs) {
    LexState s_save = *s;
    while (*chrs != '\0' && take_char(s, *chrs++)) ++s->offset;
    if (*chrs == '\0') {
        return true;
    } else {
        *s = s_save;
        return false;
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

    // find number of chars
    char ichar;
    size_t num_chars = 0;
    while (next(s, &ichar)) {
        if (!valid_ident_char(ichar)) {
            --s->offset;
            break;
        }
        ++num_chars;
    }

    // first character isn't valid
    if (num_chars == 0) {
        return false;
    }

    tok->type = IDENT;
    tok->data.ident = str_to_slice(s->prog.text, s_save.offset, num_chars);
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

static bool lex_string (LexState *s, Token *tok) {
    LexState s_save = *s;

    size_t backticks = 0;
    TRYBOOL(lex_quote_start(s, &backticks));
    InterpolString str = (InterpolString) {
        .backticks = backticks,
        .parts = list_new()
    };

    size_t str_section_start = s_save.offset + backticks + 1;
    size_t str_section_len = 0;
    while (!lex_quote_end(s, backticks)) {
        // string interpolation
        if (take_chars(s, "$(")) {
            // dont want empty strings
            if (str_section_len > 0) {
                list_push(&str.parts, malloc(sizeof(InterpolPart)));
                *(InterpolPart *) str.parts.last->data = (InterpolPart) {
                    .type = INTERPOL_STRING,
                    .data = str_to_slice(s->prog.text, str_section_start, str_section_len)
                };
                str_section_len = 0;
            }
            Token ident;
            TRYBOOL_R(lex_ident(s, &ident), list_free(str.parts), *s = s_save);
            list_push(&str.parts, malloc(sizeof(InterpolPart)));
            *(InterpolPart *) str.parts.last->data = (InterpolPart) {
                .type = INTERPOL_IDENT,
                .data = ident.data.ident
            };

            TRYBOOL_R(take_char(s, ')'), list_free(str.parts), *s = s_save);
            str_section_start = s->offset;
        }
        char c;
        TRYBOOL_R(next(s, &c), list_free(str.parts), *s = s_save);
        ++str_section_len;
    }
    if (str_section_len > 0) {
        list_push(&str.parts, malloc(sizeof(InterpolPart)));
        *(InterpolPart *) str.parts.last->data = (InterpolPart) {
            .type = INTERPOL_STRING,
            .data = str_to_slice(s->prog.text, str_section_start, str_section_len)
        };
    }

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

bool lex (Prog prog, LList *tokens) {
    // starting state
    LexState state = (LexState) {
        .prog = prog,
        .offset = 0
    };
    // array of possible lexers
    bool (*lexers[3]) (LexState *, Token *) = { lex_symbol, lex_string, lex_ident };
    *tokens = list_new();

    // whether lexer failed while lexing
    bool failed = false;
    while (state.prog.text[state.offset] != '\0') {
        bool one_succeeded = false;
        for (size_t i = 0; i < sizeof(lexers) / sizeof(lexers[0]); ++i) {
            Token *tok = malloc(sizeof(Token));
            if (lexers[i](&state, tok)) {
                one_succeeded = true;
                list_push(tokens, tok);
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
void free_tokens (LList tokens) {
    FOREACH(Token, tok, tokens) {
        if (tok.type == STRING) {
            list_free(tok.data.string.parts);
        }
    }
    list_free(tokens);
}
