#include "pmm.h"
#include "multiboot2.h"

/* Bounds the static bookkeeping array. Also keeps the managed region well
 * inside the boot identity map (first 1 GiB, see boot.S). NUMA and
 * multi-region support (spec section 14) come once there's hardware with
 * more than one node to justify it. */
#define PMM_MANAGED_CAP_BYTES (128ULL * 1024 * 1024)
#define PMM_MAX_PAGES (PMM_MANAGED_CAP_BYTES / PMM_PAGE_SIZE)

#define PMM_ORDER_FREE_NONE 0xFF

struct free_block {
    struct free_block *prev;
    struct free_block *next;
};

static uint64_t g_base;
static uint64_t g_page_count;
static uint8_t g_page_order[PMM_MAX_PAGES]; /* PMM_ORDER_FREE_NONE, or order if this page is a free block head */
static struct free_block *g_free_list[PMM_MAX_ORDER + 1];
static uint64_t g_free_bytes;
static uint64_t g_total_bytes;

static inline uint64_t block_bytes(uint32_t order) {
    return (uint64_t)PMM_PAGE_SIZE << order;
}

static inline uint64_t page_index(uint64_t addr) {
    return (addr - g_base) / PMM_PAGE_SIZE;
}

static void list_push(uint32_t order, uint64_t addr) {
    struct free_block *node = (struct free_block *)addr;
    node->prev = 0;
    node->next = g_free_list[order];
    if (g_free_list[order]) {
        g_free_list[order]->prev = node;
    }
    g_free_list[order] = node;
    g_page_order[page_index(addr)] = (uint8_t)order;
}

static void list_remove(uint32_t order, uint64_t addr) {
    struct free_block *node = (struct free_block *)addr;
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        g_free_list[order] = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    }
    g_page_order[page_index(addr)] = PMM_ORDER_FREE_NONE;
}

static void add_free_region(uint64_t addr, uint64_t len) {
    while (len >= PMM_PAGE_SIZE) {
        uint32_t order = 0;
        while (order < PMM_MAX_ORDER) {
            uint64_t next_size = block_bytes(order + 1);
            if ((addr % next_size) != 0 || next_size > len) {
                break;
            }
            order++;
        }
        uint64_t size = block_bytes(order);
        list_push(order, addr);
        g_free_bytes += size;
        g_total_bytes += size;
        addr += size;
        len -= size;
    }
}

int pmm_init(uint64_t mb_info_addr) {
    uint64_t base, len;
    if (!multiboot2_find_largest_region(mb_info_addr, &base, &len)) {
        return 0;
    }

    /* align base up, length down, to page size */
    uint64_t aligned_base = (base + PMM_PAGE_SIZE - 1) & ~(uint64_t)(PMM_PAGE_SIZE - 1);
    len -= (aligned_base - base);
    base = aligned_base;
    len &= ~(uint64_t)(PMM_PAGE_SIZE - 1);

    if (len > PMM_MANAGED_CAP_BYTES) {
        len = PMM_MANAGED_CAP_BYTES;
    }

    g_base = base;
    g_page_count = len / PMM_PAGE_SIZE;
    g_free_bytes = 0;
    g_total_bytes = 0;

    for (uint64_t i = 0; i < g_page_count; i++) {
        g_page_order[i] = PMM_ORDER_FREE_NONE;
    }
    for (uint32_t o = 0; o <= PMM_MAX_ORDER; o++) {
        g_free_list[o] = 0;
    }

    add_free_region(base, len);
    return 1;
}

uint64_t pmm_alloc(uint32_t order) {
    if (order > PMM_MAX_ORDER) {
        return 0;
    }

    uint32_t found_order = order;
    while (found_order <= PMM_MAX_ORDER && !g_free_list[found_order]) {
        found_order++;
    }
    if (found_order > PMM_MAX_ORDER) {
        return 0; /* out of memory at this order */
    }

    uint64_t addr = (uint64_t)g_free_list[found_order];
    list_remove(found_order, addr);

    /* split down to the requested order, pushing the unused buddy halves back */
    while (found_order > order) {
        found_order--;
        uint64_t buddy = addr + block_bytes(found_order);
        list_push(found_order, buddy);
    }

    g_free_bytes -= block_bytes(order);
    return addr;
}

void pmm_free(uint64_t addr, uint32_t order) {
    /* Only the freed block's own bytes are newly free; any buddy we
     * coalesce with was already free and already counted. */
    g_free_bytes += block_bytes(order);

    while (order < PMM_MAX_ORDER) {
        uint64_t buddy = (addr - g_base) ^ block_bytes(order);
        buddy += g_base;

        if (buddy < g_base || page_index(buddy) >= g_page_count) {
            break;
        }
        if (g_page_order[page_index(buddy)] != order) {
            break; /* buddy not free at this order: can't coalesce */
        }

        list_remove(order, buddy);
        if (buddy < addr) {
            addr = buddy;
        }
        order++;
    }

    list_push(order, addr);
}

uint64_t pmm_free_bytes(void) {
    return g_free_bytes;
}

uint64_t pmm_total_bytes(void) {
    return g_total_bytes;
}
