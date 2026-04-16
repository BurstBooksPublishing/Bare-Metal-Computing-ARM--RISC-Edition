ARM AArch64 Exception Handler Code


#include <stdint.h>

#define UART_TX  ((volatile uint32_t *)0x09000000)  // Platform-specific UART TX register

static void uart_putc(char c) {
    *UART_TX = (uint32_t)c;
}

static void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

static void uart_puthex(uint64_t v) {
    const char *hex = "0123456789abcdef";
    uart_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uart_putc(hex[(v >> i) & 0xf]);
    }
}

void early_fault_handler(uint64_t esr, uint64_t far, uint64_t elr) {
    uart_puts("\r\n[FAULT] ESR=");
    uart_puthex(esr);
    uart_puts(" FAR=");
    uart_puthex(far);
    uart_puts(" ELR=");
    uart_puthex(elr);
    uart_puts("\r\n");
    
    // Infinite loop with wait-for-event instruction
    while (1) {
        __asm__ volatile("wfe");
    }
}