#include "multiboot2.h"

int multiboot2_find_largest_region(uint64_t mb_info_addr, uint64_t *base, uint64_t *len) {
    uint32_t total_size = *(uint32_t *)mb_info_addr;
    uint8_t *ptr = (uint8_t *)(mb_info_addr + 8); /* skip total_size + reserved */
    uint8_t *end = (uint8_t *)(mb_info_addr + total_size);

    int found = 0;
    uint64_t best_base = 0, best_len = 0;

    while (ptr < end) {
        struct multiboot_tag *tag = (struct multiboot_tag *)ptr;
        if (tag->type == 0) {
            break; /* end tag */
        }

        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            struct multiboot_tag_mmap *mmap = (struct multiboot_tag_mmap *)tag;
            uint8_t *entry_ptr = (uint8_t *)mmap->entries;
            uint8_t *mmap_end = ptr + mmap->size;

            while (entry_ptr < mmap_end) {
                struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)entry_ptr;
                if (entry->type == MULTIBOOT_MEMORY_AVAILABLE && entry->len > best_len) {
                    best_base = entry->addr;
                    best_len = entry->len;
                    found = 1;
                }
                entry_ptr += mmap->entry_size;
            }
        }

        /* tags are 8-byte aligned */
        ptr += (tag->size + 7) & ~7u;
    }

    if (found) {
        *base = best_base;
        *len = best_len;
    }
    return found;
}
