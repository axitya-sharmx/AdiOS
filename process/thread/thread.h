#pragma once

#include <stdint.h>

enum thread_state {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_TERMINATED,
};

typedef void (*thread_entry_fn)(void *arg);

struct thread {
    int id;
    enum thread_state state;
    uint64_t rsp;   /* saved stack pointer while not running; garbage while THREAD_RUNNING */
    void *stack;    /* kmalloc'd base, for eventual thread_destroy (spec section 28) */
    thread_entry_fn entry;
    void *arg;
};

/* Allocates a stack and a struct thread, primes the stack so the first
 * context_switch into it starts at entry(arg), and hands it to the
 * scheduler's ready queue. Returns NULL if the thread pool is full or the
 * stack allocation failed. */
struct thread *thread_create(thread_entry_fn entry, void *arg);

/* Called automatically when a thread's entry function returns; can also be
 * called explicitly to end the current thread early. Marks it terminated
 * and switches away. Never returns. */
__attribute__((noreturn)) void thread_exit(void);
