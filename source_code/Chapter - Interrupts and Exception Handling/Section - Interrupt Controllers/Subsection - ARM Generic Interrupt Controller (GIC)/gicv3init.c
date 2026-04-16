ARM (AArch64) GICv3 Driver Initialization Code


#include <stdint.h>

#define GICD_BASE           0x2F000000UL  /* Example Distributor base (platform-specific) */
#define GICR_BASE           0x2F100000UL  /* Example Redistributor base (per-CPU region base) */
#define GICD_CTLR           0x000
#define GICD_TYPER          0x004
#define GICD_ISENABLER(n)   (0x100 + ((n) * 4))
#define GICD_IPRIORITYR(n)  (0x400 + (n))
#define GICD_ITARGETSR(n)   (0x800 + (n))

/* Simple MMIO accessors */
static inline void mmio_write32(uintptr_t address, uint32_t value)
{
    *((volatile uint32_t*)address) = value;
}

static inline uint32_t mmio_read32(uintptr_t address)
{
    return *((volatile uint32_t*)address);
}

static inline void dsb(void)
{
    __asm__ volatile("dsb sy" ::: "memory");
}

static inline void isb(void)
{
    __asm__ volatile("isb" ::: "memory");
}

/* Write system register helpers for ICC_* registers */
static inline void write_icc_sre_el1(uint64_t value)
{
    __asm__ volatile("msr ICC_SRE_EL1, %0" :: "r"(value));
    isb();
}

static inline void write_icc_igrpen1_el1(uint64_t value)
{
    __asm__ volatile("msr ICC_IGRPEN1_EL1, %0" :: "r"(value));
    isb();
}

static inline void write_icc_pmr_el1(uint64_t value)
{
    __asm__ volatile("msr ICC_PMR_EL1, %0" :: "r"(value));
    isb();
}

/* Initialize distributor and enable a single SPI */
void gicv3_init_enable_spi(unsigned int spi_id, unsigned int cpu_target_bit)
{
    unsigned int idx32 = spi_id / 32;
    unsigned int bit = spi_id % 32;
    uintptr_t isenabler = GICD_BASE + GICD_ISENABLER(idx32);
    uintptr_t ipriority = GICD_BASE + GICD_IPRIORITYR(spi_id);
    uintptr_t itarget = GICD_BASE + GICD_ITARGETSR(spi_id);

    /* Basic Distributor init: enable both groups (platform may require selective bits) */
    mmio_write32(GICD_BASE + GICD_CTLR, 0x3); /* EnableGrp0 | EnableGrp1 (platform-specific) */
    dsb();
    isb();

    /* Set default priority (0xA0 is typical medium priority) */
    mmio_write32(ipriority, 0xA0U);
    dsb();

    /* Target CPU (one byte per SPI); CPU0 -> bit 0 */
    mmio_write32(itarget, (1U << cpu_target_bit));
    dsb();

    /* Enable SPI bit */
    mmio_write32(isenabler, (1U << bit));
    dsb();
    isb();
}

/* Enable CPU interface: allow EL1 system register access and forward group1 interrupts */
void gicv3_cpu_interface_enable(void)
{
    write_icc_sre_el1(0x1);     /* Enable system register interface for EL1 */
    write_icc_pmr_el1(0xFF);    /* Priority mask: accept all priorities */
    write_icc_igrpen1_el1(1);   /* Enable group1 non-secure interrupts to be signaled to EL1 */
}