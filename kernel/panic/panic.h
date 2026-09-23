#pragma once

/* Prints "KERNEL PANIC: " + the formatted message (kprintf-style, see
 * kernel/logging/log.h) to serial, then halts the CPU with interrupts
 * disabled. Never returns. For a CPU exception's own panic path (with a
 * full register dump) see arch/x86_64/interrupts/isr.c instead — this is
 * for software-detected failures (assertion failures, OOM, an invariant
 * a subsystem checks for itself) that have no `struct registers` to dump. */
_Noreturn void panic(const char *fmt, ...);
