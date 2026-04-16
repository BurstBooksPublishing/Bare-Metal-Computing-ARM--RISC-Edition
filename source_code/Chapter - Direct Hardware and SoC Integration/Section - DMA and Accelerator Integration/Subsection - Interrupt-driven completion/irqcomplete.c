Generic DMA Driver with Interrupt Handling (AArch64/RISC-V)


#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>

/* Platform hooks: implement these for your SoC. */
extern void platform_cache_clean_range(void *addr, size_t len); // device-visible
extern void platform_cache_invalidate_range(void *addr, size_t len); // cpu-visible
extern void platform_irq_eoi(uint32_t id); // GIC EOI or PLIC complete
extern uint32_t platform_irq_claim(void); // GIC IAR or PLIC claim
extern void platform_irq_enable(uint32_t id);
extern void platform_memory_barrier(void); // dmb ish or fence rw, rw

static atomic_uint_fast32_t dma_completion_flag = 0; // 0 = not done, 1 = done

/* Initiate DMA transfer: ensure descriptor is cleaned before start. */
void dma_start(void *desc, size_t desc_len, uint32_t irq_id)
{
    platform_cache_clean_range(desc, desc_len); // make descriptor visible to device
    platform_memory_barrier();                  // ordering: device sees descriptor
    platform_irq_enable(irq_id);                // enable interrupt at controller
    // write doorbell to device (MMIO) to start DMA - platform specific
}

/* Generic IRQ handler registered with vector table. */
void dma_irq_handler(void)
{
    uint32_t claim = platform_irq_claim(); // claim interrupt id
    // read device status register (MMIO) to determine channel and status
    // uint32_t status = mmio_read(DMA_STATUS_BASE + offset);

    // clear device completion bit/acknowledge device (MMIO)
    // mmio_write(DMA_CTRL_BASE + offset, CLEAR_COMPLETE);

    // ensure device-written payload and descriptors are visible to CPU
    platform_cache_invalidate_range((void*)/*desc_addr*/, /*desc_len*/);

    // publish completion to waiting code
    atomic_store_explicit(&dma_completion_flag, 1, memory_order_release);

    platform_memory_barrier(); // ensure store is globally visible before EOI
    platform_irq_eoi(claim);  // signal interrupt controller that handling is done
}

/* Wait for completion (simple, low-power friendly). */
void wait_for_dma_completion(void)
{
    while (atomic_load_explicit(&dma_completion_flag, memory_order_acquire) == 0) {
        // On ARM: wfi will return on IRQ; on RISC-V: wfi similarly
        __asm__ volatile("wfi"); // low-power wait until interrupt wakes CPU
    }
    // Process completed buffer now; caches already invalidated in handler.
    atomic_store_explicit(&dma_completion_flag, 0, memory_order_release);
}