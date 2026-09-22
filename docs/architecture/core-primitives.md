# Core kernel primitives

Small, dependency-light building blocks added ahead of the phases that
will consume them (buddy/heap allocators, the scheduler, the trace
subsystem). Each is host-unit-tested independently of the freestanding
kernel build (`make test`).

| Primitive | Where | Depends on | Used by (intended) |
|---|---|---|---|
| `memset`/`memcpy`/`memmove`/`memcmp`, `strlen`/`strcmp`/`strncmp` | `kernel/core/string.{c,h}` | nothing | everything; also what a compiler-generated call to these symbols resolves to (`-fno-builtin` is set so these definitions aren't turned back into calls to themselves) |
| `kprintf`/`ksnprintf`/`kvsnprintf` | `kernel/logging/log.{c,h}` | `serial_write` | any future diagnostic output; a `vsnprintf`-style bounded formatter (`%d %u %x %p %s %c %%`, no width/precision) so callers don't hand-roll integer-to-string like `kernel/init/main.c`'s `serial_write_uint` and `arch/x86_64/interrupts/isr.c`'s `write_hex64` currently do |
| `panic()`, `KASSERT()` | `kernel/panic/panic.c`, `kernel/core/assert.h` | `kprintf` | software-detected failures (OOM, invariant violations) — CPU exceptions keep their own register-dump path in `isr.c` |
| `spinlock_t` (+ `_irqsave`/`_irqrestore`) | `sync/spinlock/spinlock.{c,h}` | nothing (GCC atomic builtins) | any state shared with interrupt/exception context; single-CPU-correct now, the right shape for SMP later |
| intrusive `list_node` | `kernel/core/list.h` | nothing (header-only) | free-lists/queues (PMM free blocks, heap free chunks, scheduler run queues) without allocating a list node |
| SPSC `ring_buffer` | `trace/ring-buffer/ring_buffer.{c,h}` | nothing (GCC atomic builtins) | trace/event buffers; one writer + one reader without a lock |

## Known follow-up

`kernel/init/main.c` and `arch/x86_64/interrupts/isr.c` predate `kprintf`
and still hand-roll their own integer formatting. Refactoring them onto
`kprintf` is deliberately not bundled into the primitive's own PR, since
both files are being actively extended by concurrent phase work — revisit
once those phases settle to avoid rebase churn on files under active
development.

## Reading further

- [`README.md`](README.md) — index of all architecture notes.
- [`design-principles.md`](design-principles.md) — the "no universal
  primitive" and "explicit ownership" rules these were built against.
