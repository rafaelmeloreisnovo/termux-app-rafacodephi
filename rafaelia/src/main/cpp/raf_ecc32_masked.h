#ifndef RAF_ECC32_MASKED_H
#define RAF_ECC32_MASKED_H

/*
 * Loop-free equivalent of the legacy positional ECC transform.
 *
 * For source bit i (0-based), the legacy implementation uses position i+1
 * and XORs the source bit into ECC output bit b when position bit b is set.
 * Each output is therefore a linear GF(2) parity over one fixed mask.
 */

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
    return (unsigned char)(
        (unsigned int)raf_parity32_fold(v & RAF_ECC32_MASK_0) |
        ((unsigned int)raf_parity32_fold(v & RAF_ECC32_MASK_1) << 1u) |
        ((unsigned int)raf_parity32_fold(v & RAF_ECC32_MASK_2) << 2u) |
        ((unsigned int)raf_parity32_fold(v & RAF_ECC32_MASK_3) << 3u) |
        ((unsigned int)raf_parity32_fold(v & RAF_ECC32_MASK_4) << 4u) |
        ((unsigned int)raf_parity32_fold(v & RAF_ECC32_MASK_5) << 5u)
    );
}

#endif /* RAF_ECC32_MASKED_H */
