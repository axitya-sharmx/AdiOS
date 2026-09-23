#pragma once

#define PIC1_VECTOR_OFFSET 32 /* IRQ0-7  -> vectors 32-39 */
#define PIC2_VECTOR_OFFSET 40 /* IRQ8-15 -> vectors 40-47 */

/* Remaps the legacy 8259 PIC pair off vectors 0-15 (where they'd collide
 * with CPU exceptions) onto PIC1_VECTOR_OFFSET/PIC2_VECTOR_OFFSET. */
void pic_remap(void);

/* Must be called at the end of any IRQ handler. `irq` is 0-15. */
void pic_send_eoi(unsigned irq);

void pic_set_mask(unsigned irq);
void pic_clear_mask(unsigned irq);
