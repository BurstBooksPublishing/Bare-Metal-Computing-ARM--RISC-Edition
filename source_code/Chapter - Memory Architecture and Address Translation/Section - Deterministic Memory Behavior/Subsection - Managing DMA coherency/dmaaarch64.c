//ARM (AArch64) Cache Maintenance for DMA Operations


#include <stdint.h>
#include <stddef.h>

#define CACHE_LINE_SIZE 64

/* Clean (write-back) [addr, addr+len) to point of coherency. */
static inline void dma_clean_range(void *addr, size_t len)
{
    uintptr_t start = (uintptr_t)addr & ~(CACHE_LINE_SIZE - 1);
    uintptr_t end   = ((uintptr_t)addr + len + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);

    for (uintptr_t p = start; p < end; p += CACHE_LINE_SIZE) {
        __asm__ volatile("dc cvac, %x[pa]" :: [pa] "r"(p) : "memory");
    }

    /* Ensure completion of the cache maintenance before DMA start. */
    __asm__ volatile("dsb ish" ::: "memory");
}

/* Invalidate [addr, addr+len) from data cache so CPU reads from memory. */
static inline void dma_invalidate_range(void *addr, size_t len)
{
    uintptr_t start = (uintptr_t)addr & ~(CACHE_LINE_SIZE - 1);
    uintptr_t end   = ((uintptr_t)addr + len + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);

    for (uintptr_t p = start; p < end; p += CACHE_LINE_SIZE) {
        __asm__ volatile("dc ivac, %x[pa]" :: [pa] "r"(p) : "memory");
    }

    /* Ensure invalidation is observed before CPU accesses buffer. */
    __asm__ volatile("dsb ish" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
}