ARM (AArch64) Production-Ready DMA and Accelerator Streaming Driver


#include <stdint.h>
#include <stddef.h>

#define DMA_BASE       ((volatile uint32_t*)0x40000000) // platform-specific
#define ACCEL_BASE     ((volatile uint32_t*)0x50000000)
#define BUF_BYTES      4096      // aligned size
#define RING_SIZE      2

typedef struct {
    uint64_t phys_addr;
    uint32_t length;
    uint32_t control;
} dma_desc_t;

static dma_desc_t __attribute__((aligned(64))) ring[RING_SIZE];
static uint8_t __attribute__((aligned(64))) buffers[RING_SIZE][BUF_BYTES];
static volatile int head = 0;        // producer index
static volatile int tail = 0;        // consumer index

// Platform-specific cache maintenance (must be implemented per SoC).
extern void arch_clean_dcache(void *addr, size_t len);    // CPU->device
extern void arch_invalidate_dcache(void *addr, size_t len); // device->CPU

// Start DMA transfer for descriptor index i (memory-mapped DMA controller).
static void dma_start_desc(int i) {
    // write source phys, dst phys, len, start, depending on DMA
    DMA_BASE[0] = (uint32_t)(ring[i].phys_addr & 0xFFFFFFFF);
    DMA_BASE[1] = (uint32_t)(ring[i].phys_addr >> 32);
    DMA_BASE[2] = ring[i].length;
    DMA_BASE[3] = 1; // start bit (platform-specific)
}

// Accelerator kick with physical pointer to input buffer.
static void accel_kick(uint64_t phys_input) {
    ACCEL_BASE[0] = (uint32_t)(phys_input & 0xFFFFFFFF);
    ACCEL_BASE[1] = (uint32_t)(phys_input >> 32);
    ACCEL_BASE[2] = 1; // start
}

// DMA completion interrupt handler (bound to NVIC/PLIC). Runs in interrupt context.
void dma_irq_handler(void) {
    // acknowledge DMA interrupt (platform-specific)
    DMA_BASE[4] = 1;
    // advance consumer index
    tail = (tail + 1) % RING_SIZE;
    // signal higher-level that buffer tail is free (could set flag or wake task)
}

// Main producer loop (sensor + preprocessing).
void streaming_main_loop(void) {
    while (1) {
        int idx = head;
        uint8_t *buf = buffers[idx];

        // 1) Fill buffer from sensor and preprocess in-place
        sensor_read_and_preprocess(buf, BUF_BYTES); // user-implemented

        // 2) Cache clean before DMA / accelerator read
        arch_clean_dcache(buf, BUF_BYTES);

        // 3) Populate descriptor and start DMA or kick accelerator
        ring[idx].phys_addr = virt_to_phys(buf); // platform mapping
        ring[idx].length = BUF_BYTES;
        ring[idx].control = 0;
        dma_start_desc(idx);       // or accel_kick(ring[idx].phys_addr)
        accel_kick(ring[idx].phys_addr);

        // 4) Advance producer index; if ring full, wait for tail to advance
        head = (head + 1) % RING_SIZE;
        while (((head + RING_SIZE - tail) % RING_SIZE) == 0) {
            // busy-wait, or sleep with WFI; monitors should ensure bounded wait.
            __asm__ volatile ("wfi");
        }
    }
}