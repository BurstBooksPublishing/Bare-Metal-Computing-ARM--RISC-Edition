AArch64 Compare-and-Swap for 64-bit Pointers


static inline int cas_ptr_aarch64(uint64_t *addr, uint64_t *expected, uint64_t desired)
{
    uint64_t old;
    unsigned int res;
    __asm__ volatile(
        "1: ldaxr  %0, [%2]\n"          /* old = *addr (acquire, exclusive) */
        "   cmp    %0, %3\n"            /* compare with expected */
        "   b.ne   2f\n"                /* if not equal, exit with observed value */
        "   stlxr  %w1, %4, [%2]\n"     /* try store; res = 0 on success */
        "   cbnz   %w1, 1b\n"           /* retry if store failed */
        "2:\n"
        : "=&r"(old), "=&r"(res)
        : "r"(addr), "r"(*expected), "r"(desired)
        : "cc", "memory");
    if (old != *expected) {
        *expected = old;
        return 0;
    }
    return 1;
}


RISC-V RV64 Compare-and-Swap Using LR.D/SC.D


static inline int cas_ptr_riscv(uint64_t *addr, uint64_t *expected, uint64_t desired)
{
    uint64_t old;
    uint64_t res;
    __asm__ volatile(
        "1: lr.d   %0, (%2)\n"          /* old = *addr (load-reserved) */
        "   bne    %0, %3, 2f\n"        /* if not equal, exit with observed value */
        "   sc.d   %1, %4, (%2)\n"      /* try store-conditional; res==0 => success */
        "   bnez   %1, 1b\n"            /* retry if store failed */
        "2:\n"
        : "=&r"(old), "=&r"(res)
        : "r"(addr), "r"(*expected), "r"(desired)
        : "memory");
    if (old != *expected) {
        *expected = old;
        return 0;
    }
    return 1;
}