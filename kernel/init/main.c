#include "../logging/serial.h"

void kernel_main(void) {
    serial_init();
    serial_write("[BOOT] Kernel starting\n");
    serial_write("[CPU ] x86_64 long mode active\n");
    serial_write("[INIT] Kernel initialized\n");

    for (;;) {
        __asm__ volatile("hlt");
    }
}
