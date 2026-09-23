#include "spinlock.h"

void spinlock_init(struct spinlock *lock) {
    lock->locked = 0;
}

void spinlock_acquire(struct spinlock *lock) {
    while (__atomic_exchange_n(&lock->locked, 1, __ATOMIC_ACQUIRE) == 1) {
        __asm__ volatile("pause"); /* hint the CPU this is a spin loop, not real work */
    }
}

void spinlock_release(struct spinlock *lock) {
    __atomic_store_n(&lock->locked, 0, __ATOMIC_RELEASE);
}

int spinlock_try_acquire(struct spinlock *lock) {
    return __atomic_exchange_n(&lock->locked, 1, __ATOMIC_ACQUIRE) == 0;
}
