#pragma once

#include <stdint.h>

struct registers {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t vector, err_code;
    uint64_t rip, cs, rflags, user_rsp, ss;
};

void isr_handler(struct registers *regs);
