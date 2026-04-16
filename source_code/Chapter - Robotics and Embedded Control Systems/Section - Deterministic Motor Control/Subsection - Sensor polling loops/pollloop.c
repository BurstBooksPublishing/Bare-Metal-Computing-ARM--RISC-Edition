The provided code is written in C with inline assembly and is designed to run on embedded systems, interacting directly with hardware peripherals like ADC and PWM. It uses architecture-specific instructions for cycle counting and memory barriers on both ARM (AArch64) and RISC-V platforms.

Below is the upgraded, production-ready version of the code:

    Deterministic ADC Sampling and PWM Control Loop for ARM (AArch64) and RISC-V

#include <stdint.h>
#include <stdbool.h>

/* Platform MMIO addresses (adjust for your SoC) */
#define ADC_BASE    ((volatile uint32_t*)0x40012000)  // ADC data register
#define ADC_STATUS  ((volatile uint32_t*)0x40012004)  // ADC status (data ready bit)
#define PWM_BASE    ((volatile uint32_t*)0x40021000)  // PWM duty register

/* Control parameters */
const uint64_t SAMPLE_HZ = 1000;                      // 1 kHz loop
const double alpha = 0.1;                             // single-pole low-pass factor

/* High-resolution counter access */
static inline uint64_t read_cycles(void) {
#if defined(__aarch64__)
    uint64_t v;
    asm volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
#elif defined(__riscv)
    uint64_t v;
    asm volatile("rdcycle %0" : "=r"(v));
    return v;
#else
    #error "Unsupported architecture"
#endif
}

/* Timebase conversion: cycles per second must be configured for platform */
extern const uint64_t CYCLES_PER_SEC; /* provide in platform init */

/* Memory barriers for ordering MMIO */
static inline void mmio_barrier(void) {
#if defined(__aarch64__)
    asm volatile("dmb ish" ::: "memory");
#elif defined(__riscv)
    asm volatile("fence rw, rw" ::: "memory");
#endif
}

void log_deadline_miss(void); /* platform-specific logger */

/* Deterministic polling loop */
void polling_loop(void) {
    const uint64_t T_cycles = CYCLES_PER_SEC / SAMPLE_HZ;
    uint64_t next_deadline = read_cycles() + T_cycles;
    double filtered = 0.0;

    while (true) {
        uint64_t t_start = read_cycles();

        /* Read ADC with MMIO ordering and data-ready check */
        while ((*ADC_STATUS & 1u) == 0u) {
            /* spin for data ready; add timeout if needed */
        }
        mmio_barrier();
        uint32_t raw = *ADC_BASE; /* volatile MMIO read */

        /* Simple conversion and filter (fixed-point alternatives recommended for heavy loops) */
        double sample = (double)raw;
        filtered = alpha * sample + (1.0 - alpha) * filtered;

        /* Compute control command (example: scale filtered to PWM duty) */
        uint32_t duty = (uint32_t)(filtered * 0.001); // scale according to hardware

        mmio_barrier();
        *PWM_BASE = duty; /* volatile MMIO write */

        /* Deadline management: absolute-time waiting */
        if (read_cycles() > next_deadline) {
            log_deadline_miss();                 /* missed deadline */
            next_deadline = read_cycles() + T_cycles; /* resync to avoid permanent slip */
        } else {
            while (read_cycles() < next_deadline) {
                /* busy-wait until deadline */
                asm volatile("nop"); /* yield minimal pipeline work */
            }
            next_deadline += T_cycles;
        }

        /* Optional: monitor jitter or iterate count for diagnostics */
        (void)t_start; /* keep optimizer happy if unused in release */
    }
}