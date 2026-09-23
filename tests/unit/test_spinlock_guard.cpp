// Host-buildable: covers SpinlockGuard only. SpinlockGuardIrqsave executes
// cli/sti, which fault outside ring 0 — see test_spinlock.c's note.
#include <cassert>

#include "../../sync/spinlock/spinlock_guard.hpp"

static void test_guard_locks_and_unlocks(void) {
    spinlock_t lock = SPINLOCK_INIT;
    {
        SpinlockGuard guard(lock);
        assert(lock.locked == 1);
    }
    assert(lock.locked == 0);
}

static void test_guard_excludes_reentrant_try_lock(void) {
    spinlock_t lock = SPINLOCK_INIT;
    SpinlockGuard guard(lock);
    assert(spinlock_try_lock(&lock) == 0);
}

int main() {
    test_guard_locks_and_unlocks();
    test_guard_excludes_reentrant_try_lock();
    return 0;
}
