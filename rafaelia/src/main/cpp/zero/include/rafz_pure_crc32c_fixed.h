#ifndef RAFZ_PURE_CRC32C_FIXED_H
#define RAFZ_PURE_CRC32C_FIXED_H

/*
 * RAFAELIA ZERO — fixed-block CRC32C leaf.
 *
 * Semantics preserved from authoritative rafz_crc32c():
 * - reflected Castagnoli polynomial 0x82F63B78;
 * - initial state 0xFFFFFFFF;
 * - final bitwise complement.
 *
 * Contract:
 * - caller-owned bytes only;
 * - no headers, libc, heap, I/O, syscall, JNI or platform ABI;
 * - no if/for/while/switch/goto/ternary runtime syntax;
 * - one byte is exactly eight explicit reflected CRC steps;
 * - fixed block widths are compile-time compositions.
 */

#define RAFZ_CRC32C_POLY_REFLECTED 0x82F63B78u
#define RAFZ_CRC32C_INIT 0xFFFFFFFFu

RAFZ_INLINE rafz_u32 rafz_pure_crc32c_bit(rafz_u32 crc) {
    const rafz_u32 mask = 0u - (crc & 1u);
    return (crc >> 1u) ^ (RAFZ_CRC32C_POLY_REFLECTED & mask);
}

RAFZ_INLINE rafz_u32 rafz_pure_crc32c_byte(rafz_u32 crc, rafz_u8 byte) {
    crc ^= (rafz_u32)byte;
    crc = rafz_pure_crc32c_bit(crc);
    crc = rafz_pure_crc32c_bit(crc);
    crc = rafz_pure_crc32c_bit(crc);
    crc = rafz_pure_crc32c_bit(crc);
    crc = rafz_pure_crc32c_bit(crc);
    crc = rafz_pure_crc32c_bit(crc);
    crc = rafz_pure_crc32c_bit(crc);
    crc = rafz_pure_crc32c_bit(crc);
    return crc;
}

RAFZ_INLINE rafz_u32 rafz_pure_crc32c_update8(
    rafz_u32 crc,
    const rafz_u8 block[8]) {
    crc = rafz_pure_crc32c_byte(crc, block[0]);
    crc = rafz_pure_crc32c_byte(crc, block[1]);
    crc = rafz_pure_crc32c_byte(crc, block[2]);
    crc = rafz_pure_crc32c_byte(crc, block[3]);
    crc = rafz_pure_crc32c_byte(crc, block[4]);
    crc = rafz_pure_crc32c_byte(crc, block[5]);
    crc = rafz_pure_crc32c_byte(crc, block[6]);
    crc = rafz_pure_crc32c_byte(crc, block[7]);
    return crc;
}

RAFZ_INLINE rafz_u32 rafz_pure_crc32c_fixed8(const rafz_u8 block[8]) {
    return ~rafz_pure_crc32c_update8(RAFZ_CRC32C_INIT, block);
}

RAFZ_INLINE rafz_u32 rafz_pure_crc32c_fixed9(const rafz_u8 block[9]) {
    rafz_u32 crc = rafz_pure_crc32c_update8(RAFZ_CRC32C_INIT, block);
    crc = rafz_pure_crc32c_byte(crc, block[8]);
    return ~crc;
}

RAFZ_INLINE rafz_u32 rafz_pure_crc32c_fixed40(const rafz_u8 block[40]) {
    rafz_u32 crc = RAFZ_CRC32C_INIT;
    crc = rafz_pure_crc32c_update8(crc, block + 0u);
    crc = rafz_pure_crc32c_update8(crc, block + 8u);
    crc = rafz_pure_crc32c_update8(crc, block + 16u);
    crc = rafz_pure_crc32c_update8(crc, block + 24u);
    crc = rafz_pure_crc32c_update8(crc, block + 32u);
    return ~crc;
}

RAFZ_INLINE rafz_u32 rafz_pure_crc32c_fixed64(const rafz_u8 block[64]) {
    rafz_u32 crc = RAFZ_CRC32C_INIT;
    crc = rafz_pure_crc32c_update8(crc, block + 0u);
    crc = rafz_pure_crc32c_update8(crc, block + 8u);
    crc = rafz_pure_crc32c_update8(crc, block + 16u);
    crc = rafz_pure_crc32c_update8(crc, block + 24u);
    crc = rafz_pure_crc32c_update8(crc, block + 32u);
    crc = rafz_pure_crc32c_update8(crc, block + 40u);
    crc = rafz_pure_crc32c_update8(crc, block + 48u);
    crc = rafz_pure_crc32c_update8(crc, block + 56u);
    return ~crc;
}

#endif /* RAFZ_PURE_CRC32C_FIXED_H */
