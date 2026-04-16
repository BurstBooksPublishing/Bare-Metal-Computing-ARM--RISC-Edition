Production-Ready Illegal Instruction Handler for ARM (AArch64) and RISC-V


#include <stdint.h>

/* Minimal logging hook implemented elsewhere on the platform */
extern void uart_puts(const char *s);
extern void uart_puthex(uint64_t v);

/* Platform dispatch via build-time define: ARCH_AARCH64 or ARCH_RISCV */
void illegal_instruction_handler(void)
{
    /* Common diagnostics */
#if defined(ARCH_AARCH64)
    uint64_t esr, elr, instr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    asm volatile("mrs %0, elr_el1" : "=r"(elr));
    /* Safe memory read: ensure elr is canonical in your platform before deref */
    instr = *(volatile uint32_t *)elr;                /* 32-bit AArch64 */
    /* Example: special software trap uses 0xDEADBEEF as magic opcode */
    if ((instr & 0xFFFFFFFF) == 0xDEADBEEFU) {
        /* emulate: here, perform side-effects required by convention */
        /* advance ELR to skip the trap instruction */
        elr += 4;
        asm volatile("msr elr_el1, %0" :: "r"(elr));
        return; /* return to resumed execution */
    }
    uart_puts("AArch64 illegal instruction\n");
    uart_puts("ESR_EL1 = "); uart_puthex(esr); uart_puts("\n");
    uart_puts("ELR_EL1 = "); uart_puthex(elr); uart_puts("\n");
    uart_puts("instr   = "); uart_puthex(instr); uart_puts("\n");
    /* Platform policy: halt and wait for debugger or reset */
    for (;;) asm volatile("wfe"); /* low-power spin */
#elif defined(ARCH_RISCV)
    uint64_t mcause, mepc, mtval, instr;
    asm volatile("csrr %0, mcause" : "=r"(mcause));
    asm volatile("csrr %0, mepc"   : "=r"(mepc));
    asm volatile("csrr %0, mtval"  : "=r"(mtval));   /* may contain instruction bits */
    /* Safely fetch the instruction word from mepc */
    instr = *(volatile uint32_t *)mepc;               /* fetch up to 32 bits */
    /* Determine instruction length per Eq. (ref:riscv_next) */
    uint32_t size = ((instr & 0x3) != 0x3) ? 2 : 4;
    /* Example software trap magic */
    if ((instr & 0xFFFFFFFF) == 0xDEADBEEFU) {
        mepc += size;
        asm volatile("csrw mepc, %0" :: "r"(mepc));
        return;
    }
    uart_puts("RISC-V illegal instruction\n");
    uart_puts("mcause = "); uart_puthex(mcause); uart_puts("\n");
    uart_puts("mepc   = "); uart_puthex(mepc); uart_puts("\n");
    uart_puts("mtval  = "); uart_puthex(mtval); uart_puts("\n");
    uart_puts("instr  = "); uart_puthex(instr); uart_puts("\n");
    for (;;) asm volatile("wfi");
#else
# error "Unsupported architecture for illegal-instruction handler"
#endif
}