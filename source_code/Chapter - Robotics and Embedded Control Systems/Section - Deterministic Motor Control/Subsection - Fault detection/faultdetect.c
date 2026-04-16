Production-Ready Fault Detection and Control Loop for Embedded Systems (C Language)


#include <stdint.h>
#include <stdbool.h>

// Platform defines provided at build: ARCH_ARM_CORTEX_M or ARCH_RISCV
// Q16.16 fixed-point helpers
static inline int32_t q_mul(int32_t a, int32_t b)
{
    return (int32_t)(((int64_t)a * b) >> 16);
}

static inline int32_t q_sub(int32_t a, int32_t b)
{
    return a - b;
}

static inline int32_t q_abs(int32_t a)
{
    return (a < 0) ? -a : a;
}

// Config parameters (Q16.16)
#define I_MAX_Q16            (int32_t)(30 << 16)         // 30 A
#define V_DELTA_Q16          (int32_t)(0.5 * 65536)      // 0.5 units
#define RESID_THRESH_Q16     (int32_t)(0.1 * 65536)      // residual threshold
#define WINDOW_N             8
#define WINDOW_ALPHA_NUM     3  // degrade if > 3/8

// Persistent state
static int window_count = 0;
static uint8_t fault_level = 0; // 0=normal,1=degraded,2=hard

// Platform hooks (implement per SoC)
void hw_feed_watchdog(void);         // refresh hardware watchdog
int32_t adc_read_current_q16(void);  // signed Q16.16 current
int32_t read_encoder_velocity_q16(void); // Q16.16
int32_t commanded_velocity_q16(void);   // Q16.16
void pwm_disable(void);
void enable_brake(void);
void log_fault_event(const char *s);

// Control-loop call (deterministic, bounded-time)
void control_cycle_fault_check(int32_t y_meas_q16, int32_t pred_y_q16)
{
    // 1) simple overcurrent immediate trip
    int32_t I_q16 = adc_read_current_q16();
    if (q_abs(I_q16) > I_MAX_Q16) {
        fault_level = 2;
        pwm_disable();
        enable_brake();
        log_fault_event("overcurrent");
        return;
    }

    // 2) residual check (scalar example)
    int32_t r_q16 = q_sub(y_meas_q16, pred_y_q16);
    int32_t r_abs_q16 = q_abs(r_q16);
    bool r_bad = (r_abs_q16 > RESID_THRESH_Q16);

    // sliding window logic
    if (r_bad) {
        if (window_count < WINDOW_N) {
            window_count++;
        }
    } else {
        if (window_count > 0) {
            window_count--;
        }
    }

    if (window_count > WINDOW_ALPHA_NUM) {
        // degrade mode: reduce torque limits in higher-level code
        if (fault_level < 1) {
            fault_level = 1;
            log_fault_event("residual_degrade");
        }
    }

    // 3) encoder vs command plausibility
    int32_t v_enc = read_encoder_velocity_q16();
    int32_t v_cmd = commanded_velocity_q16();
    int32_t dv = q_abs(q_sub(v_enc, v_cmd));
    if (dv > V_DELTA_Q16) {
        // escalate quickly
        fault_level = 2;
        pwm_disable();
        enable_brake();
        log_fault_event("encoder_mismatch");
        return;
    }

    // 4) refresh watchdog deterministically at cycle end
    hw_feed_watchdog();
}