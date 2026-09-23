#include "log.h"

#include <stdint.h>

#include "serial.h"

#define KPRINTF_BUF_SIZE 512

/* Writes the string representation of `value` in `base` (2-16) to `out`,
 * appending it starting at `*pos` (bounded by `size`), most-significant
 * digit first. `uppercase` selects A-F vs a-f for base 16. */
static void append_uint(char *out, size_t size, size_t *pos, uintmax_t value,
                         unsigned base, int uppercase) {
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    char digit_buf[32]; /* enough for a 64-bit value in base 2 */
    size_t n = 0;

    do {
        digit_buf[n++] = digits[value % base];
        value /= base;
    } while (value != 0 && n < sizeof(digit_buf));

    while (n > 0) {
        if (*pos < size) {
            out[*pos] = digit_buf[n - 1];
        }
        (*pos)++;
        n--;
    }
}

static void append_str(char *out, size_t size, size_t *pos, const char *s) {
    for (; *s; s++) {
        if (*pos < size) {
            out[*pos] = *s;
        }
        (*pos)++;
    }
}

static void append_char(char *out, size_t size, size_t *pos, char c) {
    if (*pos < size) {
        out[*pos] = c;
    }
    (*pos)++;
}

size_t kvsnprintf(char *buf, size_t size, const char *fmt, va_list args) {
    size_t pos = 0;

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            append_char(buf, size, &pos, *p);
            continue;
        }

        p++;
        switch (*p) {
        case 'd': {
            int v = va_arg(args, int);
            if (v < 0) {
                append_char(buf, size, &pos, '-');
                append_uint(buf, size, &pos, (uintmax_t)(-(intmax_t)v), 10, 0);
            } else {
                append_uint(buf, size, &pos, (uintmax_t)v, 10, 0);
            }
            break;
        }
        case 'u':
            append_uint(buf, size, &pos, (uintmax_t)va_arg(args, unsigned int), 10, 0);
            break;
        case 'x':
            append_uint(buf, size, &pos, (uintmax_t)va_arg(args, unsigned int), 16, 0);
            break;
        case 'p':
            append_str(buf, size, &pos, "0x");
            append_uint(buf, size, &pos, (uintmax_t)(uintptr_t)va_arg(args, void *), 16, 0);
            break;
        case 's':
            append_str(buf, size, &pos, va_arg(args, const char *));
            break;
        case 'c':
            append_char(buf, size, &pos, (char)va_arg(args, int));
            break;
        case '%':
            append_char(buf, size, &pos, '%');
            break;
        case '\0':
            /* Trailing '%' with nothing after it: emit literally and stop. */
            append_char(buf, size, &pos, '%');
            goto done;
        default:
            /* Unknown conversion: emit both characters verbatim so a typo
             * is visible in the output instead of silently eating input. */
            append_char(buf, size, &pos, '%');
            append_char(buf, size, &pos, *p);
            break;
        }
    }

done:
    if (size > 0) {
        buf[pos < size ? pos : size - 1] = '\0';
    }
    return pos;
}

size_t ksnprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t n = kvsnprintf(buf, size, fmt, args);
    va_end(args);
    return n;
}

void kprintf(const char *fmt, ...) {
    char buf[KPRINTF_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    kvsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    serial_write(buf);
}
