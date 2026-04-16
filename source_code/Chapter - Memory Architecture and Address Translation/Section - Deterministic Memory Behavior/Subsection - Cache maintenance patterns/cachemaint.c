\title{Cache Maintenance for ARM (AArch64) and RISC-V Architectures}

#include <stddef.h>
#include <stdint.h>

#ifndef CACHE_LINE
#define CACHE_LINE 64U /* replace with runtime detection from system registers if available */
#endif

/* Platform hook for RISC-V data cache maintenance (implementation-specific) */
extern void riscv_platform_dc_maint(void *addr, size_t len, int op); /* op: 0=clean,1=invalidate,2=clean_inval */

/* Clean and/or invalidate a virtual-address range on AArch64; uses VA maintenance.
   op: 0=clean, 1=invalidate, 2=clean+invalidate. */
static inline void cache_maint_aarch64(void *addr, size_t len, int op)
{
    uintptr_t start = (uintptr_t)addr & ~(CACHE_LINE - 1);
    uintptr_t end   = ((uintptr_t)addr + len + CACHE_LINE - 1) & ~(CACHE_LINE - 1);

    for (uintptr_t p = start; p < end; p += CACHE_LINE) {
        switch (op) {
            case 0:
                __asm__ volatile("dc cvac, %0" :: "r"(p) : "memory"); /* clean to PoC */
                break;
            case 1:
                __asm__ volatile("dc ivac, %0" :: "r"(p) : "memory"); /* invalidate to PoC */
                break;
            case 2:
                __asm__ volatile("dc civac, %0" :: "r"(p) : "memory"); /* clean + invalidate to PoC */
                break;
        }
    }
    __asm__ volatile("dsb ish" ::: "memory"); /* ensure completion globally */
}

/* Invalidate instruction cache for a virtual-address range on AArch64. */
static inline void invalidate_icache_aarch64(void *addr, size_t len)
{
    uintptr_t start = (uintptr_t)addr & ~(CACHE_LINE - 1);
    uintptr_t end   = ((uintptr_t)addr + len + CACHE_LINE - 1) & ~(CACHE_LINE - 1);

    for (uintptr_t p = start; p < end; p += CACHE_LINE) {
        __asm__ volatile("ic ivau, %0" :: "r"(p) : "memory");
    }
    __asm__ volatile("dsb ish" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
}

/* Public API: make instruction memory modifications visible to fetch. */
static inline void sync_icache_after_modify(void *addr, size_t len)
{
#if defined(__aarch64__)
    cache_maint_aarch64(addr, len, 0);    /* clean D-cache to PoC */
    invalidate_icache_aarch64(addr, len); /* invalidate I-cache to PoU + barriers */
#elif defined(__riscv)
    /* On RISC-V, writeback is platform-specific; ensure device/implementation coherence
       then use FENCE.I to synchronize instruction fetches. */
    riscv_platform_dc_maint(addr, len, 0); /* platform-specific clean (if required) */
    __asm__ volatile("fence.i" ::: "memory");
#else
#error "Unsupported architecture"
#endif
}