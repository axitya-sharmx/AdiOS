#include "spinlock.h"

void spinlock_init(spinlock_t *lock) {
    __atomic_store_n(&lock->locked, 0, __ATOMIC_RELAXED);
}

int spinlock_try_lock(spinlock_t *lock) {
    int expected = 0;
    return __atomic_compare_exchange_n(&lock->locked, &expected, 1,
                                        /*weak=*/0, __ATOMIC_ACQUIRE,
                                        __ATOMIC_RELAXED);
}

void spinlock_lock(spinlock_t *lock) {
    while (!spinlock_try_lock(lock)) {
        /* Spin on a plain read first so the cache line stays shared while
         * contended, only retrying the (expensive, always-dirties-the-line)
         * compare-exchange once it looks free. */
        while (__atomic_load_n(&lock->locked, __ATOMIC_RELAXED)) {
            __asm__ volatile("pause");
        }
    }
}

void spinlock_unlock(spinlock_t *lock) {
    __atomic_store_n(&lock->locked, 0, __ATOMIC_RELEASE);
}

unsigned long spinlock_lock_irqsave(spinlock_t *lock) {
    unsigned long flags;
    __asm__ volatile("pushfq; pop %0; cli" : "=r"(flags)::"memory");
    spinlock_lock(lock);
    return flags;
}

void spinlock_unlock_irqrestore(spinlock_t *lock, unsigned long flags) {
    spinlock_unlock(lock);
    /* Only re-enable interrupts if they were enabled before we disabled
     * them (bit 9 of RFLAGS is IF) — a nested irqsave section must not
     * turn interrupts back on early. */
    if (flags & (1UL << 9)) {
        __asm__ volatile("sti" ::: "memory");
    }
}
