\title{Cache Coherency for ARM (AArch64) and RISC-V Platforms}

#include <stdint.h>
#include <stddef.h>

#define DCACHE_LINE 64U

#ifdef __aarch64__
static inline void dsb_ish(void)
{
    __asm__ volatile("dsb ish" ::: "memory");
}

static inline void isb(void)
{
    __asm__ volatile("isb" ::: "memory");
}

/* Clean cache lines to Point of Coherency (CPU->device). */
void dma_sync_for_device(void *addr, size_t size)
{
    uintptr_t start = (uintptr_t)addr & ~(DCACHE_LINE - 1);
    uintptr_t end   = ((uintptr_t)addr + size + DCACHE_LINE - 1) & ~(DCACHE_LINE - 1);

    for (uintptr_t p = start; p < end; p += DCACHE_LINE) {
        __asm__ volatile("dc cvac, %0" :: "r"(p) : "memory"); // clean by VA to PoC
    }

    dsb_ish(); // ensure cleaning has completed and is visible to device
}

/* Invalidate cache lines after device wrote (device->CPU). */
void dma_sync_for_cpu(void *addr, size_t size)
{
    uintptr_t start = (uintptr_t)addr & ~(DCACHE_LINE - 1);
    uintptr_t end   = ((uintptr_t)addr + size + DCACHE_LINE - 1) & ~(DCACHE_LINE - 1);

    for (uintptr_t p = start; p < end; p += DCACHE_LINE) {
        __asm__ volatile("dc ivac, %0" :: "r"(p) : "memory"); // invalidate by VA to PoC
    }

    dsb_ish(); // ensure invalidation completes before CPU access
    isb();     // optional: serialize subsequent instruction fetches if buffer contained code
}

#elif defined(__riscv)

/* Example platform-specific cache controller registers (replace with SoC values). */
#define CACHE_CTRL_BASE     0x10000000UL
#define CACHE_CMD_REG       (*(volatile uint32_t*)(CACHE_CTRL_BASE + 0x00))
#define CACHE_ADDR_REG      (*(volatile uint32_t*)(CACHE_CTRL_BASE + 0x04))
#define CACHE_LEN_REG       (*(volatile uint32_t*)(CACHE_CTRL_BASE + 0x08))
#define CACHE_STATUS        (*(volatile uint32_t*)(CACHE_CTRL_BASE + 0x0C))
#define CACHE_CMD_FLUSH     0x1
#define CACHE_STATUS_DONE   0x0

void dma_sync_for_device(void *addr, size_t size)
{
    // Map buffer as uncached or use controller to clean range.
    CACHE_ADDR_REG = (uint32_t)(uintptr_t)addr;
    CACHE_LEN_REG  = (uint32_t)size;
    CACHE_CMD_REG  = CACHE_CMD_FLUSH; // request clean to PoC

    while (CACHE_STATUS != CACHE_STATUS_DONE) {
        /* poll */
    }
}

void dma_sync_for_cpu(void *addr, size_t size)
{
    CACHE_ADDR_REG = (uint32_t)(uintptr_t)addr;
    CACHE_LEN_REG  = (uint32_t)size;
    CACHE_CMD_REG  = (CACHE_CMD_FLUSH | 0x2); // 0x2 => invalidate mode (SoC-specific)

    while (CACHE_STATUS != CACHE_STATUS_DONE) {
        /* poll */
    }
}

#else
#error "Platform must provide cache maintenance or use uncached mappings."
#endif