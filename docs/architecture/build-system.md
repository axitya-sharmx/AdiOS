# Build system and CI

## Toolchain

Everything is built with the host `gcc` (no cross-compiler yet — relies on
targeting x86-64 flags directly, which works because development happens
on x86-64 Linux hosts/CI runners). Assembly (`.S`) files are also passed
through `gcc` (`AS := gcc`), relying on it invoking `as`/preprocessing as
needed, rather than a separate `nasm`/`yasm` toolchain.

## Compile flags (`Makefile`)

```text
-ffreestanding        no hosted-environment assumptions (no libc, no CRT)
-fno-stack-protector   no __stack_chk_fail — not available freestanding
-fno-pic               absolute addressing; simpler for early boot code
-mno-red-zone          required: interrupt/exception handlers can't rely
                        on the SysV red zone once the kernel takes
                        interrupts on the current stack
-mcmodel=kernel        code/data assumed to live in the top 2 GiB of the
                        address space (standard for a kernel linked high,
                        even though this kernel currently links at 1 MiB —
                        see docs/architecture/memory-map.md)
-Wall -Wextra          on by default; keep warnings at zero
```

Link flags: `-nostdlib -static` (no libc/CRT, no dynamic linking),
`-T linker/linker.ld` (custom layout, see memory-map.md), `-ffreestanding
-O2`.

## Targets

| Target | Does |
|--------|------|
| `make` / `make all` | Compile + link `build/kernel.elf`. |
| `make iso` | `kernel.elf` + `boot/grub.cfg` → `build/adios.iso` via `grub-mkrescue`. |
| `make run` | `make iso`, then boot it in QEMU: `-serial stdio -display none -no-reboot -no-shutdown` (headless, serial to the terminal, and a crash/triple-fault halts instead of silently rebooting — critical for seeing panics rather than looping). |
| `make clean` | Remove `build/`. |

Object files land under `build/` mirroring the source tree
(`build/kernel/init/main.o`, etc.) via pattern rules with `@mkdir -p
$(dir $@)`; there's no separate `obj/` directory.

## Adding a new source file

Add it to `C_SOURCES` or `ASM_SOURCES` in the `Makefile` — there's no
directory globbing, sources are listed explicitly. This is deliberate at
this stage: with a handful of files, an explicit list is more obviously
correct than a glob that could silently pick up stray files.

## CI (`.github/workflows/ci.yml`)

Runs on every push and PR, on `ubuntu-latest`:

1. Install `grub-pc-bin grub-common xorriso mtools qemu-system-x86` (the
   `make`/`gcc` toolchain is already on the runner image).
2. `make iso`.
3. Boot the ISO in QEMU with serial redirected to `serial.log`, under a
   20-second `timeout` (so a hang doesn't stall the job — the boot run
   itself is expected to take a couple of seconds).
4. `grep -q "[INIT] Kernel initialized"` on the log — this is the
   pass/fail gate for the whole job. No separate unit-test step exists
   yet; correctness is currently verified entirely by observing kernel
   behavior via serial output, not by inspecting code paths in isolation.

As of Phase 2, a second boot pass triggers a deliberate CPU fault (`int3`)
and asserts the fault-dump text appears on serial, exercising the
exception-handling path end to end rather than only confirming compile
success. Expect more such "boot with condition X, assert output Y" passes
as more of the kernel becomes observable this way, and eventually a
real host-side unit test suite (`tests/`) for logic that doesn't need to
run inside the kernel to be verified (see `OS_MASTER_SPEC.md`'s testing
expectations).

## Reading further

- [`boot.md`](boot.md), [`memory-map.md`](memory-map.md) — what the built
  kernel actually does and looks like in memory.
- `Makefile`, `.github/workflows/ci.yml` — source of truth; update this
  doc if they change.
