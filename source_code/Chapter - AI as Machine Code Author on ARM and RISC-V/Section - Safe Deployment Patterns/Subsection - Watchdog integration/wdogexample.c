Production-Ready Watchdog Driver for ARM (AArch64) and RISC-V Systems


#include <stdint.h>

#define WDOG_BASE       0x40000000UL
#define WDOG_UNLOCK     ((volatile uint32_t*)(WDOG_BASE + 0x00))
#define WDOG_PRESCALER  ((volatile uint32_t*)(WDOG_BASE + 0x04))
#define WDOG_RELOAD     ((volatile uint32_t*)(WDOG_BASE + 0x08))
#define WDOG_CTRL       ((volatile uint32_t*)(WDOG_BASE + 0x0C))
#define WDOG_INTCLR     ((volatile uint32_t*)(WDOG_BASE + 0x10))

static inline void cpu_barrier(void) {
#if defined(__aarch64__)
    __asm__ volatile("dsb ish\nisb" ::: "memory"); // Data and instruction synchronization barriers
#else
    __asm__ volatile("fence rw, rw" ::: "memory"); // Full memory fence for RISC-V
#endif
}

/* Initialize watchdog with timeout_ms and optional early_interrupt flag */
void wdog_init(uint32_t timeout_ms, int early_interrupt) {
    const uint32_t fclk_hz = 1000000U; // 1 MHz watchdog clock
    uint32_t prescaler = 1;
    uint32_t reload;

    // Choose prescaler and reload to satisfy timing requirements
    while (prescaler <= 256) {
        uint64_t ticks = (uint64_t)timeout_ms * fclk_hz / 1000U;
        reload = (uint32_t)((ticks + prescaler - 1) / prescaler - 1);
        if (reload <= 0xFFFFFFFFU) {
            break;
        }
        prescaler <<= 1;
    }

    // Unlock watchdog registers with hardware-specific key
    *WDOG_UNLOCK = 0x1ACCE551;
    cpu_barrier();

    // Configure prescaler and reload value
    *WDOG_PRESCALER = prescaler;
    *WDOG_RELOAD = reload;
    cpu_barrier();

    // Configure control register: enable watchdog and optionally interrupt-before-reset
    uint32_t ctrl = (1U << 0); // Enable bit
    if (early_interrupt) {
        ctrl |= (1U << 1); // Interrupt enable bit
    }
    *WDOG_CTRL = ctrl;
    cpu_barrier();
}

/* Service (pet) watchdog; must be fast and non-blocking */
void wdog_service(void) {
    // Clear interrupt and reload timer
    *WDOG_INTCLR = 0xFFFFFFFFU;
    cpu_barrier();
}