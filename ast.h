#include <stddef.h>
#include "lexer.h"

// string types, just ident and string for now
typedef struct {
    enum {
        STRING_IDENT,
        STRING_INTERPOL_STRING
    } type;
    union {
        char *ident;
        InterpolString interpol_string;
    } data;
} PossString;

typedef struct {
    size_t length;
    PossString *elems;
} ASTConcatString;

typedef struct {
    enum {
        ASTLIST_IDENT,
        ASTLIST_STRING
    } type;
    union {
        char *ident;
        ASTConcatString string;
    } data;
} ASTListElem;

typedef struct {
    size_t length;
    ASTListElem *elems;
} ASTList;

typedef struct {
    ASTList reqs;
    char *name;
    ASTList commands;
    ASTList updates;
} ASTAction;
