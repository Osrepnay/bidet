// linked list

#include <stdlib.h>

// macro to make a list for a type
// type is actual type
// typename_c is pretty name for the struct (capitalized)
// typename_l is type name for use in function names and the like (lowercase)
#define GENLIST_TYPE(type, typename_c, typename_l) \
    struct LL##typename_c##Node { \
        type data; \
        struct LL##typename_c##Node *next; \
    }; \
    typedef struct LL##typename_c##Node LL##typename_c##Node; \
    typedef struct { \
        LL##typename_c##Node *head; \
        LL##typename_c##Node *last; \
    } LL##typename_c; \
    LL##typename_c list_##typename_l##_new () { \
        return (LL##typename_c) { .head = NULL, .last = NULL }; \
    } \
    void list_##typename_l##_push (LL##typename_c *list, type elem) { \
        if (list->head != NULL) { \
            list->last->next = malloc(sizeof(LL##typename_c##Node)); \
            list->last->next->data = elem; \
            list->last = list->last->next; \
        } else { \
            list->last = malloc(sizeof(LL##typename_c##Node)); \
            list->last->data = elem; \
            list->head = list->last; \
        } \
    } \
    void list_##typename_l##_free (LL##typename_c list) { \
        LL##typename_c##Node *curr_node = list.head; \
        while (curr_node != NULL) { \
            LL##typename_c##Node *tmp = curr_node; \
            curr_node = curr_node->next; \
            free(tmp); \
        } \
    }

#define FOREACH(type, var, list) for (type *var = list.head; var != NULL; var = var->next)
