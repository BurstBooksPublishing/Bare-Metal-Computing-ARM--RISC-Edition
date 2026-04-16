Production-Ready MMIO Helper Functions for Bare Metal Systems\\
\textit{Supporting ARM (AArch64) and RISC-V Architectures}


#include <stdint.h>
#include <stddef.h>

static inline uint32_t mmio_read32(uintptr_t addr) {
    return *(volatile uint32_t *)addr; /* exact-width volatile read */
}

static inline void mmio_write32(uintptr_t addr, uint32_t val) {
    *(volatile uint32_t *)addr = val;  /* write emitted immediately by compiler */
}

/* Architecture-specific full-system barrier */
static inline void arch_barrier_full(void) {
#if defined(__aarch64__)
    asm volatile("dmb sy" ::: "memory");      /* order MMIO and data accesses */
#elif defined(__riscv)
    asm volatile("fence rw, rw" ::: "memory");/* order loads/stores and I/O */
#else
    __asm__ volatile("" ::: "memory");
#endif
}

/* Write then ensure device has observed the write (use when required) */
static inline void mmio_write32_flush(uintptr_t addr, uint32_t val) {
    mmio_write32(addr, val);
    arch_barrier_full(); /* ensures ordering/drain to peripheral */
}

/* Simple RMW that is safe on single-core: disable interrupts externally. */
/* For multi-core, replace with LL/SC sequence or device-specific set/clear regs. */
static inline void mmio_update_bits32(uintptr_t addr, uint32_t clear_mask, uint32_t set_mask) {
    uint32_t v = mmio_read32(addr);
    v = (v & ~clear_mask) | set_mask;
    mmio_write32_flush(addr, v);
}