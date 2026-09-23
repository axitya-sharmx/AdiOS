#pragma once

/* Single-CPU-correct spinlock: safe today (one bootstrap CPU only) and
 * the right shape for SMP later, but it does not yet do anything a plain
 * IRQ-disable wouldn't — there's no second CPU to contend with. The
 * irqsave/irqrestore variants are the ones worth using now, since they
 * protect state shared with interrupt/exception handlers (e.g. isr_handler)
 * even on a single CPU. */

typedef struct {
    volatile int locked;
} spinlock_t;

#define SPINLOCK_INIT { .locked = 0 }

#ifdef __cplusplus
extern "C" {
#endif

void spinlock_init(spinlock_t *lock);

/* Plain lock/unlock: only safe if the caller knows interrupts are already
 * disabled, or that this lock is never touched from interrupt context.
 * Prefer the _irqsave variants below unless you've actually checked that. */
void spinlock_lock(spinlock_t *lock);
void spinlock_unlock(spinlock_t *lock);
int spinlock_try_lock(spinlock_t *lock); /* returns 1 if acquired, 0 if not */

/* Disables interrupts, acquires the lock, and returns the prior RFLAGS
 * (pass it to spinlock_unlock_irqrestore to restore the caller's
 * interrupt state rather than unconditionally re-enabling interrupts). */
unsigned long spinlock_lock_irqsave(spinlock_t *lock);
void spinlock_unlock_irqrestore(spinlock_t *lock, unsigned long flags);

#ifdef __cplusplus
} /* extern "C" */
#endif
