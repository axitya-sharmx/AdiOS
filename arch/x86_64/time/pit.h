#pragma once

#include <stdint.h>

/* Programs PIT channel 0 to fire IRQ0 at `hz` and installs its handler.
 * hz must be >= 19 (the PIT's 16-bit divisor can't go slower). */
void pit_init(uint32_t hz);

uint64_t pit_ticks(void);
