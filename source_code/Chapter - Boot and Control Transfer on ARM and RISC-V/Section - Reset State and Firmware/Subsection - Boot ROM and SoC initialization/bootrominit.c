Boot ROM Implementation for ARM AArch64 and RISC-V Platforms


#include <stdint.h>
#include <stddef.h>

#define MMIO32(addr) (*(volatile uint32_t *)(addr))

/* Replace these with SoC-specific addresses */
enum {
    REG_PLL_CTRL      = 0x40000000,
    REG_PLL_STATUS    = 0x40000004,
    REG_UART_CTRL     = 0x40010000,
    REG_UART_BAUD     = 0x40010004,
    REG_DDR_CTRL      = 0x40020000,
    REG_FLASH_CTRL    = 0x40030000,
    REG_MAILBOX_BASE  = 0x40040000, /* per-core mailboxes */
};

static inline void mmio_write(uint32_t addr, uint32_t val) {
    MMIO32(addr) = val;
}

static inline uint32_t mmio_read(uint32_t addr) {
    return MMIO32(addr);
}

/* Busy-wait with timeout; returns 0 on success */
static int wait_for_mask(uint32_t addr, uint32_t mask, uint32_t timeout) {
    while (timeout--) {
        if (mmio_read(addr) & mask) {
            return 0;
        }
    }
    return -1;
}

/* Minimal UART init for diagnostics */
static void uart_init(void) {
    mmio_write(REG_UART_BAUD, 115200);
    mmio_write(REG_UART_CTRL, 0x1); /* enable */
}

/* Simple image loader: read from flash into DRAM */
extern void ddr_init(void);               /* platform-specific DDR bring-up */
extern int flash_read(uintptr_t off, void *dst, size_t len); /* driver */

static int load_next_stage(uintptr_t dst, uintptr_t src_off, size_t len) {
    /* assume DDR already initialized */
    return flash_read(src_off, (void *)dst, len);
}

/* Authenticate image -- platform implements verifier */
extern int verify_image(void *addr, size_t len);

/* Release secondary cores by writing entry to per-core mailbox */
static void release_secondary_cpus(uintptr_t entry_addr, int cpu_count) {
    for (int i = 1; i < cpu_count; ++i) {
        mmio_write(REG_MAILBOX_BASE + i * 4, (uint32_t)entry_addr); /* write entry */
        /* optional interrupt to wake core if required */
    }
}

/* Boot ROM main */
void bootrom_main(void) {
    uart_init(); /* diagnostics early */

    /* Configure PLL: platform-specific values */
    mmio_write(REG_PLL_CTRL, 0x12345);                     /* set N/M/P */
    if (wait_for_mask(REG_PLL_STATUS, 0x1, 1000000) != 0) { /* wait lock */
        for (;;); /* fatal: hang after diagnostic */
    }

    ddr_init(); /* strong platform init, must enable DRAM */

    /* Load and verify FSBL */
    const uintptr_t fsbl_dest = 0x80000000; /* DRAM base */
    const size_t fsbl_size = 0x20000;
    if (load_next_stage(fsbl_dest, 0x100000, fsbl_size) != 0) {
        for (;;);
    }
    if (verify_image((void *)fsbl_dest, fsbl_size) != 0) {
        for (;;);
    }

    /* Release secondaries and jump to FSBL */
    release_secondary_cpus(fsbl_dest, 4);

    /* Branch to FSBL (set up minimal registers per ABI) */
#if defined(__aarch64__)
    register void (*entry)(void) = (void (*)(void))fsbl_dest;
    asm volatile("mov x0, #0\n" "br %0\n" : : "r"(entry) : "x0");
#elif defined(__riscv)
    /* a0=boot_device_id per platform convention */
    register void (*entry)(void) = (void (*)(void))fsbl_dest;
    asm volatile("mv a0, zero\n" "jr %0\n" : : "r"(entry) : "a0");
#else
    ((void (*)(void))fsbl_dest)();
#endif
    for (;;);
}