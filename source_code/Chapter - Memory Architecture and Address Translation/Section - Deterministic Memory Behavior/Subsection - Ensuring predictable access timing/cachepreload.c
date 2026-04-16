\title{Cache Preloading and Memory Ordering for ARM (AArch64) and RISC-V}
\caption{High-quality implementation of cache line preloading with architecture-specific memory ordering}

#include <stdint.h>
#include <stddef.h>

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

void preload_cache_lines(void *addr, size_t len, int do_clean)
{
    uintptr_t p = (uintptr_t)addr;
    uintptr_t end = p + len;

    for (; p < end; p += CACHE_LINE_SIZE) {
        volatile uint8_t *ptr = (volatile uint8_t *)p;
        (void)*ptr;
    }

#if defined(__aarch64__)
    __asm__ volatile ("dsb ish" ::: "memory");
    __asm__ volatile ("isb" ::: "memory");

    if (do_clean) {
        __asm__ volatile ("dsb ish" ::: "memory");
    }
#elif defined(__riscv)
    __asm__ volatile ("fence rw, rw" ::: "memory");

    if (do_clean) {
        // Platform-specific cache flush required
    }
#else
    __asm__ volatile ("" ::: "memory");
#endif
}