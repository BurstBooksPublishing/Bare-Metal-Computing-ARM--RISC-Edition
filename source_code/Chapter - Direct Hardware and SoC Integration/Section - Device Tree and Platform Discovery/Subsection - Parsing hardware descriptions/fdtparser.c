\begin{lstlisting}[language=C,caption={Extract first MMIO region (base,size) for a node using libfdt},label={lst:extract_reg}]

Device Tree Utility Functions in C for ARM (AArch64) and RISC-V Systems


#include <stdint.h>
#include <stddef.h>
#include <libfdt.h>

static uint64_t cells_to_u64(const uint32_t *cells, int count) {
    uint64_t v = 0;
    for (int i = 0; i < count; ++i) {
        v = (v << 32) | (uint32_t)fdt32_to_cpu(cells[i]);
    }
    return v;
}

static int read_prop_int(const void *fdt, int node, const char *prop, int def) {
    const fdt32_t *p = fdt_getprop(fdt, node, prop, NULL);
    if (!p) return def;
    return (int)fdt32_to_cpu(*p);
}

static int translate_via_ranges(const void *fdt, int parent, uint64_t child_addr, uint64_t *out_base) {
    int a_parent = read_prop_int(fdt, parent, "#address-cells", 2);
    int a_child  = read_prop_int(fdt, parent, "#address-cells", 2);
    const void *ranges = fdt_getprop(fdt, parent, "ranges", NULL);
    if (!ranges) return -1;

    const uint32_t *r = ranges;
    size_t len = fdt_getprop_len(fdt, parent, "ranges", NULL);
    const uint32_t *end = (const uint32_t *)((const char *)r + len);

    int s_parent = read_prop_int(fdt, parent, "#size-cells", 1);
    while (r + (a_child + a_parent + s_parent) <= end) {
        uint64_t cbase = cells_to_u64(r, a_child); r += a_child;
        uint64_t pbase = cells_to_u64(r, a_parent); r += a_parent;
        uint64_t sz    = cells_to_u64(r, s_parent); r += s_parent;
        if (child_addr >= cbase && child_addr < cbase + sz) {
            *out_base = pbase + (child_addr - cbase);
            return 0;
        }
    }
    return -1;
}

int fdt_get_first_reg(const void *fdt, const char *node_path, uint64_t *out_base, uint64_t *out_size) {
    int node = fdt_path_offset(fdt, node_path);
    if (node < 0) return -1;

    int a = read_prop_int(fdt, node, "#address-cells", -1);
    int s = read_prop_int(fdt, node, "#size-cells", -1);
    if (a < 0 || s < 0) {
        int parent = fdt_parent_offset(fdt, node);
        if (parent < 0) return -1;
        if (a < 0) a = read_prop_int(fdt, parent, "#address-cells", 2);
        if (s < 0) s = read_prop_int(fdt, parent, "#size-cells", 1);
    }

    int prop_len;
    const uint32_t *reg = fdt_getprop(fdt, node, "reg", &prop_len);
    if (!reg || prop_len < (int)4*(a+s)) return -1;

    uint64_t addr = cells_to_u64(reg, a);
    uint64_t size = cells_to_u64(reg + a, s);

    int parent = fdt_parent_offset(fdt, node);
    if (parent >= 0) {
        uint64_t translated;
        if (translate_via_ranges(fdt, parent, addr, &translated) == 0) addr = translated;
    }

    if (size == 0) return -1;
    *out_base = addr;
    *out_size = size;
    return 0;
}