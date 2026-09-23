#include "ring_buffer.h"

static int is_power_of_two(size_t n) {
    return n != 0 && (n & (n - 1)) == 0;
}

int ring_buffer_init(struct ring_buffer *rb, uint8_t *buf, size_t capacity) {
    if (!is_power_of_two(capacity)) {
        return -1;
    }
    rb->buf = buf;
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    return 0;
}

/* head/tail are monotonically increasing counters, not indices into buf
 * directly — the buffer position is (counter & (capacity - 1)). This
 * (rather than wrapping the counters themselves) is what lets len/
 * free_space tell "empty" and "full" apart without a separate flag. */

size_t ring_buffer_len(const struct ring_buffer *rb) {
    size_t head = __atomic_load_n(&rb->head, __ATOMIC_ACQUIRE);
    size_t tail = __atomic_load_n(&rb->tail, __ATOMIC_ACQUIRE);
    return head - tail;
}

size_t ring_buffer_free_space(const struct ring_buffer *rb) {
    return rb->capacity - ring_buffer_len(rb);
}

size_t ring_buffer_write(struct ring_buffer *rb, const uint8_t *data, size_t len) {
    size_t head = __atomic_load_n(&rb->head, __ATOMIC_RELAXED);
    size_t tail = __atomic_load_n(&rb->tail, __ATOMIC_ACQUIRE);
    size_t free_space = rb->capacity - (head - tail);

    if (len > free_space) {
        len = free_space;
    }

    size_t mask = rb->capacity - 1;
    for (size_t i = 0; i < len; i++) {
        rb->buf[(head + i) & mask] = data[i];
    }

    __atomic_store_n(&rb->head, head + len, __ATOMIC_RELEASE);
    return len;
}

size_t ring_buffer_read(struct ring_buffer *rb, uint8_t *out, size_t len) {
    size_t tail = __atomic_load_n(&rb->tail, __ATOMIC_RELAXED);
    size_t head = __atomic_load_n(&rb->head, __ATOMIC_ACQUIRE);
    size_t available = head - tail;

    if (len > available) {
        len = available;
    }

    size_t mask = rb->capacity - 1;
    for (size_t i = 0; i < len; i++) {
        out[i] = rb->buf[(tail + i) & mask];
    }

    __atomic_store_n(&rb->tail, tail + len, __ATOMIC_RELEASE);
    return len;
}
