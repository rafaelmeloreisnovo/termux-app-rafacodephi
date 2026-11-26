/* RAF_OPT:PROFILE=size;UNROLL=1 */
/* SPDX-License-Identifier: MIT */
#include "raf_core.h"
int raf_decimal_u32(const unsigned char *data, unsigned int count,
                    unsigned int *out) {
    unsigned int i;
    unsigned int value = 0u;
    if (data == (const unsigned char *)0 ||
        out == (unsigned int *)0 ||
        count == 0u || count > 10u) return 0;
    for (i = 0u; i != count; ++i) {
        unsigned int digit = (unsigned int)data[i] - (unsigned int)'0';
        if (digit > 9u || value > 429496729u ||
            (value == 429496729u && digit > 5u)) return 0;
        value = value * 10u + digit;
    }
    *out = value;
    return 1;
}
