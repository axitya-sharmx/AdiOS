#include "syscall.h"
#include "../interrupts/idt.h"

static syscall_fn g_table[SYSCALL_MAX];

extern void syscall_entry_stub(void);

void syscall_init(void) {
    idt_set_user_gate(SYSCALL_VECTOR, syscall_entry_stub);
}

void syscall_register(int num, syscall_fn fn) {
    if (num >= 0 && num < SYSCALL_MAX) {
        g_table[num] = fn;
    }
}

void syscall_dispatch(struct registers *regs) {
    long num = (long)regs->rax;
    if (num < 0 || num >= SYSCALL_MAX || !g_table[num]) {
        regs->rax = (uint64_t)-1;
        return;
    }
    regs->rax = (uint64_t)g_table[num](regs);
}
