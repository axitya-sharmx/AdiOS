BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/iso

CC := gcc
AS := gcc
CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -Wall -Wextra -c
ASFLAGS := -c
LDFLAGS := -T linker/linker.ld -ffreestanding -O2 -nostdlib -static

C_SOURCES := kernel/init/main.c kernel/logging/serial.c
ASM_SOURCES := arch/x86_64/boot/boot.S

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

clean:
	rm -rf $(BUILD_DIR)
