ARM (AArch64) and RISC-V Latency-Hiding Atomic Update


#include <stdint.h>

static inline uint64_t hide_load_latency_and_release(uint64_t *ptr, uint64_t v) {
#if defined(__aarch64__)
    uint64_t val;
    __asm__ volatile(
        "ldr %0, [%1]\n\t"
        "add x9, xzr, #1\n\t"
        "add %0, %0, %2\n\t"
        "stlr %0, [%1]\n\t"
        : "=&r"(val)
        : "r"(ptr), "r"(v)
        : "x9", "memory");
    return val;
#elif defined(__riscv)
    uint64_t val;
    __asm__ volatile(
        "ld %0, 0(%1)\n\t"
        "addi t0, x0, 1\n\t"
        "add %0, %0, %2\n\t"
        "fence rw, rw\n\t"
        "sd %0, 0(%1)\n\t"
        : "=&r"(val)
        : "r"(ptr), "r"(v)
        : "t0", "memory");
    return val;
#else
    uint64_t tmp = *ptr;
    tmp += 1;
    tmp += v;
    *ptr = tmp;
    return tmp;
#endif
}