/* Cooperative round-robin scheduler (spec section 30.1).
 *
 * Threads only switch at an explicit scheduler_yield() call, not on a
 * timer tick — preemptive time-slicing needs the IRQ path (Phase 7) to
 * call into this from interrupt context, which is real work (saving/
 * restoring a full trap frame instead of just the callee-saved registers
 * context_switch handles) that deserves its own change once there's a
 * workload to preempt. Tracked as explicit follow-up, not an oversight.
 */
#include "scheduler.h"

extern void context_switch(uint64_t *old_rsp, uint64_t new_rsp);

#define READY_QUEUE_CAP 8

static struct thread *g_ready[READY_QUEUE_CAP];
static int g_head, g_tail, g_count;
static struct thread *g_current;
static struct thread g_main_thread; /* represents whatever called scheduler_init() */

static void enqueue(struct thread *t) {
    g_ready[g_tail] = t;
    g_tail = (g_tail + 1) % READY_QUEUE_CAP;
    g_count++;
}

static struct thread *dequeue(void) {
    struct thread *t = g_ready[g_head];
    g_head = (g_head + 1) % READY_QUEUE_CAP;
    g_count--;
    return t;
}

void scheduler_init(void) {
    g_main_thread.id = 0;
    g_main_thread.state = THREAD_RUNNING;
    g_current = &g_main_thread;
}

void scheduler_enqueue(struct thread *t) {
    enqueue(t);
}

struct thread *scheduler_current(void) {
    return g_current;
}

static void switch_to(struct thread *next) {
    struct thread *prev = g_current;
    next->state = THREAD_RUNNING;
    g_current = next;
    context_switch(&prev->rsp, next->rsp);
}

void scheduler_yield(void) {
    if (g_count == 0) {
        return;
    }

    struct thread *prev = g_current;
    if (prev->state == THREAD_RUNNING) {
        prev->state = THREAD_READY;
        enqueue(prev);
    }
    switch_to(dequeue());
}

void scheduler_terminate_current(void) {
    g_current->state = THREAD_TERMINATED;

    if (g_count == 0) {
        /* No idle thread yet (spec section 30 hasn't reached that far):
         * nothing left to run. Halt rather than resume a dead context. */
        __asm__ volatile("cli");
        for (;;) {
            __asm__ volatile("hlt");
        }
    }

    switch_to(dequeue());

    /* Unreachable: this thread was never re-enqueued, so nothing can
     * switch back into it. Guard against a scheduler bug silently
     * resuming a terminated thread's stale stack. */
    for (;;) {
        __asm__ volatile("hlt");
    }
}
