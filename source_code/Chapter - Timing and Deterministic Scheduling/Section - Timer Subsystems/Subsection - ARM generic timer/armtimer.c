AArch64 One-Shot Physical Timer Configuration from EL1


#include <stdint.h>

int arm_one_shot_timer_us(uint64_t delay_us)
{
    uint64_t cntfrq, now, ticks, cval;
    uint64_t tmp64;
    uint32_t ctl;

    // Read timer frequency
    __asm__ volatile("mrs %0, CNTFRQ_EL0" : "=r"(cntfrq));

    // Compute ticks = ceil(cntfrq * delay_us / 1e6)
    // Use integer arithmetic avoiding overflow: (cntfrq * delay_us + 999999) / 1000000
    if (delay_us == 0) {
        ticks = 1;
    } else {
        __uint128_t prod = (__uint128_t)cntfrq * delay_us + 999999ULL;
        ticks = (uint64_t)(prod / 1000000ULL);
    }

    // Read current physical counter
    __asm__ volatile("mrs %0, CNTPCT_EL0" : "=r"(now));

    // Compute compare value and check overflow
    cval = now + ticks;
    if (cval < now) {
        return -1; // overflow (practically unlikely)
    }

    // Disable timer (CTL = 0)
    tmp64 = 0;
    __asm__ volatile("msr CNTP_CTL_EL0, %0" :: "r"(tmp64) : "memory");

    // Ensure previous writes complete
    __asm__ volatile("dsb ish");
    __asm__ volatile("isb");

    // Program compare value (64-bit)
    __asm__ volatile("msr CNTP_CVAL_EL0, %0" :: "r"(cval) : "memory");

    // Ensure CVAL is visible before enabling interrupt
    __asm__ volatile("dsb ish");
    __asm__ volatile("isb");

    // Enable timer: ENABLE=1, IMASK=0 (unmasked)
    ctl = 1U;
    __asm__ volatile("msr CNTP_CTL_EL0, %0" :: "r"((uint64_t)ctl) : "memory");
    __asm__ volatile("dsb ish");
    __asm__ volatile("isb");

    return 0;
}