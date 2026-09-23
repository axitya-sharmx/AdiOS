BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/iso

CC := gcc
AS := gcc
CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -Wall -Wextra -c
ASFLAGS := -c
LDFLAGS := -T linker/linker.ld -ffreestanding -O2 -nostdlib -static

C_SOURCES := kernel/init/main.c kernel/logging/serial.c \
             arch/x86_64/cpu/gdt.c arch/x86_64/cpu/percpu.c \
             arch/x86_64/interrupts/idt.c arch/x86_64/interrupts/isr.c
ASM_SOURCES := arch/x86_64/boot/boot.S arch/x86_64/interrupts/isr_stubs.S

OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES)) \
           $(patsubst %.S,$(BUILD_DIR)/%.o,$(ASM_SOURCES))

KERNEL := $(BUILD_DIR)/kernel.elf
ISO := $(BUILD_DIR)/adios.iso

.PHONY: all iso run clean

all: $(KERNEL)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(KERNEL): $(OBJECTS) linker/linker.ld
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

iso: $(KERNEL)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.elf
	cp boot/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISO_DIR)

run: iso
	qemu-system-x86_64 -cdrom $(ISO) -serial stdio -display none -no-reboot -no-shutdown

# Exercises the IDT/ISR path: triggers INT3 after init and expects the fault
# dump on serial instead of the normal idle loop.
iso-fault-test:
	$(MAKE) clean
	$(MAKE) iso CFLAGS="$(CFLAGS) -DTRIGGER_TEST_FAULT"

clean:
	rm -rf $(BUILD_DIR)
