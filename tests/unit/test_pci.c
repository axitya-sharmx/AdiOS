/* Host-buildable: covers pci_config_address only, the pure bit-packing
 * logic. pci_config_read32/pci_enumerate execute outl/inl, which fault
 * outside ring 0 — those are exercised by the real kernel. */
#include <assert.h>

#include "../../drivers/pci/pci.h"

static void test_enable_bit_always_set(void) {
    uint32_t addr = pci_config_address(0, 0, 0, 0);
    assert((addr & (1u << 31)) != 0);
}

static void test_bus_slot_func_placement(void) {
    uint32_t addr = pci_config_address(0x12, 0x03, 0x05, 0x00);
    assert(((addr >> 16) & 0xFF) == 0x12);
    assert(((addr >> 11) & 0x1F) == 0x03);
    assert(((addr >> 8) & 0x07) == 0x05);
}

static void test_offset_dword_aligned(void) {
    /* Offset 0x07 must be masked down to 0x04 (config space is
     * addressed in 32-bit units; the low 2 bits select nothing). */
    uint32_t addr = pci_config_address(0, 0, 0, 0x07);
    assert((addr & 0xFF) == 0x04);
}

static void test_slot_func_are_masked(void) {
    /* Out-of-range slot (>5 bits) / func (>3 bits) must not bleed into
     * adjacent fields. */
    uint32_t addr = pci_config_address(0, 0xFF, 0xFF, 0);
    assert(((addr >> 11) & 0x1F) == 0x1F);
    assert(((addr >> 8) & 0x07) == 0x07);
    assert(((addr >> 16) & 0xFF) == 0); /* bus untouched */
}

int main(void) {
    test_enable_bit_always_set();
    test_bus_slot_func_placement();
    test_offset_dword_aligned();
    test_slot_func_are_masked();
    return 0;
}
