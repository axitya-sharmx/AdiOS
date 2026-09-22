#include "isr.h"
#include "../../../kernel/logging/serial.h"

static const char *exception_name(uint64_t vector) {
    static const char *names[32] = {
        "Divide Error", "Debug", "NMI", "Breakpoint",
        "Overflow", "Bound Range Exceeded", "Invalid Opcode", "Device Not Available",
        "Double Fault", "Coprocessor Segment Overrun", "Invalid TSS", "Segment Not Present",
        "Stack Fault", "General Protection Fault", "Page Fault", "Reserved",
        "x87 Floating Point", "Alignment Check", "Machine Check", "SIMD Floating Point",
        "Virtualization", "Control Protection", "Reserved", "Reserved",
        "Reserved", "Reserved", "Reserved", "Reserved",
        "Hypervisor Injection", "VMM Communication", "Security", "Reserved",
    };
    return vector < 32 ? names[vector] : "Unknown";
}

static void write_hex64(uint64_t v) {
    char buf[19];
    const char *digits = "0123456789abcdef";
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 16; i++) {
        buf[17 - i] = digits[v & 0xF];
        v >>= 4;
    }
    buf[18] = '\0';
    serial_write(buf);
}

static void write_field(const char *label, uint64_t value) {
    serial_write(label);
    write_hex64(value);
    serial_write("\n");
}

void isr_handler(struct registers *regs) {
    serial_write("\nKERNEL PANIC\n\n");
    serial_write("Reason: ");
    serial_write(exception_name(regs->vector));
    serial_write("\n\n");

    write_field("RIP:    ", regs->rip);
    write_field("CS:     ", regs->cs);
    write_field("RFLAGS: ", regs->rflags);
    write_field("RSP:    ", regs->user_rsp);
    write_field("SS:     ", regs->ss);
    write_field("ERR:    ", regs->err_code);

    serial_write("\nRegisters:\n");
    write_field("RAX: ", regs->rax);
    write_field("RBX: ", regs->rbx);
    write_field("RCX: ", regs->rcx);
    write_field("RDX: ", regs->rdx);
    write_field("RSI: ", regs->rsi);
    write_field("RDI: ", regs->rdi);
    write_field("RBP: ", regs->rbp);
    write_field("R8:  ", regs->r8);
    write_field("R9:  ", regs->r9);
    write_field("R10: ", regs->r10);
    write_field("R11: ", regs->r11);
    write_field("R12: ", regs->r12);
    write_field("R13: ", regs->r13);
    write_field("R14: ", regs->r14);
    write_field("R15: ", regs->r15);

    __asm__ volatile("cli");
    for (;;) {
        __asm__ volatile("hlt");
    }
}
