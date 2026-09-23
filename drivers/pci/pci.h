#pragma once

#include <stddef.h>
#include <stdint.h>

/* PCI configuration space access (mechanism #1, I/O ports 0xCF8/0xCFC —
 * every x86 PCI host bridge since the original PCI spec supports this,
 * including under QEMU/TCG). Enumeration walks all 256 buses x 32
 * device slots x 8 functions and reads the header registers directly;
 * no ACPI/MCFG (memory-mapped, PCIe-only) support yet — see
 * docs/architecture for what that would take. */

#define PCI_MAX_DEVICES 64 /* enough for a QEMU q35/i440fx guest with room */

struct pci_device {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision;
};

/* Builds the CONFIG_ADDRESS dword for mechanism #1: enable bit, bus,
 * device, function, and a register offset aligned down to a dword
 * (the low 2 bits of `offset` are always cleared, since config space is
 * addressed in 32-bit units). Pure/host-testable — no I/O performed. */
uint32_t pci_config_address(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

/* Performs the actual 0xCF8/0xCFC I/O port read. Not host-testable
 * (`out`/`in` on those ports fault outside ring 0). */
uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

/* Scans every bus/slot/function, filling `out` (capacity `out_capacity`)
 * with each present device (vendor ID != 0xFFFF) found, and returns the
 * number written — capped at `out_capacity` even if more devices exist,
 * same "short write, not an error" contract as ring_buffer_write. */
size_t pci_enumerate(struct pci_device *out, size_t out_capacity);

/* Logs one line per device via kprintf: bus:slot.func vendor:device
 * class/subclass/prog_if. */
void pci_log_devices(const struct pci_device *devices, size_t count);
