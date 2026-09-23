/* void context_switch(uint64_t *old_rsp, uint64_t new_rsp);
 *
 * Saves the callee-saved registers (the caller-saved ones are already on
 * the stack or dead by the System V ABI at a call site) onto the current
 * stack, stashes the resulting rsp through `old_rsp`, switches to
 * `new_rsp`, and pops the same six registers back off — which, for a
 * thread that has never run yet, are just don't-care values primed by
 * thread_create(); see process/thread/thread.c.
 */
.section .text
.code64
.global context_switch
.type context_switch, @function
context_switch:
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, (%rdi)
    mov %rsi, %rsp

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx
    ret
