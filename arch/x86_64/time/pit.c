#include "pit.h"
#include "../interrupts/irq.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_BASE_HZ  1193182u

static volatile uint64_t g_ticks;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void pit_irq(struct registers *regs) {
    (void)regs;
    g_ticks++;
}

void pit_init(uint32_t hz) {
    uint32_t divisor = PIT_BASE_HZ / hz;

    outb(PIT_COMMAND, 0x36); /* channel 0, lobyte/hibyte, mode 3 (square wave) */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    irq_install_handler(0, pit_irq);
}

uint64_t pit_ticks(void) {
    return g_ticks;
}
