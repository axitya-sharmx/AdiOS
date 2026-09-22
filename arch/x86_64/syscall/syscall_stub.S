/* int 0x80 entry point. Same trap-frame layout and register-save/restore
 * convention as arch/x86_64/interrupts/isr_stubs.S — kept as its own file
 * rather than folded into that table since it's DPL3 and semantically a
 * different kind of gate (a deliberate call, not a fault or IRQ). */
.code64
.global syscall_entry_stub
.extern syscall_dispatch
syscall_entry_stub:
    pushq $0
    pushq $0x80

    push %r15
    push %r14
    push %r13
    push %r12
    push %r11
    push %r10
    push %r9
    push %r8
    push %rbp
    push %rdi
    push %rsi
    push %rdx
    push %rcx
    push %rbx
    push %rax

    mov %rsp, %rdi
    call syscall_dispatch

    pop %rax
    pop %rbx
    pop %rcx
    pop %rdx
    pop %rsi
    pop %rdi
    pop %rbp
    pop %r8
    pop %r9
    pop %r10
    pop %r11
    pop %r12
    pop %r13
    pop %r14
    pop %r15

    add $16, %rsp
    iretq
