#pragma once

#include "spinlock.h"

/* RAII wrapper over the C spinlock_t (OS_MASTER_SPEC.md §4.2: "resource
 * wrappers" / §4.3: prefer RAII). Non-copyable, non-movable — a lock
 * guard's identity is its scope; letting it move would make "who releases
 * the lock" ambiguous, which is exactly the kind of lifetime bug C++ is
 * meant to make hard to express here (§4.4). */
class SpinlockGuard {
public:
    explicit SpinlockGuard(spinlock_t &lock) : lock_(lock) {
        spinlock_lock(&lock_);
    }

    ~SpinlockGuard() {
        spinlock_unlock(&lock_);
    }

    SpinlockGuard(const SpinlockGuard &) = delete;
    SpinlockGuard &operator=(const SpinlockGuard &) = delete;
    SpinlockGuard(SpinlockGuard &&) = delete;
    SpinlockGuard &operator=(SpinlockGuard &&) = delete;

private:
    spinlock_t &lock_;
};

/* Same, but for a section that must also run with interrupts disabled
 * (state shared with an interrupt/exception handler) — see spinlock.h's
 * note on why irqsave is the variant worth using pre-SMP. */
class SpinlockGuardIrqsave {
public:
    explicit SpinlockGuardIrqsave(spinlock_t &lock) : lock_(lock) {
        flags_ = spinlock_lock_irqsave(&lock_);
    }

    ~SpinlockGuardIrqsave() {
        spinlock_unlock_irqrestore(&lock_, flags_);
    }

    SpinlockGuardIrqsave(const SpinlockGuardIrqsave &) = delete;
    SpinlockGuardIrqsave &operator=(const SpinlockGuardIrqsave &) = delete;
    SpinlockGuardIrqsave(SpinlockGuardIrqsave &&) = delete;
    SpinlockGuardIrqsave &operator=(SpinlockGuardIrqsave &&) = delete;

private:
    spinlock_t &lock_;
    unsigned long flags_;
};
