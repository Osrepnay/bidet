// arraylists/vectors

#include <stdlib.h>

// default capacity
#define CAPACITY_START 8
// how much to extend the capacity when it runs out
#define CAPACITY_EXTEND_FACTOR 2

// macro to make a list for a type
// type is actual type
// typename_c is pretty name for the struct (capitalized)
// typename_l is type name for use in function names and the like (lowercase)
#define GENLIST_TYPE(type, typename_c, typename_l) \
    typedef struct { \
        type *elems; \
        size_t len; \
        size_t cap; \
    } List ## typename_c; \
    \
    List ## typename_c list_ ## typename_l ## _new () { \
        return (List ## typename_c) { \
            .elems = malloc(sizeof(type) * CAPACITY_START), \
            .len = 0, \
            .cap = CAPACITY_START \
        }; \
    } \
    \
    void list_ ## typename_l ## _push (List ## typename_c *list, type elem) { \
        if (list->len == list->cap) { \
            list->cap *= CAPACITY_EXTEND_FACTOR; \
            list = realloc(list, sizeof(type) * list->cap); \
        } \
        list->elems[list->len++] = elem; \
    }

// cannot be empty!!
#define list_last(list) (list).elems[(list).len - 1]
