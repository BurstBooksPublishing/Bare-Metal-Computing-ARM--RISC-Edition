RISC-V Event Logger for Low-Level System Tracing


#include <stdint.h>
#include <stdatomic.h>

#define LOG_ENTRIES 16384

typedef struct {
    uint64_t minstret;
    uint32_t ev_type;
    uint32_t ev_arg;
} log_entry_t;

static log_entry_t logbuf[LOG_ENTRIES];
static _Atomic uint_least32_t log_idx = 0;

/* Read CSR minstret (retired instructions). */
static inline uint64_t read_minstret(void) {
    uint64_t v;
    __asm__ volatile ("csrr %0, minstret" : "=r"(v));
    return v;
}

/* Called in IRQ context; keep small and atomic. */
void irq_logger(uint32_t ev_type, uint32_t ev_arg) {
    uint32_t idx = atomic_fetch_add_explicit(&log_idx, 1, memory_order_relaxed);
    if (idx >= LOG_ENTRIES) return;                // drop if full; production: signal overflow
    logbuf[idx].minstret = read_minstret();        // capture precise retired count
    logbuf[idx].ev_type  = ev_type;                // e.g., TIMER, UART_RX, DMA_DONE
    logbuf[idx].ev_arg   = ev_arg;                 // vector or payload summary
    __asm__ volatile ("" ::: "memory");            // compiler barrier
}

/* Example IRQ entry must call irq_logger(...) early, before clobbering registers. */