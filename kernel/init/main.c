#include <stdint.h>
#include "../logging/serial.h"
#include "../../arch/x86_64/cpu/gdt.h"
#include "../../arch/x86_64/cpu/percpu.h"
#include "../../arch/x86_64/interrupts/idt.h"
#include "../../mm/pmm/pmm.h"
#include "../../mm/vmm/vmm.h"

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

/* Maps a freshly allocated physical page at a virtual address boot.S never
 * touched (0x40000000 is past the 1 GiB boot.S identity-maps with huge
 * pages, so this exercises vmm_map actually building new PDPT/PD/PT
 * levels, not just reusing the boot mapping). Writes through the new
 * virtual address, reads the same byte back via the page's physical
 * identity mapping to confirm it lands in the right place, then unmaps
 * and checks the translation is gone. */
static int vmm_self_test(void) {
    const uint64_t test_virt = 0x40000000ULL;
    const uint8_t marker = 0xA5;

    uint64_t phys = pmm_alloc(0);
    if (!phys) {
        return 0;
    }

    int ok = 1;
    ok &= vmm_map(test_virt, phys, VMM_WRITABLE);
    ok &= (vmm_translate(test_virt) == phys);

    *(volatile uint8_t *)test_virt = marker;
    ok &= (*(volatile uint8_t *)phys == marker);

    vmm_unmap(test_virt);
    ok &= (vmm_translate(test_virt) == 0);

    pmm_free(phys, 0);
    return ok;
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

        if (vmm_self_test()) {
            serial_write("[VMM ] self-test passed\n");
        } else {
            serial_write("[VMM ] self-test FAILED\n");
        }
    }

    serial_write("[INIT] Kernel initialized\n");

#ifdef TRIGGER_TEST_FAULT
    __asm__ volatile("int3");
#endif

    for (;;) {
        __asm__ volatile("hlt");
    }
}
