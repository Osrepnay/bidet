#include <stdbool.h>
#include <stddef.h>
#include "prog.h"

typedef enum {
    IDENT,
    ARROW,
    BRACKET_OPEN, BRACKET_CLOSE,
    COMMA,
    CONCAT,
    SEMICOLON,
    STRING,
} TokenType;

// part of interpolated string
typedef struct {
    enum {
        INTERPOL_IDENT,
        INTERPOL_STRING
    } type;
    char *data;
} InterpolStringElem;

// interpolated string
typedef struct {
    size_t backticks;
    size_t length;
    InterpolStringElem *elems;
} InterpolString; // wee woo wee woo

typedef struct {
    TokenType type;
    union {
        char *ident;
        InterpolString string;
    } data;
    size_t offset;
    size_t length;
} Token;

bool lex (Prog prog, Token **, size_t *);
void free_tokens (Token *, size_t);
