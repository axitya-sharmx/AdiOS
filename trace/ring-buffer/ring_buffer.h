#pragma once

#include <stddef.h>
#include <stdint.h>

/* Single-producer/single-consumer fixed-capacity byte ring buffer.
 * SPSC only: exactly one writer and one reader may call into this
 * concurrently (e.g. an interrupt handler producing trace events, a
 * background drain consuming them) without a lock, because head/tail are
 * each written by only one side. Multiple producers or multiple
 * consumers need external locking — this does not become MPMC-safe on
 * its own. Capacity must be a power of two (checked by ring_buffer_init). */

struct ring_buffer {
    uint8_t *buf;
    size_t capacity; /* power of two */
    volatile size_t head; /* next write index (producer-owned) */
    volatile size_t tail; /* next read index (consumer-owned) */
};

/* `buf` must be `capacity` bytes, `capacity` must be a power of two.
 * Returns 0 on success, -1 if capacity is not a power of two. */
int ring_buffer_init(struct ring_buffer *rb, uint8_t *buf, size_t capacity);

size_t ring_buffer_len(const struct ring_buffer *rb);
size_t ring_buffer_free_space(const struct ring_buffer *rb);

/* Writes as many of `len` bytes as fit; returns the number actually
 * written (a full buffer is not an error, just a short write). */
size_t ring_buffer_write(struct ring_buffer *rb, const uint8_t *data, size_t len);

/* Reads up to `len` bytes into `out`; returns the number actually read. */
size_t ring_buffer_read(struct ring_buffer *rb, uint8_t *out, size_t len);
