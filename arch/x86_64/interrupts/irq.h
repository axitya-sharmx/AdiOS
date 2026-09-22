#pragma once

#include "isr.h"

typedef void (*irq_handler_fn)(struct registers *regs);

/* Registers `fn` for IRQ `irq` (0-15) and unmasks it on the PIC. */
void irq_install_handler(unsigned irq, irq_handler_fn fn);

void irq_handler(struct registers *regs);
