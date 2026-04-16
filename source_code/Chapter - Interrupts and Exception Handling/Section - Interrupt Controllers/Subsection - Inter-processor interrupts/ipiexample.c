ARM (AArch64) and RISC-V Inter-Processor Interrupt (IPI) Support


#include <stdint.h>

/* Platform constants (set to your platform values) */
#define GICD_BASE        0x2F000000UL         /* GIC Distributor base address */
#define GICD_SGIR_OFFSET 0xF00UL              /* SGI register offset (GICv2) */
#define CLINT_MSIP_BASE  0x02000000UL         /* CLINT MSIP base on many SiFive parts */

static inline void dsb(void)
{
    __asm__ volatile("dsb sy" ::: "memory");  /* Data Synchronization Barrier */
}

static inline void fence_rw(void)
{
    __asm__ volatile("fence rw, rw" ::: "memory");  /* RISC-V memory fence */
}

/* ARM: send SGI via GICv2 distributor MMIO */
static inline void arm_send_sgi(uint32_t sgi_id, uint16_t target_mask)
{
    volatile uint32_t *gicd_sgir = (uint32_t *)(GICD_BASE + GICD_SGIR_OFFSET);
    dsb();                                    /* Ensure memory ordering */
    uint32_t val = ((target_mask & 0xFFFF) << 16) | (sgi_id & 0xF);
    *gicd_sgir = val;                         /* Write triggers SGI */
}

/* RISC-V: send software IPI by setting MSIP for target hart */
static inline void riscv_send_ipi(unsigned int hart)
{
    volatile uint32_t *msip = (uint32_t *)(CLINT_MSIP_BASE + (4UL * hart));
    fence_rw();                               /* Ensure ordering before IPI */
    *msip = 1U;                               /* Set software interrupt */
}

/* RISC-V handler: invoked on machine software interrupt */
void riscv_msip_handler(unsigned int hart)
{
    volatile uint32_t *msip = (uint32_t *)(CLINT_MSIP_BASE + (4UL * hart));
    *msip = 0U;                               /* Clear MSIP to acknowledge */
    fence_rw();                               /* Ensure visibility of clear */
    /* Perform requested action (e.g., TLB invalidate, scheduler tick) */
}

/* ARM handler: SGI interrupt path should read ICC_IAR1/INTACK and then clear/EOI per GIC design. */
/* Handler code varies by GIC version and is omitted here; follow the GIC specification. */