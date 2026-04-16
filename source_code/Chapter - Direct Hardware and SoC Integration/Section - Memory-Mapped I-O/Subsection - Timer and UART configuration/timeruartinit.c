Low-Level Timer and UART Driver for RISC-V and ARM (AArch64)


#include <stdint.h>

#ifndef TIMER_BASE
#error "Define TIMER_BASE (CLINT base) at build"
#endif
#ifndef UART_BASE
#error "Define UART_BASE (PL011 base) at build"
#endif

static inline void mmio_write32(uintptr_t addr, uint32_t val) {
    volatile uint32_t *p = (volatile uint32_t *)addr;
    *p = val;
    __asm__ volatile ("" ::: "memory"); /* compiler barrier */
}

static inline uint32_t mmio_read32(uintptr_t addr) {
    volatile uint32_t *p = (volatile uint32_t *)addr;
    uint32_t v = *p;
    __asm__ volatile ("" ::: "memory");
    return v;
}

#if defined(__riscv)
static inline void cpu_barrier(void) {
    __asm__ volatile("fence rw, rw" ::: "memory");
}
#elif defined(__aarch64__)
static inline void cpu_barrier(void) {
    __asm__ volatile("dsb sy" ::: "memory");
}
#else
static inline void cpu_barrier(void) {
    __asm__ volatile("" ::: "memory");
}
#endif

/* CLINT offsets (platform should define or match spec) */
#define CLINT_MTIME             (TIMER_BASE + 0xBFF8)
#define CLINT_MTIMECMP(hart)    (TIMER_BASE + 0x4000 + 8 * (hart))

/* Safe 64-bit write sequence for mtimecmp on systems with 32-bit accesses */
static void clint_set_mtimecmp(uint32_t hart, uint64_t cmp) {
    uintptr_t addr = CLINT_MTIMECMP(hart);
    uint32_t hi = (uint32_t)(cmp >> 32);
    uint32_t lo = (uint32_t)(cmp & 0xFFFFFFFFU);

    /* Prevent accidental early match: set high to 0xFFFFFFFF, then low, then real high */
    mmio_write32(addr + 4, 0xFFFFFFFFU);
    cpu_barrier();
    mmio_write32(addr + 0, lo);
    cpu_barrier();
    mmio_write32(addr + 4, hi);
    cpu_barrier();
}

/* PL011 UART registers (offsets) */
#define UART_DR     (UART_BASE + 0x00)
#define UART_FR     (UART_BASE + 0x18)
#define UART_IBRD   (UART_BASE + 0x24)
#define UART_FBRD   (UART_BASE + 0x28)
#define UART_LCRH   (UART_BASE + 0x2C)
#define UART_CR     (UART_BASE + 0x30)
#define UART_IMSC   (UART_BASE + 0x38)
#define UART_ICR    (UART_BASE + 0x44)

/* Initialize PL011 UART: baud (e.g., 115200), 8N1, FIFOs enabled */
void uart_init(uint32_t uartclk_hz, uint32_t baud) {
    /* Disable UART */
    mmio_write32(UART_CR, 0);
    cpu_barrier();

    /* Clear interrupts */
    mmio_write32(UART_ICR, 0x7FF);
    cpu_barrier();

    /* Compute integer and fractional divisors per equation */
    double bauddiv = (double)uartclk_hz / (16.0 * (double)baud);
    uint32_t ibrd = (uint32_t)bauddiv;
    uint32_t fbrd = (uint32_t)((bauddiv - (double)ibrd) * 64.0 + 0.5);

    mmio_write32(UART_IBRD, ibrd);
    mmio_write32(UART_FBRD, fbrd);

    /* 8-bit, FIFO enabled (WLEN=3), no parity, one stop bit */
    mmio_write32(UART_LCRH, (0x3 << 5) | (1 << 4)); /* WLEN=3, FEN=1 */
    cpu_barrier();

    /* Mask/unmask interrupts as needed; here we enable RX interrupt in peripheral */
    mmio_write32(UART_IMSC, (1 << 4)); /* RXIM */
    cpu_barrier();

    /* Enable UART, TX and RX */
    mmio_write32(UART_CR, (1 << 0) | (1 << 8) | (1 << 9)); /* UARTEN | TXE | RXE */
    cpu_barrier();
}

/* Example: configure a periodic timer interrupt at f_irq Hz for hart 0 */
void timer_init_periodic(uint32_t hart, uint64_t clk_hz, uint64_t f_irq) {
    uint64_t ticks = (uint64_t)(clk_hz / f_irq); /* integer division */
    uint64_t now = ((uint64_t)mmio_read32(CLINT_MTIME + 4) << 32) | mmio_read32(CLINT_MTIME);
    clint_set_mtimecmp(hart, now + ticks);
    /* Enable machine timer interrupt (platform-specific CSR write required) */
}