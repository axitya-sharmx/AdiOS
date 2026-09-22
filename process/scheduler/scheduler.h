#pragma once

#include "../thread/thread.h"

void scheduler_init(void);

/* Called by thread_create() to hand it a newly primed thread; not meant
 * to be called directly. */
void scheduler_enqueue(struct thread *t);

struct thread *scheduler_current(void);

/* Cooperative round-robin (spec section 30.1 — timer-driven preemption is
 * explicit future work, see scheduler.c's header comment): gives up the
 * CPU to the next ready thread, if any, and returns to the caller once
 * scheduled back in. A no-op if no other thread is ready. */
void scheduler_yield(void);

/* Marks the current thread terminated (it is never re-enqueued) and
 * switches away. Never returns. */
__attribute__((noreturn)) void scheduler_terminate_current(void);
