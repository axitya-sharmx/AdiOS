#include "irq.h"
#include "pic.h"

static irq_handler_fn g_handlers[16];

void irq_install_handler(unsigned irq, irq_handler_fn fn) {
    if (irq >= 16) {
        return;
    }
    g_handlers[irq] = fn;
    pic_clear_mask(irq);
}

void irq_handler(struct registers *regs) {
    unsigned irq = (unsigned)(regs->vector - PIC1_VECTOR_OFFSET);
    if (irq < 16 && g_handlers[irq]) {
        g_handlers[irq](regs);
    }
    pic_send_eoi(irq);
}
