\title{RISC-V Trap Handler Implementation}
\caption{Minimal trap handler for RV64 systems with inline assembly for CSR access and basic exception dispatch.}

#include <stdint.h>
#include <stdbool.h>

// Read CSR 'mcause' into uint64_t safely for either XLEN.
static inline uint64_t read_mcause(void) {
    uint64_t v;
    asm volatile("csrr %0, mcause" : "=r"(v));
    return v;
}

static inline uint64_t read_mtval(void) {
    uint64_t v;
    asm volatile("csrr %0, mtval" : "=r"(v));
    return v;
}

static inline uint64_t read_mepc(void) {
    uint64_t v;
    asm volatile("csrr %0, mepc" : "=r"(v));
    return v;
}

// Decode and dispatch minimal handlers.
void handle_trap(void) {
    uint64_t mc = read_mcause();
    // Determine XLEN at compile time via pointer width.
    const unsigned XLEN = sizeof(void *) * 8;
    const uint64_t MSB = (uint64_t)1 << (XLEN - 1);
    bool is_interrupt = (mc & MSB) != 0;
    uint64_t cause = mc & (MSB - 1);

    if (is_interrupt) {
        // Minimal interrupt handling; extend as needed.
        if (cause == 7) {
            // machine timer interrupt number differs by spec version
            // acknowledge timer, schedule next tick
        } else {
            // platform-specific external interrupt
        }
        return; // return to trap entry caller to perform mret/sret
    }

    // Exception handling
    switch (cause) {
        case 2: { // Illegal instruction
            uint64_t pc = read_mepc();
            // Option: emulate compressed/extension instruction or kill.
            // Example: advance PC to next instruction to skip bad instruction.
            pc += 4; // conservative: assume 32-bit instruction. For compress, detect actual size.
            asm volatile("csrw mepc, %0" :: "r"(pc));
            break;
        }
        case 13: { // Load page fault
            uint64_t fault_vaddr = read_mtval();
            // Attempt to map page or report fault.
            // map_page_for(fault_vaddr); // implement platform mapping
            // If mapped, return to retry by leaving mepc unchanged.
            break;
        }
        default:
            // Structured crash report: bundle mcause, mtval, mepc to console or memory.
            uint64_t pc = read_mepc();
            uint64_t mtv = read_mtval();
            // report_fault(cause, pc, mtv); // implement reporting sink
            // Halt or reset depending on system policy.
            for (;;)
                asm volatile("wfi");
    }
}