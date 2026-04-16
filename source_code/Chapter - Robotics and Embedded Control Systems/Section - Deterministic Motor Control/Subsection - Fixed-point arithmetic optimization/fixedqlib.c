\begin{titlepage}

Fixed-Point Q1.31 Arithmetic Library in C for ARM (AArch64) and RISC-V

\end{titlepage}

#include <stdint.h>
#include <limits.h>

// Q1.31 typedef
typedef int32_t q31_t;
typedef int64_t q63_t;

static inline q31_t q31_from_float(float f) { // convert float->Q1.31
    q63_t v = (q63_t)(f * (float)(1U << 31));
    if (v > INT32_MAX) return INT32_MAX;
    if (v < INT32_MIN) return INT32_MIN;
    return (q31_t)v;
}

static inline float q31_to_float(q31_t a) { return (float)a / (float)(1U << 31); }

// Multiply two Q1.31 numbers with rounding and saturation
static inline q31_t q31_mul(q31_t a, q31_t b) {
    q63_t prod = (q63_t)a * (q63_t)b;           // full 64-bit product
    // rounding: add 1<<(30) before shift (for symmetric rounding)
    const q63_t round = (q63_t)1 << 30;
    prod += prod >= 0 ? round : -round;
    q63_t shifted = prod >> 31;                 // normalize back to Q1.31
    if (shifted > INT32_MAX) return INT32_MAX;
    if (shifted < INT32_MIN) return INT32_MIN;
    return (q31_t)shifted;
}

// Multiply-accumulate: acc += a * b (acc is Q1.31)
static inline q31_t q31_mac(q31_t acc, q31_t a, q31_t b) {
    q63_t prod = (q63_t)a * (q63_t)b;
    const q63_t round = (q63_t)1 << 30;
    prod += prod >= 0 ? round : -round;
    q63_t shifted = prod >> 31;
    q63_t res = (q63_t)acc + shifted;
    if (res > INT32_MAX) return INT32_MAX;
    if (res < INT32_MIN) return INT32_MIN;
    return (q31_t)res;
}

// Example: fixed-point PID inner update (all gains in Q1.31)
static inline q31_t pid_update(q31_t setpoint, q31_t measurement,
                               q31_t *state_integral, q31_t Kp, q31_t Ki, q31_t Kd,
                               q31_t derivative) {
    q31_t error = setpoint - measurement;
    // Proportional term
    q31_t P = q31_mul(Kp, error);
    // Integrator state update (simple Euler)
    *state_integral = q31_mac(*state_integral, Ki, error);
    q31_t I = *state_integral;
    // Derivative term assumed precomputed in Q1.31
    q31_t D = q31_mul(Kd, derivative);
    // Sum P + I + D with saturation
    q31_t out = q31_mac(P, I, 1); // P + I
    out = q31_mac(out, D, 1);     // + D
    return out;
}