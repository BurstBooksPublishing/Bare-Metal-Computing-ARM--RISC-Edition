//Safe Indexed Load for ARM (AArch64) and RISC-V Architectures


#include <stdint.h>
#include <stddef.h>

static inline uint64_t safe_indexed_load(const uint64_t *base, size_t len, size_t idx) {
    // Compute mask: all ones if idx < len, else zero (works for 2's complement)
    size_t less = (idx < len);
    // Convert boolean to full-width mask: 0 or ~0ULL
    uint64_t mask = (uint64_t)0 - (uint64_t)less;
    // Effective index is masked to zero when out-of-range: prevents speculative addressing outside base..base+len-1
    size_t eff_idx = idx & mask;
    const uint64_t *addr = base + eff_idx;

#if defined(__aarch64__)
    uint64_t val;
    asm volatile(
        "ldr %x0, [%1]\n\t"   // safe unconditional load from masked address
        : "=&r"(val) : "r"(addr) : "memory"
    );
    // Ensure no subsequent instructions execute with stale speculative state
    asm volatile("dsb sy\n\t" "isb\n\t" ::: "memory");
    return val & mask;       // zero result if out-of-range
#elif defined(__riscv) && __riscv_xlen == 64
    uint64_t val;
    asm volatile(
        "ld %0, (%1)\n\t"    // load from masked address
        : "=&r"(val) : "r"(addr) : "memory"
    );
    // Full fence to order and limit speculative effects
    asm volatile("fence rw, rw" ::: "memory");
    return val & mask;
#else
    // Fallback: conservative, with volatile access (may be slower)
    volatile uint64_t v = *addr;
    return v & mask;
#endif
}