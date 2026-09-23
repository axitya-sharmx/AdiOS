#include "../logging/serial.h"
#include "../../arch/x86_64/cpu/gdt.h"
#include "../../arch/x86_64/interrupts/idt.h"

void kernel_main(void) {
    serial_init();
    serial_write("[BOOT] Kernel starting\n");
    serial_write("[CPU ] x86_64 long mode active\n");

    gdt_init();
    serial_write("[CPU ] GDT/TSS loaded\n");

    idt_init();
    serial_write("[CPU ] IDT loaded, exceptions installed\n");

    serial_write("[INIT] Kernel initialized\n");

#ifdef TRIGGER_TEST_FAULT
    __asm__ volatile("int3");
#endif

    for (;;) {
        __asm__ volatile("hlt");
    }
}
