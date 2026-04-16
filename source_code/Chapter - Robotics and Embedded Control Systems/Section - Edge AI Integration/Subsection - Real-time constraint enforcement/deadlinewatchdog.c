\title{High-Performance Deadline Enforcement for ARM (AArch64) and RISC-V}
\caption{Production-ready cycle-accurate deadline enforcement with recovery callback support.}

#include <stdint.h>
#include <stdbool.h>

#ifndef RV_CYCLES_PER_SEC
#define RV_CYCLES_PER_SEC 1000000000ULL
#endif

typedef void (*recovery_cb_t)(void);

static inline uint64_t read_cycles(void) {
#ifdef __aarch64__
    uint64_t v;
    asm volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
#elif defined(__riscv) && __riscv_xlen == 64
    uint64_t v;
    asm volatile("rdcycle %0" : "=r"(v));
    return v;
#else
    #error "Unsupported architecture: define cycle read method"
#endif
}

static inline uint64_t cycles_per_sec(void) {
#ifdef __aarch64__
    uint64_t f;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(f));
    return f;
#elif defined(__riscv) && __riscv_xlen == 64
    return RV_CYCLES_PER_SEC;
#else
    #error "Unsupported architecture: cannot determine frequency"
#endif
}

static inline uint64_t cycles_to_ns(uint64_t cycles) {
    return (cycles * 1000000000ULL) / cycles_per_sec();
}

bool run_with_deadline(uint64_t deadline_ns, recovery_cb_t cb, void (*critical_fn)(void)) {
    uint64_t start = read_cycles();
    critical_fn();
    uint64_t elapsed_ns = cycles_to_ns(read_cycles() - start);
    if (elapsed_ns > deadline_ns) {
        if (cb) cb();
        return false;
    }
    return true;
}