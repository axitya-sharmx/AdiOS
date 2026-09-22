#include <cassert>

#include "../../kernel/core/span.hpp"

static void test_default_is_empty() {
    Span<int> s;
    assert(s.empty());
    assert(s.size() == 0);
    assert(s.data() == nullptr);
}

static void test_construct_and_index() {
    int arr[] = {10, 20, 30, 40};
    Span<int> s(arr, 4);
    assert(!s.empty());
    assert(s.size() == 4);
    assert(s[0] == 10);
    assert(s[3] == 40);
}

static void test_mutation_through_span_is_visible() {
    int arr[] = {1, 2, 3};
    Span<int> s(arr, 3);
    s[1] = 99;
    assert(arr[1] == 99);
}

static void test_iteration() {
    int arr[] = {1, 2, 3, 4, 5};
    Span<int> s(arr, 5);
    int sum = 0;
    for (int v : s) {
        sum += v;
    }
    assert(sum == 15);
}

static void test_subspan() {
    int arr[] = {0, 1, 2, 3, 4, 5};
    Span<int> s(arr, 6);
    Span<int> sub = s.subspan(2, 3);
    assert(sub.size() == 3);
    assert(sub[0] == 2);
    assert(sub[2] == 4);
}

int main() {
    test_default_is_empty();
    test_construct_and_index();
    test_mutation_through_span_is_visible();
    test_iteration();
    test_subspan();
    return 0;
}
