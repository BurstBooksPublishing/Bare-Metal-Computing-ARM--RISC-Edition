ARM AArch64 and RISC-V DMA Transfer Code


#include <stdint.h>
#include <stddef.h>

/* MMIO base and offsets (example values; use SoC-specific addresses) */
#define DMA_BASE        0x40000000UL
#define DMA_SRC_OFF     0x00
#define DMA_DST_OFF     0x08
#define DMA_LEN_OFF     0x10
#define DMA_CTRL_OFF    0x18
#define DMA_STATUS_OFF  0x1C

static inline volatile uint32_t *mmio32(uintptr_t off) {
    return (volatile uint32_t *)(DMA_BASE + off);
}

static inline volatile uint64_t *mmio64(uintptr_t off) {
    return (volatile uint64_t *)(DMA_BASE + off);
}

/* Platform barrier: ARM aarch64 uses DSB/ISB; RISC-V uses fence */
static inline void dma_write_barrier(void) {
#if defined(__aarch64__)
    __asm__ volatile("dsb ish\nisb\n" ::: "memory");
#elif defined(__riscv)
    __asm__ volatile("fence rw, rw" ::: "memory");
#else
    __asm__ volatile("" ::: "memory");
#endif
}

/* Platform-specific cache maintenance stubs; implement per platform */
void platform_dma_clean_range(void *addr, size_t len);      /* clean D-cache */
void platform_dma_invalidate_range(void *addr, size_t len); /* invalidate D-cache */

/* Starts a simple memory-to-peripheral DMA transfer */
int dma_start_transfer(uint64_t phys_src, uint64_t phys_dst, uint32_t len) {
    if (len == 0 || len > 0x00FFFFFFU) {
        return -1; /* bounds check */
    }

    /* Ensure buffers are cache-coherent for DMA device reads */
    platform_dma_clean_range((void *)(uintptr_t)phys_src, len);

    /* Program source and destination (64-bit capable) */
    *mmio64(DMA_SRC_OFF) = phys_src;
    *mmio64(DMA_DST_OFF) = phys_dst;
    *mmio32(DMA_LEN_OFF) = len;

    /* Ensure MMIO writes are ordered before starting DMA */
    dma_write_barrier();

    /* Start transfer: set START bit (bit 0) in DMA_CTRL */
    *mmio32(DMA_CTRL_OFF) = 0x1U;

    return 0; /* success: completion via IRQ or polling on DMA_STATUS */
}