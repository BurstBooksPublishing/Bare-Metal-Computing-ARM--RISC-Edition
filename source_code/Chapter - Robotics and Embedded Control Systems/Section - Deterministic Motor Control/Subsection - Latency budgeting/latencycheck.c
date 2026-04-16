High-Resolution Latency Measurement for ARM AArch64 and RISC-V


#include <stdint.h>
#include <stdbool.h>

static inline uint64_t read_cycles(void)
{
#if defined(__aarch64__)
    uint64_t v;
    __asm__ volatile(
        "isb\n\t"
        "mrs %0, CNTVCT_EL0"
        : "=r"(v)
    );
    return v;
#elif defined(__riscv)
    uint64_t v;
    __asm__ volatile(
        "rdcycle %0"
        : "=r"(v)
    );
    return v;
#else
#error "Unsupported architecture"
#endif
}

/* check_latency: runs callback and verifies its duration <= budget_cycles.
   callback should be non-blocking and reentrant in the real-time context. */
bool check_latency(uint64_t budget_cycles, void (*callback)(void))
{
    uint64_t t0 = read_cycles();
    __asm__ volatile("" ::: "memory");
    callback();
    __asm__ volatile("" ::: "memory");
    uint64_t t1 = read_cycles();
    return (t1 - t0) <= budget_cycles;
}