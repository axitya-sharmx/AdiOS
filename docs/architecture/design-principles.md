# Design principles (working summary)

Distilled from `OS_MASTER_SPEC.md` §0–§4 for quick reference while
implementing — read the spec itself for full rationale; this is a
checklist, not a replacement.

## Priority order when trade-offs come up

```text
Correctness → Isolation → Observability → Security → Scalability
→ Performance → Convenience
```

A faster implementation that regresses anything earlier in this list is a
regression, not an improvement. In practice: don't skip a bounds check to
save a branch, don't share state across CPUs without a stated concurrency
rule to save a lock, don't drop a diagnostic to save a few bytes.

## Rules that apply to every subsystem

- **No universal primitive.** RCU, lock-free structures, huge pages, SLUB —
  each has a real cost and a real fit. Pick per-case, justify the pick,
  don't default to "the fancy one."
- **Measure, don't assert.** "X is faster" is not a design justification.
  "X reduced <metric> by <amount> for <workload> on <hardware>" is.
- **Failure is part of the design, not an afterthought.** Every subsystem
  should have an answer for: invalid input, resource exhaustion, timeout,
  cancellation, hardware failure, concurrency failure, and — critically —
  what it reports when something goes wrong. A subsystem with no failure
  story isn't done.
- **Explicit ownership.** Every resource needs a stated owner, an
  ownership-transfer rule, a lifetime, a destruction rule, and a
  concurrency rule. If you can't state these for something you're adding,
  the design isn't finished yet.
- **No hidden behavior.** An API shouldn't silently allocate, block,
  touch global state, take an unstated lock, or cross a security boundary.
  If it does any of those, say so in its contract.
- **Some interfaces are contracts, not implementation details**: syscall
  ABI, capability semantics, handle semantics, VFS/driver/IPC interfaces,
  memory mapping rules, object lifetime rules. Changing these later is a
  breaking change, not a refactor — get them right before wide use, or
  version them.

## What the system is trying to be

Capability-oriented authority, explicit object lifetimes, NUMA/CPU
locality, per-CPU fast paths, selectively-applied lock-free/RCU, huge-page
awareness, strong user/kernel isolation, first-class tracing, explainable
failures, measurement-driven optimization. (`OS_MASTER_SPEC.md` §1.3)

## What it's explicitly not trying to be

A kernel demo with no userland, a shell glued onto an unstable kernel, an
unrelated pile of drivers, an unreasoned Linux clone, a feature list with
no compatibility contracts, a benchmark project with weak correctness, or
a security product resting on unverified assumptions. (§1.2)

## Layering (§3)

User space sits above a syscall/handle ABI and a capability/authorization
boundary. Inside the kernel, Security/Scheduler/Observability sit above
Memory/IPC/VFS, which sit above a device framework (PCI/USB/Storage), which
sits above networking. See the full diagram in `OS_MASTER_SPEC.md` §3 for
how subsystems are expected to depend on each other — a lower layer should
not reach up into a higher one.

## Reading further

- `OS_MASTER_SPEC.md` §0–§4 — the source this is distilled from.
- [`README.md`](README.md) — index of the other architecture notes.
