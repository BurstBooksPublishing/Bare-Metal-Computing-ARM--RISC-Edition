Production-Ready DMA Memory Management for ARM (AArch64) and RISC-V Systems


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Platform must define these according to target architecture. */
/* Convert virtual address to physical address for device programming. */
extern uintptr_t platform_phys_addr(void *vaddr);
/* Clean (write-back) D-cache lines covering [addr, addr+len). */
extern void platform_cache_clean(void *addr, size_t len);
/* Invalidate D-cache lines covering [addr, addr+len). */
extern void platform_cache_invalidate(void *addr, size_t len);

/* Architecture-specific ordering barriers. */
static inline void cpu_memory_barrier_before_device(void)
{
#if defined(__aarch64__)
    asm volatile("dsb ish" ::: "memory"); /* ensure stores reach outer domain */
#elif defined(__riscv)
    asm volatile("fence rw, rw" ::: "memory");
#else
    asm volatile("" ::: "memory");
#endif
}

static inline void cpu_memory_barrier_after_device(void)
{
#if defined(__aarch64__)
    asm volatile("dsb ish" ::: "memory"); /* ensure device writes are observed */
#elif defined(__riscv)
    asm volatile("fence rw, rw" ::: "memory");
#else
    asm volatile("" ::: "memory");
#endif
}

/* Example static buffer pool; place in a linker section mapped as desired. */
#define SHARED_POOL_SIZE    (64 * 1024)
#define CACHE_LINE          64
static uint8_t shared_pool[SHARED_POOL_SIZE] __attribute__((aligned(CACHE_LINE), section(".dma_shared")));

/* Return a CPU virtual pointer and the device physical address. */
bool dma_shared_alloc(size_t size, void **cpu_ptr, uintptr_t *device_phys)
{
    static size_t offset = 0;
    /* Simple bump allocator for example; production code needs locking. */
    size = (size + CACHE_LINE - 1) & ~(CACHE_LINE - 1);
    if (offset + size > SHARED_POOL_SIZE) {
        return false;
    }
    *cpu_ptr = &shared_pool[offset];
    *device_phys = platform_phys_addr(*cpu_ptr);
    offset += size;
    return true;
}

/* Call before programming device to read CPU-prepared data. */
static inline void dma_shared_prepare_for_device(void *buf, size_t len)
{
    platform_cache_clean(buf, len);             /* noop if region non-cacheable */
    cpu_memory_barrier_before_device();         /* ordering before device sees memory */
}

/* Call after device completes DMA to CPU-visible memory. */
static inline void dma_shared_complete_from_device(void *buf, size_t len)
{
    cpu_memory_barrier_after_device();          /* ordering before CPU loads */
    platform_cache_invalidate(buf, len);        /* ensure CPU sees device writes */
}