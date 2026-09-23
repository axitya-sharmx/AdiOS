#pragma once

/* Spec section 32/33: the lowest-level mutual exclusion primitive.
 * Single-CPU today (no SMP until spec section 81), so acquiring an
 * already-held lock would just spin forever with nothing else ever able
 * to run and release it — see spinlock_acquire()'s comment. Its job right
 * now is to establish the API and the memory-ordering contract that
 * every higher-level lock (mutex, rwlock, ...) and SMP itself will
 * depend on, not to already be safe to hold across a yield or an IRQ. */
struct spinlock {
    volatile int locked;
};

void spinlock_init(struct spinlock *lock);

/* Blocks until the lock is acquired. Rule (spec section 33): never sleep
 * or yield while holding one — there's nothing here yet to stop you, but
 * every caller must honor it regardless, since a spinlock is specified
 * to protect the shortest possible critical section, not to be a
 * general-purpose blocking lock. */
void spinlock_acquire(struct spinlock *lock);

void spinlock_release(struct spinlock *lock);

/* Non-blocking: returns 1 if the lock was free and is now held, 0 if it
 * was already held (and does nothing in that case). */
int spinlock_try_acquire(struct spinlock *lock);
