Flat Device Tree Property Finder in C


#include <stdint.h>
#include <string.h>

// Convert big-endian to host.
static inline uint32_t be32(const void *p) {
    const uint8_t *b = (const uint8_t *)p;
    return ((uint32_t)b[0] << 24) |
           ((uint32_t)b[1] << 16) |
           ((uint32_t)b[2] << 8)  |
           (uint32_t)b[3];
}

static inline uint64_t be64(const void *p) {
    const uint8_t *b = (const uint8_t *)p;
    return ((uint64_t)be32(b) << 32) | be32(b + 4);
}

// Search a path like "/soc/uart@..." in the structure block.
// Returns pointer to the FDT_PROP payload and its length via out_len, or NULL.
void *fdt_find_prop(const void *dtb, const char *path, uint32_t *out_len) {
    const uint8_t *base = (const uint8_t *)dtb;
    const uint32_t magic = be32(base);
    if (magic != 0xd00dfeed) return NULL;

    const uint32_t off_struct = be32(base + 8);
    const uint32_t off_strings = be32(base + 16);
    const uint32_t size_struct = be32(base + 36);

    const uint8_t *strs = base + off_strings;
    const uint8_t *p = base + off_struct;
    const uint8_t *end = p + size_struct;

    // Simple path traversal: split components and walk nodes.
    // For brevity, assume path starts with '/' and no '..' tokens.
    const char *comp = path + 1;
    char cur_name[64];

    while (p < end) {
        uint32_t tok = be32(p);
        p += 4;

        if (tok == 0x1) { // FDT_BEGIN_NODE
            // node name (NUL-terminated)
            const uint8_t *name = p;
            size_t nlen = 0;
            while (p < end && *p) {
                if (nlen < sizeof(cur_name) - 1) {
                    cur_name[nlen++] = *p;
                }
                p++;
            }
            cur_name[nlen] = 0;
            p++; // skip NUL

            // align to 4-byte boundary
            while (((uintptr_t)p & 3) && p < end) {
                p++;
            }

            // If name matches current component, advance comp to next.
            size_t comp_len = 0;
            while (comp[comp_len] && comp[comp_len] != '/') {
                comp_len++;
            }

            if (comp_len > 0 && comp_len < sizeof(cur_name) &&
                comp_len == nlen && memcmp(cur_name, comp, comp_len) == 0) {

                if (comp[comp_len] == 0) {
                    // target node entered; scan for properties
                    while (p < end) {
                        uint32_t t2 = be32(p);
                        p += 4;

                        if (t2 == 0x2) break; // FDT_END_NODE
                        if (t2 == 0x3) { // FDT_PROP
                            uint32_t len = be32(p);
                            uint32_t nameoff = be32(p + 4);
                            p += 8;

                            const char *pname = (const char *)(strs + nameoff);
                            if (strcmp(pname, "reg") == 0) {
                                if (out_len) *out_len = len;
                                return (void *)p; // payload (big-endian)
                            }

                            // skip property payload and realign
                            p += len;
                            while (((uintptr_t)p & 3) && p < end) {
                                p++;
                            }
                            continue;
                        }
                        if (t2 == 0x4) continue; // FDT_NOP
                        if (t2 == 0x9) break;    // FDT_END
                    }
                    return NULL;
                } else {
                    // descend: move comp pointer past this component and '/'.
                    comp += comp_len + 1;
                }
            }
            // otherwise, continue scanning child nodes.
        } else if (tok == 0x2) { // FDT_END_NODE
            continue;
        } else if (tok == 0x3) { // FDT_PROP
            uint32_t len = be32(p);
            uint32_t nameoff = be32(p + 4);
            p += 8 + len;
            while (((uintptr_t)p & 3) && p < end) {
                p++;
            }
        } else if (tok == 0x4) { // FDT_NOP
            continue;
        } else if (tok == 0x9) { // FDT_END
            break;
        } else {
            return NULL; // malformed token
        }
    }

    return NULL;
}