Device Tree Interrupt Resolution in C


#include <libfdt.h>
#include <stdint.h>
#include <string.h>

/* Resolve first interrupt: returns 0 on success, negative libfdt error on failure.
 * out_phandle receives the controller phandle.
 * out_spec must point to an array sized at least max_spec_cells.
 * out_spec_len receives number of cells written.
 */
int resolve_first_interrupt(const void *fdt, int nodeoffset,
                            uint32_t *out_phandle, uint32_t *out_spec,
                            int max_spec_cells, int *out_spec_len)
{
    const fdt32_t *prop;
    int lenp;
    const char *prop_name = "interrupts-extended";

    prop = fdt_getprop(fdt, nodeoffset, prop_name, &lenp);
    if (prop) {
        /* Format: <phandle> <specifier>... */
        uint32_t phandle = fdt32_to_cpu(*prop);
        prop++;

        /* Locate the controller node to read its \#interrupt-cells. */
        int ctrl = fdt_node_offset_by_phandle(fdt, phandle);
        if (ctrl < 0) {
            return ctrl;
        }

        const fdt32_t *cells = fdt_getprop(fdt, ctrl, "#interrupt-cells", &lenp);
        if (!cells) {
            return -FDT_ERR_NOTFOUND;
        }

        int c = fdt32_to_cpu(*cells);
        if (c > max_spec_cells) {
            return -FDT_ERR_INTERNAL;
        }

        for (int i = 0; i < c; i++) {
            out_spec[i] = fdt32_to_cpu(prop[i]);
        }

        *out_phandle = phandle;
        *out_spec_len = c;
        return 0;
    }

    /* Fallback to 'interrupts' + interrupt-parent */
    prop_name = "interrupts";
    prop = fdt_getprop(fdt, nodeoffset, prop_name, &lenp);
    if (!prop) {
        return -FDT_ERR_NOTFOUND;
    }

    const fdt32_t *pprop = fdt_getprop(fdt, nodeoffset, "interrupt-parent", &lenp);
    uint32_t phandle = pprop ? fdt32_to_cpu(*pprop) : 0;

    if (!phandle) {
        /* If missing, DT spec uses parent's interrupt-controller; walk up */
        int parent = fdt_parent_offset(fdt, nodeoffset);
        while (parent >= 0 && !phandle) {
            const fdt32_t *tmp = fdt_getprop(fdt, parent, "interrupt-parent", &lenp);
            if (tmp) {
                phandle = fdt32_to_cpu(*tmp);
                break;
            }
            parent = fdt_parent_offset(fdt, parent);
        }

        if (!phandle) {
            return -FDT_ERR_NOTFOUND;
        }
    }

    int ctrl = fdt_node_offset_by_phandle(fdt, phandle);
    if (ctrl < 0) {
        return ctrl;
    }

    const fdt32_t *cells = fdt_getprop(fdt, ctrl, "#interrupt-cells", &lenp);
    if (!cells) {
        return -FDT_ERR_NOTFOUND;
    }

    int c = fdt32_to_cpu(*cells);
    if (c > max_spec_cells) {
        return -FDT_ERR_INTERNAL;
    }

    for (int i = 0; i < c; i++) {
        out_spec[i] = fdt32_to_cpu(prop[i]);
    }

    *out_phandle = phandle;
    *out_spec_len = c;
    return 0;
}