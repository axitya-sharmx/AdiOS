#include "object.h"

void kobject_init(struct kobject *obj, uint64_t id) {
    obj->id = id;
    obj->generation = 1; /* 0 is reserved so a zeroed struct is never mistaken for a live generation 0 */
    obj->refcount = 1;
}

void kobject_ref(struct kobject *obj) {
    obj->refcount++;
}

void kobject_unref(struct kobject *obj) {
    if (--obj->refcount == 0) {
        obj->generation++;
    }
}
