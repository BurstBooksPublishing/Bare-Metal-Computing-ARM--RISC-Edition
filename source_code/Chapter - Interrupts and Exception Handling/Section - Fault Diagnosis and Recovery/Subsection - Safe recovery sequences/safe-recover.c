\title{AArch64 Illegal Instruction Handler}

#include <stdint.h>

extern int try_emulate_instruction(void *pc, uint64_t *regs);
extern int determine_instr_length(void *pc);
extern void record_crash_info_aarch64(uint64_t esr, uint64_t *regs);
extern void system_halt(void);

void handle_illegal_aarch64(uint64_t *regs) {
    uint64_t esr;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));              // read syndrome
    uint32_t ec = (esr >> 26) & 0x3F;                         // exception class
    if (ec == 0x0A) {                                         // undefined instruction
        uint64_t fault_pc;
        asm volatile("mrs %0, elr_el1" : "=r"(fault_pc));     // original return PC
        if (try_emulate_instruction((void *)fault_pc, regs)) {
            uint64_t new_pc = fault_pc + determine_instr_length((void *)fault_pc);
            asm volatile("msr elr_el1, %0" :: "r"(new_pc));
            asm volatile("eret");                             // return to lower EL
            __builtin_unreachable();
        }
    }
    record_crash_info_aarch64(esr, regs);
    system_halt();
}


\title{RISC-V Page Fault Handler (RV64)}

#include <stdint.h>

#define PTE_R 0x01
#define PTE_W 0x02
#define PTE_U 0x10

extern void *allocate_phys_page(void);
extern void free_phys_page(void *page);
extern int map_virtual_page(void *vaddr, void *paddr, int flags);
extern void record_crash_info_rv64(uint64_t scause, uint64_t stval, uint64_t *regs);
extern void system_halt(void);

void handle_page_fault_rv64(uint64_t *regs) {
    uint64_t scause, stval;
    asm volatile("csrr %0, scause" : "=r"(scause));           // cause code
    asm volatile("csrr %0, stval"  : "=r"(stval));            // faulting virtual addr
    if ((scause & 0xF) == 12 || (scause & 0xF) == 13) {      // load/store page fault
        void *phys = allocate_phys_page();
        if (phys) {
            if (map_virtual_page((void *)stval, phys, PTE_R | PTE_W | PTE_U)) {
                asm volatile("sfence.vma zero, zero" ::: "memory");
                asm volatile("sret");                         // return to supervisor
                __builtin_unreachable();
            } else {
                free_phys_page(phys);
            }
        }
    }
    record_crash_info_rv64(scause, stval, regs);
    system_halt();
}