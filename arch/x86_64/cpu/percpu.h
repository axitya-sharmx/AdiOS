#pragma once

#include <stdint.h>

struct percpu {
    struct percpu *self; /* first field: lets %gs:0 resolve the struct's own address */
    uint32_t cpu_id;
};

void percpu_init(uint32_t cpu_id);
struct percpu *percpu_current(void);
