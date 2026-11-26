/* RAF_OPT:PROFILE=speed;UNROLL=4 */
/* SPDX-License-Identifier: MIT */
#include "raf_core.h"
static inline unsigned int raf_fold_step(unsigned int hash,
                                         unsigned char byte) {
    return (hash ^ (unsigned int)byte) * 16777619u;
}
int raf_fold32(const unsigned char *data, unsigned int count,
               unsigned int *out) {
    unsigned int hash = 2166136261u;
    if (out == (unsigned int *)0 ||
        (data == (const unsigned char *)0 && count != 0u)) return 0;
#if RAF_UNROLL == 4
    while (count >= 4u) {
        hash = raf_fold_step(hash, *data++);
        hash = raf_fold_step(hash, *data++);
        hash = raf_fold_step(hash, *data++);
        hash = raf_fold_step(hash, *data++);
        count -= 4u;
    }
#endif
    while (count != 0u) {
        hash = raf_fold_step(hash, *data++);
        --count;
    }
    *out = hash;
    return 1;
}
