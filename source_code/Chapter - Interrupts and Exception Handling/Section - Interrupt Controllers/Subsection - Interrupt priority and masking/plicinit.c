\title{RISC-V Platform-Level Interrupt Controller (PLIC) Driver}

\begin{lstlisting}
#include <stdint.h>

/* Replace with platform PLIC base and hart id */
#define PLIC_BASE 0x0C000000UL
#define HART_ID   0          /* hart/CPU 0 */

static inline void mmio_write32(uintptr_t address, uint32_t value) {
    *((volatile uint32_t *)address) = value;
}

static inline uint32_t mmio_read32(uintptr_t address) {
    return *((volatile uint32_t *)address);
}

/* Offsets per RISC-V PLIC spec (32-bit) */
#define PLIC_PRIORITY(id)     (PLIC_BASE + 0x0000 + ((id) * 4))
#define PLIC_ENABLE_WORD(hart) (PLIC_BASE + 0x2000 + ((hart) * 0x80)) /* word offset base */
#define PLIC_THRESHOLD(hart)  (PLIC_BASE + 0x200000 + ((hart) * 0x1000))
#define PLIC_CLAIM(hart)      (PLIC_BASE + 0x200004 + ((hart) * 0x1000))

/* Set priority for interrupt id */
void plic_set_priority(unsigned int irq, uint32_t priority) {
    mmio_write32(PLIC_PRIORITY(irq), priority);
}

/* Enable irq for hart: set bit in enable bitmap (word granularity) */
void plic_enable_for_hart(unsigned int irq, unsigned int hart) {
    uintptr_t enable_word_addr = PLIC_ENABLE_WORD(hart) + ((irq / 32) * 4);
    uint32_t mask = 1U << (irq % 32);
    uint32_t value = mmio_read32(enable_word_addr);
    value |= mask;
    mmio_write32(enable_word_addr, value);
}

/* Set the threshold for hart (only interrupts with p > threshold are delivered) */
void plic_set_threshold(unsigned int hart, uint32_t threshold) {
    mmio_write32(PLIC_THRESHOLD(hart), threshold);
}

/* Claim an interrupt (returns irq id, 0 means none) */
unsigned int plic_claim(unsigned int hart) {
    return mmio_read32(PLIC_CLAIM(hart));
}

/* Complete a handled interrupt */
void plic_complete(unsigned int hart, unsigned int irq) {
    mmio_write32(PLIC_CLAIM(hart), irq);
}

/* Example: enable UART IRQ 10 at priority 3 and allow interrupts > 2 */
void setup_uart_irq(void) {
    const unsigned int UART_IRQ = 10;
    plic_set_priority(UART_IRQ, 3);        /* p_i = 3 */
    plic_enable_for_hart(UART_IRQ, HART_ID);
    plic_set_threshold(HART_ID, 2);        /* allow p>2, so UART (3) delivers */
}