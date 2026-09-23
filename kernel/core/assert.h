#pragma once

#include "../panic/panic.h"

/* Freestanding assert: the standard <assert.h> assumes a hosted abort(),
 * which doesn't exist here. KASSERT always evaluates its condition
 * (there's no NDEBUG-style stripping) — a kernel invariant check is cheap
 * next to what it guards against, and a silently-skipped check is worse
 * than a slightly slower build. */
#define KASSERT(cond)                                                        \
    do {                                                                     \
        if (!(cond)) {                                                       \
            panic("assertion failed: %s, at %s:%d", #cond, __FILE__,         \
                  __LINE__);                                                 \
        }                                                                    \
    } while (0)
