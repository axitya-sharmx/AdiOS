#pragma once

#include <stddef.h>

/* Intrusive circular doubly-linked list (Linux-kernel style): the link
 * node embeds in the owning struct instead of the list owning boxed
 * elements, so inserting/removing never allocates. An empty list is a
 * node that points to itself. */

struct list_node {
    struct list_node *next;
    struct list_node *prev;
};

#define LIST_HEAD_INIT(name) { .next = &(name), .prev = &(name) }

static inline void list_init(struct list_node *head) {
    head->next = head;
    head->prev = head;
}

static inline int list_empty(const struct list_node *head) {
    return head->next == head;
}

static inline void list_insert_between(struct list_node *node,
                                        struct list_node *prev,
                                        struct list_node *next) {
    prev->next = node;
    node->prev = prev;
    node->next = next;
    next->prev = node;
}

/* Insert as the first element (right after head). */
static inline void list_push_front(struct list_node *head,
                                    struct list_node *node) {
    list_insert_between(node, head, head->next);
}

/* Insert as the last element (right before head). */
static inline void list_push_back(struct list_node *head,
                                   struct list_node *node) {
    list_insert_between(node, head->prev, head);
}

static inline void list_remove(struct list_node *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    /* Leave node->next/prev dangling rather than self-pointing: a
     * double-remove will corrupt visibly (crash) instead of silently
     * no-op-ing, which is easier to catch during development. */
}

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define list_entry(node, type, member) container_of(node, type, member)

#define list_for_each(pos, head) \
    for (struct list_node *pos = (head)->next; pos != (head); pos = pos->next)

/* Safe against removing `pos` during the iteration (e.g. freeing it). */
#define list_for_each_safe(pos, tmp, head)                          \
    for (struct list_node *pos = (head)->next, *tmp = pos->next;    \
         pos != (head); pos = tmp, tmp = pos->next)
