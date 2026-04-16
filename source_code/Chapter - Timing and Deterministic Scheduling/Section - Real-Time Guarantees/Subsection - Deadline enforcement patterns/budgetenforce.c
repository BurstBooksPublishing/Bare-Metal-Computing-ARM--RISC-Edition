ARM (AArch64) and RISC-V Portable Per-Task Timer Programming and Handler


#include <stdint.h>

/* Portable per-task timer programming and handler. Assumes:
 * - task_t structure with fields: uint64_t remaining_cycles, uint64_t deadline;
 * - scheduler_yield() performs context switch; enforce_deadline_violation() handles failure.
 * - Architecture selection via __aarch64__ or __riscv.
 */

/* Schedule timer for next event: min(remaining, slice) */
static inline void program_timer(uint64_t cycles)
{
#ifdef __aarch64__
    __asm__ volatile(
        "msr CNTP_TVAL_EL0, %0\n"
        "isb"
        :
        : "r"(cycles)
        : "memory"
    );
#elif defined(__riscv)
    /* CLINT: write mtimecmp = mtime + cycles; mtime base access platform-specific */
    volatile uint64_t *mtime = (volatile uint64_t *)CLINT_MTIME;
    volatile uint64_t *mtimecmp = (volatile uint64_t *)CLINT_MTIMECMP(hartid());
    uint64_t t = *mtime;
    *mtimecmp = t + cycles;
#else
# error "Architecture not supported"
#endif
}

/* Timer interrupt handler invoked on compare match */
void timer_irq_handler(void)
{
    task_t *current = scheduler_current_task();
    uint64_t elapsed = measure_elapsed_cycles(); /* architecture-specific, e.g. CNTPCT or cycle CSR */
    if (elapsed >= current->remaining_cycles) {
        /* Budget exhausted: enforce violation */
        current->remaining_cycles = 0;
        enforce_deadline_violation(current); /* log, notify, or kill */
        scheduler_yield(); /* preempt offending task */
    } else {
        current->remaining_cycles -= elapsed;
        /* Program next timer for remaining budget or scheduler slice */
        uint64_t next = (current->remaining_cycles < SCHED_SLICE) ? current->remaining_cycles : SCHED_SLICE;
        program_timer(next);
    }
    clear_timer_interrupt(); /* ack interrupt source */
}