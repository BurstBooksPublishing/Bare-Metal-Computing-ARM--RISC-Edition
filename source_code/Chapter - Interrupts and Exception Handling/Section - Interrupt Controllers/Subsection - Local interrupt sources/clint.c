RISC-V Timer and IPI Management Utilities


#include <stdint.h>

#define CLINT_BASE       0x02000000UL  /* platform-dependent */
#define MSIP_BASE        (CLINT_BASE + 0x0000)
#define MTIME_BASE       (CLINT_BASE + 0xBFF8)
#define MTIMECMP_BASE    (CLINT_BASE + 0x4000)

static inline void fence_rw(void)
{
    __asm__ volatile("fence rw, rw" ::: "memory");
}

/* send software interrupt to hart 'to' */
static inline void clint_send_ipi(unsigned to)
{
    volatile uint32_t *msip = (volatile uint32_t *)(MSIP_BASE + (4UL * to));
    *msip = 1U;           /* write nonzero to assert MSIP */
    fence_rw();           /* ensure write is visible to target hart */
}

/* clear software interrupt on hart 'h' (called on target hart) */
static inline void clint_clear_ipi(unsigned h)
{
    volatile uint32_t *msip = (volatile uint32_t *)(MSIP_BASE + (4UL * h));
    *msip = 0U;
    fence_rw();
}

/* set a one-shot timer delta_cycles from now for hart 'h' */
static inline void clint_set_timer(unsigned h, uint64_t delta_cycles)
{
    volatile uint64_t *mtime = (volatile uint64_t *)MTIME_BASE;
    volatile uint64_t *mtimecmp = (volatile uint64_t *)(MTIMECMP_BASE + (8UL * h));
    uint64_t target = *mtime + delta_cycles;

    /* Prevent spurious earlier interrupts by setting a large compare, then final value. */
    *mtimecmp = UINT64_MAX;
    fence_rw();
    *mtimecmp = target;
    fence_rw();
}