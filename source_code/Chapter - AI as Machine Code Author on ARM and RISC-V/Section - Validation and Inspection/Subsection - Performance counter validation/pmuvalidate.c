#include <stdint.h>

/* ARM (AArch64): enable user access and reset cycle counter, then read PMCCNTR\_EL0 */
#if defined(__aarch64__)
static inline void aarch64_pmu_enable_user(void) {
    uint64_t one = 1, zero = 0;
    /* Allow EL0 user reads of counters */
    asm volatile("msr PMUSERENR_EL0, %0" :: "r"(one) : );
    /* Reset cycle counter to 0 */
    asm volatile("msr PMCCNTR_EL0, %0" :: "r"(zero) : );
    /* Enable cycle counter via PMCNTENSET_EL0: set bit 31 for cycle counter */
    uint64_t cycle_bit = (1ULL << 31);
    asm volatile("msr PMCNTENSET_EL0, %0" :: "r"(cycle_bit) : );
    /* Enable the PMU globally: set bit 0 (E) in PMCR_EL0, preserve other bits */
    uint64_t pmcr;
    asm volatile("mrs %0, PMCR_EL0" : "=r"(pmcr));
    pmcr |= 1;             /* enable */
    asm volatile("msr PMCR_EL0, %0" :: "r"(pmcr) : );
}

static inline uint64_t aarch64_read_cycle(void) {
    uint64_t v;
    asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(v));
    return v;
}
#endif

/* RISC-V RV64: write mhpmevent3 and read mhpmcounter3; read mcycle for cycles */
#if defined(__riscv)
static inline void riscv_pmu_init_event3(uint64_t event) {
    /* In machine mode: program mhpmevent3 then zero counter and enable */
    asm volatile("csrw mhpmevent3, %0" :: "r"(event));
    uint64_t zero = 0;
    asm volatile("csrw mhpmcounter3, %0" :: "r"(zero));
}

static inline uint64_t riscv_read_mcycle(void) {
    uint64_t v;
    asm volatile("csrr %0, mcycle" : "=r"(v));
    return v;
}

static inline uint64_t riscv_read_hpm3(void) {
    uint64_t v;
    asm volatile("csrr %0, mhpmcounter3" : "=r"(v));
    return v;
}
#endif

/* Example measurement harness: run a tight loop that performs N iterations of a fixed instruction */
uint64_t measure_loop_cycles(void (*fn)(void), uint64_t iterations) {
    /* caller must prepare PMU before calling */
    uint64_t before = 0, after = 0;
#if defined(__aarch64__)
    before = aarch64_read_cycle();
    for (uint64_t i = 0; i < iterations; ++i) fn();
    after = aarch64_read_cycle();
#elif defined(__riscv)
    before = riscv_read_mcycle();
    for (uint64_t i = 0; i < iterations; ++i) fn();
    after = riscv_read_mcycle();
#endif
    return after - before;
}