#ifndef RAF_ECC32_MASKED_H
#define RAF_ECC32_MASKED_H

/*
 * Fixed-mask equivalent of the legacy positional ECC transform.
 *
 * For source bit i (0-based), the legacy implementation uses position i+1
 * and XORs the source bit into ECC output bit b when position bit b is set.
 * Each output is therefore a linear GF(2) parity over one fixed mask.
 *
 * Compile-time policy:
 *   -Os/-Oz or RAF_ECC32_FORCE_COMPACT: six mask steps, no 32-position loop.
 *   RAF_ECC32_FORCE_UNROLL: fully unrolled speed-oriented form.
 */

_Static_assert(sizeof(unsigned int) == 4u, "RAFCODE-Phi ECC32 requires 32-bit unsigned int");

#if defined(RAF_ECC32_FORCE_COMPACT) && defined(RAF_ECC32_FORCE_UNROLL)
#error "Choose only one RAFCODE-Phi ECC32 compile policy"
#endif

#define RAF_ECC32_MASK_0 0x55555555u
#define RAF_ECC32_MASK_1 0x66666666u
#define RAF_ECC32_MASK_2 0x78787878u
#define RAF_ECC32_MASK_3 0x7F807F80u
#define RAF_ECC32_MASK_4 0x7FFF8000u
#define RAF_ECC32_MASK_5 0x80000000u

static inline unsigned char raf_parity32_fold(unsigned int v) {
    v ^= v >> 16u;
    v ^= v >> 8u;
    v ^= v >> 4u;
    return (unsigned char)((0x6996u >> (v & 0x0Fu)) & 1u);
}

static inline unsigned char raf_ecc32_masked(unsigned int v) {
#if defined(RAF_ECC32_FORCE_COMPACT) || \
    (!defined(RAF_ECC32_FORCE_UNROLL) && defined(__OPTIMIZE_SIZE__))
    static const unsigned int masks[6] = {
        RAF_ECC32_MASK_0,
        RAF_ECC32_MASK_1,
        RAF_ECC32_MASK_2,
        RAF_ECC32_MASK_3,
        RAF_ECC32_MASK_4,
        RAF_ECC32_MASK_5
    };
    unsigned char ecc = 0u;
    for (unsigned char bit = 0u; bit < 6u; ++bit) {
        ecc |= (unsigned char)(raf_parity32_fold(v & masks[bit]) << bit);
    }
    return ecc;
#else
    return (unsigned char)(
        (unsigned int)raf_parity32_fold(v & RAF_ECC32_MASK_0) |
        ((unsigned int)raf_parity32_fold(v & RAF_ECC32_MASK_1) << 1u) |
        ((unsigned int)raf_parity32_fold(v & RAF_ECC32_MASK_2) << 2u) |
        ((unsigned int)raf_parity32_fold(v & RAF_ECC32_MASK_3) << 3u) |
        ((unsigned int)raf_parity32_fold(v & RAF_ECC32_MASK_4) << 4u) |
        ((unsigned int)raf_parity32_fold(v & RAF_ECC32_MASK_5) << 5u)
    );
#endif
}

#endif /* RAF_ECC32_MASKED_H */
