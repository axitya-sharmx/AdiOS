#include <assert.h>

#include "../../kernel/core/list.h"

struct item {
    int value;
    struct list_node link;
};

static void test_empty_list(void) {
    struct list_node head;
    list_init(&head);
    assert(list_empty(&head));
}

static void test_push_back_order(void) {
    struct list_node head;
    list_init(&head);

    struct item a = {.value = 1};
    struct item b = {.value = 2};
    struct item c = {.value = 3};
    list_push_back(&head, &a.link);
    list_push_back(&head, &b.link);
    list_push_back(&head, &c.link);

    int expected[] = {1, 2, 3};
    int i = 0;
    list_for_each(pos, &head) {
        struct item *it = list_entry(pos, struct item, link);
        assert(it->value == expected[i]);
        i++;
    }
    assert(i == 3);
}

static void test_push_front_order(void) {
    struct list_node head;
    list_init(&head);

    struct item a = {.value = 1};
    struct item b = {.value = 2};
    list_push_front(&head, &a.link);
    list_push_front(&head, &b.link);

    struct list_node *first = head.next;
    assert(list_entry(first, struct item, link)->value == 2);
}

static void test_remove(void) {
    struct list_node head;
    list_init(&head);

    struct item a = {.value = 1};
    struct item b = {.value = 2};
    struct item c = {.value = 3};
    list_push_back(&head, &a.link);
    list_push_back(&head, &b.link);
    list_push_back(&head, &c.link);

    list_remove(&b.link);

    int expected[] = {1, 3};
    int i = 0;
    list_for_each(pos, &head) {
        struct item *it = list_entry(pos, struct item, link);
        assert(it->value == expected[i]);
        i++;
    }
    assert(i == 2);
}

static void test_for_each_safe_removes_all(void) {
    struct list_node head;
    list_init(&head);

    struct item a = {.value = 1};
    struct item b = {.value = 2};
    list_push_back(&head, &a.link);
    list_push_back(&head, &b.link);

    list_for_each_safe(pos, tmp, &head) {
        list_remove(pos);
    }
    assert(list_empty(&head));
}

int main(void) {
    test_empty_list();
    test_push_back_order();
    test_push_front_order();
    test_remove();
    test_for_each_safe_removes_all();
    return 0;
}
