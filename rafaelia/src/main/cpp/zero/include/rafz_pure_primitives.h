#ifndef RAFZ_PURE_PRIMITIVES_H
#define RAFZ_PURE_PRIMITIVES_H

/*
 * RAFAELIA ZERO pure arithmetic leaf.
 *
 * Contract:
 * - caller-provided scalar values only;
 * - no headers, libc, heap, I/O, syscall, JNI or platform ABI;
 * - no runtime decision or iteration syntax in this leaf;
 * - architecture selection remains a compile-time concern outside this file.
 *
 * Machine-code branchlessness is verified separately from source shape.
 */

RAFZ_INLINE rafz_u8 rafz_pure_bagua_rol3(rafz_u8 value) {
    const rafz_u8 v = (rafz_u8)(value & 7u);
    return (rafz_u8)((((rafz_u32)v << 1u) | ((rafz_u32)v >> 2u)) & 7u);
}

RAFZ_INLINE rafz_u8 rafz_pure_bagua_ror3(rafz_u8 value) {
    const rafz_u8 v = (rafz_u8)(value & 7u);
    return (rafz_u8)((((rafz_u32)v >> 1u) | ((rafz_u32)v << 2u)) & 7u);
}

RAFZ_INLINE rafz_s32 rafz_pure_q16_step(rafz_s32 current) {
    const rafz_s32 high = current >> 16;
    const rafz_u32 low = (rafz_u32)current & 0xFFFFu;
    const rafz_s32 scaled_high = high * (rafz_s32)RAFZ_Q16_GEOM;
    const rafz_s32 scaled_low =
        (rafz_s32)((low * (rafz_u32)RAFZ_Q16_GEOM) >> 16u);
    return scaled_high + scaled_low + (rafz_s32)RAFZ_Q16_FORCE;
}

RAFZ_INLINE rafz_u32 rafz_pure_select_u32(
    rafz_u32 mask,
    rafz_u32 when_mask,
    rafz_u32 when_clear) {
    return (when_mask & mask) | (when_clear & ~mask);
}

#endif /* RAFZ_PURE_PRIMITIVES_H */
