#ifndef RAFZ_PURE_BITRAF42_H
#define RAFZ_PURE_BITRAF42_H

/*
 * RAFAELIA ZERO — BITRAF42-LOWLEVEL-V1 pure packing leaf.
 *
 * Authority:
 *   app/src/main/cpp/lowlevel/raf_bitraf.c
 *
 * Wire layout, least-significant bit first:
 *   flags  : bits  0..10  (11)
 *   imm    : bits 11..22  (12)
 *   layer  : bits 23..32  (10)
 *   dir    : bits 33..35  (3)
 *   opcode : bits 36..41  (6)
 *
 * This is intentionally NOT the separate six-fields-by-seven-bits
 * experimental ISA found under rmr/Rrr/generated.
 *
 * Contract:
 * - no external headers, heap, syscall, JNI, I/O or platform ABI;
 * - no if/for/while/switch/goto/ternary runtime syntax;
 * - fixed 42-bit encode/getters/validation only.
 */

typedef __UINT64_TYPE__ rafz_bitraf42_u64;

#define RAFZ_BITRAF42_MASK ((rafz_bitraf42_u64)0x000003FFFFFFFFFFULL)

RAFZ_INLINE rafz_bitraf42_u64 rafz_pure_bitraf42_encode(
    rafz_u8 opcode,
    rafz_u8 dir,
    rafz_u16 layer,
    rafz_u16 imm,
    rafz_u16 flags) {
    return
        (((rafz_bitraf42_u64)(opcode & 0x3Fu)) << 36u) |
        (((rafz_bitraf42_u64)(dir & 0x07u)) << 33u) |
        (((rafz_bitraf42_u64)(layer & 0x03FFu)) << 23u) |
        (((rafz_bitraf42_u64)(imm & 0x0FFFu)) << 11u) |
        ((rafz_bitraf42_u64)(flags & 0x07FFu));
}

RAFZ_INLINE rafz_u8 rafz_pure_bitraf42_opcode(rafz_bitraf42_u64 word) {
    return (rafz_u8)((word >> 36u) & 0x3Fu);
}

RAFZ_INLINE rafz_u8 rafz_pure_bitraf42_dir(rafz_bitraf42_u64 word) {
    return (rafz_u8)((word >> 33u) & 0x07u);
}

RAFZ_INLINE rafz_u16 rafz_pure_bitraf42_layer(rafz_bitraf42_u64 word) {
    return (rafz_u16)((word >> 23u) & 0x03FFu);
}

RAFZ_INLINE rafz_u16 rafz_pure_bitraf42_imm(rafz_bitraf42_u64 word) {
    return (rafz_u16)((word >> 11u) & 0x0FFFu);
}

RAFZ_INLINE rafz_u16 rafz_pure_bitraf42_flags(rafz_bitraf42_u64 word) {
    return (rafz_u16)(word & 0x07FFu);
}

RAFZ_INLINE rafz_u32 rafz_pure_bitraf42_is_valid(rafz_bitraf42_u64 word) {
    return (rafz_u32)((word >> 42u) == 0u);
}

RAFZ_INLINE rafz_s32 rafz_pure_bitraf42_validate(rafz_bitraf42_u64 word) {
    const rafz_u32 invalid = (rafz_u32)((word >> 42u) != 0u);
    return (rafz_s32)(0u - invalid);
}

RAFZ_INLINE rafz_bitraf42_u64 rafz_pure_bitraf42_canonicalize(
    rafz_bitraf42_u64 word) {
    return word & RAFZ_BITRAF42_MASK;
}

#endif /* RAFZ_PURE_BITRAF42_H */
