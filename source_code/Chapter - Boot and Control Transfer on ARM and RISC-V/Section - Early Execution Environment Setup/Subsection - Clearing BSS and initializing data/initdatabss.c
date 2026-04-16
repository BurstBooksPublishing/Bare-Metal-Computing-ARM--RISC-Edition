ARM (AArch64) Runtime Initialization Code


#include <stdint.h>
#include <stddef.h>

/* Linker-provided symbols (addresses only). */
extern uint8_t _sidata[]; /* source (ROM) for .data */
extern uint8_t _sdata[];  /* start of .data in RAM */
extern uint8_t _edata[];  /* end of .data in RAM */
extern uint8_t _sbss[];   /* start of .bss in RAM */
extern uint8_t _ebss[];   /* end of .bss in RAM */

void init_data_bss(void) {
    /* Copy .data: handle head bytes, word-copy, then tail bytes. */
    uintptr_t *wsrc = (uintptr_t*) (((uintptr_t)_sidata + (sizeof(uintptr_t)-1))
                                    & ~(sizeof(uintptr_t)-1));
    uintptr_t *wdst = (uintptr_t*) (((uintptr_t)_sdata + (sizeof(uintptr_t)-1))
                                    & ~(sizeof(uintptr_t)-1));

    uint8_t *byte_src = _sidata;
    uint8_t *byte_dst = _sdata;

    /* Copy up to alignment boundary byte-wise. */
    while (((uintptr_t)byte_dst & (sizeof(uintptr_t)-1)) && (byte_dst < _edata)) {
        *byte_dst++ = *byte_src++;
    }

    /* Fast word copy. */
    size_t words = 0;
    if (byte_dst < _edata) {
        uintptr_t *wsrc_aligned = (uintptr_t*)byte_src;
        uintptr_t *wdst_aligned = (uintptr_t*)byte_dst;
        words = ((uintptr_t)_edata - (uintptr_t)wdst_aligned) / sizeof(uintptr_t);
        for (size_t i = 0; i < words; ++i) {
            wdst_aligned[i] = wsrc_aligned[i];
        }
        byte_dst = (uint8_t*)(wdst_aligned + words);
        byte_src = (uint8_t*)(wsrc_aligned + words);
    }

    /* Tail bytes. */
    while (byte_dst < _edata) {
        *byte_dst++ = *byte_src++;
    }

    /* Zero .bss: use word stores when aligned. */
    uint8_t *b = _sbss;
    /* Align to word boundary */
    while (((uintptr_t)b & (sizeof(uintptr_t)-1)) && (b < _ebss)) {
        *b++ = 0;
    }
    uintptr_t *wb = (uintptr_t*)b;
    size_t bwords = ((uintptr_t)_ebss - (uintptr_t)wb) / sizeof(uintptr_t);
    for (size_t i = 0; i < bwords; ++i) {
        wb[i] = (uintptr_t)0;
    }
    b = (uint8_t*)(wb + bwords);
    while (b < _ebss) {
        *b++ = 0;
    }
}
/* Note: if you relocate executable code into RAM, perform architecture-specific
   cache maintenance (ARM: DCache clean + DSB + ISB; RISC-V: FENCE.I) afterwards. */