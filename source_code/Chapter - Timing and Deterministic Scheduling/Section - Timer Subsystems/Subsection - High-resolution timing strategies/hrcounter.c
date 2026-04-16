\title{High-Resolution Cycle Counter Calibration}
\caption{ARM (AArch64) and RISC-V compatible implementation for reading hardware cycle counters and calibrating their frequency using a platform-provided millisecond delay.}

#include <stdint.h>

// Platform supplies this blocking delay for calibration (ms granularity).
typedef void (*delay_ms_fn)(uint32_t);

// Read a 64-bit hardware cycle/time counter.
static inline uint64_t read_cycle_counter(void) {
#if defined(__aarch64__)
    uint64_t v;
    __asm__ volatile(
        "isb\n"                     // ensure ordering
        "mrs %0, CNTVCT_EL0\n"      // virtual count (EL0 accessible)
        : "=r"(v) :: "memory");
    return v;
#elif defined(__riscv)
    uint64_t hi, lo;
    // On RV64 a single CSR read is sufficient; compilers provide 'rdcycle'.
#if __riscv_xlen == 64
    __asm__ volatile("rdcycle %0" : "=r"(lo));
    return lo;
#else
    // RV32: atomic read of mcycle via mcycleh/mcycle loop
    do {
        __asm__ volatile("csrr %0, mcycleh" : "=r"(hi));
        __asm__ volatile("csrr %0, mcycle"  : "=r"(lo));
        uint64_t hi2;
        __asm__ volatile("csrr %0, mcycleh" : "=r"(hi2));
        if (hi == hi2) return ((uint64_t)hi << 32) | lo;
    } while (1);
#endif
#else
#error "Unsupported architecture for high-resolution counter"
#endif
}

// Convert cycles to nanoseconds given frequency in Hz.
static inline uint64_t cycles_to_ns(uint64_t cycles, uint64_t freq_hz) {
    // Use 128-bit intermediate to avoid overflow on 64-bit systems.
    __uint128_t v = (__uint128_t)cycles * 1000000000ULL;
    return (uint64_t)(v / freq_hz);
}

// Calibrate counter frequency using a blocking reference delay.
// Returns estimated frequency in Hz.
uint64_t calibrate_counter(delay_ms_fn delay_ms, uint32_t ref_ms) {
    uint64_t c1 = read_cycle_counter();
    delay_ms(ref_ms);                   // platform-supplied millisecond delay
    uint64_t c2 = read_cycle_counter();
    uint64_t delta = c2 - c1;
    return (uint64_t)((__uint128_t)delta * 1000ULL / ref_ms); // Hz
}