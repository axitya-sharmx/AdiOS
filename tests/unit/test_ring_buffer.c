#include <assert.h>
#include <string.h>

#include "../../trace/ring-buffer/ring_buffer.h"

static void test_init_rejects_non_power_of_two(void) {
    uint8_t buf[10];
    struct ring_buffer rb;
    assert(ring_buffer_init(&rb, buf, 10) == -1);
    assert(ring_buffer_init(&rb, buf, 8) == 0);
}

static void test_write_read_roundtrip(void) {
    uint8_t backing[8];
    struct ring_buffer rb;
    ring_buffer_init(&rb, backing, sizeof(backing));

    const uint8_t in[] = {1, 2, 3, 4};
    size_t written = ring_buffer_write(&rb, in, sizeof(in));
    assert(written == sizeof(in));
    assert(ring_buffer_len(&rb) == 4);
    assert(ring_buffer_free_space(&rb) == 4);

    uint8_t out[4] = {0};
    size_t read = ring_buffer_read(&rb, out, sizeof(out));
    assert(read == 4);
    assert(memcmp(in, out, 4) == 0);
    assert(ring_buffer_len(&rb) == 0);
}

static void test_write_short_when_full(void) {
    uint8_t backing[4];
    struct ring_buffer rb;
    ring_buffer_init(&rb, backing, sizeof(backing));

    const uint8_t in[] = {1, 2, 3, 4, 5, 6};
    size_t written = ring_buffer_write(&rb, in, sizeof(in));
    assert(written == 4); /* capacity-limited, not an error */
    assert(ring_buffer_free_space(&rb) == 0);
}

static void test_read_short_when_empty(void) {
    uint8_t backing[4];
    struct ring_buffer rb;
    ring_buffer_init(&rb, backing, sizeof(backing));

    uint8_t out[4];
    size_t read = ring_buffer_read(&rb, out, sizeof(out));
    assert(read == 0);
}

static void test_wraparound(void) {
    uint8_t backing[4];
    struct ring_buffer rb;
    ring_buffer_init(&rb, backing, sizeof(backing));

    uint8_t scratch[3];
    /* Fill, drain 3, refill 3: forces the write index past the physical
     * end of the backing array, exercising the wrap. */
    ring_buffer_write(&rb, (uint8_t[]){9, 9, 9, 9}, 4);
    ring_buffer_read(&rb, scratch, 3);

    const uint8_t in[] = {10, 11, 12};
    ring_buffer_write(&rb, in, 3);

    uint8_t out[4];
    size_t read = ring_buffer_read(&rb, out, sizeof(out));
    assert(read == 4);
    assert(out[0] == 9); /* the one byte left over from the first fill */
    assert(out[1] == 10);
    assert(out[2] == 11);
    assert(out[3] == 12);
}

int main(void) {
    test_init_rejects_non_power_of_two();
    test_write_read_roundtrip();
    test_write_short_when_full();
    test_read_short_when_empty();
    test_wraparound();
    return 0;
}
