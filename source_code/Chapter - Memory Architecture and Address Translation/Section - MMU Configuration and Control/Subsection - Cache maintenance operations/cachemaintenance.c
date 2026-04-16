\title{Cache Coherence Operations for ARM (AArch64) and RISC-V}
\caption{Production-ready implementations of cache maintenance operations for ARM and RISC-V architectures.}

#include <stddef.h>
#include <stdint.h>

static const size_t CACHE_LINE = 64; /* probe or platform constant */

/* Align address down to cache line */
static inline uintptr_t align_down(uintptr_t a)
{
    return a & ~(CACHE_LINE - 1);
}

/* AArch64: clean D-cache to PoU, then invalidate I-cache for a range. */
void arm64_sync_icache(void *addr, size_t len)
{
    uintptr_t a = align_down((uintptr_t)addr);
    uintptr_t end = (uintptr_t)addr + len;

    /* Clean D-cache to Point of Unification */
    for (; a < end; a += CACHE_LINE) {
        __asm__ volatile("dc cvau, %0" :: "r"(a) : "memory");
    }
    __asm__ volatile("dsb sy" ::: "memory");

    /* Invalidate I-cache to Point of Unification */
    for (a = align_down((uintptr_t)addr); a < end; a += CACHE_LINE) {
        __asm__ volatile("ic ivau, %0" :: "r"(a) : "memory");
    }
    __asm__ volatile("dsb sy" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
}

/* RISC-V: use platform-specific writeback then fence.i for I-cache sync.
   platform_cache_writeback must be implemented per SoC (SBI, CSR, MMIO). */
extern void platform_cache_writeback(void *addr, size_t len); /* vendor API */

void riscv_sync_icache(void *addr, size_t len)
{
    /* Ensure D-cache lines are written back to memory (implementation-specific). */
    platform_cache_writeback(addr, len);
    __asm__ volatile("fence.i" ::: "memory"); /* make stores visible to instruction fetch */
}

/* DMA example: before handing buffer to device, clean and optionally invalidate. */
void arm64_prepare_dma_tx(void *buf, size_t len)
{
    uintptr_t a = align_down((uintptr_t)buf);
    uintptr_t end = (uintptr_t)buf + len;

    /* Clean and invalidate D-cache to Point of Coherency */
    for (; a < end; a += CACHE_LINE) {
        __asm__ volatile("dc civac, %0" :: "r"(a) : "memory");
    }
    __asm__ volatile("dsb sy" ::: "memory"); /* ensure visibility to device */
}