ARM (AArch64) DVFS Transition Implementation


#include <stdint.h>
#include <stdbool.h>

#define MMIO32(addr) (*(volatile uint32_t *)(addr))

/* Platform-specific addresses (example placeholders) */
#define REG_PMIC_VOLTAGE   0x40000000U
#define REG_PMIC_STATUS    0x40000004U
#define REG_PLL_CFG        0x40001000U
#define REG_PLL_STATUS     0x40001004U
#define REG_CLK_CTRL       0x40002000U

/* Bitfields (replace with platform definitions) */
#define PMIC_STATUS_READY  (1U << 0)
#define PLL_STATUS_LOCK    (1U << 0)
#define CLK_CTRL_UPDATE    (1U << 0)

static inline void dsb(void)
{
    __asm__ volatile("dsb sy" ::: "memory");
}

static inline void isb(void)
{
    __asm__ volatile("isb" ::: "memory");
}

/* Poll MMIO until condition true or timeout cycles expire. Returns true if ok. */
static bool mmio_poll(uint32_t addr, uint32_t mask, uint32_t val, uint32_t timeout)
{
    while (timeout--) {
        if ((MMIO32(addr) & mask) == val) {
            return true;
        }
    }
    return false;
}

/* Set regulator voltage and wait until stable. Voltage value is SoC-specific. */
static bool set_voltage(uint32_t voltage_value)
{
    MMIO32(REG_PMIC_VOLTAGE) = voltage_value;   /* request new voltage */
    dsb();
    return mmio_poll(REG_PMIC_STATUS, PMIC_STATUS_READY, PMIC_STATUS_READY, 1000000);
}

/* Program PLL and wait for lock. pll_cfg is SoC-specific encoding. */
static bool set_frequency_via_pll(uint32_t pll_cfg)
{
    MMIO32(REG_PLL_CFG) = pll_cfg;              /* program PLL */
    dsb();
    return mmio_poll(REG_PLL_STATUS, PLL_STATUS_LOCK, PLL_STATUS_LOCK, 1000000);
}

/* Public API: increase => true to raise freq, false to lower. */
bool dvfs_transition(uint32_t target_voltage, uint32_t target_pll_cfg, bool increase)
{
    if (increase) {
        if (!set_voltage(target_voltage)) {
            return false;          /* raise V first */
        }
        if (!set_frequency_via_pll(target_pll_cfg)) {
            return false;          /* then f */
        }
        MMIO32(REG_CLK_CTRL) |= CLK_CTRL_UPDATE;
        dsb();
        isb();                     /* advertise change */
        return true;
    } else {
        MMIO32(REG_CLK_CTRL) &= ~CLK_CTRL_UPDATE;
        dsb();
        isb();                     /* reduce f first */
        if (!set_voltage(target_voltage)) {
            return false;          /* then lower V */
        }
        return true;
    }
}