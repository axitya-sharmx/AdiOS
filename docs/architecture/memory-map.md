# Memory map (Phase 1 boot state)

This describes the memory layout as of Phase 1 — the boot-time state
before the physical/virtual memory managers exist. It will be superseded
by proper documentation once Phase 4 ("Physical Memory Manager") and the
virtual-memory phase land; treat this as a snapshot of "what's true right
now", not the target design (see `OS_MASTER_SPEC.md` §12 onward for that).

## Link layout (`linker/linker.ld`)

The kernel is linked as a flat ELF, loaded starting at the conventional
1 MiB mark (the lowest address usable without colliding with real-mode /
BIOS / video memory regions below it):

```text
0x00100000 (1 MiB)  .boot     — Multiboot2 header (must be early: GRUB scans
                                 only the first 32 KiB of the file for it)
             ALIGN(4K)
                     .text     — code
             ALIGN(4K)
                     .rodata   — read-only data (includes the temporary
                                 64-bit GDT built in boot.S)
             ALIGN(4K)
                     .data     — initialized data
             ALIGN(4K)
                     .bss      — zero-initialized data, including the
                                 boot stack and the P4/P3/P2 page tables
                                 built by boot.S
```

There is no separate load address vs. link address split (no higher-half
kernel yet) — the kernel currently links and runs at the same 1 MiB-based
addresses it's loaded at.

## Identity mapping set up by `boot.S`

Before `kernel_main` runs, `_start` builds a 3-level page table (PAE-style,
2 MiB pages, no PML4 self-map) covering:

```text
Virtual 0x00000000 – 0x3FFFFFFF  →  Physical 0x00000000 – 0x3FFFFFFF
(identity-mapped, first 1 GiB, 512 × 2 MiB pages via a single p2_table)
```

This is sufficient to cover the kernel image, its boot-time page tables,
and the boot stack, all of which live in the first 1 MiB–few MiB of
physical memory. It is a bootstrap convenience, not a final layout — there
is currently no guard against the kernel image or heap growing past 1 GiB,
no unmapped guard pages, and no separation between kernel and (future)
user address space.

## Boot stack

A 64 KiB stack (`stack_bottom`/`stack_top` in `.bss`) is used both in
32-bit mode (protected-mode setup) and carried into long mode — `boot.S`
reloads `rsp = stack_top` right before calling `kernel_main`. There is
currently one stack, used by the single bootstrap CPU; per-CPU stacks are
part of Phase 3 ("Per-CPU Infrastructure"), and Phase 2 additionally
introduces a dedicated kernel interrupt stack via the TSS (`rsp0`) —
see `arch/x86_64/cpu/gdt.c`.

## Physical memory beyond the kernel image

Not yet discovered or tracked. GRUB passes a Multiboot2 information
structure (memory map, reserved regions, etc.) via `ebx` at `_start`, but
`boot.S` does not currently forward that pointer into `kernel_main` — see
[`boot.md`](boot.md#whats-not-wired-up-yet). Until Phase 4 lands, the
kernel has no notion of total RAM, reserved/MMIO regions, or a physical
frame allocator; anything beyond the statically-sized boot structures in
`.bss` is unmanaged.

## Reading further

- [`boot.md`](boot.md) — the boot sequence that establishes this state.
- `linker/linker.ld`, `arch/x86_64/boot/boot.S` — source of truth; update
  this doc if they change.
- `OS_MASTER_SPEC.md` §12 ("Physical Memory Manager") and the virtual
  memory phase — target design this snapshot precedes.
