Device Tree Register Decoder in C


/* Requires libfdt (from dtc). Compile with -I/path/to/libfdt -ldtc or link libfdt. */
#include <stdio.h>
#include <stdint.h>
#include <libfdt.h>

static uint64_t read_cells_be(const uint32_t *cells, int count) {
    uint64_t val = 0;
    for (int i = 0; i < count; ++i) {
        val = (val << 32) | (uint32_t)fdt32_to_cpu(cells[i]);
    }
    return val;
}

/* Print decoded 'reg' entries for a node path. Returns 0 on success. */
int print_reg_entries(const void *fdt, const char *node_path) {
    int err = fdt_check_header(fdt);
    if (err) {
        fprintf(stderr, "FDT header invalid: %d\n", err);
        return -1;
    }

    int root_off = fdt_path_offset(fdt, "/");
    if (root_off < 0) {
        fprintf(stderr, "Root node not found\n");
        return -1;
    }

    int len;
    const uint32_t *p;

    p = (const uint32_t *)fdt_getprop(fdt, root_off, "#address-cells", &len);
    if (!p || len < 4) {
        fprintf(stderr, "Missing \#address-cells\n");
        return -1;
    }
    int ac = (int)fdt32_to_cpu(p[0]);

    p = (const uint32_t *)fdt_getprop(fdt, root_off, "#size-cells", &len);
    if (!p || len < 4) {
        fprintf(stderr, "Missing \#size-cells\n");
        return -1;
    }
    int sc = (int)fdt32_to_cpu(p[0]);

    int node_off = fdt_path_offset(fdt, node_path);
    if (node_off < 0) {
        fprintf(stderr, "Node '%s' not found\n", node_path);
        return -1;
    }

    const uint32_t *reg = (const uint32_t *)fdt_getprop(fdt, node_off, "reg", &len);
    if (!reg) {
        fprintf(stderr, "Node has no 'reg' property\n");
        return -1;
    }

    int entry_words = ac + sc;
    if (entry_words <= 0) {
        fprintf(stderr, "Invalid cells: ac=%d sc=%d\n", ac, sc);
        return -1;
    }
    if (len % (4 * entry_words) != 0) {
        fprintf(stderr, "Malformed 'reg' length\n");
        return -1;
    }

    int entries = len / (4 * entry_words);
    const uint32_t *cur = reg;
    for (int i = 0; i < entries; ++i) {
        uint64_t addr = read_cells_be(cur, ac);
        cur += ac;
        uint64_t size = read_cells_be(cur, sc);
        cur += sc;
        printf("reg[%d]: addr=0x%016llx size=0x%016llx\n",
               i, (unsigned long long)addr, (unsigned long long)size);
    }
    return 0;
}