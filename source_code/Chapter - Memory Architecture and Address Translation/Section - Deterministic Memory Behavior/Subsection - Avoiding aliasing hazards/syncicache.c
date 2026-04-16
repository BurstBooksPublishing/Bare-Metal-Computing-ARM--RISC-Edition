#include <stdint.h>
#include <stddef.h>

/* Sync instruction stream after writing 'len' bytes at 'addr'. 
   For AArch64: clean D-cache by VA, then invalidate I-cache, with DSB/ISB.
   For RISC-V: use FENCE.I; platforms with separate D/I caches require
   platform-specific cache flush primitives (not provided here). */

static inline void sync_icache(void *addr, size_t len) {
    uintptr_t a = (uintptr_t)addr;
    uintptr_t end = a + len;
#if defined(__aarch64__)
    const size_t line = 64; /* typical line size; detect at boot if necessary */
    for (; a < end; a += line) {
        asm volatile("dc cvau, %0" :: "r"(a) : "memory"); /* clean to Point of Unification */
    }
    asm volatile("dsb ish" ::: "memory");
    for (a = (uintptr_t)addr; a < end; a += line) {
        asm volatile("ic ivau, %0" :: "r"(a) : "memory"); /* invalidate I-cache by VA to PoU */
    }
    asm volatile("dsb ish" ::: "memory");
    asm volatile("isb" ::: "memory");
#elif defined(__riscv)
    /* RISC-V: fence.i ensures instruction fetch sees stores.
       If implementation requires explicit D-cache flush, call platform routine. */
    asm volatile("fence.i" ::: "memory");
#else
# error "Unsupported architecture for sync_icache"
#endif
}