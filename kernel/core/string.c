#include "string.h"

#include <stdint.h>

typedef unsigned long word_t;
#define WORD_SIZE sizeof(word_t)
#define WORD_MASK (WORD_SIZE - 1)

void *memset(void *dst, int value, size_t count) {
    unsigned char *d = dst;
    const unsigned char byte = (unsigned char)value;

    /* Small or misaligned runs aren't worth the alignment dance. */
    if (count >= WORD_SIZE * 2 && ((uintptr_t)d & WORD_MASK) == 0) {
        word_t word = byte;
        for (size_t i = 1; i < WORD_SIZE; i++) {
            word |= word << 8;
        }

        word_t *wd = (word_t *)d;
        size_t words = count / WORD_SIZE;
        for (size_t i = 0; i < words; i++) {
            wd[i] = word;
        }

        d += words * WORD_SIZE;
        count -= words * WORD_SIZE;
    }

    for (size_t i = 0; i < count; i++) {
        d[i] = byte;
    }
    return dst;
}

void *memcpy(void *restrict dst, const void *restrict src, size_t count) {
    unsigned char *d = dst;
    const unsigned char *s = src;

    /* Word-copy only when src and dst share alignment; a mismatched
     * offset would need per-byte shifting to combine, not worth it here. */
    if (count >= WORD_SIZE * 2 &&
        ((uintptr_t)d & WORD_MASK) == ((uintptr_t)s & WORD_MASK) &&
        ((uintptr_t)d & WORD_MASK) == 0) {
        word_t *wd = (word_t *)d;
        const word_t *ws = (const word_t *)s;
        size_t words = count / WORD_SIZE;
        for (size_t i = 0; i < words; i++) {
            wd[i] = ws[i];
        }

        d += words * WORD_SIZE;
        s += words * WORD_SIZE;
        count -= words * WORD_SIZE;
    }

    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
    return dst;
}

/* ponytail: byte-at-a-time even on the non-overlapping path; add a
 * word-copy fast path like memcpy's if profiling ever shows memmove hot. */
void *memmove(void *dst, const void *src, size_t count) {
    unsigned char *d = dst;
    const unsigned char *s = src;

    if (d == s || count == 0) {
        return dst;
    }

    if (d < s || d >= s + count) {
        for (size_t i = 0; i < count; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = count; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t count) {
    const unsigned char *pa = a;
    const unsigned char *pb = b;
    for (size_t i = 0; i < count; i++) {
        if (pa[i] != pb[i]) {
            return (int)pa[i] - (int)pb[i];
        }
    }
    return 0;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (a[i] != b[i] || a[i] == '\0') {
            return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];
        }
    }
    return 0;
}
