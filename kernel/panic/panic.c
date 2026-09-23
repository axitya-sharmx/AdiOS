#include "panic.h"

#include <stdarg.h>

#include "../logging/log.h"

_Noreturn void panic(const char *fmt, ...) {
    char msg[256];
    va_list args;
    va_start(args, fmt);
    kvsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    kprintf("\nKERNEL PANIC: %s\n", msg);

    __asm__ volatile("cli");
    for (;;) {
        __asm__ volatile("hlt");
    }
}
