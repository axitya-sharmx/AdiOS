BUILD_DIR := build
ISO_DIR := $(BUILD_DIR)/iso

CC := gcc
CXX := g++
AS := gcc
CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -Wall -Wextra -c
# See OS_MASTER_SPEC.md §4.3: no exceptions/RTTI, no hosted-runtime
# assumptions (static constructors need a real .init_array + a run-once
# guard neither of which exist yet, so global constructors are unsupported
# until that's wired up — avoid them for now).
CXXFLAGS := -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics \
            -fno-use-cxa-atexit -fno-stack-protector -fno-pic -mno-red-zone \
            -mcmodel=kernel -Wall -Wextra -std=c++20 -c
ASFLAGS := -c
LDFLAGS := -T linker/linker.ld -ffreestanding -O2 -nostdlib -static

C_SOURCES := kernel/init/main.c kernel/logging/serial.c \
             arch/x86_64/cpu/gdt.c arch/x86_64/cpu/percpu.c \
             arch/x86_64/interrupts/idt.c arch/x86_64/interrupts/isr.c \
             mm/pmm/multiboot2.c mm/pmm/pmm.c mm/vmm/vmm.c kernel/heap/heap.c \
             sync/spinlock/spinlock.c
CXX_SOURCES :=
ASM_SOURCES := arch/x86_64/boot/boot.S arch/x86_64/interrupts/isr_stubs.S

OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES)) \
           $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CXX_SOURCES)) \
           $(patsubst %.S,$(BUILD_DIR)/%.o,$(ASM_SOURCES))

KERNEL := $(BUILD_DIR)/kernel.elf
ISO := $(BUILD_DIR)/adios.iso

.PHONY: all iso run test clean

all: $(KERNEL)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< -o $@

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

test: $(BUILD_DIR)/test_spinlock $(BUILD_DIR)/test_spinlock_guard $(BUILD_DIR)/test_span
	$(BUILD_DIR)/test_spinlock
	$(BUILD_DIR)/test_spinlock_guard
	$(BUILD_DIR)/test_span

$(BUILD_DIR)/test_spinlock: tests/unit/test_spinlock.c sync/spinlock/spinlock.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -Wall -Wextra -o $@ $^

$(BUILD_DIR)/test_spinlock_guard: tests/unit/test_spinlock_guard.cpp sync/spinlock/spinlock.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -Wall -Wextra -c sync/spinlock/spinlock.c -o $(BUILD_DIR)/host_spinlock.o
	$(CXX) -std=c++20 -Wall -Wextra tests/unit/test_spinlock_guard.cpp $(BUILD_DIR)/host_spinlock.o -o $@

$(BUILD_DIR)/test_span: tests/unit/test_span.cpp kernel/core/span.hpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) -std=c++20 -Wall -Wextra tests/unit/test_span.cpp -o $@

clean:
	rm -rf $(BUILD_DIR)
