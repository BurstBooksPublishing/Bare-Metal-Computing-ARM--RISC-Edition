ARM (AArch64) and RISC-V Memory-Mapped I/O Utilities


#include <stdint.h>

/* Platform-specific fence intrinsics. Keep "memory" clobber to prevent
   compiler reordering around inline asm. */
static inline void arch_mb(void) {
#if defined(__aarch64__)
    __asm__ volatile("dsb sy" ::: "memory"); /* ensure completion of prior stores */
#elif defined(__riscv)
    __asm__ volatile("fence rw, rw" ::: "memory"); /* full memory fence */
#else
    /* Fallback: compiler barrier only (unsafe for hardware ordering). */
    __asm__ volatile("" ::: "memory");
#endif
}

/* Write 32-bit MMIO register. Caller must ensure pointer maps to device memory. */
static inline void mmio_write32(volatile uint32_t *addr, uint32_t val) {
    *addr = val;           /* compiler emits store; volatile prevents elision */
    arch_mb();             /* ensure store has reached device before continuing */
}

/* Read 32-bit MMIO register. Use after necessary fences if ordering required. */
static inline uint32_t mmio_read32(volatile uint32_t *addr) {
    uint32_t v = *addr;    /* volatile load */
    /* Optionally enforce ordering for subsequent operations */
    return v;
}

/* Example: enable peripheral and wait for ready bit to be set. */
static inline void enable_and_wait(volatile uint32_t *ctrl, volatile uint32_t *stat,
                                   uint32_t enable_mask, uint32_t ready_mask) {
    mmio_write32(ctrl, enable_mask);              /* issue enable */
    /* Ensure the write is visible to peripheral before polling its status. */
    arch_mb();                                    /* completion barrier */
    while (!(mmio_read32(stat) & ready_mask)) {
        /* poll loop; avoid tight-spin power issues in production hardware */
    }
}