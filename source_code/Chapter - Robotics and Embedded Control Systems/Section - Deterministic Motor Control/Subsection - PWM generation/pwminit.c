\begin{figure}[ht]
\centering
\caption{Production-Ready PWM Initialization and Control for ARM (AArch64) Timer Peripherals}
\end{figure}

#include <stdint.h>
#include <stdbool.h>

#define ARR_MAX 0xFFFFU

/* Abstract timer register layout common to many MCUs (map to real base). */
typedef struct {
    volatile uint32_t CR1;    /* control */
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;   /* DMA/IRQ enable */
    volatile uint32_t SR;
    volatile uint32_t EGR;    /* event generation */
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;    /* counter */
    volatile uint32_t PSC;    /* prescaler */
    volatile uint32_t ARR;    /* auto-reload */
    volatile uint32_t RCR;
    volatile uint32_t CCR1;   /* capture/compare */
    /* ... other CCRs and registers ... */
} TimerRegs;

/* Platform must define TIMER as a pointer to real timer base */
extern TimerRegs * const TIMER;

/* Initialize PWM: timer_clk_hz must be known from clock tree */
static inline void pwm_init(TimerRegs *T, uint32_t timer_clk_hz,
                            uint32_t pwm_hz, uint16_t duty_permille,
                            bool center_aligned)
{
    uint32_t prescaler = 1;
    if (pwm_hz == 0) {
        return;
    }

    /* Compute minimal prescaler so ARR fits */
    prescaler = (timer_clk_hz / pwm_hz) / (ARR_MAX + 1U) + 1U;
    uint32_t arr = (timer_clk_hz / (prescaler * pwm_hz));
    if (arr == 0) {
        arr = 1;
    }
    arr = arr - 1U;
    if (arr > ARR_MAX) {
        arr = ARR_MAX;
    }

    /* Program PSC and ARR into shadow (PSC often loads immediately on UG). */
    T->PSC = prescaler - 1U;  /* many timers store PSC-1 */
    T->ARR = arr;

    /* Compute CCR for desired duty (permille -> fraction) */
    uint32_t ccr = ((uint32_t)duty_permille * (arr + 1U)) / 1000U;
    if (ccr > arr) {
        ccr = arr;
    }

    /* Configure channel for PWM mode and enable compare output:
       - set CCMR to PWM mode (platform-specific bits)
       - set CCER to enable output
       These bit patterns must be set per SoC. */
    /* Example platform-specific placeholders (replace with real bitfields): */
    // T->CCMR1 = (PWM_MODE_BITMASK | OUTPUT_POLARITY);
    // T->CCER = CC1E; /* enable channel 1 output */

    /* Write CCR into shadow */
    T->CCR1 = ccr;

    /* Optionally select center-aligned (CMS bits in CR1) */
    if (center_aligned) {
        /* Set CMS bits to center-aligned mode (implementation-specific) */
        // T->CR1 |= (CMS_BITS_CENTER_ALIGNED);
    } else {
        // T->CR1 &= ~(CMS_BITS_MASK);
    }

    /* Enable update DMA/IRQ if using DMA for CCR streaming (optional) */
    // T->DIER |= TIM_DIER_UDE;

    /* Generate an update to load PSC immediately if required */
    T->EGR = 1U; /* UG: update generation */

    /* Enable counter (CEN bit) */
    T->CR1 |= 1U;
}

/* Atomically update duty safely (writes to shadow CCR). */
static inline void pwm_set_duty(TimerRegs *T, uint16_t duty_permille)
{
    uint32_t arr = T->ARR;
    uint32_t ccr = ((uint32_t)duty_permille * (arr + 1U)) / 1000U;
    if (ccr > arr) {
        ccr = arr;
    }
    T->CCR1 = ccr;
    /* CCR shadow will transfer on next update (overflow / UEV) */
}