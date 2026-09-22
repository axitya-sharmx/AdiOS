/* Host-buildable: exercises kvsnprintf/ksnprintf in isolation, not
 * kprintf (which requires the kernel's serial_write). */
#include <assert.h>
#include <string.h>

#include "../../kernel/logging/log.h"

static void test_literal_text(void) {
    char buf[32];
    size_t n = ksnprintf(buf, sizeof(buf), "hello");
    assert(n == 5);
    assert(strcmp(buf, "hello") == 0);
}

static void test_percent_d(void) {
    char buf[32];
    ksnprintf(buf, sizeof(buf), "%d %d %d", 0, 42, -7);
    assert(strcmp(buf, "0 42 -7") == 0);
}

static void test_percent_u_x(void) {
    char buf[32];
    ksnprintf(buf, sizeof(buf), "%u %x", 255u, 255u);
    assert(strcmp(buf, "255 ff") == 0);
}

static void test_percent_p(void) {
    char buf[32];
    ksnprintf(buf, sizeof(buf), "%p", (void *)0x1000);
    assert(strcmp(buf, "0x1000") == 0);
}

static void test_percent_s_c(void) {
    char buf[32];
    ksnprintf(buf, sizeof(buf), "%s-%c", "adios", '!');
    assert(strcmp(buf, "adios-!") == 0);
}

static void test_percent_literal(void) {
    char buf[32];
    ksnprintf(buf, sizeof(buf), "100%%");
    assert(strcmp(buf, "100%") == 0);
}

static void test_unknown_conversion_passthrough(void) {
    char buf[32];
    ksnprintf(buf, sizeof(buf), "%q");
    assert(strcmp(buf, "%q") == 0);
}

static void test_truncation(void) {
    char buf[4];
    size_t n = ksnprintf(buf, sizeof(buf), "hello");
    assert(n == 5);          /* full length reported, like vsnprintf */
    assert(strcmp(buf, "hel") == 0); /* but output is truncated + NUL-terminated */
}

static void test_zero_size_buffer(void) {
    /* Must not write to buf at all, and must not crash. */
    size_t n = ksnprintf(NULL, 0, "hello");
    assert(n == 5);
}

int main(void) {
    test_literal_text();
    test_percent_d();
    test_percent_u_x();
    test_percent_p();
    test_percent_s_c();
    test_percent_literal();
    test_unknown_conversion_passthrough();
    test_truncation();
    test_zero_size_buffer();
    return 0;
}
