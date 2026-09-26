#ifndef RAFZ_PURE_BITRAF42_H
#define RAFZ_PURE_BITRAF42_H

/*
 * RAFAELIA ZERO — fixed-width BITRAF42 codec leaf.
 *
 * Semantic authority:
 * - asm/RAFAELIA_ISA_BITRAF.md
 * - app/src/main/cpp/lowlevel/raf_bitraf.c
 *
 * Layout:
 *   [41:36] opcode 6
 *   [35:33] dir    3
 *   [32:23] layer 10
 *   [22:11] imm   12
 *   [10:0]  flags 11
 *
 * Contract:
 * - caller-owned scalar values only;
 * - no hosted headers, libc, heap, I/O, syscall, JNI or platform ABI;
 * - no if/for/while/switch/goto/ternary runtime syntax;
 * - no variable-length iteration;
 * - inputs are masked to the exact field width;
 * - bits >= 42 are invalid and are never produced by encode.
 */

#define RAFZ_BITRAF42_BITS 42u
#define RAFZ_BITRAF42_OPCODE_SHIFT 36u
#define RAFZ_BITRAF42_DIR_SHIFT 33u
#define RAFZ_BITRAF42_LAYER_SHIFT 23u
#define RAFZ_BITRAF42_IMM_SHIFT 11u

#define RAFZ_BITRAF42_OPCODE_MASK 0x3Fu
#define RAFZ_BITRAF42_DIR_MASK 0x07u
#define RAFZ_BITRAF42_LAYER_MASK 0x03FFu
#define RAFZ_BITRAF42_IMM_MASK 0x0FFFu
#define RAFZ_BITRAF42_FLAGS_MASK 0x07FFu
#define RAFZ_BITRAF42_WORD_MASK ((rafz_u64)0x000003FFFFFFFFFFULL)

RAFZ_INLINE rafz_u64 rafz_pure_bitraf42_encode(
    rafz_u8 opcode,
    rafz_u8 dir,
    rafz_u16 layer,
    rafz_u16 imm,
    rafz_u16 flags) {
    return
        (((rafz_u64)opcode & (rafz_u64)RAFZ_BITRAF42_OPCODE_MASK)
            << RAFZ_BITRAF42_OPCODE_SHIFT) |
        (((rafz_u64)dir & (rafz_u64)RAFZ_BITRAF42_DIR_MASK)
            << RAFZ_BITRAF42_DIR_SHIFT) |
        (((rafz_u64)layer & (rafz_u64)RAFZ_BITRAF42_LAYER_MASK)
            << RAFZ_BITRAF42_LAYER_SHIFT) |
        (((rafz_u64)imm & (rafz_u64)RAFZ_BITRAF42_IMM_MASK)
            << RAFZ_BITRAF42_IMM_SHIFT) |
        ((rafz_u64)flags & (rafz_u64)RAFZ_BITRAF42_FLAGS_MASK);
}

RAFZ_INLINE rafz_u8 rafz_pure_bitraf42_opcode(rafz_u64 word) {
    return (rafz_u8)(
        (word >> RAFZ_BITRAF42_OPCODE_SHIFT) &
        (rafz_u64)RAFZ_BITRAF42_OPCODE_MASK);
}

RAFZ_INLINE rafz_u8 rafz_pure_bitraf42_dir(rafz_u64 word) {
    return (rafz_u8)(
        (word >> RAFZ_BITRAF42_DIR_SHIFT) &
        (rafz_u64)RAFZ_BITRAF42_DIR_MASK);
}

RAFZ_INLINE rafz_u16 rafz_pure_bitraf42_layer(rafz_u64 word) {
    return (rafz_u16)(
        (word >> RAFZ_BITRAF42_LAYER_SHIFT) &
        (rafz_u64)RAFZ_BITRAF42_LAYER_MASK);
}

RAFZ_INLINE rafz_u16 rafz_pure_bitraf42_imm(rafz_u64 word) {
    return (rafz_u16)(
        (word >> RAFZ_BITRAF42_IMM_SHIFT) &
        (rafz_u64)RAFZ_BITRAF42_IMM_MASK);
}

RAFZ_INLINE rafz_u16 rafz_pure_bitraf42_flags(rafz_u64 word) {
    return (rafz_u16)(word & (rafz_u64)RAFZ_BITRAF42_FLAGS_MASK);
}

RAFZ_INLINE rafz_u32 rafz_pure_bitraf42_is_valid(rafz_u64 word) {
    const rafz_u64 high = word >> RAFZ_BITRAF42_BITS;
    const rafz_u64 nonzero =
        (high | ((rafz_u64)0u - high)) >> 63u;
    return ((rafz_u32)nonzero) ^ 1u;
}

#endif /* RAFZ_PURE_BITRAF42_H */
