Enhanced Device Tree Node Probing for ARM (AArch64) Platforms


#include <stdint.h>
#include <stddef.h>
#include <libfdt.h>

// Platform callbacks supplied per SoC: map/mmio/irq translate.
struct platform_ops {
    void *(*mmio_map)(uint64_t phys, size_t size); // map physical address
    int (*irq_translate)(int parent_phandle, const uint32_t *spec, int spec_len);
    int (*register_driver)(const char *compatible, void *dev);
};

// compact device descriptor
struct dev_desc {
    const char *name;
    void *mmio;
    size_t mmio_size;
    int irq;
};

// Probe a single node offset matching 'compatible'
static int probe_node(const void *fdt, int nodeoff,
                      const struct platform_ops *ops, struct dev_desc *out)
{
    const uint32_t *prop;
    int lenp;
    uint64_t addr = 0;
    uint64_t size = 0;
    int phandle_parent = -1;
    int rc;

    // Extract 'reg' (assume 64-bit address/size cells or use parent #address-cells)
    prop = fdt_getprop(fdt, nodeoff, "reg", &lenp);
    if (!prop || lenp < 16) {
        return -1; // require at least one 64/64 pair
    }
    addr = ((uint64_t)fdt32_to_cpu(prop[0]) << 32) | fdt32_to_cpu(prop[1]);
    size = ((uint64_t)fdt32_to_cpu(prop[2]) << 32) | fdt32_to_cpu(prop[3]);

    out->mmio = ops->mmio_map(addr, (size_t)size);
    out->mmio_size = (size_t)size;

    // Resolve interrupt: get interrupt-parent then 'interrupts' property
    prop = fdt_getprop(fdt, nodeoff, "interrupt-parent", &lenp);
    if (prop && lenp >= 4) {
        phandle_parent = fdt32_to_cpu(prop[0]);
    } else {
        phandle_parent = fdt_parent_offset(fdt, nodeoff); // fallback
    }

    prop = fdt_getprop(fdt, nodeoff, "interrupts", &lenp);
    if (!prop || phandle_parent < 0) {
        return 0; // no IRQ required
    }

    // spec length depends on parent's #interrupt-cells
    // For simplicity assume spec_len == lenp/4
    int spec_len = lenp / 4;
    // Translate to platform IRQ
    rc = ops->irq_translate(phandle_parent, prop, spec_len);
    if (rc < 0) {
        return -1;
    }
    out->irq = rc;
    return 0;
}