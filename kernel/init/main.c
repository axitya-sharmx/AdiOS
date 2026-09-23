#include <stddef.h>
#include <stdint.h>
#include "../logging/serial.h"
#include "../../arch/x86_64/cpu/gdt.h"
#include "../../arch/x86_64/cpu/percpu.h"
#include "../../arch/x86_64/interrupts/idt.h"
#include "../../mm/pmm/pmm.h"
#include "../../drivers/pci/pci.h"
#include "../logging/log.h"

static void serial_write_uint(uint64_t v) {
    char buf[21];
    int i = 20;
    buf[i] = '\0';
    do {
        buf[--i] = '0' + (v % 10);
        v /= 10;
    } while (v);
    serial_write(&buf[i]);
}

/* Round-trips a few allocations of different orders through the buddy
 * allocator and checks that freeing them all restores the free-byte count,
 * i.e. nothing leaked and coalescing worked. */
static int pmm_self_test(void) {
    uint64_t before = pmm_free_bytes();

    uint64_t a = pmm_alloc(0);
    uint64_t b = pmm_alloc(0);
    uint64_t c = pmm_alloc(2);
    if (!a || !b || !c) {
        return 0;
    }

    pmm_free(a, 0);
    pmm_free(b, 0);
    pmm_free(c, 2);

    return pmm_free_bytes() == before;
}

void kernel_main(uint64_t multiboot_info_addr) {
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

    if (!pmm_init(multiboot_info_addr)) {
        serial_write("[PMM ] no usable memory region found\n");
    } else {
        serial_write("[PMM ] buddy allocator ready, total_bytes=");
        serial_write_uint(pmm_total_bytes());
        serial_write("\n");

        if (pmm_self_test()) {
            serial_write("[PMM ] self-test passed\n");
        } else {
            serial_write("[PMM ] self-test FAILED\n");
        }
    }

    struct pci_device pci_devices[PCI_MAX_DEVICES];
    size_t pci_count = pci_enumerate(pci_devices, PCI_MAX_DEVICES);
    kprintf("[PCI ] %u device(s) found\n", (unsigned int)pci_count);
    pci_log_devices(pci_devices, pci_count);

    serial_write("[INIT] Kernel initialized\n");

#ifdef TRIGGER_TEST_FAULT
    __asm__ volatile("int3");
#endif

    for (;;) {
        __asm__ volatile("hlt");
    }
}
