#pragma once

#include <stdint.h>

#define PMM_PAGE_SIZE 4096
#define PMM_MAX_ORDER 10 /* order N block = 4KiB << N; order 10 = 4 MiB */

/* Initializes the buddy allocator over the largest usable region found in
 * the Multiboot2 memory map. Region is capped to PMM_MANAGED_CAP_BYTES and
 * must fall inside the boot identity map (first 1 GiB) — see boot.S.
 * Returns 1 on success, 0 if no usable region was found. */
int pmm_init(uint64_t mb_info_addr);

/* Allocates 2^order pages (physically contiguous). Returns 0 on failure. */
uint64_t pmm_alloc(uint32_t order);

/* Frees a block previously returned by pmm_alloc with the same order. */
void pmm_free(uint64_t addr, uint32_t order);

uint64_t pmm_free_bytes(void);
uint64_t pmm_total_bytes(void);
