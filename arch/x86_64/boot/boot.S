/* Multiboot2 entry: 32-bit protected mode -> long mode -> kernel_main(). */

.section .multiboot
.align 8
multiboot_header:
    .long 0xe85250d6
    .long 0
    .long multiboot_header_end - multiboot_header
    .long -(0xe85250d6 + 0 + (multiboot_header_end - multiboot_header))
    .word 0
    .word 0
    .long 8
multiboot_header_end:

.section .bss
.align 16
stack_bottom:
    .skip 65536
stack_top:

.align 4096
p4_table:
    .skip 4096
p3_table:
    .skip 4096
p2_table:
    .skip 4096

.section .rodata
.align 8
gdt64:
    .quad 0
gdt64_code:
    .quad (1<<43) | (1<<44) | (1<<47) | (1<<53)
gdt64_pointer:
    .word . - gdt64 - 1
    .quad gdt64

.section .text
.code32
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp
    mov %esp, %ebp

    /* identity-map the first 1 GiB with 2 MiB pages */
    mov $p3_table, %eax
    or $0b11, %eax
    mov %eax, (p4_table)

    mov $p2_table, %eax
    or $0b11, %eax
    mov %eax, (p3_table)

    mov $0, %ecx
map_p2_table:
    mov $0x200000, %eax
    mul %ecx
    or $0b10000011, %eax
    mov %eax, p2_table(,%ecx,8)
    inc %ecx
    cmp $512, %ecx
    jne map_p2_table

    /* load P4 into CR3 */
    mov $p4_table, %eax
    mov %eax, %cr3

    /* enable PAE */
    mov %cr4, %eax
    or $(1 << 5), %eax
    mov %eax, %cr4

    /* set long mode bit in EFER */
    mov $0xC0000080, %ecx
    rdmsr
    or $(1 << 8), %eax
    wrmsr

    /* enable paging */
    mov %cr0, %eax
    or $(1 << 31), %eax
    mov %eax, %cr0

    lgdt (gdt64_pointer)
    ljmp $0x8, $long_mode_start

.code64
long_mode_start:
    mov $0, %ax
    mov %ax, %ss
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    mov $stack_top, %rsp
    call kernel_main

hang:
    cli
    hlt
    jmp hang
