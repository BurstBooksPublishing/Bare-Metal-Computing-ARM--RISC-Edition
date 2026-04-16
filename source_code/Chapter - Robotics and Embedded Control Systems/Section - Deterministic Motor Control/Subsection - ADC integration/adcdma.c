Production-Ready ADC DMA Circular Buffer Handler for ARM (AArch64)


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SAMPLES_PER_BATCH    128         // power-of-two preferred
static volatile uint16_t adc_buffer[SAMPLES_PER_BATCH] __attribute__((aligned(32)));
static volatile bool batch_ready = false;

// Platform abstraction points (implement per SoC)
extern void timer_config_trgo(uint32_t pwm_freq_hz, uint32_t trigger_phase_us); // sets TRGO
extern void adc_hw_config(unsigned sample_time_cycles, unsigned resolution_bits);
extern void dma_start_circular(void *dst, size_t len_bytes);
extern void cache_invalidate(void *addr, size_t len);
extern void cache_clean(void *addr, size_t len);
extern void nvic_set_priority(unsigned irq, unsigned prio);
extern void nvic_enable_irq(unsigned irq);

// DMA half/full callbacks called in IRQ context (minimal work)
void dma_half_transfer_irq(void)
{
    batch_ready = true; /* process half buffer index 0..S/2-1 */
}

void dma_full_transfer_irq(void)
{
    batch_ready = true; /* process half buffer index S/2..S-1 */
}

// Application call at startup (blocking init)
void adc_dma_start(uint32_t pwm_freq_hz, uint32_t trigger_phase_us)
{
    // Compute ADC timing externally using Eq. (1); choose sample_time_cycles accordingly.
    adc_hw_config(/*sample_time_cycles=*/3, /*resolution_bits=*/12);

    // Ensure DMA buffer is cache aligned and clean before peripheral writes.
    cache_clean((void*)adc_buffer, sizeof(adc_buffer));

    // Configure timer to emit TRGO synchronized to PWM center.
    timer_config_trgo(pwm_freq_hz, trigger_phase_us);

    // Start circular DMA to fill adc_buffer on each ADC conversion complete.
    dma_start_circular((void*)adc_buffer, sizeof(adc_buffer));

    // Configure and enable DMA interrupts with deterministic priority.
    nvic_set_priority(/*ADC_DMA_IRQ=*/10, /*prio=*/2);
    nvic_enable_irq(/*ADC_DMA_IRQ=*/10);
}

// Called from main control loop at deterministic times or low-priority task.
void process_adc_batches(void)
{
    if (!batch_ready) {
        return;
    }

    // Invalidate cache so CPU reads latest DMA writes.
    cache_invalidate((void*)adc_buffer, sizeof(adc_buffer));

    // Process half-buffers in fixed-order to preserve determinism.
    // Example: average S/2 samples for current estimate.
    uint32_t sum = 0;
    for (size_t i = 0; i < SAMPLES_PER_BATCH / 2; ++i) {
        sum += adc_buffer[i];
    }

    uint16_t avg = (uint16_t)(sum / (SAMPLES_PER_BATCH / 2));
    // Use avg in deterministic control update.
    batch_ready = false;
}