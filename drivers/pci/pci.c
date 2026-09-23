#include "pci.h"

#include "../../kernel/logging/log.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC
#define PCI_VENDOR_ID_NONE 0xFFFF

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint32_t pci_config_address(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t enable_bit = 1u << 31;
    return enable_bit | ((uint32_t)bus << 16) | (((uint32_t)slot & 0x1F) << 11) |
           (((uint32_t)func & 0x07) << 8) | (offset & 0xFC);
}

uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    outl(PCI_CONFIG_ADDRESS, pci_config_address(bus, slot, func, offset));
    return inl(PCI_CONFIG_DATA);
}

/* Register 0x00: device ID (high 16) : vendor ID (low 16). */
static uint16_t read_vendor_id(uint8_t bus, uint8_t slot, uint8_t func) {
    return (uint16_t)(pci_config_read32(bus, slot, func, 0x00) & 0xFFFF);
}

static void read_device_header(uint8_t bus, uint8_t slot, uint8_t func,
                                struct pci_device *dev) {
    uint32_t reg0 = pci_config_read32(bus, slot, func, 0x00);
    uint32_t reg2 = pci_config_read32(bus, slot, func, 0x08);

    dev->bus = bus;
    dev->slot = slot;
    dev->func = func;
    dev->vendor_id = (uint16_t)(reg0 & 0xFFFF);
    dev->device_id = (uint16_t)(reg0 >> 16);
    dev->revision = (uint8_t)(reg2 & 0xFF);
    dev->prog_if = (uint8_t)((reg2 >> 8) & 0xFF);
    dev->subclass = (uint8_t)((reg2 >> 16) & 0xFF);
    dev->class_code = (uint8_t)((reg2 >> 24) & 0xFF);
}

/* Register 0x0C bits 16-23: header type. Bit 7 set means multi-function;
 * function 0 always exists if the slot is populated at all, so this is
 * only checked once we already know func 0 is present. */
static int is_multifunction(uint8_t bus, uint8_t slot) {
    uint32_t reg3 = pci_config_read32(bus, slot, 0, 0x0C);
    uint8_t header_type = (uint8_t)((reg3 >> 16) & 0xFF);
    return (header_type & 0x80) != 0;
}

size_t pci_enumerate(struct pci_device *out, size_t out_capacity) {
    size_t count = 0;

    for (int bus = 0; bus <= 0xFF; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            if (read_vendor_id((uint8_t)bus, (uint8_t)slot, 0) == PCI_VENDOR_ID_NONE) {
                continue;
            }

            int max_func = is_multifunction((uint8_t)bus, (uint8_t)slot) ? 8 : 1;
            for (int func = 0; func < max_func; func++) {
                if (read_vendor_id((uint8_t)bus, (uint8_t)slot, (uint8_t)func) ==
                    PCI_VENDOR_ID_NONE) {
                    continue;
                }
                if (count >= out_capacity) {
                    return count;
                }
                read_device_header((uint8_t)bus, (uint8_t)slot, (uint8_t)func,
                                    &out[count]);
                count++;
            }
        }
    }

    return count;
}

void pci_log_devices(const struct pci_device *devices, size_t count) {
    for (size_t i = 0; i < count; i++) {
        const struct pci_device *d = &devices[i];
        kprintf("[PCI ] %u:%u.%u vendor=%x device=%x class=%x subclass=%x prog_if=%x\n",
                d->bus, d->slot, d->func, d->vendor_id, d->device_id,
                d->class_code, d->subclass, d->prog_if);
    }
}
