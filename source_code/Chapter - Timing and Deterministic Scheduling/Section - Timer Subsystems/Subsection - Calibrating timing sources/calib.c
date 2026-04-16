\begin{lstlisting}[language=C,caption={Minimal latency-capture ISR for RV64 and AArch64 (bare metal). Replace MMIO addresses and vector install per platform).},label={lst:irq_latency}]
irq\_latency.c - Minimal ISR captures cycle counter and toggles capture GPIO

\begin{lstlisting}
/* irq_latency.c - minimal ISR captures cycle counter and toggles capture GPIO */
#include <stdint.h>
#include <stddef.h>

/* Platform-specific definitions (replace with real addresses) */
#define GPIO_OUT_ADDR   ((volatile uint32_t*)0x40000000) /* capture pin out */
#define GPIO_DIR_ADDR   ((volatile uint32_t*)0x40000004)
#define EVENT_GPIO_ADDR ((volatile uint32_t*)0x40000008) /* optional event pin */

/* Buffer for recorded timestamps */
volatile uint64_t timestamp_log[256];
volatile size_t ts_index = 0;

/* Read cycle counter: RV64 uses rdcycle, AArch64 uses MRS CNTVCT_EL0 */
static inline uint64_t read_cycle(void) {
#if defined(__riscv)
    uint64_t v;
    asm volatile("rdcycle %0" : "=r"(v));
    return v;
#elif defined(__aarch64__)
    uint64_t v;
    asm volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
#else
# error "Unsupported architecture"
#endif
}

/* Minimal ISR: do not use C prologue/epilogue if you need sub-microsecond accuracy.
   Use compiler attributes to avoid extra code. Install vector to point directly here. */
void __attribute__((interrupt("supervisor"))) irq_handler_minimal(void) {
    /* Capture cycle counter as first action. */
    uint64_t t = read_cycle();
    size_t i = __atomic_fetch_add(&ts_index, 1, __ATOMIC_RELAXED);
    if (i < sizeof(timestamp_log)/sizeof(timestamp_log[0])) timestamp_log[i] = t;

    /* Toggle capture GPIO to provide oscilloscope-visible edge. */
    *GPIO_OUT_ADDR ^= 1u;

    /* Acknowledge/platform-specific EOI here (GIC/PLIC/etc.) */
    /* e.g., write to interrupt controller EOI register. */
}

/* Stimulus generator: toggle event pin, or program timer compare */
void generate_event(void) {
    /* Toggle event GPIO to create external interrupt pulse */
    *EVENT_GPIO_ADDR = 1;
    asm volatile("dmb ish"); /* ensure store reaches bus */
    *EVENT_GPIO_ADDR = 0;
}

/* Main bootstrap: set GPIO direction, install vector, enable interrupts. */
int main(void) {
    *GPIO_DIR_ADDR |= 1; /* make capture pin output */
    ts_index = 0;

    /* Platform-specific: install irq_handler_minimal at vector base */
    /* Enable interrupt lines and unmask CPU IRQ; configure timer or external event. */

    for (int i = 0; i < 1000; ++i) {
        generate_event();
        /* small delay to avoid overrunning the handler */
        for (volatile int d=0; d<10000; ++d) asm volatile("");
    }

    /* After test, transfer timestamp_log off-chip for analysis. */
    while (1) asm volatile("wfi");
    return 0;
}