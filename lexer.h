#include <stdbool.h>
#include <stddef.h>

typedef enum {
    IDENT,
    ARROW,
    COMMA,
    S_BRACE_OPEN, S_BRACE_CLOSE, // square
    SEMICOLON,
    STRING,
} TokenType;

typedef struct {
    TokenType type;
    char *val; // string when STRING, identifier when IDENT
    size_t offset;
    size_t length;
} Token;

bool lex (const char *, const char *, Token **, size_t *);
