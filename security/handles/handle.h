#pragma once

#include "../../kernel/object/object.h"

/* Generic rights bits (spec section 46 gives per-type meanings once real
 * object types exist — READ/WRITE/EXECUTE for files, CONNECT/BIND/... for
 * sockets, etc. — these are the type-agnostic ones the handle layer
 * itself understands: whether a handle can be used at all, and whether
 * it can administer the object it points to). */
#define RIGHT_READ  (1u << 0)
#define RIGHT_WRITE (1u << 1)
#define RIGHT_ADMIN (1u << 2)

/* Opaque to callers: an index into the handle table plus that slot's own
 * generation (distinct from the kobject's generation — this one detects
 * a stale handle after its slot has been closed and reused for something
 * else entirely; the kobject's generation, checked separately, detects
 * a handle whose object was destroyed while the handle itself lives on
 * through a shared reference). {0, 0} is never valid — it's the
 * zero-initialized/failure value. */
typedef struct {
    uint32_t index;
    uint32_t generation;
} handle_t;

void handle_table_init(void);

/* Creates a new handle granting `rights` on `obj` (takes a reference).
 * Returns the zero handle {0,0} if the table is full. */
handle_t handle_create(struct kobject *obj, uint32_t rights);

/* Resolves `h` to its object iff the handle is live (not stale, not
 * closed) and `required_rights` is a subset of what it was granted.
 * Returns NULL otherwise — a stale/closed/under-privileged handle is
 * indistinguishable from the caller's point of view, by design. */
struct kobject *handle_resolve(handle_t h, uint32_t required_rights);

/* Derives a new handle to the same object with a (non-strict) subset of
 * h's rights — never more. Fails (returns the zero handle) if
 * `reduced_rights` requests anything h doesn't itself have, which is
 * what makes "duplicate then reacquire a removed right" impossible
 * (spec section 47). The derived handle is independent: closing it
 * doesn't affect h, and vice versa (spec section 47's worked example). */
handle_t handle_derive(handle_t h, uint32_t reduced_rights);

/* Closes `h`: drops the object reference and bumps the slot's generation
 * so this exact handle value can never resolve again. Returns 1 on
 * success, 0 if `h` was already invalid. */
int handle_close(handle_t h);
