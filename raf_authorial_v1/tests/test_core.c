/* SPDX-License-Identifier: MIT */
/* Hosted test only; never linked into freestanding object. */
#include <assert.h>
#include <stdio.h>
#include "../src/raf_core.h"
static unsigned int independent_fnv(const unsigned char *p, unsigned int n) {
    unsigned int h = 2166136261u, i;
    for (i = 0; i < n; ++i) h = (h ^ (unsigned int)p[i]) * 16777619u;
    return h;
}
int main(void) {
    unsigned int out = 0xdeadbeefu, i;
    static const unsigned char abc[] = "abc";
    static const unsigned char max_u32[] = "4294967295";
    static const unsigned char overflow[] = "4294967296";
    static const unsigned char leading[] = "00042";
    static const unsigned char invalid[] = "42x";
    unsigned char sample[257];
    assert(raf_fold32((const unsigned char *)0, 0u, &out) == 1);
    assert(out == 2166136261u);
    assert(raf_fold32((const unsigned char *)0, 1u, &out) == 0);
    assert(raf_fold32(abc, 3u, (unsigned int *)0) == 0);
    assert(raf_fold32(abc, 3u, &out) == 1);
    assert(out == 0x1a47e90bu);
    for (i = 0u; i != 257u; ++i) sample[i] = (unsigned char)i;
    for (i = 0u; i != 258u; ++i) {
        assert(raf_fold32(sample, i, &out) == 1);
        assert(out == independent_fnv(sample, i));
    }
    out = 0xdeadbeefu;
    assert(raf_decimal_u32(max_u32, 10u, &out) == 1);
    assert(out == 4294967295u);
    assert(raf_decimal_u32(overflow, 10u, &out) == 0);
    assert(out == 4294967295u);
    assert(raf_decimal_u32(leading, 5u, &out) == 1);
    assert(out == 42u);
    assert(raf_decimal_u32(invalid, 3u, &out) == 0);
    assert(out == 42u);
    assert(raf_decimal_u32(abc, 0u, &out) == 0);
    assert(raf_decimal_u32((const unsigned char *)0, 1u, &out) == 0);
    assert(raf_decimal_u32(abc, 11u, &out) == 0);
    assert(raf_decimal_u32(abc, 1u, (unsigned int *)0) == 0);
    puts("PASS_SCOPED_HOST: bounded C11 core vectors");
    return 0;
}
