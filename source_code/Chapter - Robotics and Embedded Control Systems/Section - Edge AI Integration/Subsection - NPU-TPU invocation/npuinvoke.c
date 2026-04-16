#include <stdint.h>
#include <stddef.h>

#define NPU_BASE        0x40000000U
#define OFF_CTRL        0x00U
#define OFF_DOORBELL    0x10U
#define OFF_DESC_LO     0x20U
#define OFF_DESC_HI     0x24U
#define OFF_STATUS      0x30U

/* platform barriers */
static inline void cpu_dsb(void) {
#if defined(__aarch64__)
    __asm__ volatile("dsb ish" ::: "memory");
#elif defined(__riscv)
    __asm__ volatile("fence iorw,iorw" ::: "memory");
#else
    __sync_synchronize();
#endif
}

static inline void cpu_isb(void) {
#if defined(__aarch64__)
    __asm__ volatile("isb" ::: "memory");
#elif defined(__riscv)
    /* RISC-V lacks direct isb; fence suffices for ordering here */
    __asm__ volatile("fence iorw,iorw" ::: "memory");
#else
    __sync_synchronize();
#endif
}

/* simple MMIO helpers */
static inline void mmio_write32(uintptr_t base, uint32_t off, uint32_t val) {
    volatile uint32_t *p = (volatile uint32_t *)(base + off);
    *p = val;
    cpu_dsb();
}

static inline uint32_t mmio_read32(uintptr_t base, uint32_t off) {
    volatile uint32_t *p = (volatile uint32_t *)(base + off);
    uint32_t v = *p;
    cpu_dsb();
    return v;
}

/* Example descriptor placed in DMA-able memory; align to 64 bytes */
struct npu_desc {
    uint64_t phys_addr;     /* input buffer physical address */
    uint32_t input_size;
    uint32_t output_size;
    uint32_t flags;
    uint32_t reserved;
} __attribute__((aligned(64)));

static volatile struct npu_desc dma_desc;

/* stubs for cache maintenance; platform must provide real implementations */
static void dcache_clean_range(void *addr, size_t len) {
    /* platform-specific */
}

static void dcache_invalidate_range(void *addr, size_t len) {
    /* platform-specific */
}

/* synchronous invocation with timeout loop */
int npu_invoke_phys(
    uint64_t in_phys,
    void *out_virt,
    size_t in_sz,
    size_t out_sz,
    uint32_t flags,
    uint32_t timeout_loops
) {
    /* populate descriptor in uncached DMA region */
    dma_desc.phys_addr = in_phys;
    dma_desc.input_size = (uint32_t)in_sz;
    dma_desc.output_size = (uint32_t)out_sz;
    dma_desc.flags = flags;
    cpu_dsb();

    /* cache maintenance: clean descriptor and input, invalidate output */
    dcache_clean_range((void *)&dma_desc, sizeof(dma_desc));
    dcache_clean_range((void *)(uintptr_t)in_phys, in_sz);
    dcache_invalidate_range(out_virt, out_sz);
    cpu_dsb();
    cpu_isb();

    /* program descriptor address and ring the doorbell */
    uint32_t desc_lo = (uint32_t)((uintptr_t)&dma_desc & UINT32_MAX);
    uint32_t desc_hi = (uint32_t)(((uintptr_t)&dma_desc >> 32) & UINT32_MAX);
    mmio_write32(NPU_BASE, OFF_DESC_LO, desc_lo);
    mmio_write32(NPU_BASE, OFF_DESC_HI, desc_hi);

    /* doorbell write triggers accelerator */
    mmio_write32(NPU_BASE, OFF_DOORBELL, 1U);

    /* poll status */
    uint32_t loops = 0;
    while (loops++ < timeout_loops) {
        uint32_t st = mmio_read32(NPU_BASE, OFF_STATUS);
        if ((st & 0x1U) != 0U) { /* DONE */
            if ((st & 0x2U) != 0U) {
                return -2; /* ERROR */
            }
            /* invalidate output cache to observe results */
            dcache_invalidate_range(out_virt, out_sz);
            return 0; /* success */
        }
    }

    return -1; /* timeout */
}