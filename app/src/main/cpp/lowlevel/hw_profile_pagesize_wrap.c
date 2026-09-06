/*
 * hw_profile_pagesize_wrap.c — runtime page-size correction for bare-metal profile
 *
 * The legacy/no-malloc implementations historically inferred Android page size
 * from the build target. That is not a runtime proof: supported Android devices
 * can expose 4 KiB or 16 KiB pages. Resolve AT_PAGESZ directly from
 * /proc/self/auxv using the freestanding syscall layer.
 *
 * Link with -Wl,--wrap=get_hw_profile. The original provider remains available
 * as __real_get_hw_profile(); this wrapper changes only the page-size claim.
 * If AT_PAGESZ cannot be observed, the field is cleared instead of guessing.
 */

#include "baremetal.h"

#include <stdint.h>
#include <stddef.h>

#include "../../../../../src/bootstrap/freestanding_syscalls.h"

#ifndef AT_PAGESZ
#define AT_PAGESZ 6u
#endif

extern void __real_get_hw_profile(hw_profile_t *p);

static int raf_auxv_page_size(uint32_t *out_page_size) {
    struct raf_auxv_entry {
        uintptr_t type;
        uintptr_t value;
    } ent;

    if (!out_page_size) return 0;
    *out_page_size = 0u;

    int64_t fd = freestanding_open("/proc/self/auxv", O_RDONLY, 0u);
    if (fd < 0) return 0;

    int found = 0;
    while (freestanding_read((int)fd, &ent, (uint32_t)sizeof(ent)) ==
           (int64_t)sizeof(ent)) {
        if (ent.type == 0u) break;
        if (ent.type != (uintptr_t)AT_PAGESZ) continue;

        const uintptr_t value = ent.value;
        /* Fail closed on malformed values: page size must be a sensible power of 2. */
        if (value >= 1024u && value <= (uintptr_t)(1024u * 1024u) &&
            (value & (value - 1u)) == 0u) {
            *out_page_size = (uint32_t)value;
            found = 1;
        }
        break;
    }

    freestanding_close((int)fd);
    return found;
}

void __wrap_get_hw_profile(hw_profile_t *p) {
    if (!p) return;

    __real_get_hw_profile(p);

    uint32_t page_size = 0u;
    if (raf_auxv_page_size(&page_size)) {
        p->page_size = page_size;
        p->access_flags |= HW_ACCESS_HAS_PAGE_SIZE;
    } else {
        p->page_size = 0u;
        p->access_flags &= ~HW_ACCESS_HAS_PAGE_SIZE;
    }
}
