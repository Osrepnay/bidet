#include <stdlib.h>
#include "list.h"

LList list_new () {
    return (LList) { .head = NULL, .last = NULL };
}

// converts one node to a full list
LList node_to_list (LLNode *head) {
    LLNode *last = head;
    LLNode *end = head->next;
    for (; end != NULL; end = end->next) last = end;
    return (LList) {
        .head = head,
        .last = last
    };
}

size_t list_len (LList list) {
    size_t len = 0;
    LLNode *curr = list.head;
    for (; curr != NULL; curr = curr->next) ++len;
    return len;
}

void list_push (LList *list, void *elem) {
    if (list->head != NULL) {
        list->last->next = malloc(sizeof(LLNode));
        list->last->next->data = elem;
        list->last->next->next = NULL;
        list->last = list->last->next;
    } else {
        list->last = malloc(sizeof(LLNode));
        list->last->data = elem;
        list->last->next = NULL;
        list->head = list->last;
    }
}

void list_free (LList list) {
    LLNode *node = list.head;
    while (node != NULL) {
        LLNode *tmp = node;
        node = node->next;
        free(tmp->data);
        free(tmp);
    }
}
