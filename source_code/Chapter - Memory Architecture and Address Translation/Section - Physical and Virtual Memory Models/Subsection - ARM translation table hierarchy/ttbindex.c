#include <stdint.h>
#include <stddef.h>

int compute_tt_indices(uint64_t va, int va_bits, int granule_bits,
                       int *indices, size_t max_levels)
{
    const int B = 9;
    if (va_bits <= granule_bits) return 0;
    int L = (va_bits - granule_bits + B - 1) / B;
    if ((size_t)L > max_levels) return -1;
    int shift = granule_bits + B * (L - 1);
    for (int i = 0; i < L; ++i) {
        indices[i] = (va >> shift) & ((1U << B) - 1);
        shift -= B;
    }
    return L;
}