.macro irq_stub num
.global irq\num
irq\num:
    pushq $0
    pushq $(32 + \num)
    jmp irq_common_stub
.endm

.code64

irq_stub 0
irq_stub 1
irq_stub 2
irq_stub 3
irq_stub 4
irq_stub 5
irq_stub 6
irq_stub 7
irq_stub 8
irq_stub 9
irq_stub 10
irq_stub 11
irq_stub 12
irq_stub 13
irq_stub 14
irq_stub 15

.extern irq_handler
irq_common_stub:
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
    call irq_handler

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

.section .data
.align 8
.global irq_stub_table
irq_stub_table:
    .quad irq0, irq1, irq2,  irq3,  irq4,  irq5,  irq6,  irq7
    .quad irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15
