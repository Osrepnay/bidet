#include <stdbool.h>
#include <stddef.h>
#include "prog.h"

typedef enum {
    IDENT,
    ARROW,
    BRACKET_OPEN, BRACKET_CLOSE,
    COMMA,
    SEMICOLON,
    STRING,
} TokenType;

typedef struct {
    char *name;
} TokenIdent;

typedef struct {
    char *text;
    size_t backticks;
} TokenString;

typedef struct {
    TokenType type;
    union {
        TokenIdent ident;
        TokenString string;
    } data;
    size_t offset;
    size_t length;
} Token;

bool lex (Prog prog, Token **, size_t *);
void free_tokens (Token *, size_t);
