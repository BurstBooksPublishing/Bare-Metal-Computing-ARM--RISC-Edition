Enhanced MMIO Device Probing for ARM (AArch64) and RISC-V Systems


#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Platform constants: replace with real SoC ranges. */
#define PERIPH_BASE_START 0x40000000UL
#define PERIPH_BASE_END   0x5FFFFFFFUL

/* Callback types: return 0 on success, non-zero on failure. */
typedef int (*clk_enable_cb)(void);    /* enable peripheral clock */
typedef int (*rst_deassert_cb)(void);  /* deassert peripheral reset */

/* Small platform-specific busy-wait (must be implemented for platform). */
extern void delay_ms(unsigned ms);

/* Arch-specific barriers. */
static inline void hw_barrier(void) {
#if defined(__aarch64__) || defined(__arm__)
    __asm__ volatile("dsb sy" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
#elif defined(__riscv)
    __asm__ volatile("fence iorw, iorw" ::: "memory");
#else
    __asm__ volatile("" ::: "memory"); /* conservative compiler barrier */
#endif
}

/* Safe probe: returns true if device matches (masked) value. */
bool probe_mmio_device(uintptr_t base, size_t len,
                       uint32_t id_offset, uint32_t id_mask, uint32_t id_value,
                       clk_enable_cb clk_cb, rst_deassert_cb rst_cb)
{
    uint32_t val;
    unsigned attempt = 0;
    const unsigned max_attempts = 5;
    const unsigned t0_ms = 1;

    /* Basic sanity checks: within documented peripheral region and offset valid */
    if (base < PERIPH_BASE_START || (base + len - 1) > PERIPH_BASE_END) {
        return false;
    }
    if ((uintptr_t)id_offset >= len) {
        return false;
    }

    if (clk_cb && clk_cb() != 0) {
        return false;
    }
    if (rst_cb && rst_cb() != 0) {
        return false;
    }

    while (attempt < max_attempts) {
        hw_barrier(); /* ensure previous control writes seen by device */
        volatile uint32_t *p = (volatile uint32_t *)(base + id_offset);
        val = *p; /* single read to documented ID register */

        if ((val & id_mask) == id_value) {
            return true;
        }

        /* exponential backoff before next attempt */
        unsigned wait = t0_ms << attempt;
        if (wait > 100) {
            wait = 100; /* cap per-attempt wait to 100 ms */
        }
        delay_ms(wait);
        attempt++;
    }
    return false;
}