\title{RISC-V PLIC Driver Implementation}

\InsertInfographic{Images/output/Image62.png}
\section{RISC-V PLIC Driver Implementation}

#include <stdint.h>

#define PLIC_BASE               0x0C000000UL        // platform-specific base
#define PLIC_PRIO(base, irq)    ((volatile uint32_t *)((base) + 4 * (irq)))
#define PLIC_PENDING(base)      ((volatile uint32_t *)((base) + 0x1000))
#define PLIC_ENABLE(base, ctx, word) \
                                ((volatile uint32_t *)((base) + 0x2000 + 0x80 * (ctx) + 4 * (word)))
#define PLIC_CTX(base, ctx)     ((volatile uint32_t *)((base) + 0x200000 + 0x1000 * (ctx)))

static inline void mmio_fence(void) {
    asm volatile("fence rw, rw" ::: "memory"); // order MMIO with respect to CPU
}

/* Set priority: 0 disables the irq, >0 sets priority */
void plic_set_priority(uintptr_t base, unsigned irq, uint32_t priority) {
    volatile uint32_t *p = PLIC_PRIO(base, irq);
    *p = priority;
    mmio_fence();
}

/* Enable irq for context (ctx is implementation-assigned, usually hart id*2 + target) */
void plic_enable_irq(uintptr_t base, unsigned ctx, unsigned irq) {
    unsigned idx = (irq - 1) / 32;
    unsigned bit = (irq - 1) % 32;
    volatile uint32_t *en = PLIC_ENABLE(base, ctx, idx);
    uint32_t val = *en;
    val |= (1u << bit);
    *en = val;
    mmio_fence();
}

/* Set priority threshold for context (0 allows all priorities >0) */
void plic_set_threshold(uintptr_t base, unsigned ctx, uint32_t threshold) {
    volatile uint32_t *ctxreg = PLIC_CTX(base, ctx) + 0; // offset 0 = threshold
    *ctxreg = threshold;
    mmio_fence();
}

/* Claim and complete operations: read then write same id */
uint32_t plic_claim(uintptr_t base, unsigned ctx) {
    volatile uint32_t *claim = PLIC_CTX(base, ctx) + 1; // offset 4 bytes -> index +1
    uint32_t id = *claim;
    mmio_fence();
    return id;
}

void plic_complete(uintptr_t base, unsigned ctx, uint32_t id) {
    volatile uint32_t *claim = PLIC_CTX(base, ctx) + 1;
    *claim = id;
    mmio_fence();
}

/* A simple interrupt dispatcher to be called from trap entry for external interrupts */
void plic_handle(uintptr_t base, unsigned ctx, void (*service)(uint32_t)) {
    uint32_t irq = plic_claim(base, ctx);
    if (irq == 0) return;
    service(irq);                // user-supplied service routine for irq
    plic_complete(base, ctx, irq);
}