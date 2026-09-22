#pragma once

#include "../interrupts/isr.h"

#define SYSCALL_VECTOR 0x80
#define SYSCALL_MAX 8

/* A syscall handler reads its arguments from `regs` (spec section 52
 * leaves the exact ABI ours to define; for now: number in rax, first
 * argument in rdi) and returns the value that ends up back in the
 * caller's rax. */
typedef long (*syscall_fn)(struct registers *regs);

/* Installs the int 0x80 gate (DPL3, so ring 3 can reach it once there's
 * user-mode code to call it — spec section 51/71). */
void syscall_init(void);

void syscall_register(int num, syscall_fn fn);
