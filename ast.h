#include <stddef.h>
#include "lexer.h"

typedef struct {
    size_t length;
    Token *elems;
} ASTList;

typedef struct {
    ASTList reqs;
    TokenIdent name;
    ASTList commands;
    ASTList updates;
} ASTAction;
