RISC-V Supervisor Mode Delegation Configuration


#include <stdint.h>

/* Cause numbers from the RISC-V privileged specification */
#define CAUSE_ECALL_U            8U
#define CAUSE_INST_PAGE_FAULT   12U
#define CAUSE_LOAD_PAGE_FAULT   13U
#define CAUSE_STORE_PAGE_FAULT  15U

/* Platform-specific supervisor interrupt indices (define per platform header) */
#ifndef INT_SUPERVISOR_TIMER
#define INT_SUPERVISOR_TIMER    5U  /* placeholder: override from platform header */
#endif
#ifndef INT_SUPERVISOR_EXTERNAL
#define INT_SUPERVISOR_EXTERNAL 9U  /* placeholder: override from platform header */
#endif

static inline void write_medeleg(uint64_t v) {
    asm volatile("csrw medeleg, %0" :: "r"(v) : "memory");
}

static inline uint64_t read_medeleg(void) {
    uint64_t v;
    asm volatile("csrr %0, medeleg" : "=r"(v));
    return v;
}

static inline void write_mideleg(uint64_t v) {
    asm volatile("csrw mideleg, %0" :: "r"(v) : "memory");
}

/* Configure delegation: delegate user ECALL and page faults to S-mode.
   Also demonstrate pattern for interrupt delegation (indices are platform-defined). */
void configure_delegation_early(void) {
    uint64_t medeleg = 0;

    medeleg |= (1ULL << CAUSE_ECALL_U);
    medeleg |= (1ULL << CAUSE_INST_PAGE_FAULT);
    medeleg |= (1ULL << CAUSE_LOAD_PAGE_FAULT);
    medeleg |= (1ULL << CAUSE_STORE_PAGE_FAULT);

    /* Write delegation mask atomically; ensure no interrupts enable during this step. */
    write_medeleg(medeleg);

    /* Delegate supervisor-level timer and external interrupts (indices platform-specific). */
    uint64_t mideleg = 0;
    mideleg |= (1ULL << INT_SUPERVISOR_TIMER);
    mideleg |= (1ULL << INT_SUPERVISOR_EXTERNAL);
    write_mideleg(mideleg);

    /* Optional verification: read back */
    (void)read_medeleg();
}