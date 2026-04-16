Generic GPIO Driver for ARM (AArch64) and RISC-V Systems


#include <stdint.h>

// Configure these per SoC: base MMIO for GPIO region and bank stride.
#ifndef GPIO_BASE
#define GPIO_BASE       0x40020000u      // placeholder; override at build time
#endif
#define GPIO_BANK_STRIDE 0x1000u         // bytes per bank
// Bank register offsets (adjust to platform)
#define GPIO_OFF_DIR     0x00u
#define GPIO_OFF_OUTSET  0x04u
#define GPIO_OFF_OUTCLR  0x08u
#define GPIO_OFF_IN      0x0Cu
#define GPIO_OFF_PULL    0x10u
#define GPIO_OFF_ALT     0x14u

static inline void mmio_write32(uintptr_t addr, uint32_t val) {
    volatile uint32_t *p = (volatile uint32_t *)addr;
    *p = val;
}

static inline uint32_t mmio_read32(uintptr_t addr) {
    volatile uint32_t *p = (volatile uint32_t *)addr;
    return *p;
}

/* Architecture-specific ordering primitives */
static inline void dev_barrier(void) {
#if defined(__aarch64__) || defined(__arm__)
    __asm__ volatile ("dsb sy\nisb\n" : : : "memory");
#elif defined(__riscv)
    __asm__ volatile ("fence iorw, iorw" : : : "memory");
#else
    __asm__ volatile ("" : : : "memory");
#endif
}

/* Compute bank base and bitmask from pin number */
static inline uintptr_t bank_base(unsigned pin) {
    unsigned bank = pin / 32u;
    return GPIO_BASE + (uintptr_t)bank * GPIO_BANK_STRIDE;
}

static inline uint32_t bitmask(unsigned pin) {
    return 1u << (pin % 32u);
}

/* Mode: 0=input, 1=output, 2=alt; pull: 0=none,1=up,2=down */
int gpio_init_pin(unsigned pin, unsigned mode, unsigned pull, unsigned alt_sel) {
    uintptr_t base = bank_base(pin);
    uint32_t mask = bitmask(pin);

    // Ensure peripheral clock already enabled by caller.
    // Configure direction: assume DIR bit = 1 => output.
    if (mode == 1) {
        // If set/clear registers exist, use OUTCLR/OUTSET only for value changes.
        mmio_write32(base + GPIO_OFF_DIR, mmio_read32(base + GPIO_OFF_DIR) | mask);
    } else {
        mmio_write32(base + GPIO_OFF_DIR, mmio_read32(base + GPIO_OFF_DIR) & ~mask);
    }
    dev_barrier();

    // Configure pull (packaging depends on SoC; here a simple per-bit reg).
    uint32_t pull_reg = mmio_read32(base + GPIO_OFF_PULL);
    pull_reg &= ~mask;                   // clear
    if (pull == 1) pull_reg |= mask;     // set pull-up
    // pull==2 would need an alternate register on many SoCs; omitted.
    mmio_write32(base + GPIO_OFF_PULL, pull_reg);
    dev_barrier();

    // Configure alternate function if requested.
    if (mode == 2) {
        uint32_t alt = mmio_read32(base + GPIO_OFF_ALT);
        // example: per-pin alt selector stored as bit presence; real SoC uses fields.
        if (alt_sel) alt |= mask; else alt &= ~mask;
        mmio_write32(base + GPIO_OFF_ALT, alt);
        dev_barrier();
    }
    return 0;
}

/* Output control using set/clear registers to avoid RMW races */
static inline void gpio_set(unsigned pin) {
    uintptr_t base = bank_base(pin);
    mmio_write32(base + GPIO_OFF_OUTSET, bitmask(pin));
    dev_barrier();
}

static inline void gpio_clear(unsigned pin) {
    uintptr_t base = bank_base(pin);
    mmio_write32(base + GPIO_OFF_OUTCLR, bitmask(pin));
    dev_barrier();
}

static inline int gpio_read(unsigned pin) {
    uintptr_t base = bank_base(pin);
    return (mmio_read32(base + GPIO_OFF_IN) & bitmask(pin)) != 0;
}

/* Example usage:
   gpio_init_pin(17, 1, 0, 0); // pin 17 output, no pull
   gpio_set(17);
*/