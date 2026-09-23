# OS MASTER SPECIFICATION
## A High-Performance, Capability-Oriented, Observable C/C++ Operating System

**Document status:** Master planning specification  
**Target architecture:** x86-64 first  
**Boot model:** UEFI  
**Kernel architecture:** Modular monolithic, microkernel-inspired internal boundaries  
**Primary implementation languages:** C + freestanding C++  
**Initial development platform:** QEMU  
**Long-term target:** Real x86-64 hardware  
**Project objective:** Build a coherent, secure, observable, scalable operating system rather than a demonstration kernel.

---

# 0. EXECUTIVE SUMMARY

This project is intended to become a serious operating-system engineering effort with a clear identity:

> **A high-performance OS built around locality, isolation, explicit ownership, scalable concurrency, strong diagnostics, and first-class observability.**

The OS should not attempt to literally be “fool proof” or mathematically bug-free. Instead, its architecture should make common mistakes difficult to express, isolate failures where practical, reject invalid inputs, provide deep diagnostics, and make every critical subsystem testable.

The system should be developed in this order:

```text
Toolchain
  ↓
UEFI Boot
  ↓
CPU / Interrupts
  ↓
Per-CPU Infrastructure
  ↓
Physical Memory
  ↓
Virtual Memory
  ↓
Kernel Heap
  ↓
Threads / Scheduler
  ↓
Processes
  ↓
Syscalls
  ↓
Handles / Capabilities
  ↓
ELF / User Space
  ↓
VFS / Filesystem
  ↓
Device Model / PCI
  ↓
Storage / Input / USB
  ↓
Networking
  ↓
SMP / NUMA
  ↓
Advanced Memory / RCU / Huge Pages
  ↓
Security Hardening
  ↓
Observability / Trace VM
  ↓
Graphics / Desktop
  ↓
Package Manager / Updates / Recovery
  ↓
Fuzzing / Stress / Fault Injection
  ↓
v1.0
```

The most important principle is:

> **Architecture first, implementation second, optimization third.**

---

# 1. PROJECT VISION

## 1.1 What this OS should be

The final system should be able to:

- Boot modern 64-bit x86 hardware through UEFI.
- Safely execute multiple isolated user processes.
- Support preemptive multitasking and multicore execution.
- Manage physical and virtual memory efficiently.
- Scale across multiple CPUs and NUMA nodes.
- Provide a clean syscall and native user-space API.
- Provide capability-oriented security.
- Support reliable persistent storage.
- Provide a VFS and multiple filesystem implementations.
- Provide networking and sockets.
- Support common PC hardware through a structured device model.
- Provide a command-line environment.
- Eventually provide a graphical desktop.
- Provide package management and verified software installation.
- Provide strong crash diagnostics and tracing.
- Support reproducible builds and automated testing.
- Offer a developer-friendly environment for debugging and performance analysis.

## 1.2 What this OS should not become

Do not allow the project to become:

- a kernel demo with no userland,
- a desktop shell glued to an unstable kernel,
- a collection of unrelated drivers,
- a clone of Linux without a reasoned architecture,
- an endless collection of features with no compatibility contracts,
- an optimization benchmark project with poor correctness,
- a security product built on unverified assumptions.

## 1.3 Design identity

The system should distinguish itself through:

1. Capability-oriented authority.
2. Explicit object lifetimes.
3. NUMA and CPU locality.
4. Per-CPU fast paths.
5. Carefully selected lock-free and RCU techniques.
6. Huge-page-aware memory management.
7. Strong user/kernel isolation.
8. First-class event tracing.
9. Explainable failures.
10. Measurement-driven optimization.

---

# 2. NON-NEGOTIABLE ENGINEERING PRINCIPLES

## 2.1 Correctness before performance

A fast bug is still a bug.

The order of priority is:

```text
Correctness
→ Isolation
→ Observability
→ Security
→ Scalability
→ Performance
→ Convenience
```

## 2.2 No universal primitive

Do not attempt to solve every problem with one mechanism.

Examples:

- Not every shared structure should use RCU.
- Not every queue should be lock-free.
- Not every allocation should use SLUB.
- Not every mapping should use huge pages.
- Not every object needs reference counting.
- Not every task should be NUMA-balanced.

## 2.3 Measure before optimizing

Never make claims such as:

> “RCU is faster.”

Instead measure:

> “For workload X, on hardware Y, the RCU implementation reduced reader-side latency by Z%.”

## 2.4 Failure is part of the design

Every subsystem must explicitly define:

- normal operation,
- invalid input,
- resource exhaustion,
- timeout,
- cancellation,
- hardware failure,
- concurrency failure,
- recovery path,
- diagnostic output.

## 2.5 Explicit ownership

Every important resource must have:

- an owner,
- an ownership transfer rule,
- a lifetime,
- a destruction rule,
- a concurrency rule.

## 2.6 No hidden behavior

Avoid APIs that silently:

- allocate unexpectedly,
- block unexpectedly,
- modify global state,
- acquire unknown locks,
- perform network access,
- cross security boundaries.

## 2.7 Stable contracts

The following are contracts, not implementation details:

- syscall ABI,
- capability semantics,
- handle semantics,
- VFS interfaces,
- driver interfaces,
- IPC interfaces,
- memory mapping rules,
- object lifetime rules.

---

# 3. HIGH-LEVEL SYSTEM ARCHITECTURE

```text
                              USER SPACE
┌─────────────────────────────────────────────────────────────────────┐
│ Applications │ Shell │ GUI │ Services │ Diagnostics │ Package Tools│
└──────────────────────────────┬──────────────────────────────────────┘
                               │
                       Native Runtime / libc
                               │
                     System API / Handle ABI
                               │
                 Capability + Authorization Boundary
                               │
═══════════════════════════════╪═══════════════════════════════════════
                               KERNEL
═══════════════════════════════╪═══════════════════════════════════════
                               │
      ┌────────────────────────┼─────────────────────────┐
      │                        │                         │
      ▼                        ▼                         ▼
 Security                 Scheduler               Observability
      │                        │                         │
 Capabilities             Per-CPU queues            Tracepoints
 Handles                   CPU affinity              Event buffers
 Sandboxing                NUMA affinity              Filters
 Credentials              Load balancing             Metrics
 IOMMU                     Timers                     Audit
      │                        │                         │
      └────────────────────────┼─────────────────────────┘
                               │
        ┌──────────────────────┼──────────────────────┐
        │                      │                      │
        ▼                      ▼                      ▼
     Memory                   IPC                    VFS
        │                      │                      │
 ┌──────┼───────┐              │               ┌──────┼──────┐
 │      │       │              │               │      │      │
 PMM   VMM     Heap            │             Page    FS    Cache
 │      │       │              │             Cache
 NUMA  TLB    SLUB             │
 huge  PCID  arenas             │
 pages  KPTI                    │
        │                       │
        └───────────┬───────────┘
                    │
              Device Framework
                    │
      ┌─────────────┼─────────────┐
      ▼             ▼             ▼
    PCI/PCIe       USB         Storage
      │             │             │
     NIC           HID         NVMe/virtio
     GPU          Input
      │             │             │
      └─────────────┼─────────────┘
                    │
                Networking
                    │
         Ethernet / IP / TCP / UDP
                    │
                 Hardware
```

---

# 4. LANGUAGE STRATEGY

## 4.1 C

Use C for:

- architecture-specific low-level code,
- interrupt entry/exit where useful,
- boot-adjacent code,
- early memory initialization,
- hardware register manipulation,
- low-level drivers,
- simple kernel APIs,
- ABI-sensitive interfaces.

## 4.2 Freestanding C++

Use C++ for:

- object-oriented kernel subsystems,
- VFS structures,
- resource wrappers,
- networking objects,
- device abstractions,
- service infrastructure,
- type-safe APIs,
- GUI infrastructure,
- higher-level kernel components.

## 4.3 Initial C++ restrictions

Initially disable or avoid:

- exceptions,
- RTTI,
- `dynamic_cast`,
- uncontrolled global constructors,
- hosted C++ assumptions,
- hidden dynamic allocation,
- dependencies on a normal desktop STL/runtime.

Prefer:

- RAII,
- move semantics,
- templates,
- `constexpr`,
- strongly typed enums,
- compile-time validation,
- explicit allocators,
- non-owning views,
- custom lightweight containers.

## 4.4 C++ safety philosophy

C++ should not be treated as a magic memory-safe language.

Its role is to make incorrect ownership and lifetime patterns difficult to express.

---

# 5. DEVELOPMENT TOOLCHAIN

## 5.1 Required tools

- Cross compiler.
- Binutils or LLVM toolchain.
- Linker.
- `make`, CMake, or another reproducible build system.
- QEMU.
- GDB.
- objdump/readelf-equivalent tooling.
- Git.
- Python for tooling and tests where useful.
- CI runner.
- Image generation tools.

## 5.2 Build pipeline

Every major commit should be capable of:

```text
source
 ↓
build
 ↓
link
 ↓
create bootable image
 ↓
boot QEMU
 ↓
run automated tests
 ↓
collect logs
 ↓
produce artifacts
```

## 5.3 Build modes

Maintain at least:

### Debug

Maximum diagnostics, assertions, poisoning, tracing, and sanitizing checks where feasible.

### Hardened

Security validation enabled with realistic optimization.

### Release

Optimized with essential safety checks preserved.

---

# 6. REPOSITORY STRUCTURE

Recommended top-level tree:

```text
os/
├── arch/
│   └── x86_64/
│       ├── boot/
│       ├── cpu/
│       ├── context/
│       ├── interrupts/
│       ├── msr/
│       ├── paging/
│       ├── syscall/
│       ├── apic/
│       └── time/
│
├── kernel/
│   ├── core/
│   ├── init/
│   ├── panic/
│   ├── logging/
│   ├── topology/
│   ├── object/
│   └── module/
│
├── mm/
│   ├── pmm/
│   ├── vmm/
│   ├── heap/
│   ├── buddy/
│   ├── slub/
│   ├── percpu-cache/
│   ├── arena/
│   ├── hugepage/
│   ├── reclaim/
│   └── dma/
│
├── process/
│   ├── process/
│   ├── thread/
│   ├── scheduler/
│   ├── signal/
│   └── credentials/
│
├── sync/
│   ├── atomic/
│   ├── spinlock/
│   ├── mutex/
│   ├── rwlock/
│   ├── semaphore/
│   ├── futex/
│   ├── seqlock/
│   └── rcu/
│
├── ipc/
│   ├── pipe/
│   ├── queue/
│   ├── shared-memory/
│   ├── channel/
│   └── socket/
│
├── security/
│   ├── capability/
│   ├── handles/
│   ├── permissions/
│   ├── sandbox/
│   ├── audit/
│   └── crypto/
│
├── fs/
│   ├── vfs/
│   ├── page-cache/
│   ├── devfs/
│   ├── procfs/
│   ├── sysfs/
│   ├── tmpfs/
│   ├── initramfs/
│   └── ext2/
│
├── block/
│   ├── block-layer/
│   ├── io-scheduler/
│   └── bio/
│
├── drivers/
│   ├── pci/
│   ├── storage/
│   ├── network/
│   ├── usb/
│   ├── input/
│   ├── display/
│   ├── audio/
│   ├── timer/
│   └── virtual/
│
├── net/
│   ├── ethernet/
│   ├── arp/
│   ├── ipv4/
│   ├── ipv6/
│   ├── icmp/
│   ├── udp/
│   ├── tcp/
│   ├── dns/
│   ├── dhcp/
│   ├── tls/
│   └── socket/
│
├── trace/
│   ├── events/
│   ├── ring-buffer/
│   ├── bytecode/
│   ├── verifier/
│   └── consumers/
│
├── user/
│   ├── init/
│   ├── libc/
│   ├── shell/
│   ├── coreutils/
│   ├── services/
│   ├── gui/
│   ├── login/
│   ├── package-manager/
│   └── applications/
│
├── tools/
│   ├── image-builder/
│   ├── debugger/
│   ├── symbolizer/
│   ├── trace-tool/
│   ├── profiler/
│   └── test-runner/
│
├── tests/
│   ├── unit/
│   ├── kernel/
│   ├── syscall/
│   ├── fs/
│   ├── net/
│   ├── driver/
│   ├── stress/
│   └── fuzz/
│
├── docs/
│   ├── architecture/
│   ├── api/
│   ├── subsystems/
│   ├── design-decisions/
│   └── developer-guide/
│
├── scripts/
├── config/
├── linker/
├── boot/
├── Makefile / CMakeLists.txt
├── OS-SPEC.md
├── ROADMAP.md
├── ARCHITECTURE.md
└── SECURITY.md
```

---

# 7. BOOT ARCHITECTURE

## 7.1 Initial boot path

```text
UEFI
 ↓
Bootloader
 ↓
Kernel ELF
 ↓
Kernel entry
 ↓
Early CPU setup
 ↓
Memory map
 ↓
GDT / TSS
 ↓
IDT
 ↓
Interrupt controller
 ↓
Physical memory manager
 ↓
Virtual memory manager
 ↓
Kernel heap
 ↓
Scheduler
 ↓
Drivers
 ↓
Root filesystem
 ↓
init
 ↓
User space
```

## 7.2 Bootloader strategy

Start with a mature bootloader such as Limine or another suitable UEFI-capable loader.

Do not spend the first stage building a sophisticated bootloader.

Once the kernel is stable, optionally build a custom loader to deepen knowledge of:

- UEFI protocols,
- ELF loading,
- framebuffer discovery,
- ACPI discovery,
- memory maps,
- secure boot concepts.

---

# 8. PHASE 0 — DEVELOPMENT FOUNDATION

Before major kernel work:

- create repository,
- create contribution/development guide,
- establish coding rules,
- establish compiler warnings,
- establish static analysis where practical,
- create cross compiler,
- configure QEMU,
- configure GDB,
- establish serial logging,
- establish image generation,
- establish CI,
- establish boot smoke test.

## Exit criteria

```text
✓ Clean reproducible build
✓ Bootable image generated automatically
✓ QEMU starts automatically
✓ Kernel serial output collected
✓ Failure causes nonzero test result
✓ Symbols available to debugger
```

---

# 9. PHASE 1 — KERNEL BRING-UP

Implement:

- kernel entry,
- serial console,
- framebuffer console,
- boot logging,
- CPU identification,
- firmware memory map parsing,
- panic handler,
- assertions,
- basic stack trace.

Example log:

```text
[BOOT] Kernel starting
[CPU ] x86_64 detected
[MEM ] 4096 MiB discovered
[INIT] Architecture initialized
[INIT] Kernel initialized
```

## Exit criteria

```text
✓ Deterministic boot
✓ Structured logs
✓ Panic handler works
✓ Backtrace works
✓ Kernel can report memory and CPU topology basics
```

---

# 10. PHASE 2 — CPU SUBSYSTEM

## 10.1 GDT/TSS

Implement:

- kernel code/data segments,
- user code/data segments,
- TSS,
- interrupt stacks where needed.

## 10.2 IDT

Implement a complete exception/interrupt dispatch framework.

## 10.3 Exceptions

Handle at least:

- divide error,
- debug,
- NMI,
- breakpoint,
- overflow,
- bound range,
- invalid opcode,
- device-not-available,
- double fault,
- invalid TSS,
- segment not present,
- stack fault,
- general protection fault,
- page fault,
- x87/SIMD related exceptions as applicable.

## 10.4 Fault diagnostics

Example:

```text
KERNEL PANIC

Reason: General Protection Fault
CPU: 3
RIP: 0xffffffff801023fa
RSP: 0xffff90000122b7f0
RFLAGS: ...
Current process: shell
Current thread: 17

Registers:
...

Stack trace:
...

Recent events:
...
```

---

# 11. PHASE 3 — PER-CPU INFRASTRUCTURE

Introduce this early even before full SMP.

Per-CPU state should include:

- current CPU ID,
- current thread pointer,
- scheduler state,
- interrupt state,
- local allocator cache,
- local statistics,
- RCU state,
- tracing state,
- timer state,
- deferred work queues.

Conceptually:

```cpp
struct PerCpu {
    CpuId id;
    Thread* current;
    SchedulerState scheduler;
    LocalAllocatorCache allocator;
    RcuCpuState rcu;
    TraceCpuState trace;
    TimerState timers;
};
```

Avoid putting heavily modified global data on a single cache line.

---

# 12. PHASE 4 — PHYSICAL MEMORY MANAGER

## 12.1 Memory discovery

Discover:

- usable RAM,
- reserved RAM,
- firmware regions,
- MMIO regions,
- kernel image ranges,
- boot structures.

## 12.2 NUMA model

The architecture should support NUMA even if development begins on a one-node QEMU VM.

Model:

```text
Machine
├── NUMA Node 0
│   ├── CPUs
│   ├── memory ranges
│   └── zones
├── NUMA Node 1
│   ├── CPUs
│   ├── memory ranges
│   └── zones
└── topology/distances
```

## 12.3 Physical page metadata

Each physical page may eventually need metadata such as:

- node,
- zone,
- reference state,
- allocation order,
- flags,
- ownership/domain,
- mapping state.

Avoid excessive metadata overhead; choose representations deliberately.

---

# 13. HIERARCHICAL BUDDY ALLOCATOR

Use a buddy allocator for raw physical frames.

Responsibilities:

- allocate order N blocks,
- free order N blocks,
- split higher-order blocks,
- merge buddies,
- reserve physical ranges,
- release ranges,
- track fragmentation.

Base example:

```text
Order 0 → 4 KiB
Order 1 → 8 KiB
...
Order 9 → 2 MiB
Order 18 → 1 GiB
```

Exact limits depend on the supported physical-address space.

## Required statistics

- free pages,
- free blocks by order,
- allocation failures,
- fragmentation,
- local/remote NUMA allocations,
- high-order allocation failures.

---

# 14. NUMA-AWARE PMM

## 14.1 Allocation order

Preferred policy:

```text
CPU-local node
 ↓
Preferred node
 ↓
Near node
 ↓
Remote node
```

## 14.2 Policies

Eventually support:

- local,
- preferred node,
- interleaved,
- bind-to-node-set,
- fallback policies.

## 14.3 NUMA metrics

Track:

```text
memory.local_allocations
memory.remote_allocations
memory.remote_bytes
memory.node_pressure
memory.high_order_failures
```

## 14.4 Scheduler integration

Later, scheduling can consider:

- thread CPU locality,
- memory locality,
- device locality,
- cache locality,
- IRQ locality.

Do not make aggressive migration policies before benchmarks exist.

---

# 15. DMA-AWARE MEMORY

Separate generic memory from memory intended for devices.

Support concepts such as:

- DMA-capable regions,
- physically contiguous allocations where required,
- DMA mapping/unmapping,
- cache coherency rules,
- bounce buffers,
- IOMMU integration.

Never pass a normal kernel object to a device simply because it has a virtual address.

---

# 16. VIRTUAL MEMORY MANAGER

## 16.1 Base page support

Support x86-64 paging with normal 4 KiB pages first.

Implement:

- page table allocation,
- page mapping,
- unmapping,
- permission changes,
- address-space creation,
- page-fault handling,
- user/kernel separation.

## 16.2 Page permissions

Use explicit permissions:

```text
Executable code → RX
Read-only data → R
Writable data → RW
```

Avoid `RWX` mappings whenever possible.

## 16.3 Future features

- copy-on-write,
- lazy allocation,
- demand paging,
- memory-mapped files,
- page cache integration,
- guard pages,
- huge-page mappings,
- address-space randomization.

---

# 17. DIRECT PHYSICAL MEMORY MAPPING

Provide a controlled kernel mapping of physical memory where appropriate.

Requirements:

- correct cache attributes,
- correct permission model,
- separation of RAM and MMIO,
- no user exposure of kernel physical addresses.

---

# 18. TLB MANAGEMENT

Design explicitly for:

- TLB invalidation,
- TLB shootdowns,
- PCID where supported,
- INVPCID where supported,
- address-space switch optimization,
- per-CPU TLB state.

## Metrics

```text
TLB shootdowns/sec
shootdown latency
TLB-related faults/misses where measurable
address-space switch cost
```

---

# 19. HUGE PAGES

Supported page sizes:

```text
4 KiB
2 MiB
1 GiB
```

Terminology must stay precise.

`PGE` refers to Page Global Enable; it is not the mechanism by which 1 GiB pages are represented.

## 19.1 Policy

Do not use huge pages indiscriminately.

Possible policies:

```text
NORMAL
PREFER_2M
PREFER_1G
REQUIRE_HUGE
NO_HUGE
```

Potential use cases:

- kernel text/data where alignment permits,
- large anonymous mappings,
- large database-like workloads,
- large caches,
- large shared mappings.

## 19.2 Promotion/demotion

Later implement:

```text
4 KiB pages × 512
        ↓
2 MiB mapping
```

and potentially:

```text
2 MiB pages × 512
        ↓
1 GiB mapping
```

Demotion must also exist when a large mapping is no longer appropriate.

---

# 20. KPTI / KERNEL ADDRESS-SPACE ISOLATION

User execution should use page tables that do not expose the full kernel mapping.

Conceptually:

```text
USER CR3
├── user mappings
└── minimal entry/trampoline mappings

KERNEL CR3
├── kernel text
├── kernel data
├── kernel heap
├── direct map
└── device mappings
```

Design with:

- KPTI-style isolation,
- PCID,
- INVPCID,
- syscall entry/exit paths,
- minimal trampoline mappings,
- speculative-execution mitigations,

in mind.

Do not claim that one mechanism solves all speculative-execution vulnerabilities.

---

# 21. KERNEL HEAP

## 21.1 First implementation

Provide:

```text
kmalloc()
kfree()
```

with debugging support.

## 21.2 Production architecture

Eventually:

```text
kmalloc
 ↓
per-CPU cache
 ↓
SLUB/object cache
 ↓
per-node memory
 ↓
buddy allocator
```

---

# 22. SLUB-STYLE OBJECT ALLOCATOR

Use caches for high-frequency kernel types:

- process,
- thread,
- inode,
- file,
- socket,
- network buffer,
- VMA-like structures,
- page-table objects,
- capability objects.

Each cache should be aware of:

- object alignment,
- constructor/destructor semantics,
- per-CPU free objects,
- NUMA locality,
- poisoning/debug mode.

---

# 23. PER-CPU MAGAZINES / ALLOCATION CACHES

Fast path:

```text
CPU
 ↓
local cache
 ↓
allocate/free
```

Slow path:

```text
local cache empty
 ↓
refill batch from node-local cache
 ↓
buddy allocator if needed
```

Goals:

- eliminate global allocator locks for common paths,
- reduce cache-line bouncing,
- preserve NUMA locality,
- make allocation latency predictable.

---

# 24. SPECIALIZED MEMORY ALLOCATORS

Maintain separate allocation domains for:

- physical frames,
- page tables,
- small kernel objects,
- DMA memory,
- contiguous memory,
- executable memory,
- network buffers,
- per-request arenas,
- graphics buffers.

Do not create one allocator with dozens of mysterious flags and undocumented interactions.

---

# 25. LIFETIME-BOUND ARENAS

Arena/region allocation is suitable for:

- syscall argument processing,
- packet parsing,
- temporary path resolution,
- configuration parsing,
- ELF loading stages,
- short-lived kernel tasks.

Example:

```cpp
Arena arena(64_KiB);

auto header = arena.allocate<Header>();
auto parsed = arena.allocate<ParsedObject>();
auto buffer = arena.allocate_bytes(4096);

// all temporary allocations are released together
arena.reset();
```

## Rules

- arena-owned objects must not outlive the arena,
- long-lived structures must not silently retain arena pointers,
- arena destruction must invalidate outstanding borrows in debug builds where practical.

---

# 26. MEMORY RECLAIM

Later implement:

- cache reclaim,
- page cache reclaim,
- memory pressure notifications,
- background reclaim,
- OOM handling,
- configurable reclaim policies.

Very late-stage candidates:

- compressed memory,
- swap,
- deduplication.

Do not implement these before the core memory system is proven stable.

---

# 27. MEMORY DEBUGGING

Debug/hardened builds should support:

- guard pages,
- allocation cookies,
- red zones,
- memory poisoning,
- double-free detection,
- quarantine lists,
- use-after-free detection,
- page ownership diagnostics,
- leak tracking where practical.

Possible lifecycle:

```text
ALLOCATED
 ↓
ACTIVE
 ↓
FREED
 ↓
QUARANTINED
 ↓
RECLAIMABLE
 ↓
REUSED
```

---

# 28. CPU / THREAD MODEL

Core entities:

```text
Machine
 └── CPU
      └── Thread
            └── Process / Address Space
```

A process should contain:

- address space,
- handle table,
- credentials,
- current directory,
- environment,
- threads,
- resource limits.

A thread should contain:

- CPU state,
- stack,
- scheduling metadata,
- TLS/thread-local state,
- signal/event state,
- accounting metadata.

---

# 29. THREAD STATES

Use an explicit state machine:

```text
NEW
 ↓
READY
 ↓
RUNNING
 ├── BLOCKED
 ├── SLEEPING
 ├── STOPPED
 ├── EXITING
 └── TERMINATED
```

Invalid state transitions should be rejected in debug builds.

---

# 30. SCHEDULER

## 30.1 Initial scheduler

Implement:

- preemption,
- round robin,
- sleep/wakeup,
- timer-driven timeslices,
- blocking/unblocking,
- basic priorities.

## 30.2 Multicore scheduler

Later implement:

- per-CPU run queues,
- CPU affinity,
- load balancing,
- work stealing where justified,
- NUMA awareness,
- interrupt affinity.

## 30.3 Scheduler classes

Long-term support may include:

- normal class,
- high-priority interactive class,
- real-time class,
- optional deadline class.

Do not overbuild the scheduler before workload measurements exist.

---

# 31. CONTEXT SWITCHING

Implement and benchmark:

- register save/restore,
- stack switching,
- address-space switching,
- FPU/SIMD state handling as appropriate,
- thread-local state.

Metrics:

```text
context_switch_ns
scheduler_latency_ns
runqueue_wait_ns
```

---

# 32. SYNCHRONIZATION MODEL

Provide:

- atomics,
- spinlocks,
- mutexes,
- read/write locks,
- semaphores,
- condition variables,
- futex-like primitives,
- seqlocks where appropriate,
- RCU,
- lock-free structures where justified.

---

# 33. LOCKING RULES

Mandatory rules:

- Do not sleep while holding a spinlock.
- Do not block in an interrupt handler.
- Do not call arbitrary blocking allocators from interrupt context.
- Avoid nested locks unless the lock hierarchy is documented.
- Avoid giant global locks in hot paths.
- Document lock ownership.
- Document lock ordering.
- Measure contention.

---

# 34. LOCK HIERARCHY

Maintain a documented lock ordering system:

```text
higher-level subsystem lock
        ↓
object lock
        ↓
cache lock
        ↓
low-level hardware lock
```

Actual hierarchy must be recorded per subsystem.

Debug builds should optionally detect lock-order inversions.

---

# 35. RCU

Use RCU for read-mostly structures where it improves scalability and lifetime management.

Potential candidates:

- read-mostly VFS metadata,
- routing tables,
- credentials snapshots,
- namespace metadata,
- device registries,
- immutable configuration snapshots.

Avoid RCU as a universal synchronization mechanism.

Conceptual API:

```cpp
rcu_read_lock();
Node* n = rcu_dereference(root);
use(n);
rcu_read_unlock();

call_rcu(old, reclaim);
```

The key semantics are:

```text
remove from publication
 ↓
wait for grace period
 ↓
reclaim old object
```

---

# 36. RCU INTERNALS

Eventually track:

- per-CPU read-side state,
- grace-period epoch/state,
- callback queues,
- quiescent states,
- stalled grace periods.

Tracing should expose:

```text
rcu.grace_period_started
rcu.grace_period_completed
rcu.callbacks_pending
rcu.stall_detected
```

---

# 37. LOCK-FREE RING BUFFERS

Start with SPSC.

Then, if justified:

- MPSC,
- SPMC,
- MPMC.

Candidates:

- tracing,
- driver event delivery,
- high-throughput IPC,
- network queues,
- logging.

Every lock-free structure must document:

- producer ownership,
- consumer ownership,
- memory ordering,
- wraparound behavior,
- reclamation policy,
- ABA defense.

---

# 38. ATOMIC MEMORY ORDERING

Allowed semantic levels include:

```text
relaxed
acquire
release
acq_rel
seq_cst
```

Do not use `seq_cst` everywhere merely because it is easiest.

Do not weaken ordering merely because it is faster.

For every non-trivial lock-free algorithm document the happens-before relationship.

---

# 39. ABA / OBJECT RECLAMATION

Potential mechanisms:

- generation counters,
- tagged pointers,
- hazard pointers,
- epochs,
- RCU,
- ownership transfer.

Never implement lock-free pointer structures without an explicit reclamation strategy.

---

# 40. OBJECT LIFECYCLE MODEL

Core lifecycle:

```text
ALLOCATED
 ↓
INITIALIZED
 ↓
PUBLISHED
 ↓
IN_USE
 ↓
CLOSING
 ↓
UNPUBLISHED
 ↓
RETIRED
 ↓
RECLAIMABLE
 ↓
FREED
```

An object must not be reclaimed while any valid consumer can still access it.

---

# 41. TYPE-SAFE OWNERSHIP

Provide internal pointer abstractions such as:

```text
Unique<T>
Shared<T>
Weak<T>
Borrowed<T>
Ref<T>
```

Meaning must be unambiguous.

### Unique

Single owner; move-only.

### Shared

Reference-counted ownership; use sparingly on hot paths.

### Weak

Non-owning reference that can detect expiration.

### Borrowed

Non-owning temporary reference.

### Ref

Explicit intrusive/reference-counted ownership where required.

---

# 42. RAII RESOURCE MANAGEMENT

Use RAII for:

- lock guards,
- interrupt-state guards,
- temporary mappings,
- transaction scopes,
- file handles,
- device references,
- arena scopes,
- resource reservations.

Example:

```cpp
{
    LockGuard guard(lock);
    modify_state();
} // lock is released here
```

The kernel must remain able to reason about lifetimes without relying on exceptions.

---

# 43. USER/KERNEL BOUNDARY

The syscall boundary should look like:

```text
User code
 ↓
syscall instruction
 ↓
entry trampoline
 ↓
CPU state capture
 ↓
security context
 ↓
syscall dispatch
 ↓
argument validation
 ↓
handle validation
 ↓
capability validation
 ↓
subsystem operation
 ↓
result
 ↓
return path
```

Never trust user memory.

---

# 44. HANDLE MODEL

User applications receive opaque handles rather than internal pointers.

Conceptually:

```text
User handle 17
 ↓
process handle table
 ↓
object capability
 ↓
object identity
 ↓
rights
 ↓
generation
 ↓
object
```

A stale handle must fail instead of accidentally referring to a newly reused object.

---

# 45. CAPABILITY-BASED SECURITY

This should be a defining architectural property.

A capability represents authority to operate on an object.

Conceptually:

```text
Capability<T>
├── object identity
├── generation/version
├── rights
└── security domain/context
```

Examples:

```text
Capability<File>
Capability<Socket>
Capability<Device>
Capability<Process>
Capability<SharedMemory>
```

---

# 46. RIGHTS MODEL

Example file rights:

```text
READ
WRITE
EXECUTE
STAT
MAP
ADMIN
```

Socket rights:

```text
CONNECT
BIND
LISTEN
ACCEPT
SEND
RECEIVE
ADMIN
```

Device rights:

```text
READ
WRITE
MMIO
DMA
RESET
ADMIN
```

Rights should be independently represented where practical.

---

# 47. CAPABILITY DERIVATION

A process should be able to derive a restricted capability from a broader capability.

Example:

```text
FILE_READ + FILE_WRITE
        ↓
remove FILE_WRITE
        ↓
FILE_READ only
```

A lower-rights capability must never be able to reacquire removed rights merely by duplication.

---

# 48. CAPABILITY REVOCATION

Support object generation/versioning and revocation.

Example:

```text
Object 51 generation 3
        ↓
destroy
        ↓
Object 51 generation 4
```

Old capability:

```text
51:g3
```

must fail validation against:

```text
51:g4
```

---

# 49. NO RAW KERNEL ADDRESSES TO USER SPACE

Never expose kernel virtual addresses or physical kernel addresses through ordinary application interfaces.

Use:

- opaque handles,
- capabilities,
- explicitly mapped memory,
- validated shared-memory objects.

---

# 50. PROCESS API

Provide coherent primitives such as:

```text
spawn
exec
exit
wait
kill
getpid
gettid
mmap
munmap
mprotect
```

The exact ABI may differ from POSIX/Linux, but the semantics must be documented.

---

# 51. ELF EXECUTION

User-space process loading should support:

- ELF parsing,
- program headers,
- section/segment mapping,
- permissions,
- stack creation,
- argument passing,
- environment,
- auxiliary metadata,
- dynamic linking later.

The ELF loader must be heavily fuzzed because executable parsing is untrusted-input parsing.

---

# 52. SYSTEM CALL ABI

Organize syscalls by domain:

```text
process
memory
filesystem
IPC
network
system
security
```

Example filesystem calls:

```text
open
close
read
write
seek
stat
mkdir
unlink
rename
```

Network:

```text
socket
bind
listen
accept
connect
send
receive
```

Every syscall needs:

- ABI definition,
- argument layout,
- return values,
- errors,
- blocking behavior,
- cancellation rules,
- security requirements.

---

# 53. ERROR MODEL

Use stable symbolic errors such as:

```text
SUCCESS
INVALID_ARGUMENT
NOT_FOUND
PERMISSION_DENIED
OUT_OF_MEMORY
TIMEOUT
BUSY
IO_ERROR
NOT_SUPPORTED
INTERRUPTED
ALREADY_EXISTS
INVALID_HANDLE
STALE_HANDLE
```

Do not make debugging depend on opaque numeric failures.

---

# 54. IPC ARCHITECTURE

Implement progressively:

1. Pipes.
2. Events/signals.
3. Shared memory.
4. Message queues.
5. Local/domain sockets.
6. Typed capability-based channels.

Possible future native IPC model:

```text
Sender
 ↓
Capability/channel
 ↓
Message
 ↓
Receiver
```

---

# 55. VIRTUAL FILESYSTEM

VFS should abstract:

- open,
- close,
- read,
- write,
- seek,
- stat,
- mkdir,
- unlink,
- rename,
- mount,
- unmount.

Architecture:

```text
Application
 ↓
Syscall
 ↓
VFS
 ↓
Filesystem implementation
 ↓
Block layer
 ↓
Storage driver
```

---

# 56. INITIAL FILESYSTEM STRATEGY

Start with:

```text
initramfs
```

Then a simple persistent filesystem such as ext2 for early development.

Only after VFS and block I/O are reliable should you design a custom filesystem.

---

# 57. FUTURE NATIVE FILESYSTEM

A future custom filesystem could support:

- journaling,
- checksums,
- copy-on-write,
- snapshots,
- atomic metadata updates,
- quotas,
- extended attributes,
- encryption,
- compression,
- integrity verification.

The custom filesystem should be a late-stage project, not the first filesystem implementation.

---

# 58. SPECIAL FILESYSTEMS

Eventually provide:

```text
/dev
/proc
/sys
/tmp
```

Examples:

```text
/proc/cpuinfo
/proc/meminfo
/proc/uptime
/proc/processes
/sys/devices
/sys/memory
/sys/cpus
```

These interfaces are useful for observability and administration.

---

# 59. PAGE CACHE

Add a page-cache layer between VFS/filesystems and storage.

Responsibilities:

- caching file data,
- dirty page tracking,
- writeback,
- cache lookup,
- eviction,
- memory-pressure interaction.

Track:

```text
cache_hits
cache_misses
dirty_pages
writeback_bytes
reclaim_events
```

---

# 60. BLOCK LAYER

Create an abstraction between filesystems and device drivers.

Conceptually:

```text
Filesystem
 ↓
Block request
 ↓
Block layer
 ↓
I/O scheduling
 ↓
Storage driver
 ↓
Hardware
```

Later implement:

- request merging,
- queueing,
- priority,
- batching,
- asynchronous I/O.

---

# 61. DEVICE MODEL

Create a unified device model with:

- discovery,
- registration,
- driver matching,
- lifecycle,
- resource ownership,
- power state,
- hotplug.

Conceptually:

```text
Device
├── bus
├── driver
├── resources
├── state
├── capabilities
└── children
```

---

# 62. PCI / PCIe

Implement:

- enumeration,
- BAR discovery,
- configuration space access,
- MSI,
- MSI-X,
- interrupt routing,
- device reset where supported.

PCI is an important dependency for storage/network/GPU support.

---

# 63. STORAGE DRIVERS

Prioritize:

1. virtio-blk.
2. AHCI/SATA.
3. NVMe.

Later support additional controllers as required.

NVMe should become an important native modern-storage target.

---

# 64. INPUT

Initial:

- PS/2 keyboard,
- PS/2 mouse.

Then:

- USB HID keyboard,
- USB HID mouse.

Later:

- touchpads,
- touchscreens,
- game controllers.

---

# 65. USB

USB deserves its own staged roadmap.

```text
USB core
 ↓
xHCI
 ↓
device enumeration
 ↓
USB HID
 ↓
keyboard/mouse
```

Later:

- storage,
- webcams,
- audio,
- other classes.

---

# 66. INTERRUPT HANDLING RULE

Interrupt handlers should be minimal.

Preferred model:

```text
Interrupt
 ↓
acknowledge hardware
 ↓
capture minimal state
 ↓
queue/defer work
 ↓
return
```

Avoid expensive allocation, filesystem operations, logging floods, or sleeping inside interrupt context.

---

# 67. DRIVER LIFECYCLE

Where applicable, use an interface such as:

```text
probe
init
start
stop
read
write
ioctl
interrupt
remove
suspend
resume
```

Driver APIs must clearly document which operations can block.

---

# 68. NETWORKING STACK

Build in layers:

```text
Ethernet
 ↓
ARP
 ↓
IPv4
 ↓
ICMP
 ↓
UDP
 ↓
TCP
 ↓
DNS
 ↓
Sockets
```

Then:

```text
IPv6
DHCP
TLS
HTTP
```

Later:

- Wi-Fi,
- Bluetooth,
- VPN,
- advanced packet filtering.

---

# 69. NETWORK SECURITY

Treat all packets as hostile input.

Required defenses include:

- packet length validation,
- integer-overflow checks,
- checksum validation,
- connection/resource limits,
- rate limiting where appropriate,
- firewall support,
- capability-controlled network operations.

---

# 70. SOCKET API

Provide:

- stream sockets,
- datagram sockets,
- local/domain sockets.

Eventually:

- raw sockets for privileged applications,
- packet sockets,
- higher-level networking APIs.

---

# 71. USER SPACE INIT

Once user process execution is reliable, create a minimal `init`.

Responsibilities:

- mount initial filesystems,
- start essential services,
- configure logging,
- start login/console,
- manage shutdown.

---

# 72. SHELL

Build a native shell outside the kernel.

Features:

- command execution,
- pipes,
- input/output redirection,
- environment variables,
- history,
- aliases,
- autocomplete,
- job control,
- scripting.

Initial commands:

```text
ls
cd
pwd
cat
cp
mv
rm
mkdir
rmdir
touch
echo
grep
find
ps
kill
top
mount
umount
df
du
free
ip
ping
curl
clear
reboot
shutdown
```

---

# 73. SERVICE MANAGER

Build a service manager such as `os-init` later.

Capabilities:

- service start/stop,
- dependency ordering,
- restart on failure,
- logs,
- status,
- shutdown ordering.

Example:

```text
network.service
storage.service
logger.service
display.service
login.service
```

---

# 74. SECURITY MODEL

Minimum security foundations:

- user/group identities,
- capabilities,
- file permissions,
- process isolation,
- address-space isolation,
- least privilege,
- audit events.

Advanced:

- sandboxing,
- per-application permissions,
- secure boot,
- signed modules,
- signed packages,
- disk encryption,
- IOMMU isolation,
- application policy.

---

# 75. APPLICATION SANDBOX

A long-term application model:

```text
Application
 ↓
Sandbox
├── filesystem permissions
├── network permissions
├── device permissions
├── process permissions
├── memory limits
└── resource quotas
```

Application manifests can declare required capabilities.

---

# 76. IOMMU

Later support:

- DMA remapping,
- per-device isolation,
- device capability boundaries.

This is particularly important for untrusted or third-party devices.

---

# 77. SECURE BOOT

Late-stage work:

- signed bootloader,
- signed kernel,
- trusted key chain,
- measured boot where appropriate,
- secure update verification.

Do not implement custom cryptography.

Use established primitives and libraries/implementations.

---

# 78. CRYPTOGRAPHY

Never invent cryptographic algorithms.

Use established standards and well-reviewed implementations.

The kernel may need cryptographic primitives for:

- secure boot,
- package verification,
- random-number generation,
- encrypted storage,
- secure IPC/authentication mechanisms.

---

# 79. TIME SUBSYSTEM

Provide:

- RTC,
- monotonic time,
- wall-clock time,
- high-resolution timers,
- sleep,
- timeout infrastructure,
- CPU timers.

Applications should distinguish:

```text
CLOCK_REALTIME
CLOCK_MONOTONIC
```

Timeout logic should use monotonic time.

---

# 80. POWER MANAGEMENT

Later:

- ACPI,
- CPU idle,
- frequency control,
- suspend,
- hibernate,
- shutdown,
- reboot,
- thermal management,
- battery monitoring.

Laptop support should be a dedicated milestone rather than mixed into initial kernel development.

---

# 81. SMP / MULTICORE

After single-core correctness:

- CPU discovery,
- AP startup,
- per-CPU structures,
- inter-processor interrupts,
- per-CPU scheduling,
- load balancing,
- affinity,
- cross-CPU TLB shootdowns.

Test progressively:

```text
1 CPU
2 CPUs
4 CPUs
8 CPUs
16+ CPUs
```

---

# 82. HARDWARE TOPOLOGY

Build a topology database from firmware and bus information.

```text
Machine
├── Socket
│   ├── CPU
│   ├── Cache
│   └── NUMA Node
│
├── PCI Root
│   ├── GPU
│   ├── NIC
│   └── NVMe
│
└── USB Controllers
```

This enables locality-aware decisions later.

---

# 83. RUNTIME OBJECT MODEL

Kernel objects should have explicit identity and lifecycle.

Examples:

- process,
- thread,
- file,
- inode,
- socket,
- device,
- shared-memory segment,
- capability.

Each object should document:

- allocation source,
- initialization stage,
- references,
- public visibility,
- revocation,
- retirement,
- final destruction.

---

# 84. OBSERVABILITY ARCHITECTURE

Observability is a core subsystem, not a debug-only feature.

The unified model should be:

```text
                       Kernel Event
                            │
             ┌──────────────┼──────────────┐
             ▼              ▼              ▼
           Logs           Trace           Audit
             │              │              │
             └──────────────┼──────────────┘
                            ▼
                       Event buffers
                            ▼
                         Userland
```

---

# 85. LOGGING

Levels:

```text
TRACE
DEBUG
INFO
WARN
ERROR
FATAL
```

Subsystem tags:

```text
[MM]
[SCHED]
[FS]
[NET]
[USB]
[PCI]
[GPU]
[SEC]
[RCU]
[TRACE]
```

Example:

```text
[INFO][NVME] controller initialized
[INFO][FS] mounted root filesystem
[WARN][NET] DHCP timeout
[ERROR][USB] device descriptor rejected
```

Release builds should avoid uncontrolled printf-style logging in hot paths.

---

# 86. STATIC TRACEPOINTS

Tracepoints should have very low overhead when disabled.

Example event sites:

- syscall entry,
- syscall exit,
- process create,
- process exit,
- context switch,
- page fault,
- allocation,
- free,
- block I/O,
- packet RX/TX,
- file open,
- lock contention,
- RCU grace period.

Conceptually:

```text
tracepoint hit
 ↓
enabled?
 ├── no → return quickly
 └── yes → collect event
```

---

# 87. TRACE EVENTS

Every event should include suitable metadata such as:

```text
timestamp
CPU
process ID
thread ID
event type
sequence number
payload
```

Optional:

- NUMA node,
- priority,
- callsite ID,
- object ID.

---

# 88. LOCK-FREE TRACE RING BUFFER

Tracing should use per-CPU or otherwise carefully partitioned buffers where practical.

Goals:

- low writer contention,
- bounded memory behavior,
- efficient streaming to userland,
- sequence-numbered records,
- loss reporting when buffers overflow.

The system should never silently pretend that an overloaded tracing buffer captured everything.

---

# 89. BYTECODE FILTER ENGINE

Long-term, allow safe user-space programs to filter trace events.

Example logical filter:

```text
if event == SYSCALL_EXIT
and pid == 1234
and latency > 1000 ns
then emit
```

Execution model:

```text
Tracepoint
 ↓
Filter program
 ↓
accept/reject
 ↓
event buffer
```

---

# 90. BYTECODE VERIFIER

User-provided tracing bytecode must not execute arbitrary machine instructions.

The verifier should prove/enforce properties such as:

- bounded execution,
- valid control flow,
- bounded stack usage,
- bounded memory access,
- no arbitrary kernel pointer dereference,
- restricted helper calls,
- no sleeping,
- no uncontrolled loops.

Start with an interpreter.

Only later consider JIT compilation.

---

# 91. TRACE VM ROADMAP

### v1

Interpreter.

### v2

Verifier hardening and richer event access.

### v3

JIT compilation.

### v4

Per-CPU optimization and event batching.

### v5

Persistent diagnostic profiles.

---

# 92. METRICS

Expose structured counters such as:

```text
scheduler.context_switches
scheduler.preemptions
memory.page_faults
memory.remote_allocations
memory.tlb_shootdowns
memory.oom_events
fs.cache_hits
fs.cache_misses
net.packet_drops
net.tcp_retransmits
rcu.grace_periods
locks.contention
trace.events_lost
```

---

# 93. "WHY IS THIS SLOW?" TOOLING

Eventually provide an `os-monitor` or similar tool that can correlate:

- CPU usage,
- scheduler delay,
- run-queue wait,
- page faults,
- NUMA locality,
- I/O latency,
- lock contention,
- syscall latency,
- network waits.

Example:

```text
process 182 performance summary
--------------------------------
CPU utilization:       76%
Scheduler wait:         4.2 ms
Page faults:            12,481
Remote memory:           8.4 MB
Lock wait:               1.7 ms
Disk latency:             0.9 ms
Syscall avg:             410 ns
```

The exact numbers are illustrative; the tool's job is correlation and diagnosis.

---

# 94. KERNEL CRASH HANDLING

Every panic should collect:

- reason,
- CPU,
- process,
- thread,
- registers,
- instruction pointer,
- stack pointer,
- page-fault address where applicable,
- stack trace,
- loaded modules,
- recent logs,
- recent trace events,
- memory status,
- lock ownership state where available.

Long-term output:

```text
crash dump
```

that can be symbolized offline.

---

# 95. CRASH DUMP PIPELINE

```text
Kernel failure
 ↓
freeze/stop secondary CPUs where appropriate
 ↓
collect safe diagnostic state
 ↓
write crash record
 ↓
reboot or enter diagnostic shell
 ↓
user-space symbolizer
 ↓
human-readable report
```

Avoid doing unsafe operations after catastrophic kernel corruption.

---

# 96. LOCK OBSERVABILITY

Debug/hardened builds may collect:

- acquisition count,
- contention count,
- maximum wait,
- current owner,
- CPU,
- callsite,
- hold duration.

Use these metrics to find scalability bottlenecks rather than guessing.

---

# 97. DEADLOCK DETECTION

Optionally maintain a lock dependency graph:

```text
A → B
B → C
C → A
```

A cycle should produce a diagnostic such as:

```text
Potential deadlock

Thread 17:
  holds A
  waits B

Thread 21:
  holds B
  waits C

Thread 4:
  holds C
  waits A
```

Do not depend on perfect runtime deadlock detection; prevention through hierarchy is still primary.

---

# 98. GRAPHICS ROADMAP

Do not start with a graphical desktop.

## Stage 1

- framebuffer output.

## Stage 2

- 2D renderer,
- font rendering,
- input events.

## Stage 3

- compositor,
- window manager.

## Stage 4

- desktop shell,
- file manager,
- settings,
- taskbar,
- notifications,
- clipboard.

## Stage 5

- hardware-accelerated graphics.

Potential eventual interfaces:

- OpenGL compatibility,
- Vulkan support,
- native graphics API.

Do not implement a custom GPU driver before the rest of the architecture is stable.

---

# 99. DESKTOP ENVIRONMENT

Target components:

```text
Desktop
├── Compositor
├── Window Manager
├── Taskbar/Dock
├── Launcher
├── File Manager
├── Terminal
├── Settings
├── Notifications
├── Clipboard Manager
└── Login Manager
```

Desktop components belong in user space wherever practical.

---

# 100. AUDIO ROADMAP

Later:

- audio subsystem,
- device abstraction,
- mixer,
- volume,
- audio server,
- microphone,
- speakers,
- Bluetooth audio.

Do not prioritize audio above scheduler, memory, VFS, networking, and security.

---

# 101. PACKAGE MANAGER

Eventually provide commands such as:

```text
os-pkg search <name>
os-pkg install <name>
os-pkg remove <name>
os-pkg update
os-pkg upgrade
os-pkg info <name>
```

Package metadata should include:

- name,
- version,
- architecture,
- dependencies,
- permissions,
- files,
- hashes,
- signatures,
- source repository.

---

# 102. SOFTWARE REPOSITORY

Long-term model:

```text
Official Repository
 ↓
Package Manager
 ↓
Signature verification
 ↓
Dependency resolution
 ↓
Installation sandbox
 ↓
Application/runtime
```

Never execute downloaded software solely because it was downloaded successfully.

---

# 103. APPLICATION MANIFEST

Native applications may eventually include:

```text
application
├── executable
├── manifest
├── permissions
├── resources
├── configuration
└── metadata
```

The manifest can declare required capabilities.

---

# 104. UPDATE SYSTEM

A production-oriented system should eventually support:

- signed updates,
- version verification,
- atomic updates,
- staged deployment,
- rollback,
- failure detection.

Ideal model:

```text
Current System
      │
      ├── A/B slot or snapshot A
      └── update candidate B
              ↓
          validate
              ↓
            boot B
              ↓
      health check / confirmation
              ↓
          mark B active
```

If validation fails, retain a known-working system.

---

# 105. RECOVERY MODE

Provide eventual modes such as:

- normal boot,
- safe mode,
- recovery mode,
- diagnostic mode,
- single-user mode.

Recovery features:

- filesystem repair,
- log extraction,
- configuration rollback,
- update rollback,
- driver disablement,
- boot repair.

---

# 106. BACKUP / SNAPSHOTS

Long-term support:

- filesystem snapshots,
- system snapshots,
- configuration backup,
- restore,
- rollback.

Snapshots should integrate with the update architecture where possible.

---

# 107. PERFORMANCE ENGINEERING

Do not optimize by intuition alone.

Create benchmarks for:

- allocation,
- deallocation,
- syscall latency,
- context switch,
- IPC,
- mutex contention,
- RCU read path,
- page fault,
- VFS lookup,
- filesystem throughput,
- storage latency,
- network throughput,
- TCP latency,
- NUMA-local memory access,
- NUMA-remote access,
- TLB behavior.

---

# 108. PERFORMANCE REGRESSION FORMAT

Every performance-sensitive change should capture:

```text
commit
hardware
compiler
configuration
workload
baseline
new result
delta
variance
```

Example:

```text
Benchmark: syscall_null
Baseline:  412 ns
Current:   387 ns
Delta:     -6.1%
```

Numbers shown here are examples, not project targets.

---

# 109. TESTING STRATEGY

The OS must have multiple test layers.

## Unit tests

For:

- allocators,
- parsers,
- data structures,
- state machines,
- capability checks.

## Integration tests

For:

- process creation,
- filesystem access,
- networking,
- drivers.

## System tests

Boot OS and verify behavior end-to-end.

## Stress tests

Push concurrency and resource limits.

## Fuzz tests

Feed malformed data into parser boundaries.

---

# 110. FUZZING TARGETS

Prioritize:

- ELF parser,
- filesystem metadata parser,
- network packet parser,
- syscall argument processing,
- capability/handle validation,
- USB descriptors,
- package metadata,
- configuration parser.

Any externally influenced parser should be considered hostile-input code.

---

# 111. STRESS TESTING

Examples:

```text
1000 processes
10000 threads
millions of allocations
rapid allocate/free
rapid fork/exec-like cycles
concurrent filesystem operations
network packet floods
storage queue saturation
multicore lock contention
RCU reader storms
```

The actual limits depend on available memory and hardware.

---

# 112. FAULT INJECTION

Intentionally simulate:

- out of memory,
- disk failure,
- network timeout,
- device disappearance,
- malformed packet,
- driver failure,
- invalid syscall,
- corrupted metadata,
- lock delays,
- lost device interrupts.

Verify whether the OS:

```text
recovers safely
or
fails predictably with useful diagnostics
```

---

# 113. FORMAL INVARIANTS

Maintain a written invariant list.

Examples:

```text
A user process cannot directly access kernel memory.

A user pointer is validated before kernel dereference.

A terminated thread cannot be scheduled.

A stale handle cannot reference a newly reused object.

An object cannot be reclaimed while a valid consumer can still reach it.

A spinlock-held path cannot sleep.

An interrupt handler cannot block.

A capability cannot gain rights through duplication.

Executable writable memory is prohibited unless explicitly justified.

A freed physical page cannot remain validly mapped into an active address space.
```

These invariants should become tests where practical.

---

# 114. SECURITY REGRESSION TESTING

Automate tests for:

- user/kernel isolation,
- capability checking,
- stale handles,
- permission enforcement,
- address-space permissions,
- sandbox restrictions,
- DMA isolation where available,
- syscall validation,
- package signature verification.

---

# 115. STATIC ANALYSIS / CODE QUALITY

Use:

- aggressive compiler warnings,
- static analysis,
- formatting checks,
- API documentation checks,
- dependency checks.

Aim for warnings-as-errors in controlled build stages.

Document justified exceptions.

---

# 116. MEMORY SAFETY RULES

Mandatory kernel rules:

```text
No unchecked user pointer dereference.
No unchecked pointer arithmetic.
No use-after-free.
No double free.
No sleeping in atomic contexts.
No allocation that can block in interrupt context.
No raw kernel pointer in user ABI.
No executable writable kernel mapping by default.
```

---

# 117. API DESIGN REQUIREMENTS

Every non-trivial public API must document:

- ownership,
- lifetime,
- thread safety,
- blocking behavior,
- errors,
- cancellation behavior,
- privilege requirements,
- memory allocation behavior.

Example:

```cpp
Result<FileHandle> open(const Path& path, OpenFlags flags);
```

The documentation must state exactly what the returned handle owns and what happens when the call fails.

---

# 118. DRIVER SAFETY MODEL

Drivers are high-risk code.

Eventually investigate isolating some drivers behind stronger boundaries.

Long-term possible architecture:

```text
User/Service
 ↓
Driver IPC/API
 ↓
Driver execution context
 ↓
Hardware capability
 ↓
IOMMU/device isolation
```

Do not attempt a complete driver-isolation architecture before the basic driver model works.

---

# 119. MODULE SYSTEM

Eventually support kernel modules.

Module metadata may include:

- name,
- version,
- dependencies,
- supported device IDs,
- required capabilities,
- signature.

Unsigned modules should be forbidden in hardened production mode unless an explicit development mode is enabled.

---

# 120. PORTABILITY STRATEGY

Initial platform:

```text
x86-64
```

Architectural abstractions should avoid polluting generic code with x86-specific assumptions.

Potential later ports:

```text
ARM64
RISC-V
```

But do not start implementing them before x86-64 is stable.

---

# 121. COMPATIBILITY STRATEGY

Do not initially chase Linux binary compatibility.

First build:

- native executable format usage,
- native APIs,
- native libc,
- native application model.

Later investigate:

- POSIX compatibility,
- Unix-style compatibility layer,
- Linux ABI compatibility.

Compatibility should be a deliberate subsystem, not accidental API copying.

---

# 122. WHAT TO REUSE VS WHAT TO BUILD

## Reuse standards

Use mature standards for:

- UEFI,
- ACPI,
- PCI,
- USB,
- ELF,
- TCP/IP,
- DNS,
- TLS,
- common file formats.

## Build as core differentiators

Develop your own:

- kernel architecture,
- scheduler policy,
- memory architecture,
- handle/capability model,
- VFS abstraction,
- device framework,
- IPC model,
- tracing architecture,
- shell,
- service manager,
- package system,
- developer tools.

Do not reinvent cryptography, hardware standards, or mature file formats without a strong reason.

---

# 123. FEATURES THAT SHOULD NOT BE BUILT EARLY

Do not begin with:

- browser,
- app store,
- AI assistant,
- full desktop UI,
- GPU acceleration,
- Wi-Fi,
- Bluetooth,
- audio,
- video codecs,
- cloud sync,
- virtualization,
- containers,
- distributed computing,
- own programming language,
- own compiler,
- own TLS implementation,
- own cryptographic primitives,
- support for dozens of CPU architectures.

These are later ecosystem features.

---

# 124. FEATURES TO BUILD EARLY AS ARCHITECTURAL HOOKS

Create the abstractions early even if the advanced implementation comes later:

- per-CPU state,
- topology model,
- page ownership metadata,
- object identity,
- typed handles,
- capability abstraction,
- tracepoint API,
- event record format,
- allocator interfaces,
- lock hierarchy,
- object lifecycle model.

Retrofitting these after the kernel grows becomes expensive.

---

# 125. ROADMAP OVERVIEW

## M00 — Toolchain

Cross compiler, build system, QEMU, GDB, CI.

## M01 — Boot

UEFI/bootloader, kernel entry, console, logging, panic.

## M02 — CPU

GDT, IDT, exceptions, interrupts, TSS.

## M03 — Per-CPU

CPU-local structures and infrastructure.

## M04 — Physical Memory

Memory map, buddy allocator, page metadata.

## M05 — Virtual Memory

Paging, address spaces, page faults, permissions.

## M06 — Heap

Kernel heap, early debug allocator.

## M07 — Threads

Context switching, thread state.

## M08 — Scheduler

Preemption, run queues, timers, sleep/wakeup.

## M09 — Processes

Process creation, address spaces, credentials, handles.

## M10 — Syscalls

Stable syscall ABI.

## M11 — Capabilities

Typed handles, rights, revocation.

## M12 — ELF/Userland

Executable loading, init, libc foundation.

## M13 — Shell

Core utilities, command execution, pipes.

## M14 — VFS

VFS, initramfs, mount system.

## M15 — Filesystem

Persistent FS and page cache.

## M16 — Block/Storage

virtio-blk, AHCI, NVMe.

## M17 — Device Model

PCI, driver binding.

## M18 — Input/USB

HID, keyboard, mouse, xHCI.

## M19 — Networking

Ethernet, IPv4, TCP/UDP, sockets, DNS.

## M20 — SMP

Multicore, IPI, load balancing.

## M21 — NUMA

Node-aware PMM and scheduler integration.

## M22 — Advanced Allocation

SLUB, per-CPU caches, arenas.

## M23 — Advanced VM

2 MiB/1 GiB pages, huge-page policy, TLB optimization.

## M24 — Advanced Concurrency

RCU, specialized lock-free queues.

## M25 — Security Hardening

KPTI, SMEP, SMAP, NX, KASLR, IOMMU.

## M26 — Observability

Static tracepoints, binary events, metrics.

## M27 — Trace VM

Bytecode interpreter and verifier.

## M28 — Recovery

Crash dumps, safe mode, rollback.

## M29 — Graphics

Framebuffer → compositor → desktop.

## M30 — Package System

Packages, repository, signatures, updates.

## M31 — Hardening

Fuzzing, fault injection, stress testing.

## M32 — Performance

Profiling, optimization, regression tracking.

## M33 — v1.0

Stable developer/user platform.

---

# 126. FIRST 30 DAYS

## Week 1

- repository,
- cross compiler,
- QEMU,
- UEFI boot,
- kernel entry,
- serial output,
- build automation.

## Week 2

- GDT,
- IDT,
- exception handlers,
- panic framework,
- framebuffer console,
- stack traces.

## Week 3

- memory discovery,
- physical page allocator,
- paging,
- virtual memory,
- kernel heap.

## Week 4

- APIC/timer,
- context switching,
- threads,
- basic scheduler,
- first preemption test.

### End-of-month target

```text
UEFI boot
64-bit kernel
structured logging
interrupts
memory management
heap
threads
preemptive scheduler
panic diagnostics
```

---

# 127. FIRST SIX MONTHS TARGET

A realistic first major target:

```text
Boot
 ↓
Memory
 ↓
Scheduler
 ↓
Processes
 ↓
Syscalls
 ↓
Userland
 ↓
Filesystem
 ↓
Basic drivers
 ↓
Basic networking
```

Do not require a polished GUI by this point.

---

# 128. V0.1 DEFINITION

A v0.1 release means:

- stable boot,
- memory manager works,
- scheduler works,
- multiple threads work,
- basic processes work,
- panic diagnostics work,
- automated boot tests work.

---

# 129. V0.5 DEFINITION

Potential target:

- user applications,
- filesystem,
- shell,
- storage,
- input,
- networking,
- basic security model,
- basic observability.

---

# 130. V0.9 DEFINITION

Potential target:

- SMP,
- NUMA,
- advanced allocators,
- huge pages,
- capability model,
- RCU,
- tracing engine,
- security hardening,
- basic graphics,
- package management.

---

# 131. V1.0 DEFINITION

Do not call the OS v1.0 merely because a desktop appears.

A serious v1.0 target requires:

```text
✓ reliable boot
✓ stable multitasking
✓ memory isolation
✓ reliable filesystem
✓ network stack
✓ basic hardware support
✓ native application execution
✓ coherent security model
✓ package installation
✓ crash diagnostics
✓ automated tests
✓ documentation
✓ upgrade/recovery path
✓ reproducible builds
```

---

# 132. QUALITY GATES

Before leaving each major phase:

## Gate 1 — Correctness

Does it work?

## Gate 2 — Isolation

Can one component corrupt another?

## Gate 3 — Concurrency

Does it work under contention and multicore execution?

## Gate 4 — Failure

What happens when something breaks?

## Gate 5 — Observability

Can the failure be diagnosed?

## Gate 6 — Security

Can untrusted code cross the boundary?

## Gate 7 — Performance

What does measurement say?

---

# 133. FEATURE EXIT CRITERIA

Never mark a feature complete with a vague statement like:

> “Paging implemented.”

Instead:

```text
✓ kernel mappings work
✓ user mappings work
✓ page faults work
✓ protection bits work
✓ invalid access terminates process
✓ independent processes have isolated address spaces
✓ mapping/unmapping tested
✓ out-of-memory behavior tested
✓ concurrency tested
✓ diagnostics work
```

---

# 134. GIT STRATEGY

Branches:

```text
main
develop
feature/*
bugfix/*
```

Example commits:

```text
feat(mm): implement physical page allocator
feat(sched): add preemptive scheduler
feat(cap): add generation-checked handles
fix(vmm): invalidate stale mappings
feat(trace): add syscall tracepoint
 test(fs): add concurrent lookup tests
```

Keep commits narrow enough to review.

---

# 135. DOCUMENTATION STRATEGY

Maintain:

```text
OS-SPEC.md
ROADMAP.md
ARCHITECTURE.md
SECURITY.md
```

Subsystem docs:

```text
memory.md
scheduler.md
syscalls.md
vfs.md
networking.md
drivers.md
capabilities.md
tracing.md
```

Architecture Decision Records:

```text
ADR-001 modular-monolithic-kernel.md
ADR-002 x86-64-first.md
ADR-003 C-and-freestanding-Cpp.md
ADR-004 capability-security.md
ADR-005 memory-allocation-model.md
ADR-006 trace-vm.md
```

---

# 136. ARCHITECTURE DECISION RECORD TEMPLATE

```markdown
# ADR-NNN — Title

## Status
Proposed / Accepted / Superseded

## Context
What problem are we solving?

## Decision
What are we choosing?

## Alternatives
What else was considered?

## Trade-offs
What do we gain and lose?

## Consequences
What does this mean for future components?

## Measurements
What evidence supports the decision?
```

---

# 137. DEVELOPMENT CHECKLIST

Before adding a subsystem, answer:

```text
What problem does it solve?
Who owns its resources?
What is its public API?
Can it block?
How does it fail?
How is it synchronized?
How is it traced?
How is it tested?
How is it secured?
What is the expected performance?
How will it evolve?
```

---

# 138. DESIGN CHECKLIST FOR EVERY DATA STRUCTURE

Document:

- owner,
- mutators,
- readers,
- lifetime,
- lock/RCU strategy,
- memory ordering,
- allocation source,
- NUMA policy,
- cache behavior,
- reclaim strategy,
- failure behavior.

---

# 139. DESIGN CHECKLIST FOR EVERY SYSCALL

Document:

- syscall number/name,
- input types,
- output types,
- pointer validation,
- handle validation,
- capability requirements,
- blocking behavior,
- cancellation,
- errors,
- tracing events,
- security tests.

---

# 140. DESIGN CHECKLIST FOR EVERY DRIVER

Document:

- bus,
- device IDs,
- resources,
- MMIO/PIO model,
- DMA requirements,
- interrupts,
- power management,
- initialization,
- reset behavior,
- error recovery,
- removal/hotplug,
- capability requirements.

---

# 141. "GOLDEN PATH" MEMORY ALLOCATION

```text
kmalloc(128)
      ↓
Current CPU cache
      │
      ├── object exists → return
      │
      └── empty
           ↓
       node-local slab
           │
           ├── object exists → refill CPU cache
           │
           └── empty
                ↓
             buddy
                │
                ├── local node
                │
                └── remote fallback
```

This should make the common allocation path short and local.

---

# 142. "GOLDEN PATH" SYSCALL

```text
Application
 ↓
syscall
 ↓
entry trampoline
 ↓
CPU state capture
 ↓
security context
 ↓
dispatch
 ↓
handle validation
 ↓
capability check
 ↓
argument validation
 ↓
subsystem
 ↓
result
 ↓
return
```

Tracepoints can be placed on the path without forcing permanent verbose logging.

---

# 143. "GOLDEN PATH" OBJECT LIFETIME

```text
create
 ↓
initialize
 ↓
publish
 ↓
capability distribution
 ↓
use
 ↓
revoke/unpublish
 ↓
retire
 ↓
RCU grace period / references drain
 ↓
quarantine
 ↓
free
```

---

# 144. "GOLDEN PATH" TRACE

```text
Kernel event
 ↓
static tracepoint
 ↓
enabled?
 ├── no → fast return
 └── yes
      ↓
   filter
      ↓
   accept?
   ├── no → return
   └── yes
       ↓
    event buffer
       ↓
    user-space tool
```

---

# 145. "GOLDEN PATH" SECURITY

```text
User request
 ↓
opaque handle
 ↓
handle table
 ↓
generation check
 ↓
capability check
 ↓
rights check
 ↓
argument validation
 ↓
operation
```

No stage should assume that possession of an integer automatically grants authority.

---

# 146. WHAT MAKES THE OS DISTINCTIVE

The strongest differentiators are not:

- wallpaper,
- animations,
- number of shell commands,
- number of supported file formats.

The architectural differentiators should be:

### 1. Capability-oriented authority

Access is explicit and revocable.

### 2. Locality-aware execution

CPU, memory, and devices are treated as a topology.

### 3. Explicit lifetimes

Objects are created, published, retired, and reclaimed deliberately.

### 4. Observable internals

The kernel can explain its behavior.

### 5. Strong failure diagnostics

A crash should produce actionable evidence.

### 6. Measured performance

Optimization is evidence-driven.

### 7. Modular internal boundaries

Subsystems can evolve without rewriting the kernel.

---

# 147. ANTI-PATTERNS TO BAN

```text
No random global state.
No giant "god" subsystem.
No undocumented lock ordering.
No unchecked user pointers.
No hidden allocations in hot paths.
No silent error swallowing.
No unsafe pointer exposure to userland.
No arbitrary code execution from trace programs.
No custom cryptographic algorithms.
No feature without a test strategy.
No optimization without measurement.
No major architecture decision without documentation.
No "works on my machine" acceptance criterion.
```

---

# 148. WHAT NOT TO CLAIM

Avoid project claims such as:

- “100% memory safe.”
- “Impossible to exploit.”
- “Fool-proof.”
- “The fastest OS.”
- “Lock-free everywhere.”
- “RCU eliminates locks.”
- “Huge pages always improve performance.”
- “C++ guarantees memory safety.”
- “KPTI prevents all CPU side channels.”

Use engineering language such as:

> “Designed to reduce…”

> “Provides a stronger boundary for…”

> “Measured improvement under…”

> “Mitigates a documented class of…”

---

# 149. SECURITY PHILOSOPHY

The security model should be:

```text
Least privilege
+ explicit authority
+ strong isolation
+ validation
+ defense in depth
+ auditability
+ revocation
+ secure update
```

Security is not one subsystem.

It exists across:

- MMU,
- processes,
- capabilities,
- filesystems,
- drivers,
- networking,
- package management,
- boot,
- updates.

---

# 150. SCALABILITY PHILOSOPHY

The system should scale by reducing contention:

```text
Global state
 ↓
partition
 ↓
per-node state
 ↓
per-CPU state
 ↓
local fast path
 ↓
slow global coordination only when necessary
```

This philosophy applies to:

- allocators,
- scheduler queues,
- network queues,
- tracing buffers,
- metrics,
- caches.

---

# 151. RELIABILITY PHILOSOPHY

For every subsystem:

```text
Detect
 ↓
Contain
 ↓
Diagnose
 ↓
Recover
 ↓
Report
```

Where recovery is impossible:

```text
Fail predictably
 ↓
Preserve diagnostics
 ↓
Protect persistent state
```

---

# 152. OBSERVABILITY PHILOSOPHY

The system should eventually answer questions such as:

> Why did this process crash?

> Why is this process slow?

> Why is memory fragmented?

> Which NUMA node is causing remote allocations?

> Which lock is contended?

> Which service failed first?

> Which syscall caused the I/O storm?

> Why did a device timeout?

> Why was a network packet dropped?

If the OS cannot answer these questions, it should be considered operationally immature.

---

# 153. PROJECT NORTH STAR

The project should ultimately represent:

> **A computing platform where locality, isolation, ownership, concurrency, security, and observability are first-class properties of the architecture—not features bolted on after the kernel already exists.**

---

# 154. FINAL ARCHITECTURE CHECK

Before v1.0, verify that the OS has coherent answers for:

```text
How does the machine boot?
How is physical memory managed?
How is virtual memory isolated?
How are threads scheduled?
How does a process obtain resources?
How are permissions represented?
How are stale resources rejected?
How are objects reclaimed?
How are devices discovered?
How does storage work?
How does networking work?
How are drivers contained?
How are failures reported?
How are system events traced?
How are updates verified?
How does recovery work?
How are performance regressions detected?
How is the system tested?
How does user space interact with the kernel?
How does the architecture evolve without becoming chaotic?
```

Every question should have a documented answer.

---

# 155. REFERENCE MATERIAL TO KEEP BESIDE THE PROJECT

Use authoritative specifications/documentation for implementation details, especially:

- Intel 64 and IA-32 Architectures Software Developer's Manuals.
- UEFI specifications.
- ACPI specifications.
- PCI/PCIe specifications.
- USB specifications.
- NVMe specifications.
- Virtio specifications.
- ELF specification/documentation.
- TCP/IP standards/RFCs.
- DNS standards/RFCs.
- TLS specifications and well-reviewed implementations.
- Operating-system literature and research papers.
- Linux documentation as a reference for proven subsystem design patterns, not as a requirement to copy Linux wholesale.

---

# 156. IMMEDIATE NEXT ACTIONS

The first implementation sprint should produce these artifacts:

```text
1. Repository
2. Cross compiler
3. Build system
4. QEMU boot script
5. UEFI boot path
6. Kernel linker script
7. Kernel entry point
8. Serial logger
9. Panic handler
10. GDB debugging setup
11. Basic CI pipeline
12. OS-SPEC.md
13. ROADMAP.md
14. ARCHITECTURE.md
15. First architecture decision records
```

Then start the kernel.

---

# 157. FINAL RULE

Never ask only:

> “Can I make this feature work?”

Ask:

> **“Can I make this feature work correctly, concurrently, securely, observably, recoverably, maintainably, and with a measurable performance model?”**

That question should govern the entire project.

---

# APPENDIX A — SUGGESTED INITIAL API VOCABULARY

Use stable domain terminology from the beginning:

```text
CpuId
NodeId
Page
PageOrder
AddressSpace
VirtualAddress
PhysicalAddress
Thread
Process
Handle
Capability
Rights
File
Inode
Socket
Device
DmaMapping
Arena
TraceEvent
Tracepoint
```

Avoid vague names such as:

```text
Thing
Object2
Data
Manager
GlobalContext
Utils
Helper
```

unless the abstraction is genuinely generic.

---

# APPENDIX B — RECOMMENDED FIRST BENCHMARKS

```text
bench_null_syscall
bench_thread_create
bench_context_switch
bench_kmalloc_small
bench_kmalloc_large
bench_kfree
bench_mutex_uncontended
bench_mutex_contended
bench_rcu_read
bench_pipe_ipc
bench_shared_memory_ipc
bench_vfs_lookup
bench_page_fault
bench_tlb_shootdown
bench_local_numa_alloc
bench_remote_numa_alloc
bench_hugepage_mapping
bench_file_read
bench_block_io
bench_udp
bench_tcp
```

---

# APPENDIX C — RECOMMENDED FIRST FUZZ TARGETS

```text
fuzz_elf_parser
fuzz_syscall_decoder
fuzz_capability_input
fuzz_path_parser
fuzz_fs_metadata
fuzz_usb_descriptor
fuzz_ipv4_packet
fuzz_ipv6_packet
fuzz_tcp_options
fuzz_dns_message
fuzz_package_manifest
fuzz_config_parser
```

---

# APPENDIX D — FIRST-CLASS DEBUG COMMANDS

Eventually provide:

```text
dmesg
ps
top
free
vmstat
meminfo
cpuinfo
mount
lsdev
lspci
lsusb
netstat
ss
ip
trace
perf
lockstat
rcustat
numastat
memmap
crashinfo
sysinfo
```

The exact commands can differ; the important point is that the corresponding observability exists.

---

# APPENDIX E — LONG-TERM EXPERIMENTAL FEATURES

Only after the core system is stable, consider:

- memory compression,
- swap,
- transparent page promotion/demotion,
- advanced NUMA balancing,
- heterogeneous memory,
- persistent memory,
- advanced CPU scheduling classes,
- user-space drivers,
- microkernel-like driver isolation,
- virtual machines,
- containers,
- filesystem snapshots,
- deduplication,
- GPU compute,
- advanced packet processing,
- eBPF-like JIT execution,
- live kernel updates.

These are research/advanced engineering tracks, not initial requirements.

---

# APPENDIX F — PROJECT MANTRA

```text
Small interfaces.
Explicit ownership.
Local fast paths.
Measured optimization.
Defensive boundaries.
Observable behavior.
Predictable failures.
Documented decisions.
Automated tests.
No unnecessary cleverness.
```

---

# END OF MASTER SPECIFICATION
