Production-Ready Peripheral Clock Enable Routine for ARM (AArch64) and RISC-V


#include <stdint.h>
#include <stddef.h>

#define PERIPH_BASE     0x40000000UL
#define OFF_CLK_EN      0x04
#define OFF_CLK_DIV     0x08
#define OFF_RST         0x0C
#define OFF_STATUS      0x10

/* MMIO helpers */
static inline void mmio_write32(uintptr_t addr, uint32_t value)
{
    *((volatile uint32_t*)addr) = value;
}

static inline uint32_t mmio_read32(uintptr_t addr)
{
    return *((volatile uint32_t*)addr);
}

/* Architecture-specific memory barriers */
#if defined(__aarch64__)
static inline void mmio_barrier(void)
{
    __asm__ volatile("dmb ish" ::: "memory");
}
#elif defined(__riscv)
static inline void mmio_barrier(void)
{
    __asm__ volatile("fence io, io" ::: "memory");
}
#else
static inline void mmio_barrier(void)
{
    __asm__ volatile("" ::: "memory"); /* compiler-only barrier */
}
#endif

/* Enable peripheral clock and set divider.
 * base: peripheral base address
 * src_freq_hz: input clock frequency in Hz
 * desired_hz: desired peripheral clock in Hz
 * timeout_loops: poll timeout iterations
 * returns 0 on success, -1 on failure
 */
int periph_enable_clock(uintptr_t base,
                        uint64_t src_freq_hz,
                        uint64_t desired_hz,
                        size_t timeout_loops)
{
    if (desired_hz == 0 || src_freq_hz == 0) {
        return -1;
    }

    uint64_t div = src_freq_hz / desired_hz;
    if (div == 0) {
        div = 1;
    }
    if (div > UINT32_MAX) {
        return -1;
    }
    uint32_t div_reg = (uint32_t)(div - 1); /* matches eq. (1) */

    /* 1. enable clock */
    mmio_write32(base + OFF_CLK_EN, 1U);
    mmio_barrier();
    (void)mmio_read32(base + OFF_CLK_EN); /* read-back */

    /* 2. program divider */
    mmio_write32(base + OFF_CLK_DIV, div_reg);
    mmio_barrier();

    /* 3. de-assert reset */
    mmio_write32(base + OFF_RST, 0U);
    mmio_barrier();

    /* 4. wait for clock stable/ready */
    for (size_t i = 0; i < timeout_loops; ++i) {
        uint32_t status = mmio_read32(base + OFF_STATUS);
        if (status & 0x1U) {
            return 0; /* bit 0 == clock ready */
        }
    }

    return -1; /* timeout */
}