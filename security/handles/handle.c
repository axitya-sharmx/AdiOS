#include "handle.h"

#define MAX_HANDLES 64

struct handle_slot {
    struct kobject *obj;
    uint32_t rights;
    uint32_t generation; /* starts at 1 so the zero handle {0,0} never matches a real slot */
    int used;
};

static struct handle_slot g_table[MAX_HANDLES];

void handle_table_init(void) {
    for (int i = 0; i < MAX_HANDLES; i++) {
        g_table[i].obj = 0;
        g_table[i].rights = 0;
        g_table[i].generation = 1;
        g_table[i].used = 0;
    }
}

handle_t handle_create(struct kobject *obj, uint32_t rights) {
    for (uint32_t i = 0; i < MAX_HANDLES; i++) {
        if (!g_table[i].used) {
            g_table[i].used = 1;
            g_table[i].obj = obj;
            g_table[i].rights = rights;
            kobject_ref(obj);
            handle_t h = {i, g_table[i].generation};
            return h;
        }
    }
    handle_t zero = {0, 0};
    return zero;
}

static struct handle_slot *lookup(handle_t h) {
    if (h.index >= MAX_HANDLES) {
        return 0;
    }
    struct handle_slot *slot = &g_table[h.index];
    if (!slot->used || slot->generation != h.generation) {
        return 0;
    }
    return slot;
}

struct kobject *handle_resolve(handle_t h, uint32_t required_rights) {
    struct handle_slot *slot = lookup(h);
    if (!slot) {
        return 0;
    }
    if ((required_rights & ~slot->rights) != 0) {
        return 0; /* missing at least one required right */
    }
    return slot->obj;
}

handle_t handle_derive(handle_t h, uint32_t reduced_rights) {
    struct handle_slot *slot = lookup(h);
    handle_t zero = {0, 0};
    if (!slot) {
        return zero;
    }
    if ((reduced_rights & ~slot->rights) != 0) {
        return zero; /* can't derive a right h doesn't itself have */
    }
    return handle_create(slot->obj, reduced_rights);
}

int handle_close(handle_t h) {
    struct handle_slot *slot = lookup(h);
    if (!slot) {
        return 0;
    }
    kobject_unref(slot->obj);
    slot->used = 0;
    slot->obj = 0;
    slot->rights = 0;
    slot->generation++; /* any copy of this handle_t is now stale */
    return 1;
}
