#pragma once

#include <stdint.h>
#include <stddef.h>

/* Loads a 64-bit little-endian ELF executable's PT_LOAD segments into the
 * current address space via the PMM/VMM (spec section 51). Requires each
 * segment's p_vaddr to be page-aligned (true of every ELF a normal linker
 * produces) and every mapping is currently RW + user, no matter what the
 * segment's own flags say (spec section 16.2's "avoid RWX" is aspirational
 * until the VMM gets an NX bit — tracked, not forgotten).
 *
 * Does NOT jump to the entry point — that needs a user stack and a ring-3
 * transition, which is its own piece of work once this is solid. Returns
 * 1 and writes the entry point to *out_entry on success, 0 on any
 * malformed input (never partially applies a bad image: validates the
 * whole header/program-header table before mapping anything).
 */
int elf_load(const uint8_t *image, size_t size, uint64_t *out_entry);
