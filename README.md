# AdiOS

A high-performance, capability-oriented, observable C/C++ operating system.

x86-64, UEFI boot, modular monolithic kernel with microkernel-inspired internal
boundaries. Full design goals, principles, and phased roadmap are in
[OS_MASTER_SPEC.md](OS_MASTER_SPEC.md).

## Status

Phase 1 — kernel bring-up. The kernel boots via GRUB/Multiboot2 on BIOS,
transitions to long mode, and logs to the serial console. CI builds the ISO
and boots it in QEMU on every push, asserting on the serial output.

## Layout

Top-level directories follow the spec's repository structure (section 6):
`arch/`, `kernel/`, `mm/`, `process/`, `sync/`, `ipc/`, `security/`, `fs/`,
`block/`, `drivers/`, `net/`, `trace/`, `user/`, `tools/`, `tests/`, `docs/`.

## Build

Requires `gcc`, `grub-mkrescue` (grub-pc-bin, xorriso, mtools) and, to run it,
`qemu-system-x86_64`.

```
make iso   # build build/adios.iso
make run   # build and boot in QEMU, serial output to stdout
```

## Contributing

Work proceeds phase by phase per `OS_MASTER_SPEC.md`. Each modular addition
lands as its own pull request against `main`.
