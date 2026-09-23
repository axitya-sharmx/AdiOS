/* x86-64 4-level page table walker (spec section 16.1).
 *
 * Every table page (PML4/PDPT/PD/PT) is allocated from the PMM as a
 * physical page. That's usable directly as a pointer without any extra
 * translation because boot.S identity-maps the first 1 GiB (see
 * PMM_MANAGED_CAP_BYTES in mm/pmm/pmm.c, which keeps the PMM's whole
 * region inside that same 1 GiB) — physical address N and virtual
 * address N are the same mapping until Phase 17 adds a proper direct
 * physical map and this assumption gets a real name.
 */
#include "vmm.h"
#include "../pmm/pmm.h"

#define ENTRIES_PER_TABLE 512
#define PAGE_SIZE 4096
#define ADDR_MASK 0x000FFFFFFFFFF000ULL /* bits 12-51: table/page physical address */

static inline unsigned pml4_index(uint64_t v) { return (v >> 39) & 0x1FF; }
static inline unsigned pdpt_index(uint64_t v) { return (v >> 30) & 0x1FF; }
static inline unsigned pd_index(uint64_t v)   { return (v >> 21) & 0x1FF; }
static inline unsigned pt_index(uint64_t v)   { return (v >> 12) & 0x1FF; }

static inline uint64_t *table_ptr(uint64_t entry) {
    return (uint64_t *)(entry & ADDR_MASK);
}

static inline uint64_t read_cr3(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

static inline void invlpg(uint64_t virt) {
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

/* Returns the next-level table for `entry`, allocating and zeroing a
 * fresh one from the PMM if `entry` isn't present yet. `entry` is
 * written back through `slot`. Returns 0 if the PMM is out of memory. */
static uint64_t *walk_or_create(uint64_t *slot, uint64_t flags) {
    if (*slot & VMM_PRESENT) {
        return table_ptr(*slot);
    }

    uint64_t phys = pmm_alloc(0);
    if (!phys) {
        return 0;
    }

    uint64_t *table = (uint64_t *)phys;
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        table[i] = 0;
    }

    *slot = phys | VMM_PRESENT | VMM_WRITABLE | (flags & VMM_USER);
    return table;
}

int vmm_map(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t *pml4 = (uint64_t *)(read_cr3() & ADDR_MASK);

    uint64_t *pdpt = walk_or_create(&pml4[pml4_index(virt)], flags);
    if (!pdpt) {
        return 0;
    }
    uint64_t *pd = walk_or_create(&pdpt[pdpt_index(virt)], flags);
    if (!pd) {
        return 0;
    }
    uint64_t *pt = walk_or_create(&pd[pd_index(virt)], flags);
    if (!pt) {
        return 0;
    }

    pt[pt_index(virt)] = (phys & ADDR_MASK) | VMM_PRESENT | (flags & (VMM_WRITABLE | VMM_USER));
    invlpg(virt);
    return 1;
}

/* Walks down to the PT entry for `virt` without creating anything.
 * Returns 0 (and leaves *out_pt/*out_index unset) if any level above
 * the page table itself isn't present. */
static int find_pt_entry(uint64_t virt, uint64_t **out_pt, unsigned *out_index) {
    uint64_t *pml4 = (uint64_t *)(read_cr3() & ADDR_MASK);

    uint64_t pml4e = pml4[pml4_index(virt)];
    if (!(pml4e & VMM_PRESENT)) {
        return 0;
    }
    uint64_t *pdpt = table_ptr(pml4e);

    uint64_t pdpte = pdpt[pdpt_index(virt)];
    if (!(pdpte & VMM_PRESENT)) {
        return 0;
    }
    uint64_t *pd = table_ptr(pdpte);

    uint64_t pde = pd[pd_index(virt)];
    if (!(pde & VMM_PRESENT)) {
        return 0;
    }
    uint64_t *pt = table_ptr(pde);

    *out_pt = pt;
    *out_index = pt_index(virt);
    return 1;
}

void vmm_unmap(uint64_t virt) {
    uint64_t *pt;
    unsigned index;
    if (!find_pt_entry(virt, &pt, &index)) {
        return;
    }
    pt[index] = 0;
    invlpg(virt);
}

uint64_t vmm_translate(uint64_t virt) {
    uint64_t *pt;
    unsigned index;
    if (!find_pt_entry(virt, &pt, &index)) {
        return 0;
    }
    uint64_t pte = pt[index];
    if (!(pte & VMM_PRESENT)) {
        return 0;
    }
    return (pte & ADDR_MASK) | (virt & (PAGE_SIZE - 1));
}
