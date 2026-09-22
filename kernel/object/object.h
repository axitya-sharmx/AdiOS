#pragma once

#include <stdint.h>

/* Base of every kernel object (spec section 40/83): stable identity, a
 * generation bumped on destruction so a capability referencing an old
 * generation fails instead of silently binding to whatever reused the
 * slot (spec section 48), and a refcount so multiple handles (or a
 * derived + original pair, section 47) can share ownership safely.
 *
 * Real object types (File, Socket, ...) embed this as their first member
 * once they exist; for now security/handles is exercised against a
 * generic test object until a later phase gives it something real to
 * hand out handles to. */
struct kobject {
    uint64_t id;
    uint32_t generation;
    uint32_t refcount;
};

void kobject_init(struct kobject *obj, uint64_t id);
void kobject_ref(struct kobject *obj);

/* Drops a reference; when it reaches zero, bumps the generation so any
 * handle still (incorrectly) pointing at this object's old generation
 * fails to resolve. Real destructors land with real object types. */
void kobject_unref(struct kobject *obj);
