#include "thread.h"
#include "../scheduler/scheduler.h"
#include "../../kernel/heap/heap.h"

#define THREAD_STACK_SIZE (16 * 1024)
#define MAX_THREADS 8

static struct thread g_pool[MAX_THREADS];
static int g_pool_used;

static void thread_trampoline(void) {
    struct thread *self = scheduler_current();
    self->entry(self->arg);
    thread_exit();
}

struct thread *thread_create(thread_entry_fn entry, void *arg) {
    if (g_pool_used >= MAX_THREADS) {
        return NULL;
    }

    uint8_t *stack = kmalloc(THREAD_STACK_SIZE);
    if (!stack) {
        return NULL;
    }

    struct thread *t = &g_pool[g_pool_used++];
    t->id = g_pool_used; /* 1-based; matches post-increment above */
    t->state = THREAD_READY;
    t->stack = stack;
    t->entry = entry;
    t->arg = arg;

    /* Prime the stack so the first context_switch into this thread pops
     * six (don't-care) callee-saved registers and then `ret`s into the
     * trampoline — see arch/x86_64/cpu/switch.S for what it expects. */
    uint64_t sp = (uint64_t)(stack + THREAD_STACK_SIZE);
    sp -= sizeof(uint64_t);
    *(uint64_t *)sp = (uint64_t)thread_trampoline;
    sp -= 6 * sizeof(uint64_t);
    t->rsp = sp;

    scheduler_enqueue(t);
    return t;
}

void thread_exit(void) {
    scheduler_terminate_current();
}
