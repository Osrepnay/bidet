#include <stdbool.h>
#include <stddef.h>
#include "list.h"
#include "prog.h"

typedef enum {
    ARROW,
    BRACKET_CLOSE, BRACKET_OPEN,
    COMMA,
    CONCAT,
    IDENT,
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
} InterpolPart;

// interpolated string
typedef struct {
    size_t backticks;
    LList parts;
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
