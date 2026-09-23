#pragma once

void idt_init(void);

/* Installs `handler` at `vector` as a DPL3 interrupt gate, i.e. callable
 * via `int vector` from ring 3 (every other gate idt_init() installs is
 * DPL0: only the CPU itself, via an exception or IRQ, can enter them).
 * For the syscall vector (arch/x86_64/syscall). */
void idt_set_user_gate(int vector, void (*handler)(void));
