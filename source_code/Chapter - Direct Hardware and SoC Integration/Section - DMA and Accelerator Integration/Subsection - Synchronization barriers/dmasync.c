ARM (AArch64) and RISC-V DMA Cache Coherency Handling


#include <stdint.h>
#include <stddef.h>

#define CACHE_LINE 64

/* platform-specific DMA register access must be implemented externally */
extern void dma_program_and_start(uintptr_t desc_addr);
extern int dma_transfer_complete(void); /* returns nonzero when done */

/* Prepare a transmit buffer for device read (CPU -> device) */
static inline void dma_prepare_tx(void *buf, size_t len) {
    uintptr_t p = (uintptr_t)buf;
    uintptr_t end = p + len;

#if defined(__aarch64__)
    /* Clean D-cache lines to point of coherency for DMA to see CPU writes. */
    for (; p < end; p += CACHE_LINE) {
        asm volatile("dc cvac, %0" :: "r"(p) : "memory");
    }
    /* Ensure completion of cache maintenance before starting DMA. */
    asm volatile("dsb ish" ::: "memory");
    /* Now safe to program DMA descriptors and start transfer. */
    dma_program_and_start((uintptr_t)buf);
#elif defined(__riscv)
    /* Order CPU writes before DMA using a full fence. */
    asm volatile("fence rw, rw" ::: "memory");
    /* If cache maintenance exists, perform it here (platform-specific). */
    dma_program_and_start((uintptr_t)buf);
#else
# error "Unsupported architecture"
#endif
}

/* Handle DMA completion for a receive buffer (device -> CPU) */
static inline void dma_complete_rx(void *buf, size_t len) {
    uintptr_t p = (uintptr_t)buf;
    uintptr_t end = p + len;

    /* Wait for DMA controller to report completion (interrupt or poll). */
    while (!dma_transfer_complete()) { /* busy-wait or yield */ }

#if defined(__aarch64__)
    /* Ensure observed completion is ordered before cache invalidation. */
    asm volatile("dsb ish" ::: "memory");
    /* Invalidate D-cache lines so CPU loads fetch fresh data. */
    for (; p < end; p += CACHE_LINE) {
        asm volatile("dc ivac, %0" :: "r"(p) : "memory");
    }
    /* Ensure invalidation completes before subsequent loads. */
    asm volatile("dsb ish; isb" ::: "memory");
#elif defined(__riscv)
    /* Full fence to order DMA completion before subsequent loads. */
    asm volatile("fence iorw, iorw" ::: "memory");
    /* Perform platform-specific cache invalidation if required. */
#else
# error "Unsupported architecture"
#endif
}