#include "gdt.h"
#include <stdint.h>

#define GDT_PRESENT   (1ULL << 47)
#define GDT_NOTSYS    (1ULL << 44)
#define GDT_EXEC      (1ULL << 43)
#define GDT_RW        (1ULL << 41)
#define GDT_DPL3      (3ULL << 45)
#define GDT_LONG      (1ULL << 53)

#define KERNEL_CODE (GDT_PRESENT | GDT_NOTSYS | GDT_EXEC | GDT_RW | GDT_LONG)
#define KERNEL_DATA (GDT_PRESENT | GDT_NOTSYS | GDT_RW)
#define USER_CODE   (KERNEL_CODE | GDT_DPL3)
#define USER_DATA   (KERNEL_DATA | GDT_DPL3)

#define TSS_TYPE_AVAILABLE 0x9

struct tss {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

struct gdt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

#define KERNEL_STACK_SIZE 16384
static uint8_t kernel_stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));

static struct tss g_tss;
static uint64_t g_gdt[7];
static struct gdt_pointer g_gdt_ptr;

static uint64_t tss_low_descriptor(uint64_t base, uint32_t limit) {
    return (limit & 0xFFFF)
        | ((base & 0xFFFFFF) << 16)
        | ((uint64_t)TSS_TYPE_AVAILABLE << 40)
        | GDT_PRESENT
        | (((base >> 24) & 0xFF) << 56);
}

static void load_gdt(struct gdt_pointer *ptr) {
    __asm__ volatile("lgdt (%0)" : : "r"(ptr));
}

static void load_tr(uint16_t selector) {
    __asm__ volatile("ltr %0" : : "r"(selector));
}

void gdt_init(void) {
    g_tss.rsp0 = (uint64_t)(kernel_stack + KERNEL_STACK_SIZE);
    g_tss.iomap_base = sizeof(struct tss);

    uint64_t tss_base = (uint64_t)&g_tss;
    uint32_t tss_limit = sizeof(struct tss) - 1;

    g_gdt[0] = 0; /* null */
    g_gdt[1] = KERNEL_CODE;
    g_gdt[2] = KERNEL_DATA;
    g_gdt[3] = USER_CODE;
    g_gdt[4] = USER_DATA;
    g_gdt[5] = tss_low_descriptor(tss_base, tss_limit);
    g_gdt[6] = tss_base >> 32; /* high 32 bits of TSS base */

    g_gdt_ptr.limit = sizeof(g_gdt) - 1;
    g_gdt_ptr.base = (uint64_t)&g_gdt;

    load_gdt(&g_gdt_ptr);

    /* Reload every data segment from the new GDT, and CS via a far return
     * (there's no direct far-jump-to-label form in AT&T inline asm) so the
     * CPU actually starts using the new descriptors instead of the ones
     * boot.S's temporary GDT left behind. */
    __asm__ volatile(
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "pushq $0x08\n"
        "lea 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        : : : "rax", "memory");

    load_tr(0x28);
}
