#pragma once

#include <stdint.h>

#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_MEMORY_AVAILABLE 1

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed));

struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct multiboot_mmap_entry entries[];
} __attribute__((packed));

/* Finds the largest available (type == 1) memory region in the
 * Multiboot2 info struct at `mb_info_addr`. Returns 1 and fills
 * *base/*len on success, 0 if no mmap tag / no usable region found. */
int multiboot2_find_largest_region(uint64_t mb_info_addr, uint64_t *base, uint64_t *len);
