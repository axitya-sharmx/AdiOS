# Contributing to AdiOS

## Scope and process

Work proceeds phase by phase per [`OS_MASTER_SPEC.md`](OS_MASTER_SPEC.md).
Each modular addition lands as its own pull request:

- Branch from the latest in-flight phase branch, not `main`, while earlier
  phases are still open PRs (`gh pr list` to check what's current). `main`
  stays untouched until phases merge in order.
- Scope a PR to one phase, or one coherent piece of a phase — not a mix of
  unrelated subsystems.
- Keep CI green: a PR should build (`make iso`) and boot
  (`[INIT] Kernel initialized` on the serial log) before merge. Later
  phases extend this bar (e.g. Phase 2 added a fault-injection boot pass).

## Working concurrently with someone else on this repo

If more than one contributor (human or agent) is active on this repo at
the same time:

- **Claim scope before starting.** State which phase/files you're about to
  touch (e.g. "I've got `arch/x86_64/cpu/`, `interrupts/`") so others can
  pick genuinely independent work instead of a second implementation of
  the same phase.
- **Docs are free territory** as a rule of thumb — `docs/`, `README.md` —
  but check for an open PR touching the same file before starting, since
  README in particular tends to get touched by both a docs PR and whatever
  phase PR updates the "Status" section.
- **Watch for a shared working directory.** If you and another session
  resolve to the *same* checkout on disk (not separate clones/worktrees),
  a `git checkout`/branch switch from either side changes the other's view
  mid-work and can misattribute commits to the wrong branch. Symptoms:
  a commit you just made shows up on a branch you didn't expect, or files
  you didn't touch appear modified/untracked after a checkout.
  - Prefer `git worktree add ../some-dir -b your-branch <base>` for new
    work so you're not sharing a working tree at all.
  - If you must share one, confirm `git status --short` is clean before
    switching branches, and give the other side a heads-up first.

## Style

Freestanding C (minimal C++ where used), no exceptions/RTTI, explicit
ownership, no hidden control flow. Match the conventions already in the
file you're editing over any general preference.
