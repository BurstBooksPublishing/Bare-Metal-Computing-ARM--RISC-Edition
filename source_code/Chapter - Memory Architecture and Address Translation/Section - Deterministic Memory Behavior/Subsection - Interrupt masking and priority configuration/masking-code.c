//Production-Ready Low-Level Interrupt and Cache Management for ARM (AArch64) and RISC-V


#include <stdint.h>
#include <stddef.h>

/* MMIO helpers (volatile sized access) */
static inline void mmio_write32(uintptr_t addr, uint32_t v) {
    *(volatile uint32_t*)addr = v;
}

static inline uint32_t mmio_read32(uintptr_t addr) {
    return *(volatile uint32_t*)addr;
}

/* ---------- AArch64 helpers ---------- */
/* Set per-PE GIC priority mask (ICC_PMR_EL1) */
static inline void gic_set_priority_mask(uint8_t mask) {
    __asm__ volatile("msr ICC_PMR_EL1, %0" :: "r" (mask) : "memory");
}

/* Disable IRQs locally (set I bit in DAIF) */
static inline void local_disable_irqs(void) {
    __asm__ volatile("msr DAIFSET, #2" ::: "memory"); /* #2 sets I bit */
}

/* Enable IRQs locally (clear I bit in DAIF) */
static inline void local_enable_irqs(void) {
    __asm__ volatile("msr DAIFCLR, #2" ::: "memory");
}

/* Example critical section: cache clean + brief non-preemptible window */
void aarch64_cache_critical(uintptr_t addr, size_t len, uint8_t pmr_threshold) {
    /* raise controller threshold to permit only high-urgency interrupts */
    gic_set_priority_mask(pmr_threshold);
    /* disable local IRQs for a very short period */
    local_disable_irqs();
    /* perform cache maintenance (implementation-specific) */
    // clean_dcache_range(addr, len); // implement for target
    __asm__ volatile("dsb ish; isb" ::: "memory");
    local_enable_irqs();
    /* restore PMR to allow more interrupts (set to 0xFF to allow all) */
    gic_set_priority_mask(0xFF);
}

/* ---------- RISC-V (RV64) helpers ---------- */
/* PLIC base must be set per platform */
#define PLIC_BASE 0x0C000000UL
#define PLIC_PRIORITY(src) (PLIC_BASE + 4 * (src))
#define PLIC_THRESHOLD(ctx) (PLIC_BASE + 0x200000 + 0x1000 * (ctx))

/* Set PLIC priority for source */
static inline void plic_set_priority(unsigned int src, uint32_t prio) {
    mmio_write32(PLIC_PRIORITY(src), prio);
}

/* Set PLIC threshold for a hart context */
static inline void plic_set_threshold(unsigned int context, uint32_t thr) {
    mmio_write32(PLIC_THRESHOLD(context), thr);
}

/* Disable machine-level interrupts (clear MIE in mstatus) */
static inline void local_disable_mie(void) {
    unsigned long old;
    __asm__ volatile("csrrc %0, mstatus, %1" : "=r"(old) : "r"(0x8UL)); /* clear MIE */
}

/* Enable machine-level interrupts (set MIE) */
static inline void local_enable_mie(void) {
    __asm__ volatile("csrrs x0, mstatus, %0" :: "r"(0x8UL));
}

/* Example RISC-V critical section */
void riscv_cache_critical(uintptr_t addr, size_t len, unsigned int plic_ctx, uint32_t plic_thr) {
    /* raise PLIC threshold to block routine interrupts */
    plic_set_threshold(plic_ctx, plic_thr);
    local_disable_mie(); /* prevent local preemption */
    /* perform cache maintenance (implementation-specific) */
    // riscv_clean_dcache(addr, len);
    __asm__ volatile("fence.i" ::: "memory");
    local_enable_mie();
    plic_set_threshold(plic_ctx, 0); /* allow all interrupts again */
}