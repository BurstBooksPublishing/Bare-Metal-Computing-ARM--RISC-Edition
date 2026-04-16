High-Quality C Implementation for Depthwise Convolution on ARM (AArch64) and RISC-V


#include <stdint.h>
#include <stddef.h>

// Place static weights into low-latency TCM section (linker must define .tcm).
// Align to 64 bytes for cache-line friendly access.
static const int8_t weights[/* C * 9 */] __attribute__((section(".tcm")))
    __attribute__((aligned(64))) = {
    /* model-specific quantized weights go here; filled at build time */
};

// Platform fence macros for deterministic memory ordering.
#if defined(__aarch64__)
#define MEM_FENCE() __asm__ volatile("dmb ish" ::: "memory")
#elif defined(__riscv)
#define MEM_FENCE() __asm__ volatile("fence rw, rw" ::: "memory")
#else
#define MEM_FENCE() ((void)0)
#endif

// Single-threaded depthwise 3x3 convolution.
// input: H x W x C, row-major per-channel layout, padded appropriately.
// output: (H-2) x (W-2) x C
void depthwise_conv3x3_q8(const int8_t *input, int H, int W, int C,
                          int8_t *output, const int32_t *bias,
                          int32_t input_zero_point, int32_t out_shift)
{
    const int outH = H - 2;
    const int outW = W - 2;
    const int inRowStride = W * C;
    const int outRowStride = outW * C;

    // Prefetch heuristic: warm first rows of weights/data into caches/TCM.
    MEM_FENCE(); // ensure weights are visible before compute

    for (int r = 0; r < outH; ++r) {
        for (int c = 0; c < outW; ++c) {
            // Process channels in scalar loop; vectorize at compiler level or
            // replace with intrinsics for AArch64 NEON / RVV for hot paths.
            for (int ch = 0; ch < C; ++ch) {
                int32_t acc = bias ? bias[ch] : 0;
                // 3x3 window
                for (int kr = 0; kr < 3; ++kr) {
                    const int inBase = (r + kr) * inRowStride + (c * C) + ch;
                    // Prefetch next row to reduce load jitter
                    __builtin_prefetch(&input[inBase + C], 0, 1);
                    for (int kc = 0; kc < 3; ++kc) {
                        int8_t in_val = input[inBase + kc * C];
                        int8_t w = weights[(ch * 9) + kr * 3 + kc];
                        acc += (int32_t)(in_val - input_zero_point) * (int32_t)w;
                    }
                }
                // requantize: arithmetic right shift with rounding
                if (out_shift >= 0) {
                    acc = (acc + (1 << (out_shift - 1))) >> out_shift;
                }
                // saturate to int8 range
                if (acc > 127) {
                    acc = 127;
                }
                if (acc < -128) {
                    acc = -128;
                }
                output[r * outRowStride + c * C + ch] = (int8_t)acc;
            }
        }
    }
    MEM_FENCE(); // ensure outputs are committed before returning
}