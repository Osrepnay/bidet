#include <stddef.h>
#include "lexer.h"

// type to maybe be concatted
typedef struct {
    enum {
        CATEE_IDENT,
        CATEE_INTERPOL_STRING
    } type;
    union {
        char *ident;
        InterpolString interpol_string;
    } data;
} ASTCatee;

// ident, string, concat string
typedef struct {
    LList catee;
} ASTConcat;

typedef struct {
    LList elems;
} ASTList;

typedef struct {
    ASTList reqs;
    char *name;
    ASTList commands;
    ASTList updates;
} ASTAction;
