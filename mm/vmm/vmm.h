#pragma once

#include <stdint.h>

#define VMM_PRESENT  (1ULL << 0)
#define VMM_WRITABLE (1ULL << 1)
#define VMM_USER     (1ULL << 2)

/* Maps one 4 KiB page: virt -> phys, with the given permission flags
 * (VMM_WRITABLE / VMM_USER; VMM_PRESENT is implied). Allocates any
 * missing intermediate page-table levels from the PMM. Both addresses
 * must be page-aligned. Returns 1 on success, 0 if a table page
 * couldn't be allocated. */
int vmm_map(uint64_t virt, uint64_t phys, uint64_t flags);

/* Unmaps one 4 KiB page and invalidates its TLB entry. A no-op if the
 * page (or any level above it) wasn't mapped. */
void vmm_unmap(uint64_t virt);

/* Returns the physical address virt currently maps to, or 0 if it isn't
 * mapped. Relies on physical page 0 never being handed out by the PMM in
 * practice (it's inside the BIOS/legacy region on every PC memory map we
 * expect) — not a hard guarantee, but good enough until this needs a real
 * tri-state return. */
uint64_t vmm_translate(uint64_t virt);
