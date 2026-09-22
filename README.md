# AdiOS

A high-performance, capability-oriented, observable C/C++ operating system.

x86-64, UEFI boot, modular monolithic kernel with microkernel-inspired internal
boundaries. Full design goals, principles, and phased roadmap are in
[OS_MASTER_SPEC.md](OS_MASTER_SPEC.md).

## Status

Phase 0 — development foundation. Repository skeleton is in place; toolchain,
CI, and boot smoke tests are not yet set up (see spec section 8).

## Layout

Top-level directories follow the spec's repository structure (section 6):
`arch/`, `kernel/`, `mm/`, `process/`, `sync/`, `ipc/`, `security/`, `fs/`,
`block/`, `drivers/`, `net/`, `trace/`, `user/`, `tools/`, `tests/`, `docs/`.

## Build

```
make
```

Not yet functional — cross compiler and image generation come first (Phase 0).

## Contributing

Work proceeds phase by phase per `OS_MASTER_SPEC.md`. Each modular addition
lands as its own pull request against `main`.
