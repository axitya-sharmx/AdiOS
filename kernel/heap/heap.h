#pragma once

#include <stddef.h>

void *kmalloc(size_t nbytes);
void kfree(void *ptr);
