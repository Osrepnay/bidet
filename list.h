// linked list

#include <stddef.h>

struct LLNode {
    void *data;
    struct LLNode *next;
};
typedef struct LLNode LLNode;

typedef struct {
    LLNode *head;
    LLNode *last;
} LList;

LList list_new ();
LList node_to_list (LLNode *);

size_t list_len (LList);
void list_push (LList *, void *);
void list_free (LList);

#define FOREACH(type, var, list) \
    type var; /* hope you didnt use the same name twice! (maybe use that local struct trick?) */ \
    for (LLNode *FOREACH_NODE##var = list.head; FOREACH_NODE##var != NULL && (var = *(type *) FOREACH_NODE##var->data, true); FOREACH_NODE##var = FOREACH_NODE##var->next)
