/* Host-buildable: compiled with the host libc, not the freestanding
 * kernel toolchain, to unit-test kernel/core/string.c in isolation. */
#include <assert.h>
#include <string.h>

#include "../../kernel/core/string.h"

static void test_memset(void) {
    unsigned char buf[8] = {0};
    memset(buf, 0xAB, sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); i++) {
        assert(buf[i] == 0xAB);
    }
}

/* Large + aligned + odd tail exercises the word-at-a-time fast path in
 * memset/memcpy plus the byte-wise remainder after it. */
static void test_memset_large_aligned(void) {
    static unsigned char buf[257];
    memset(buf, 0x5A, sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); i++) {
        assert(buf[i] == 0x5A);
    }
}

static void test_memcpy(void) {
    const char src[] = "adios";
    char dst[6] = {0};
    memcpy(dst, src, sizeof(src));
    assert(strcmp(dst, "adios") == 0);
}

static void test_memcpy_large_aligned(void) {
    static unsigned char src[257];
    static unsigned char dst[257];
    for (size_t i = 0; i < sizeof(src); i++) {
        src[i] = (unsigned char)i;
    }
    memcpy(dst, src, sizeof(src));
    assert(memcmp(src, dst, sizeof(src)) == 0);
}

static void test_memcpy_misaligned(void) {
    static unsigned char src[64];
    static unsigned char dst[64];
    for (size_t i = 0; i < sizeof(src); i++) {
        src[i] = (unsigned char)(i + 1);
    }
    /* Offset by 1 byte so dst/src share no common alignment relative to
     * the word size, forcing the byte-wise fallback path. */
    memcpy(dst + 1, src, sizeof(src) - 1);
    assert(memcmp(dst + 1, src, sizeof(src) - 1) == 0);
}

static void test_memmove_overlap(void) {
    char buf[] = "abcdef";
    memmove(buf + 1, buf, 5); /* forward-overlapping shift right */
    assert(strncmp(buf, "aabcde", 6) == 0);
}

static void test_memcmp(void) {
    assert(memcmp("abc", "abc", 3) == 0);
    assert(memcmp("abd", "abc", 3) > 0);
    assert(memcmp("abb", "abc", 3) < 0);
}

static void test_strlen(void) {
    assert(strlen("") == 0);
    assert(strlen("adios") == 5);
}

static void test_strcmp(void) {
    assert(strcmp("a", "a") == 0);
    assert(strcmp("a", "b") < 0);
    assert(strcmp("b", "a") > 0);
}

static void test_strncmp(void) {
    assert(strncmp("abcx", "abcy", 3) == 0);
    assert(strncmp("abc", "abcd", 4) != 0);
}

int main(void) {
    test_memset();
    test_memset_large_aligned();
    test_memcpy();
    test_memcpy_large_aligned();
    test_memcpy_misaligned();
    test_memmove_overlap();
    test_memcmp();
    test_strlen();
    test_strcmp();
    test_strncmp();
    return 0;
}
