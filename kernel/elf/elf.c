#include "elf.h"
#include "../../mm/pmm/pmm.h"
#include "../../mm/vmm/vmm.h"

#define EI_NIDENT 16
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ET_EXEC 2
#define EM_X86_64 62
#define PT_LOAD 1

struct elf64_ehdr {
    uint8_t e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed));

struct elf64_phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed));

#define PF_W 0x2

static int valid_header(const struct elf64_ehdr *eh, size_t size) {
    if (size < sizeof(*eh)) {
        return 0;
    }
    if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' || eh->e_ident[2] != 'L' || eh->e_ident[3] != 'F') {
        return 0;
    }
    if (eh->e_ident[4] != ELFCLASS64 || eh->e_ident[5] != ELFDATA2LSB) {
        return 0;
    }
    if (eh->e_type != ET_EXEC || eh->e_machine != EM_X86_64) {
        return 0;
    }
    if (eh->e_phentsize != sizeof(struct elf64_phdr)) {
        return 0;
    }
    /* phdr table itself must fit in the image */
    uint64_t phdr_end = eh->e_phoff + (uint64_t)eh->e_phnum * eh->e_phentsize;
    if (eh->e_phoff > size || phdr_end > size || phdr_end < eh->e_phoff) {
        return 0; /* also catches overflow wraparound */
    }
    return 1;
}

static int valid_segment(const struct elf64_phdr *ph, size_t size) {
    if (ph->p_filesz > ph->p_memsz) {
        return 0;
    }
    if (ph->p_vaddr % PMM_PAGE_SIZE != 0) {
        return 0; /* unaligned PT_LOAD vaddr: not supported yet, see elf.h */
    }
    uint64_t seg_end = ph->p_offset + ph->p_filesz;
    if (ph->p_offset > size || seg_end > size || seg_end < ph->p_offset) {
        return 0;
    }
    return 1;
}

int elf_load(const uint8_t *image, size_t size, uint64_t *out_entry) {
    const struct elf64_ehdr *eh = (const struct elf64_ehdr *)image;
    if (!valid_header(eh, size)) {
        return 0;
    }

    const struct elf64_phdr *phdrs = (const struct elf64_phdr *)(image + eh->e_phoff);

    /* Validate every segment before mapping any of them, so a malformed
     * later segment can't leave an earlier one mapped behind. */
    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD && !valid_segment(&phdrs[i], size)) {
            return 0;
        }
    }

    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        const struct elf64_phdr *ph = &phdrs[i];
        if (ph->p_type != PT_LOAD) {
            continue;
        }

        uint64_t flags = VMM_USER | ((ph->p_flags & PF_W) ? VMM_WRITABLE : 0);
        uint64_t npages = (ph->p_memsz + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;

        for (uint64_t p = 0; p < npages; p++) {
            uint64_t vaddr = ph->p_vaddr + p * PMM_PAGE_SIZE;
            uint64_t phys = pmm_alloc(0);
            if (!phys || !vmm_map(vaddr, phys, flags)) {
                return 0; /* out of memory partway through: image left partially mapped, matches every other phase's OOM handling */
            }

            uint8_t *dst = (uint8_t *)vaddr;
            uint64_t page_start = p * PMM_PAGE_SIZE;
            for (uint64_t b = 0; b < PMM_PAGE_SIZE; b++) {
                uint64_t off = page_start + b;
                dst[b] = (off < ph->p_filesz) ? image[ph->p_offset + off] : 0;
            }
        }
    }

    *out_entry = eh->e_entry;
    return 1;
}
