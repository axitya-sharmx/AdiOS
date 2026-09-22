#include "percpu.h"

#define IA32_GS_BASE 0xC0000101

/* Single static instance until SMP (spec §81) brings up more CPUs. */
static struct percpu g_boot_cpu;

static void write_msr(uint32_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

void percpu_init(uint32_t cpu_id) {
    g_boot_cpu.self = &g_boot_cpu;
    g_boot_cpu.cpu_id = cpu_id;
    write_msr(IA32_GS_BASE, (uint64_t)&g_boot_cpu);
}

struct percpu *percpu_current(void) {
    struct percpu *self;
    __asm__ volatile("mov %%gs:0, %0" : "=r"(self));
    return self;
}
