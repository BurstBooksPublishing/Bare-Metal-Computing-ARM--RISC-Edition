\begin{title}{PLIC Driver for RISC-V Platforms}
\end{title}

\begin{caption}{Minimal, portable PLIC helpers. Platform supplies base addresses/sizes.}
\end{caption}

#include <stdint.h>
#include <stddef.h>

#define PLIC\_MAX\_SRC 1024U  /* adjust per platform */

/* Platform-specific bases provided by the platform layer. */
extern volatile uint32\_t *const PLIC\_PRIORITY;   /* base for priority[1..] */
extern volatile uint8\_t  *const PLIC\_ENABLE\_BASE;/* per-context enable base */
extern volatile uint32\_t *const PLIC\_THRESHOLD; /* per-context threshold */
extern volatile uint32\_t *const PLIC\_CLAIM;     /* per-context claim/complete */

static inline void plic\_set\_priority(unsigned int src, uint32\_t prio)
{
    if (src == 0 || src > PLIC\_MAX\_SRC) return;
    PLIC\_PRIORITY[src] = prio;           /* write priority register */
    __asm__ volatile ("" ::: "memory");  /* ordering for MMIO */
}

/* Enable a single source only in the target hart's context enable bitmap. */
static inline void plic\_enable\_for\_hart(unsigned int hart\_context,
                                        unsigned int src, uint8\_t enable)
{
    /* context\_stride is platform-dependent; computed by platform. */
    const size\_t context\_stride = 0x100; /* example: replace per SoC */
    volatile uint8\_t *en\_base = PLIC\_ENABLE\_BASE + hart\_context * context\_stride;
    size\_t byte = src / 8;
    uint8\_t mask = (1u << (src & 7));
    if (enable)
        en\_base[byte] |= mask;          /* set enable bit */
    else
        en\_base[byte] &= (uint8\_t)~mask;/* clear enable bit */
    __asm__ volatile ("" ::: "memory");
}

static inline void plic\_set\_threshold(unsigned int hart\_context, uint32\_t thr)
{
    PLIC\_THRESHOLD[hart\_context] = thr;
    __asm__ volatile ("" ::: "memory");
}

/* Claim an interrupt and complete it (called from hart's ISR). */
static inline unsigned int plic\_claim(unsigned int hart\_context)
{
    return (unsigned int)PLIC\_CLAIM[hart\_context];
}
static inline void plic\_complete(unsigned int hart\_context, unsigned int src)
{
    PLIC\_CLAIM[hart\_context] = src;
    __asm__ volatile ("" ::: "memory");
}