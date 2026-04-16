Thermal Governor Implementation in C for ARM (AArch64)


#include <stdint.h>

// Platform-specific MMIO addresses (example placeholders)
#define THERMAL_REG    ((volatile uint32_t*)0x40030000) // read sensor
#define NPU_CTRL_REG   ((volatile uint32_t*)0x40040000) // NPU power/enable
#define CPU_FREQ_REG   ((volatile uint32_t*)0x40050000) // CPU freq select
#define PMIC_I2C_ADDR  0x2D                              // PMIC I2C addr

// Simple low-pass filter state
static float temp_filtered = 0.0f;

static float read_temperature(void) {
    uint32_t raw = *THERMAL_REG;                      // raw sensor units
    float degC = (raw * 0.1f) - 50.0f;                // example calibration
    // first-order LPF
    const float alpha = 0.2f;
    temp_filtered = alpha * degC + (1.0f - alpha) * temp_filtered;
    return temp_filtered;
}

static void set_cpu_freq(uint8_t level) {
    *CPU_FREQ_REG = level;                            // write clock divider index
}

static void npu_set_concurrency(uint8_t c) {
    *NPU_CTRL_REG = (uint32_t)c;                      // accelerator concurrency
}

void thermal_governor_tick(void) {
    float T = read_temperature();
    const float T_warn = 75.0f, T_crit = 90.0f;
    if (T > T_crit) {
        // aggressive cut: stop NPU and reduce CPU to minimum
        npu_set_concurrency(0);
        set_cpu_freq(0);
    } else if (T > T_warn) {
        // moderate reduction: reduce NPU concurrency and CPU freq
        npu_set_concurrency(1);                       // single lane
        set_cpu_freq(1);                              // reduced freq
    } else {
        // normal operation: dynamic based on workload (external hint)
        extern uint8_t workload_level(void);          // 0..3
        uint8_t w = workload_level();
        npu_set_concurrency(w > 2 ? 2 : w);
        set_cpu_freq(w);                              // map workload->freq
    }
    // optional: write PMIC via I2C if voltage change needed (omitted)
}