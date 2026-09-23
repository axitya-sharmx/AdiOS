# AdiOS

A high-performance, capability-oriented, observable operating system for
x86-64, written in freestanding C/C++.

AdiOS boots via UEFI-compatible GRUB/Multiboot2, runs a modular monolithic
kernel with microkernel-inspired internal boundaries (explicit ownership,
isolated subsystems, message/handle-style interfaces between them), and is
built to scale across cores, diagnose its own failures, and stay testable at
every layer. The full design rationale, principles, and phase-by-phase
roadmap live in [`OS_MASTER_SPEC.md`](OS_MASTER_SPEC.md) — read that first
for *why*; this file covers *how to build, run, and contribute*.

## Project identity

> A high-performance OS built around locality, isolation, explicit
> ownership, scalable concurrency, strong diagnostics, and first-class
> observability.

Guiding principle: **architecture first, implementation second,
optimization third.** The project explicitly avoids becoming a kernel demo
with no userland, a driver grab-bag, an unreasoned Linux clone, or a feature
pile with no compatibility contracts. See `OS_MASTER_SPEC.md` §0–§1 for the
full statement of vision and non-goals.

## Status

**Phase 1 complete** — kernel bring-up. The kernel boots via GRUB/Multiboot2
on BIOS, transitions to long mode, and logs to the serial console. CI builds
the ISO and boots it in QEMU on every push, asserting on the serial output
(`[INIT] Kernel initialized`).

**Phase 2 in progress** — CPU subsystem (GDT/TSS, IDT, exception handling,
fault diagnostics).

Development proceeds in this order (see `OS_MASTER_SPEC.md` §0 for the full
diagram and §8 onward for each phase's detailed scope):

```text
Toolchain → UEFI Boot → CPU/Interrupts → Per-CPU Infra → Physical Memory
  → Virtual Memory → Kernel Heap → Threads/Scheduler → Processes → Syscalls
  → Handles/Capabilities → ELF/User Space → VFS/Filesystem
  → Device Model/PCI → Storage/Input/USB → Networking → SMP/NUMA
  → Advanced Memory (RCU/Huge Pages) → Security Hardening
  → Observability/Trace VM → Graphics/Desktop
  → Package Manager/Updates/Recovery → Fuzzing/Stress/Fault Injection → v1.0
```

## Repository layout

Top-level directories follow the spec's repository structure
(`OS_MASTER_SPEC.md` §6):

| Directory  | Purpose |
|------------|---------|
| `arch/`    | Architecture-specific code (currently `x86_64/`: boot entry, low-level CPU setup). |
| `boot/`    | Bootloader configuration (`grub.cfg`). |
| `kernel/`  | Core kernel: init sequence, logging, subsystems as they land. |
| `mm/`      | Memory management (physical/virtual allocators, paging) — Phase 4+. |
| `process/` | Process and thread management — later phases. |
| `sync/`    | Synchronization primitives (locks, RCU, etc.). |
| `ipc/`     | Inter-process communication. |
| `security/`| Capability system, security hardening. |
| `fs/`      | Virtual filesystem and filesystem implementations. |
| `block/`   | Block device layer. |
| `drivers/` | Device drivers. |
| `net/`     | Networking stack. |
| `trace/`   | Tracing / observability infrastructure. |
| `user/`    | User-space programs and libraries. |
| `tools/`   | Developer and build tooling. |
| `tests/`   | Automated tests. |
| `docs/`    | Design docs and architecture notes beyond `OS_MASTER_SPEC.md`. |
| `linker/`  | Linker scripts. |

Directories not yet in use are tracked with `.gitkeep` placeholders until a
phase fills them in.

## Build

### Prerequisites

- `gcc` (freestanding-capable; used for both C and the assembly boot stub)
- `grub-mkrescue` (Debian/Ubuntu: `grub-pc-bin grub-common xorriso mtools`)
- `qemu-system-x86_64`, to run the resulting image

On Debian/Ubuntu:

```bash
sudo apt-get install -y gcc grub-pc-bin grub-common xorriso mtools qemu-system-x86
```

### Targets

```bash
make            # compile and link build/kernel.elf
make iso        # build build/adios.iso (bootable GRUB ISO)
make run        # build the ISO and boot it in QEMU, serial output to stdout
make clean      # remove build/
```

`make run` boots headless (`-display none`) with serial redirected to stdout
and no auto-reboot/shutdown on panic or triple fault, so kernel output and
crashes are visible directly in the terminal.

### Toolchain flags

The kernel is built freestanding for long mode: `-ffreestanding
-fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel`, linked with a
custom script (`linker/linker.ld`) and no standard library
(`-nostdlib -static`). See `Makefile` for the exact flags and
`OS_MASTER_SPEC.md` §8 (Phase 0) for the toolchain rationale.

## Continuous integration

Every push and pull request runs a boot smoke test
(`.github/workflows/ci.yml`):

1. Install the toolchain (`grub-pc-bin`, `xorriso`, `mtools`, `qemu-system-x86`).
2. `make iso`.
3. Boot the ISO in QEMU with serial output captured to a file, under a
   20-second timeout.
4. Assert the serial log contains `[INIT] Kernel initialized`.

A PR that fails to boot to that log line fails CI. As later phases add
subsystems, expect this workflow to grow additional assertions and,
eventually, an automated test suite invoked from `tests/`.

## Contributing

Work proceeds phase by phase per `OS_MASTER_SPEC.md`. Conventions:

- Each modular addition lands as its own pull request against `main`,
  scoped to one phase or one coherent piece of a phase.
- Branch from the latest in-flight phase branch, not from `main`, while
  earlier phases are still stacked as open PRs — check open PRs first to
  avoid basing on stale history.
- Keep CI green: a PR should build (`make iso`) and boot
  (`[INIT] Kernel initialized` on serial) before merge; later phases will
  extend this bar as new subsystems come online.
- Match the existing code style (freestanding C, minimal C++ where used,
  no exceptions/RTTI, explicit ownership) — see `OS_MASTER_SPEC.md` for the
  broader design constraints each subsystem must respect.
- If two people are working the same repo concurrently, coordinate on scope
  before starting a phase to avoid overlapping PRs.
