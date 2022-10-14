#include <stdlib.h>
#include "list.h"

LList list_new () {
    return (LList) { .head = NULL, .last = NULL };
}

void list_push (LList *list, void *elem) {
    if (list->head != NULL) {
        list->last->next = malloc(sizeof(LLNode));
        list->last->next->data = elem;
        list->last = list->last->next;
    } else {
        list->last = malloc(sizeof(LLNode));
        list->last->data = elem;
        list->head = list->last;
    }
}
