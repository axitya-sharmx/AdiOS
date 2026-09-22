#include <assert.h>

#include "../../kernel/core/bitops.h"

static void test_set_clear_test(void) {
    uint64_t words[2] = {0};
    assert(bit_test(words, 5) == 0);
    bit_set(words, 5);
    assert(bit_test(words, 5) == 1);
    bit_clear(words, 5);
    assert(bit_test(words, 5) == 0);
}

static void test_set_crosses_word_boundary(void) {
    uint64_t words[2] = {0};
    bit_set(words, 64); /* first bit of the second word */
    assert(words[0] == 0);
    assert(words[1] == 1);
    assert(bit_test(words, 64) == 1);
}

static void test_find_first_set(void) {
    assert(bit_find_first_set(0) == -1);
    assert(bit_find_first_set(1) == 0);
    assert(bit_find_first_set(0x8) == 3);
}

static void test_find_first_clear(void) {
    uint64_t words[2] = {~0ULL, ~0ULL};
    assert(bit_find_first_clear(words, 2) == -1);

    words[1] = ~((uint64_t)1 << 3); /* every bit set except bit 3 of word 1 */
    assert(bit_find_first_clear(words, 2) == 64 + 3);

    uint64_t all_clear[1] = {0};
    assert(bit_find_first_clear(all_clear, 1) == 0);
}

int main(void) {
    test_set_clear_test();
    test_set_crosses_word_boundary();
    test_find_first_set();
    test_find_first_clear();
    return 0;
}
