#pragma once

#include <stddef.h>

void *memset(void *dst, int value, size_t count);
void *memcpy(void *restrict dst, const void *restrict src, size_t count);
void *memmove(void *dst, const void *src, size_t count);
int memcmp(const void *a, const void *b, size_t count);

size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t count);
