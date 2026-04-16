\begin{lstlisting}[language=C,caption={Context switch: save current SP, load next SP, switch address-space (AArch64 and RISC-V).},label={lst:context_switch}]

ARM (AArch64) and RISC-V Context Switch Implementation


#include <stdint.h>

struct tcb {
    void *sp;        // saved stack pointer for this task
    uint64_t asid;   // architecture-specific address-space token
};

// context_switch(cur, next) called with:
//  x0/a0 = pointer to current tcb
//  x1/a1 = pointer to next tcb
static inline void context_switch(struct tcb *cur, struct tcb *next) {
#if defined(__aarch64__)
    // Store current SP into cur->sp, load next->sp into SP.
    // Then, if next->asid != cur->asid, write TTBR0_EL1 and ISB.
    asm volatile(
        "str sp, [%0]\n"        // save sp to cur->sp
        "ldr sp, [%1]\n"        // load sp from next->sp
        // load ASID into x2, compare with zero to decide install
        "ldr x2, [%1, #8]\n"    // x2 = next->asid
        "cbz x2, 1f\n"          // if zero, skip ttbr write
        // caller must have prepared next->asid as TTBR0 value
        "msr ttbr0_el1, x2\n"
        "isb\n"
        "1:\n"
        : /* no outputs */
        : "r"(cur), "r"(next)
        : "x2", "memory"
    );
#elif defined(__riscv)
    // RISC-V: store sp to cur->sp, load next->sp, write satp and SFENCE.VMA.
    asm volatile(
        "sd sp, 0(%0)\n"        // save sp
        "ld sp, 0(%1)\n"        // load sp
        "ld t0, 8(%1)\n"        // t0 = next->asid (satp value)
        "beqz t0, 1f\n"
        "csrw satp, t0\n"
        "sfence.vma\n"
        "1:\n"
        : /* no outputs */
        : "r"(cur), "r"(next)
        : "t0", "memory"
    );
#else
#error "Unsupported architecture"
#endif
}