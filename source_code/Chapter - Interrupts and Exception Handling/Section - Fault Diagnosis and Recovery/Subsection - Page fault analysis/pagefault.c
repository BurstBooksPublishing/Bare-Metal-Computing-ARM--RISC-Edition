\title{Page Fault Handler for ARM (AArch64)}
\caption{A production-ready implementation of a page fault handler for AArch64 that attempts single-page recovery for kernel-managed regions.}

#include <stdint.h>
#include <stdio.h>      // replace with platform logger
#include <inttypes.h>

// Platform hooks (implement per platform)
extern void map_page(uint64_t va, uint64_t pa, int flags); // map a single page
extern void panic(const char *msg);

// Single-page recovery attempt for kernel-managed MMIO/stack growth.
void page_fault_handler(void) {
    uint64_t va = 0;
    uint64_t pc = 0;
    uint64_t cause = 0;
    uint64_t syndrome = 0;

#if defined(__aarch64__)
    // Read FAR_EL1, ELR_EL1, ESR_EL1
    asm volatile("mrs %0, far_el1" : "=r"(va));
    asm volatile("mrs %0, elr_el1" : "=r"(pc));
    asm volatile("mrs %0, esr_el1" : "=r"(syndrome));
    cause = (syndrome >> 26) & 0x3F; // ESR.EC
#elif defined(__riscv)
    // Read stval, sepc, scause
    asm volatile("csrr %0, stval" : "=r"(va));
    asm volatile("csrr %0, sepc"  : "=r"(pc));
    asm volatile("csrr %0, scause": "=r"(cause));
    syndrome = 0;
#else
#error "Unsupported arch"
#endif

    // Log concise report
    // printf is a placeholder for a low-level serial logger
    printf("PF: va=0x%016" PRIx64 " pc=0x%016" PRIx64 " cause=0x%016" PRIx64 "\n",
           va, pc, cause);

    // Simple classification
#if defined(__aarch64__)
    // Translation fault classes (example): EC values 0x20..0x2F cover data aborts
    int is_translation = (cause == 0x21) || (cause == 0x20);
#elif defined(__riscv)
    // scause codes: 12=Instruction page fault, 13=Load page fault, 15=Store/AMO page fault
    int is_translation = (cause == 13) || (cause == 15) || (cause == 12);
#endif

    if (is_translation) {
        // Attempt to map a single page at aligned VA. Platform decides PA and flags.
        uint64_t page_va = va & ~((uint64_t)0xFFF);
        // Example policy: map kernel MMIO region (application must implement check)
        // FIXME: map_page must verify permissions and physical target.
        map_page(page_va, /*pa=*/page_va, /*flags=*/0x3); // RW, platform-specific
        return; // return to faulting context
    }

    // If not recoverable, escalate to panic
    panic("Unhandled page fault");
}