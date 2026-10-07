#ifndef RAFZ_PURE_Q16_H
#define RAFZ_PURE_Q16_H

/*
 * RAFAELIA ZERO — fixed-size Q16 pure leaf.
 *
 * Contract:
 * - caller-provided scalar/fixed-width values only;
 * - no headers, libc, heap, I/O, syscall, JNI or platform ABI;
 * - no if/for/while/switch/goto/ternary runtime syntax;
 * - no variable division or variable-length iteration;
 * - machine-code control transfer is audited separately.
 *
 * Byte mapping is deliberately b/256, i.e. [0,255] -> [0,1) in Q16.
 * It is not the alternative b/255 convention.
 */

RAFZ_INLINE rafz_u32 rafz_pure_mask_from_bit(rafz_u32 bit) {
    return 0u - (bit & 1u);
}

RAFZ_INLINE rafz_u32 rafz_pure_u32_min(rafz_u32 a, rafz_u32 b) {
    const rafz_u32 mask = rafz_pure_mask_from_bit((rafz_u32)(a < b));
    return (a & mask) | (b & ~mask);
}

RAFZ_INLINE rafz_u32 rafz_pure_u32_max(rafz_u32 a, rafz_u32 b) {
    const rafz_u32 mask = rafz_pure_mask_from_bit((rafz_u32)(a > b));
    return (a & mask) | (b & ~mask);
}

RAFZ_INLINE rafz_u32 rafz_pure_q16_sub_sat(rafz_u32 a, rafz_u32 b) {
    const rafz_u32 keep = rafz_pure_mask_from_bit((rafz_u32)(a >= b));
    return (a - b) & keep;
}

RAFZ_INLINE rafz_u32 rafz_pure_q16_avg_floor(rafz_u32 a, rafz_u32 b) {
    return (a & b) + ((a ^ b) >> 1u);
}

RAFZ_INLINE rafz_u32 rafz_pure_q16_from_u8_frac(rafz_u8 value) {
    return ((rafz_u32)value) << 8u;
}

RAFZ_INLINE rafz_u8 rafz_pure_q16_to_u8_frac(rafz_u32 value) {
    const rafz_u32 max_frac = RAFZ_Q16_ONE - 1u;
    const rafz_u32 bounded = rafz_pure_u32_min(value, max_frac);
    return (rafz_u8)(bounded >> 8u);
}

RAFZ_INLINE rafz_u32 rafz_pure_q16_mul(rafz_u32 a, rafz_u32 b) {
    const __UINT64_TYPE__ product =
        ((__UINT64_TYPE__)a) * ((__UINT64_TYPE__)b);
    return (rafz_u32)(product >> RAFZ_Q16_SHIFT);
}

RAFZ_INLINE rafz_u32 rafz_pure_q16_clamp01(rafz_u32 value) {
    return rafz_pure_u32_min(value, RAFZ_Q16_ONE);
}

RAFZ_INLINE rafz_u32 rafz_pure_q16_one_minus(rafz_u32 value) {
    return RAFZ_Q16_ONE - rafz_pure_q16_clamp01(value);
}

RAFZ_INLINE rafz_u32 rafz_pure_q16_phi(rafz_u32 entropy, rafz_u32 coherence) {
    const rafz_u32 h = rafz_pure_q16_clamp01(entropy);
    const rafz_u32 c = rafz_pure_q16_clamp01(coherence);
    return rafz_pure_q16_mul(RAFZ_Q16_ONE - h, c);
}

RAFZ_INLINE rafz_u32 rafz_pure_q16_dot8(
    const rafz_u32 *a,
    const rafz_u32 *b) {
    return
        rafz_pure_q16_mul(a[0], b[0]) +
        rafz_pure_q16_mul(a[1], b[1]) +
        rafz_pure_q16_mul(a[2], b[2]) +
        rafz_pure_q16_mul(a[3], b[3]) +
        rafz_pure_q16_mul(a[4], b[4]) +
        rafz_pure_q16_mul(a[5], b[5]) +
        rafz_pure_q16_mul(a[6], b[6]) +
        rafz_pure_q16_mul(a[7], b[7]);
}

#endif /* RAFZ_PURE_Q16_H */
