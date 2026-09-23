#include "idt.h"
#include <stdint.h>

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

#define IDT_ENTRIES 256
#define KERNEL_CODE_SELECTOR 0x08
#define IDT_TYPE_INTERRUPT_GATE 0x8E /* present, DPL0, 64-bit interrupt gate */

static struct idt_entry g_idt[IDT_ENTRIES];
static struct idt_pointer g_idt_ptr;

extern void *isr_stub_table[32];

static void idt_set_gate(int vector, void (*handler)(void)) {
    uint64_t addr = (uint64_t)handler;
    struct idt_entry *e = &g_idt[vector];
    e->offset_low = addr & 0xFFFF;
    e->selector = KERNEL_CODE_SELECTOR;
    e->ist = 0;
    e->type_attr = IDT_TYPE_INTERRUPT_GATE;
    e->offset_mid = (addr >> 16) & 0xFFFF;
    e->offset_high = (addr >> 32) & 0xFFFFFFFF;
    e->reserved = 0;
}

void idt_init(void) {
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, (void (*)(void))isr_stub_table[i]);
    }

    g_idt_ptr.limit = sizeof(g_idt) - 1;
    g_idt_ptr.base = (uint64_t)&g_idt;

    __asm__ volatile("lidt (%0)" : : "r"(&g_idt_ptr));
}
