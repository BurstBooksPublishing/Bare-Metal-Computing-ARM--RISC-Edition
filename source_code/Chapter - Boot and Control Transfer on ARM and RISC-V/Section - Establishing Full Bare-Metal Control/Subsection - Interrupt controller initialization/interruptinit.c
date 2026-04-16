Production-Ready GICv2 and PLIC Interrupt Controller Initialization for ARM (AArch64) and RISC-V


#include <stdint.h>
#include <stddef.h>

static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    (*(volatile uint32_t *)addr) = val;
}

static inline uint32_t mmio_read32(uintptr_t addr)
{
    return (*(volatile uint32_t *)addr);
}

static inline void barrier_arm(void)
{
    __asm__ volatile("dsb sy" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
}

static inline void barrier_riscv(void)
{
    __asm__ volatile("fence iorw, iorw" ::: "memory");
}

/* GICv2 minimal init */
void gicv2_init(uintptr_t dist_base, uintptr_t cpu_base, unsigned num_irqs)
{
    /* Disable distributor while configuring */
    mmio_write32(dist_base + 0x000, 0x0); /* GICD_CTLR */
    barrier_arm();

    /* Set priorities and targets for SPIs (32..num_irqs-1) */
    for (unsigned i = 32; i < num_irqs; ++i) {
        uintptr_t pri = dist_base + 0x400 + (i & ~0x3); /* GICD_IPRIORITYRn */
        uint32_t shift = (i & 0x3) * 8;
        uint32_t w = mmio_read32(pri);
        w = (w & ~(0xFFU << shift)) | (0x80U << shift); /* mid priority */
        mmio_write32(pri, w);

        /* Set target CPU0 for SPIs */
        uintptr_t tgt = dist_base + 0x800 + (i & ~0x3); /* GICD_ITARGETSRn */
        w = mmio_read32(tgt);
        w = (w & ~(0xFFU << shift)) | (0x01U << shift); /* target CPU0 */
        mmio_write32(tgt, w);
    }

    /* Enable all configured SPIs (set ISENABLER bits) */
    for (unsigned i = 32; i < num_irqs; i += 32) {
        uintptr_t isen = dist_base + 0x100 + (i / 8); /* GICD_ISENABLERn offset 0x100, word-per-32 */
        mmio_write32(isen, 0xFFFFFFFFU);
    }

    /* Enable distributor */
    mmio_write32(dist_base + 0x000, 0x1);
    barrier_arm();

    /* CPU interface: set priority mask to allow all, enable signaling */
    mmio_write32(cpu_base + 0x004, 0xFF);   /* GICC_PMR */
    mmio_write32(cpu_base + 0x000, 0x1);    /* GICC_CTLR */
    barrier_arm();
}

/* Simple PLIC init for a single hart context */
void plic_init(uintptr_t plic_base, unsigned num_sources, unsigned hart_ctx)
{
    /* Set priorities for all sources to 1 */
    for (unsigned s = 1; s <= num_sources; ++s) {
        mmio_write32(plic_base + 0x0000 + s * 4, 1);
    }
    barrier_riscv();

    /* Enable all sources for given context: enable words start at 0x2000 + ctx_offset */
    uintptr_t en_base = plic_base + 0x2000 + hart_ctx * 0x100;
    for (unsigned w = 0; w < (num_sources + 31) / 32; ++w) {
        mmio_write32(en_base + w * 4, 0xFFFFFFFFU);
    }

    /* Set threshold = 0 for context to accept all interrupts with priority > 0 */
    uintptr_t threshold = plic_base + 0x200000 + hart_ctx * 0x1000;
    mmio_write32(threshold + 0x0, 0x0);
    barrier_riscv();
}