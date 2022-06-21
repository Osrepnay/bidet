typedef struct {
    enum {
        ELEM_STR,
        ELEM_IDENT
    } type;
    char *val; // like in the token it's the same for the string and identifier
} ASTListElem;

typedef struct {
    int length;
    ASTListElem *elems;
} ASTList;

typedef struct {
    ASTList reqs;
    char *name;
    ASTList commands;
    ASTList updates;
} ASTAction;
