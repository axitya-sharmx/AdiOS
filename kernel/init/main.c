#include "../logging/serial.h"
#include "../../arch/x86_64/cpu/gdt.h"
#include "../../arch/x86_64/cpu/percpu.h"
#include "../../arch/x86_64/interrupts/idt.h"

static void serial_write_uint(uint32_t v) {
    char buf[11];
    int i = 10;
    buf[i] = '\0';
    do {
        buf[--i] = '0' + (v % 10);
        v /= 10;
    } while (v);
    serial_write(&buf[i]);
}

void kernel_main(void) {
    serial_init();
    serial_write("[BOOT] Kernel starting\n");
    serial_write("[CPU ] x86_64 long mode active\n");

    gdt_init();
    serial_write("[CPU ] GDT/TSS loaded\n");

    idt_init();
    serial_write("[CPU ] IDT loaded, exceptions installed\n");

    percpu_init(0);
    serial_write("[CPU ] per-CPU state ready, cpu_id=");
    serial_write_uint(percpu_current()->cpu_id);
    serial_write("\n");

    serial_write("[INIT] Kernel initialized\n");

#ifdef TRIGGER_TEST_FAULT
    __asm__ volatile("int3");
#endif

    for (;;) {
        __asm__ volatile("hlt");
    }
}
