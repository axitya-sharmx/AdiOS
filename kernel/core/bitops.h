#pragma once

#include <stddef.h>
#include <stdint.h>

/* Bit manipulation over a flat array of 64-bit words — the building block
 * a bitmap-based physical frame allocator, a handle table, or a CPU
 * present-mask would sit on top of (OS_MASTER_SPEC.md §4.1: "hardware
 * register manipulation" / low-level primitives belong in C, not C++).
 * `bit` is a global index; word = bit / 64, offset = bit % 64. */

static inline void bit_set(uint64_t *words, size_t bit) {
    words[bit / 64] |= (uint64_t)1 << (bit % 64);
}

static inline void bit_clear(uint64_t *words, size_t bit) {
    words[bit / 64] &= ~((uint64_t)1 << (bit % 64));
}

static inline int bit_test(const uint64_t *words, size_t bit) {
    return (words[bit / 64] >> (bit % 64)) & 1;
}

/* Index of the lowest set bit in `word`, or -1 if `word` is zero. */
static inline int bit_find_first_set(uint64_t word) {
    return word == 0 ? -1 : __builtin_ctzll(word);
}

/* Scans `words` (an array of `word_count` 64-bit words, `word_count * 64`
 * bits total) for the lowest-indexed clear bit. Returns its global index,
 * or -1 if every bit is set. */
static inline long bit_find_first_clear(const uint64_t *words, size_t word_count) {
    for (size_t i = 0; i < word_count; i++) {
        uint64_t inverted = ~words[i];
        if (inverted != 0) {
            return (long)(i * 64 + (size_t)__builtin_ctzll(inverted));
        }
    }
    return -1;
}
