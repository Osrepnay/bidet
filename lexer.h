#include <stdbool.h>
#include <stddef.h>

typedef enum {
    IDENT,
    ARROW,
    COMMA,
    BRACKET_OPEN, BRACKET_CLOSE,
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
void free_tokens (Token *, size_t);
