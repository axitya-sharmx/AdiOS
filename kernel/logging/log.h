#pragma once

#include <stdarg.h>
#include <stddef.h>

/* Formats into `buf` (always NUL-terminated if size > 0) and returns the
 * number of characters that *would* have been written, excluding the NUL,
 * same convention as vsnprintf. Supported conversions: %d %u %x %p %s %c %%.
 * No width/precision/length modifiers — add them if a caller needs one. */
size_t kvsnprintf(char *buf, size_t size, const char *fmt, va_list args);
size_t ksnprintf(char *buf, size_t size, const char *fmt, ...);

/* Formats and writes straight to the serial console via a fixed-size
 * stack buffer (see KPRINTF_BUF_SIZE in log.c); output longer than that
 * is truncated rather than overflowing. */
void kprintf(const char *fmt, ...);
