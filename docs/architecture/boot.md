# Boot sequence

Status: reflects Phase 1 (merged) as of this writing; Phase 2 (GDT/TSS, IDT)
adds steps noted inline as "Phase 2" and is tracked separately.

AdiOS boots as a Multiboot2 payload loaded by GRUB. There is no UEFI stub or
BIOS-specific setup beyond what GRUB provides — GRUB does the firmware
handshake, loads `kernel.elf` per `boot/grub.cfg`, and jumps to `_start` in
32-bit protected mode with `eax` = the Multiboot2 magic and `ebx` = a
pointer to the Multiboot2 information structure (see "What's not wired up
yet" below).

## Stages

1. **`_start` (`arch/x86_64/boot/boot.S`, 32-bit)**
   The entry point referenced by `ENTRY(_start)` in `linker/linker.ld`. Sets
   up a small (`stack_bottom`/`stack_top`, 64 KiB) boot stack in `.bss`.

2. **Identity-map the first 1 GiB**
   Long mode requires paging to be enabled first. The code builds a
   minimal 3-level page table (`p4_table` → `p3_table` → `p2_table`) using
   2 MiB pages, identity-mapping physical (and thus virtual) addresses
   `0x0`–`0x40000000`. This is enough to cover the kernel image and early
   boot structures; it is not the final kernel virtual memory layout
   (that's Phase 5+, "Virtual Memory").

3. **Enable PAE, long mode, and paging**
   In order: set `CR4.PAE`, set the long-mode-enable bit in the `EFER`
   MSR (`0xC0000080`), then set `CR0.PG`. This is the standard x86-64
   activation sequence — PAE must be on before the long-mode bit takes
   effect, and paging must be enabled last since `CR3` (loaded a step
   earlier) already points at valid page tables.

4. **Load a temporary 64-bit GDT and far-jump into long mode**
   A minimal two-entry GDT (`gdt64`: null descriptor + one 64-bit code
   segment) is loaded via `lgdt`, then `ljmp $0x8, $long_mode_start`
   reloads `CS` and enters 64-bit mode. This GDT only defines a kernel
   code segment — no data segment descriptor, no user segments, no TSS.
   It exists purely to get to 64-bit mode; Phase 2 replaces it with the
   real kernel/user GDT + TSS (`arch/x86_64/cpu/gdt.c`).

5. **`long_mode_start` (64-bit)**
   Zeroes the segment registers (`ss`/`ds`/`es`/`fs`/`gs` — flat model,
   segmentation is vestigial in long mode), sets up `rsp`, and calls
   `kernel_main`.

6. **`kernel_main` (`kernel/init/main.c`)**
   C entry point. Initializes the serial console
   (`kernel/logging/serial.c`, COM1 at `0x3f8`, 38400 baud 8N1) and logs
   `[BOOT] Kernel starting`, then `[CPU ] x86_64 long mode active`. From
   Phase 2 onward this is also where `gdt_init()` and `idt_init()` run.
   The kernel never returns from here; once init logging is done it
   parks in a `hlt` loop (or, from Phase 2, waits for interrupts).

7. **`[INIT] Kernel initialized`**
   The line CI greps for on the serial log to confirm a successful boot
   (`.github/workflows/ci.yml`). Any change to the boot path that stops
   this line from appearing fails CI.

## What's not wired up yet

- **Multiboot2 info pointer**: GRUB hands `_start` a pointer to the
  Multiboot2 information structure in `ebx`, but `boot.S` does not
  currently preserve or forward it to `kernel_main` — it's dropped on the
  floor. Phase 4 (Physical Memory Manager, "memory discovery") needs this
  pointer to read the memory map, so wiring it through (e.g. as the first
  argument in `rdi`) is a prerequisite for that phase, not yet done.
- **Kernel virtual layout**: the identity-mapped first 1 GiB is a boot-time
  convenience, not the final higher-half kernel mapping described in later
  phases of `OS_MASTER_SPEC.md` §"Virtual Memory".
- **No SMP bring-up**: only the bootstrap processor runs; secondary CPUs
  are Phase 3+ ("Per-CPU Infrastructure") / SMP phases.

## Reading further

- `arch/x86_64/boot/boot.S` — the assembly described above.
- `linker/linker.ld` — places `.boot` (containing the Multiboot2 header)
  first, kernel loaded at `1M`; see [`memory-map.md`](memory-map.md).
- `boot/grub.cfg` — the GRUB menu entry (`multiboot2 /boot/kernel.elf`).
- `OS_MASTER_SPEC.md` §8–§10 — Phase 0/1/2 scope this stage implements.
