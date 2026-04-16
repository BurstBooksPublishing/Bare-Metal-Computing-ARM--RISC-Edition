High-Resolution Periodic Execution with Interrupt Control for ARM AArch64 and RISC-V


#include <stdint.h>

#if defined(__aarch64__)
/* Read 64-bit virtual counter (CNTVCT\_EL0) for cycle-approximate timing. */
static inline uint64_t read_cyclecounter(void) {
    uint64_t v;
    asm volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
}

/* Disable IRQs by setting the I bit in DAIF; return previous DAIF state. */
static inline uint64_t disable_interrupts(void) {
    uint64_t daif;
    asm volatile("mrs %0, daif; msr daifset, #2" : "=r"(daif) :: "memory");
    return daif;
}

/* Restore interrupt state from saved DAIF value. */
static inline void restore_interrupts(uint64_t daif) {
    asm volatile("msr daif, %0" :: "r"(daif) : "memory");
}

#elif defined(__riscv)
/* Read 64-bit machine cycle counter (mcycle). */
static inline uint64_t read_cyclecounter(void) {
    uint64_t v;
    asm volatile("rdcycle %0" : "=r"(v));
    return v;
}

/* Disable interrupts by clearing MIE (bit 3) in mstatus; return previous mstatus. */
static inline uint64_t disable_interrupts(void) {
    uint64_t mstatus;
    asm volatile("csrrc %0, mstatus, %1" : "=r"(mstatus) : "r"(0x8UL) : "memory");
    return mstatus;
}

/* Restore interrupt state by writing back mstatus. */
static inline void restore_interrupts(uint64_t mstatus) {
    asm volatile("csrw mstatus, %0" :: "r"(mstatus) : "memory");
}

#else
#error "Unsupported target architecture"
#endif

/* Busy-wait until cycle counter reaches next\_cycle. Use WFI to reduce active power. */
static inline void periodic_sleep_cycles(uint64_t next_cycle) {
    while (1) {
        uint64_t now = read_cyclecounter();
        if (now >= next_cycle) {
            break;
        }
        asm volatile("wfi" ::: "memory");
    }
}

/* Run a periodic callback with deterministic start time and steady phase. */
void run_periodic(void (*callback)(void), uint64_t period_cycles) {
    uint64_t next = read_cyclecounter() + period_cycles;
    while (1) {
        periodic_sleep_cycles(next);
        uint64_t saved = disable_interrupts();
        callback();
        restore_interrupts(saved);
        next += period_cycles;
    }
}