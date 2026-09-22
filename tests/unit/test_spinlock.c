/* Host-buildable: covers the non-privileged entry points only
 * (init/try_lock/lock/unlock). spinlock_lock_irqsave/unlock_irqrestore
 * execute cli/sti, which fault outside ring 0 — those are exercised by
 * the boot-time kernel, not this host test. */
#include <assert.h>

#include "../../sync/spinlock/spinlock.h"

static void test_init_is_unlocked(void) {
    spinlock_t lock;
    spinlock_init(&lock);
    assert(lock.locked == 0);
}

static void test_try_lock_then_blocked(void) {
    spinlock_t lock = SPINLOCK_INIT;
    assert(spinlock_try_lock(&lock) == 1);
    assert(spinlock_try_lock(&lock) == 0); /* already held */
    spinlock_unlock(&lock);
    assert(spinlock_try_lock(&lock) == 1); /* free again */
}

static void test_lock_unlock_roundtrip(void) {
    spinlock_t lock = SPINLOCK_INIT;
    spinlock_lock(&lock);
    assert(lock.locked == 1);
    spinlock_unlock(&lock);
    assert(lock.locked == 0);
}

int main(void) {
    test_init_is_unlocked();
    test_try_lock_then_blocked();
    test_lock_unlock_roundtrip();
    return 0;
}
