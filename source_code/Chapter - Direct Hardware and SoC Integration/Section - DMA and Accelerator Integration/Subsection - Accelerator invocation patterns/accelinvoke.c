Production-Ready DMA Accelerator Driver for ARM (AArch64) and RISC-V


#include <stdint.h>
#include <stddef.h>

#define ACCEL_MMIO_BASE    ((uintptr_t)0x40000000) /* platform-specific */
#define ACCEL_DOORBELL_OFF 0x00
#define ACCEL_STATUS_OFF   0x04
#define ACCEL_DESCADDR_OFF 0x08

struct dma_desc {
    uintptr_t src_phys;
    uintptr_t dst_phys;
    uint32_t  len;
    uint32_t  flags;
} __attribute__((packed, aligned(64)));

/* Architecture barriers and cache maintenance */
#if defined(__aarch64__)
static inline void cpu_mb(void) {
    asm volatile("dsb ish; isb" ::: "memory");
}

static inline void cache_clean_range(void *addr, size_t len) {
    uintptr_t a = (uintptr_t)addr & ~(uintptr_t)63; /* 64B lines */
    uintptr_t end = (uintptr_t)addr + len;
    for (; a < end; a += 64) {
        asm volatile("dc civac, %0" :: "r"(a) : "memory");
    }
    asm volatile("dsb ish; isb" ::: "memory");
}

static inline void cache_invalidate_range(void *addr, size_t len) {
    uintptr_t a = (uintptr_t)addr & ~(uintptr_t)63;
    uintptr_t end = (uintptr_t)addr + len;
    for (; a < end; a += 64) {
        asm volatile("dc ivac, %0" :: "r"(a) : "memory");
    }
    asm volatile("dsb ish; isb" ::: "memory");
}
#elif defined(__riscv)
static inline void cpu_mb(void) {
    asm volatile("fence iorw, iorw" ::: "memory");
}

/* Platform-specific RISC-V cache maintenance omitted; assume coherent or handled by DMA */
static inline void cache_clean_range(void *addr, size_t len) {
    (void)addr;
    (void)len;
}

static inline void cache_invalidate_range(void *addr, size_t len) {
    (void)addr;
    (void)len;
}
#else
#error "Unsupported architecture"
#endif

/* Simple MMIO helpers */
static inline void mmio_write32(uintptr_t base, uint32_t off, uint32_t v) {
    volatile uint32_t *p = (volatile uint32_t *)(base + off);
    *p = v;
}

static inline uint32_t mmio_read32(uintptr_t base, uint32_t off) {
    return *(volatile uint32_t *)(base + off);
}

/* Return 0 on success, -1 on timeout */
int accel_invoke(struct dma_desc *desc, uintptr_t desc_phys, uint32_t timeout_loops) {
    /* Place descriptor (already filled by caller) into shared memory. */
    cache_clean_range(desc, sizeof(*desc));       /* make descriptor visible to device */
    cpu_mb();                                     /* ensure ordering before doorbell */

    /* Publish descriptor address to device via MMIO doorbell mechanism. */
    mmio_write32(ACCEL_MMIO_BASE, ACCEL_DESCADDR_OFF, (uint32_t)desc_phys);
    mmio_write32(ACCEL_MMIO_BASE, ACCEL_DOORBELL_OFF, 1); /* doorbell value defined by hw */
    cpu_mb();                                     /* ensure device sees the write */

    /* Poll for completion with timeout. */
    uint32_t loops = timeout_loops;
    while (loops--) {
        uint32_t status = mmio_read32(ACCEL_MMIO_BASE, ACCEL_STATUS_OFF);
        if (status & 0x1) {
            /* Completed: invalidate cache for result buffers before CPU use. */
            cache_invalidate_range((void *)desc->dst_phys, desc->len);
            return 0;
        }
    }
    return -1; /* timeout */
}