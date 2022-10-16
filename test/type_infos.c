#include <stdio.h>
#include <string.h>
#include "../parser.h"
#include "../try.h"
#include "type_infos.h"

#define TRYPOS(v) \
do { \
    int TRYPOSres = v; if (TRYPOSres < 0) return TRYPOSres; \
} while (0);

int slice_equal (StringSlice expd, StringSlice got) {
    return strcmp(slice_to_str(expd), slice_to_str(got)) == 0;
}

int interpolstring_equal (InterpolString expd, InterpolString got) {
    TRYBOOL(expd.backticks == got.backticks);
    LLNode *expdn = expd.parts.head;
    LLNode *gotn = got.parts.head;
    for (; expdn != NULL && gotn != NULL; expdn = expdn->next, gotn = gotn->next) {
        InterpolPart expdnp = *(InterpolPart *) expdn->data;
        InterpolPart gotnp = *(InterpolPart *) gotn->data;
        TRYBOOL(expdnp.type == gotnp.type);
        TRYBOOL(slice_equal(expdnp.data, gotnp.data));
    }
    return expdn == NULL && gotn == NULL;
}

int token_equal_cb (const void *expd_v, const void *got_v, void *udata) {
    (void) udata;

    const Token *expd = (const Token *) expd_v;
    const Token *got = (const Token *) got_v;

    TRYBOOL(expd->type == got->type)
    if (expd->type == IDENT) {
        TRYBOOL(slice_equal(expd->data.ident, got->data.ident))
    } else if (expd->type == STRING) {
        TRYBOOL(interpolstring_equal(expd->data.string, got->data.string));
    }
    TRYBOOL(expd->offset == got->offset);
    TRYBOOL(expd->length == got->length);
    return true;
}

static char *type_to_string (TokenType type) {
    switch (type) {
        case IDENT:
            return "IDENT";
        case ARROW:
            return "ARROW";
        case BRACKET_OPEN:
            return "BRACKET_OPEN";
        case BRACKET_CLOSE:
            return "BRACKET_CLOSE";
        case COMMA:
            return "COMMA";
        case CONCAT:
            return "CONCAT";
        case SEMICOLON:
            return "SEMICOLON";
        case STRING:
            return "STRING";
        default:
            return "not a type";
    }
}

// TODO make printfs use a string instead of TRYPOS and printing
int interpolstring_printf (InterpolString t) {
    for (size_t i = 0; i < t.backticks; ++i) TRYPOS(printf("`"));
    FOREACH(InterpolPart, part, t.parts) {
        if (part.type == INTERPOL_IDENT) {
            TRYPOS(printf("$(%s)", slice_to_str(part.data)));
        } else {
            TRYPOS(printf("%s", slice_to_str(part.data)))
        }
    }
    for (size_t i = 0; i < t.backticks; ++i) TRYPOS(printf("`"));
    return true;
}

int token_printf_cb (const void *t_v, void *udata) {
    (void) udata;

    const Token *t = (const Token *) t_v;

    TRYPOS(printf("Token { .type = %s, ", type_to_string(t->type)));
    // has value, print that too
    if (t->type == IDENT) {
        TRYPOS(printf(".data.ident = %s, ", slice_to_str(t->data.ident)));
    } else if (t->type == STRING) {
        TRYPOS(printf(".data.string = "));
        TRYPOS(interpolstring_printf(t->data.string));
        TRYPOS(printf(", "));
    }
    return printf(".offset = %zu, .length = %zu }", t->offset, t->length);
}

int astconcat_equal (ASTConcat expd, ASTConcat got) {
    LLNode *expdn = expd.catee.head;
    LLNode *gotn = got.catee.head;
    for (; expdn != NULL && gotn != NULL; expdn = expdn->next, gotn = gotn->next) {
        ASTCatee expdnc = *(ASTCatee *) expdn->data;
        ASTCatee gotnc = *(ASTCatee *) gotn->data;
        TRYBOOL(expdnc.type == gotnc.type);
        if (expdnc.type == CATEE_IDENT) {
            TRYBOOL(slice_equal(expdnc.data.ident, gotnc.data.ident));
        } else {
            TRYBOOL(interpolstring_equal(expdnc.data.interpol_string, gotnc.data.interpol_string));
        }
    }
    return expdn == NULL && gotn == NULL;
}

int astlist_equal (ASTList expd, ASTList got) {
    LLNode *expdn = expd.elems.head;
    LLNode *gotn = got.elems.head;
    for (; expdn != NULL && gotn != NULL; expdn = expdn->next, gotn = gotn->next) {
        ASTConcat expdnc = *(ASTConcat *) expdn->data;
        ASTConcat gotnc = *(ASTConcat *) gotn->data;
        TRYBOOL(astconcat_equal(expdnc, gotnc));
    }
    return expdn == NULL && gotn == NULL;
}

int astaction_equal_cb (const void *expd_v, const void *got_v, void *udata) {
    (void) udata;

    const ASTAction *expd = (const ASTAction *) expd_v;
    const ASTAction *got = (const ASTAction *) got_v;

    TRYBOOL(astlist_equal(expd->reqs, got->reqs));
    TRYBOOL(astlist_equal(expd->commands, got->commands));
    TRYBOOL(slice_equal(expd->name, got->name));
    TRYBOOL(astlist_equal(expd->updates, got->updates));

    return true;
}

int astlist_printf (ASTList t) {
    TRYPOS(printf("["));
    FOREACH(ASTConcat, concat, t.elems) {
        bool first = true;
        FOREACH(ASTCatee, catee, concat.catee) {
            if (!first) {
                TRYPOS(printf(" + "));
            }
            first = false;
            if (catee.type == CATEE_IDENT) {
                TRYPOS(printf("%s", slice_to_str(catee.data.ident)));
            } else {
                TRYPOS(interpolstring_printf(catee.data.interpol_string));
            }
        }
    }
    return printf("]");
}

int astaction_printf_cb (const void *t_v, void *udata) {
    (void) udata;

    const ASTAction *t = (const ASTAction *) t_v;

    TRYPOS(astlist_printf(t->reqs));
    TRYPOS(printf(" > %s ", slice_to_str(t->name)));
    TRYPOS(astlist_printf(t->commands));
    TRYPOS(printf(" > "));
    return astlist_printf(t->updates);
}
