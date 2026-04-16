#include <libfdt.h>
#include <stdint.h>
#include <string.h>

/* Parse CPU topology into user-supplied arrays.
 * - fdt: pointer to device tree blob
 * - topo_ids: output array of physical ids (64-bit)
 * - topo_count: returns number of CPUs discovered
 * Returns 0 on success, negative libfdt error on failure.
 */
int parse_cpu_topology(const void *fdt, uint64_t *topo_ids, size_t *topo_count)
{
    int cpus_off;
    size_t count;
    int node;

    cpus_off = fdt_path_offset(fdt, "/cpus");
    if (cpus_off < 0) {
        return cpus_off;
    }

    count = 0;
    node = fdt_first_subnode(fdt, cpus_off);
    while (node >= 0) {
        const char *dtype;
        const fdt32_t *regp;
        int addr_cells;
        uint64_t id;

        dtype = fdt_getprop(fdt, node, "device_type", NULL);
        if (!dtype || strcmp(dtype, "cpu") != 0) {
            goto next_node;
        }

        regp = fdt_getprop(fdt, node, "reg", NULL);
        if (!regp) {
            goto next_node;
        }

        addr_cells = fdt_address_cells(fdt, cpus_off); /* use parent's #address-cells */

        /* Read up to two cells for 64-bit id */
        if (addr_cells == 1) {
            id = (uint64_t)fdt32_to_cpu(regp[0]);
        } else {
            id = ((uint64_t)fdt32_to_cpu(regp[0]) << 32)
               | (uint64_t)fdt32_to_cpu(regp[1]);
        }

        if (count < *topo_count) {
            topo_ids[count] = id;
        }
        count++;

    next_node:
        node = fdt_next_subnode(fdt, node);
    }

    *topo_count = count;
    return 0;
}