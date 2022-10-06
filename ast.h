#include <stddef.h>
#include "lexer.h"
#include "list.h"

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

GENLIST_TYPE(ASTCatee, ASTCatee, astcatee)

// ident, string, concat string
typedef struct {
    LLASTCatee catee;
} ASTConcat;

GENLIST_TYPE(ASTConcat, ASTConcat, astconcat)

typedef struct {
    LLASTConcat elems;
} ASTList;

typedef struct {
    ASTList reqs;
    char *name;
    ASTList commands;
    ASTList updates;
} ASTAction;
