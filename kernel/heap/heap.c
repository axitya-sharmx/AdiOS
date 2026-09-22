/* First kmalloc/kfree implementation (spec section 21.1: "provide
 * kmalloc()/kfree() with debugging support"). This is the classic K&R
 * "malloc.c" algorithm (K&R2 section 8.7): a circular free list of
 * headers, sorted by address, coalescing adjacent free blocks on kfree,
 * growing on demand via a heap-extend hook instead of sbrk(). It's a
 * well-understood, easy-to-audit baseline — the production path (per-CPU
 * caches over a SLUB-style object allocator, spec section 22) replaces
 * this once there's a real workload to size it against.
 *
 * Debugging support: each live block is tagged with a magic number,
 * checked on kfree to catch double-frees and frees of garbage pointers
 * before they corrupt the free list.
 */
#include "heap.h"
#include <stdint.h>
#include "../../mm/pmm/pmm.h"
#include "../../mm/vmm/vmm.h"

typedef long Align;

union header {
    struct {
        union header *next; /* next block in the circular free list */
        size_t size;         /* size of this block, in units of sizeof(union header) */
        uint32_t magic;       /* set while allocated; checked and cleared on free */
    } s;
    Align align;
};
typedef union header Header;

#define HEAP_MAGIC 0x4B484541u /* "KHEA" */
#define NALLOC 4096            /* header-units per heap extension (16 units * 4 KiB pages worth, min) */

/* Virtual region the heap grows into. Chosen well clear of the VMM
 * self-test mapping (0x40000000) and the boot identity map (< 1 GiB). */
#define HEAP_VIRT_BASE 0x50000000ULL

static Header base;
static Header *freep = NULL;
static uint64_t g_heap_brk = HEAP_VIRT_BASE;

/* Maps `count` fresh pages at the current heap break and advances it,
 * i.e. a page-granular sbrk(). Returns the start of the new region, or 0
 * if the PMM/VMM couldn't satisfy the request (already-mapped pages, if
 * any, are left mapped — the heap simply doesn't grow further). */
static uint64_t heap_extend_pages(uint64_t count) {
    uint64_t start = g_heap_brk;

    for (uint64_t i = 0; i < count; i++) {
        uint64_t phys = pmm_alloc(0);
        if (!phys || !vmm_map(g_heap_brk, phys, VMM_WRITABLE)) {
            return 0;
        }
        g_heap_brk += PMM_PAGE_SIZE;
    }

    return start;
}

static Header *morecore(size_t nunits) {
    if (nunits < NALLOC) {
        nunits = NALLOC;
    }

    size_t bytes = nunits * sizeof(Header);
    uint64_t pages = (bytes + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;

    uint64_t region = heap_extend_pages(pages);
    if (!region) {
        return NULL;
    }

    Header *up = (Header *)region;
    up->s.size = (pages * PMM_PAGE_SIZE) / sizeof(Header);
    kfree((void *)(up + 1));
    return freep;
}

void *kmalloc(size_t nbytes) {
    if (nbytes == 0) {
        return NULL;
    }

    size_t nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;

    Header *prevp = freep;
    if (prevp == NULL) {
        base.s.next = freep = prevp = &base;
        base.s.size = 0;
    }

    for (Header *p = prevp->s.next;; prevp = p, p = p->s.next) {
        if (p->s.size >= nunits) {
            if (p->s.size == nunits) {
                prevp->s.next = p->s.next;
            } else {
                p->s.size -= nunits;
                p += p->s.size;
                p->s.size = nunits;
            }
            freep = prevp;
            p->s.magic = HEAP_MAGIC;
            return (void *)(p + 1);
        }
        if (p == freep) {
            if ((p = morecore(nunits)) == NULL) {
                return NULL; /* PMM/VMM exhausted */
            }
        }
    }
}

void kfree(void *ap) {
    if (ap == NULL) {
        return;
    }

    Header *bp = (Header *)ap - 1;
    if (bp->s.magic != HEAP_MAGIC) {
        return; /* double free, or not a kmalloc pointer: refuse rather than corrupt the free list */
    }
    bp->s.magic = 0;

    Header *p;
    for (p = freep; !(bp > p && bp < p->s.next); p = p->s.next) {
        if (p >= p->s.next && (bp > p || bp < p->s.next)) {
            break; /* bp is above the highest, or below the lowest, free block */
        }
    }

    if (bp + bp->s.size == p->s.next) {
        bp->s.size += p->s.next->s.size;
        bp->s.next = p->s.next->s.next;
    } else {
        bp->s.next = p->s.next;
    }

    if (p + p->s.size == bp) {
        p->s.size += bp->s.size;
        p->s.next = bp->s.next;
    } else {
        p->s.next = bp;
    }

    freep = p;
}
