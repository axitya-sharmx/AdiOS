# Architecture notes

Implementation-level documentation of what the kernel currently does, as
opposed to [`OS_MASTER_SPEC.md`](../../OS_MASTER_SPEC.md) which is the
target design across all phases. These notes track actual merged code and
are expected to lag or be superseded as phases land — each doc says which
phase it reflects.

- [`boot.md`](boot.md) — the boot sequence, `_start` through `kernel_main`.
- [`memory-map.md`](memory-map.md) — link layout and the boot-time
  identity mapping.
- [`build-system.md`](build-system.md) — toolchain, `Makefile` targets,
  and what CI actually verifies.

When a phase changes behavior these docs describe, update the doc in the
same PR — stale architecture notes are worse than none.
