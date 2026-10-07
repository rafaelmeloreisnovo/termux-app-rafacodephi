#ifndef RAFZ_PURE_BLAKE3_SINGLE_H
#define RAFZ_PURE_BLAKE3_SINGLE_H

/*
 * RAFAELIA ZERO — BLAKE3 unkeyed single-block root leaf.
 *
 * Provenance:
 * - producer: rafaelmeloreisnovo/BLAKE3@29e4525e5b1e6dfd79167586da8d19722c3cc65b
 * - algorithm authority in that checkout: c/ (BLAKE3 1.8.7)
 * - pinned official comparison ref: 6aab490a26124663329dfd3961b8469f8fdb158b
 * - schedule/IV semantics: c/blake3_impl.h
 * - portable compression semantics: c/blake3_portable.c
 *
 * Scope:
 * - exactly one unkeyed BLAKE3 chunk block, input length 0..64;
 * - caller supplies a 64-byte block zero-padded after block_len;
 * - counter is 0 and flags are CHUNK_START|CHUNK_END|ROOT;
 * - emits the default 32-byte root digest.
 *
 * Contract:
 * - no external headers, libc, heap, syscall, JNI or I/O;
 * - no if/for/while/switch/goto/ternary runtime syntax;
 * - seven BLAKE3 rounds are explicitly unrolled;
 * - not a streaming/tree hasher and not a replacement for the full API.
 */

RAFZ_INLINE rafz_u32 rafz_b3_rotr32(rafz_u32 w, rafz_u32 c) {
    return (w >> c) | (w << (32u - c));
}

RAFZ_INLINE rafz_u32 rafz_b3_load32(const rafz_u8 *p) {
    return
        ((rafz_u32)p[0]) |
        ((rafz_u32)p[1] << 8u) |
        ((rafz_u32)p[2] << 16u) |
        ((rafz_u32)p[3] << 24u);
}

RAFZ_INLINE void rafz_b3_store32(rafz_u8 *p, rafz_u32 w) {
    p[0] = (rafz_u8)w;
    p[1] = (rafz_u8)(w >> 8u);
    p[2] = (rafz_u8)(w >> 16u);
    p[3] = (rafz_u8)(w >> 24u);
}

RAFZ_INLINE void rafz_b3_g(
    rafz_u32 s[16],
    rafz_u32 a,
    rafz_u32 b,
    rafz_u32 c,
    rafz_u32 d,
    rafz_u32 x,
    rafz_u32 y) {
    s[a] = s[a] + s[b] + x;
    s[d] = rafz_b3_rotr32(s[d] ^ s[a], 16u);
    s[c] = s[c] + s[d];
    s[b] = rafz_b3_rotr32(s[b] ^ s[c], 12u);
    s[a] = s[a] + s[b] + y;
    s[d] = rafz_b3_rotr32(s[d] ^ s[a], 8u);
    s[c] = s[c] + s[d];
    s[b] = rafz_b3_rotr32(s[b] ^ s[c], 7u);
}

RAFZ_INLINE void rafz_b3_round_0(rafz_u32 s[16], const rafz_u32 m[16]) {
    rafz_b3_g(s, 0u, 4u, 8u, 12u, m[0], m[1]);
    rafz_b3_g(s, 1u, 5u, 9u, 13u, m[2], m[3]);
    rafz_b3_g(s, 2u, 6u, 10u, 14u, m[4], m[5]);
    rafz_b3_g(s, 3u, 7u, 11u, 15u, m[6], m[7]);
    rafz_b3_g(s, 0u, 5u, 10u, 15u, m[8], m[9]);
    rafz_b3_g(s, 1u, 6u, 11u, 12u, m[10], m[11]);
    rafz_b3_g(s, 2u, 7u, 8u, 13u, m[12], m[13]);
    rafz_b3_g(s, 3u, 4u, 9u, 14u, m[14], m[15]);
}

RAFZ_INLINE void rafz_b3_round_1(rafz_u32 s[16], const rafz_u32 m[16]) {
    rafz_b3_g(s, 0u, 4u, 8u, 12u, m[2], m[6]);
    rafz_b3_g(s, 1u, 5u, 9u, 13u, m[3], m[10]);
    rafz_b3_g(s, 2u, 6u, 10u, 14u, m[7], m[0]);
    rafz_b3_g(s, 3u, 7u, 11u, 15u, m[4], m[13]);
    rafz_b3_g(s, 0u, 5u, 10u, 15u, m[1], m[11]);
    rafz_b3_g(s, 1u, 6u, 11u, 12u, m[12], m[5]);
    rafz_b3_g(s, 2u, 7u, 8u, 13u, m[9], m[14]);
    rafz_b3_g(s, 3u, 4u, 9u, 14u, m[15], m[8]);
}

RAFZ_INLINE void rafz_b3_round_2(rafz_u32 s[16], const rafz_u32 m[16]) {
    rafz_b3_g(s, 0u, 4u, 8u, 12u, m[3], m[4]);
    rafz_b3_g(s, 1u, 5u, 9u, 13u, m[10], m[12]);
    rafz_b3_g(s, 2u, 6u, 10u, 14u, m[13], m[2]);
    rafz_b3_g(s, 3u, 7u, 11u, 15u, m[7], m[14]);
    rafz_b3_g(s, 0u, 5u, 10u, 15u, m[6], m[5]);
    rafz_b3_g(s, 1u, 6u, 11u, 12u, m[9], m[0]);
    rafz_b3_g(s, 2u, 7u, 8u, 13u, m[11], m[15]);
    rafz_b3_g(s, 3u, 4u, 9u, 14u, m[8], m[1]);
}

RAFZ_INLINE void rafz_b3_round_3(rafz_u32 s[16], const rafz_u32 m[16]) {
    rafz_b3_g(s, 0u, 4u, 8u, 12u, m[10], m[7]);
    rafz_b3_g(s, 1u, 5u, 9u, 13u, m[12], m[9]);
    rafz_b3_g(s, 2u, 6u, 10u, 14u, m[14], m[3]);
    rafz_b3_g(s, 3u, 7u, 11u, 15u, m[13], m[15]);
    rafz_b3_g(s, 0u, 5u, 10u, 15u, m[4], m[0]);
    rafz_b3_g(s, 1u, 6u, 11u, 12u, m[11], m[2]);
    rafz_b3_g(s, 2u, 7u, 8u, 13u, m[5], m[8]);
    rafz_b3_g(s, 3u, 4u, 9u, 14u, m[1], m[6]);
}

RAFZ_INLINE void rafz_b3_round_4(rafz_u32 s[16], const rafz_u32 m[16]) {
    rafz_b3_g(s, 0u, 4u, 8u, 12u, m[12], m[13]);
    rafz_b3_g(s, 1u, 5u, 9u, 13u, m[9], m[11]);
    rafz_b3_g(s, 2u, 6u, 10u, 14u, m[15], m[10]);
    rafz_b3_g(s, 3u, 7u, 11u, 15u, m[14], m[8]);
    rafz_b3_g(s, 0u, 5u, 10u, 15u, m[7], m[2]);
    rafz_b3_g(s, 1u, 6u, 11u, 12u, m[5], m[3]);
    rafz_b3_g(s, 2u, 7u, 8u, 13u, m[0], m[1]);
    rafz_b3_g(s, 3u, 4u, 9u, 14u, m[6], m[4]);
}

RAFZ_INLINE void rafz_b3_round_5(rafz_u32 s[16], const rafz_u32 m[16]) {
    rafz_b3_g(s, 0u, 4u, 8u, 12u, m[9], m[14]);
    rafz_b3_g(s, 1u, 5u, 9u, 13u, m[11], m[5]);
    rafz_b3_g(s, 2u, 6u, 10u, 14u, m[8], m[12]);
    rafz_b3_g(s, 3u, 7u, 11u, 15u, m[15], m[1]);
    rafz_b3_g(s, 0u, 5u, 10u, 15u, m[13], m[3]);
    rafz_b3_g(s, 1u, 6u, 11u, 12u, m[0], m[10]);
    rafz_b3_g(s, 2u, 7u, 8u, 13u, m[2], m[6]);
    rafz_b3_g(s, 3u, 4u, 9u, 14u, m[4], m[7]);
}

RAFZ_INLINE void rafz_b3_round_6(rafz_u32 s[16], const rafz_u32 m[16]) {
    rafz_b3_g(s, 0u, 4u, 8u, 12u, m[11], m[15]);
    rafz_b3_g(s, 1u, 5u, 9u, 13u, m[5], m[0]);
    rafz_b3_g(s, 2u, 6u, 10u, 14u, m[1], m[9]);
    rafz_b3_g(s, 3u, 7u, 11u, 15u, m[8], m[6]);
    rafz_b3_g(s, 0u, 5u, 10u, 15u, m[14], m[10]);
    rafz_b3_g(s, 1u, 6u, 11u, 12u, m[2], m[12]);
    rafz_b3_g(s, 2u, 7u, 8u, 13u, m[3], m[4]);
    rafz_b3_g(s, 3u, 4u, 9u, 14u, m[7], m[13]);
}

RAFZ_INLINE void rafz_pure_blake3_single_block_root(
    const rafz_u8 block[64],
    rafz_u32 block_len,
    rafz_u8 out[32]) {
    rafz_u32 m[16];
    rafz_u32 s[16];

    m[0] = rafz_b3_load32(block + 0u);
    m[1] = rafz_b3_load32(block + 4u);
    m[2] = rafz_b3_load32(block + 8u);
    m[3] = rafz_b3_load32(block + 12u);
    m[4] = rafz_b3_load32(block + 16u);
    m[5] = rafz_b3_load32(block + 20u);
    m[6] = rafz_b3_load32(block + 24u);
    m[7] = rafz_b3_load32(block + 28u);
    m[8] = rafz_b3_load32(block + 32u);
    m[9] = rafz_b3_load32(block + 36u);
    m[10] = rafz_b3_load32(block + 40u);
    m[11] = rafz_b3_load32(block + 44u);
    m[12] = rafz_b3_load32(block + 48u);
    m[13] = rafz_b3_load32(block + 52u);
    m[14] = rafz_b3_load32(block + 56u);
    m[15] = rafz_b3_load32(block + 60u);

    s[0] = 0x6a09e667u;
    s[1] = 0xbb67ae85u;
    s[2] = 0x3c6ef372u;
    s[3] = 0xa54ff53au;
    s[4] = 0x510e527fu;
    s[5] = 0x9b05688cu;
    s[6] = 0x1f83d9abu;
    s[7] = 0x5be0cd19u;
    s[8] = 0x6a09e667u;
    s[9] = 0xbb67ae85u;
    s[10] = 0x3c6ef372u;
    s[11] = 0xa54ff53au;
    s[12] = 0u;
    s[13] = 0u;
    s[14] = block_len;
    s[15] = 11u;

    rafz_b3_round_0(s, m);
    rafz_b3_round_1(s, m);
    rafz_b3_round_2(s, m);
    rafz_b3_round_3(s, m);
    rafz_b3_round_4(s, m);
    rafz_b3_round_5(s, m);
    rafz_b3_round_6(s, m);

    rafz_b3_store32(out + 0u, s[0] ^ s[8]);
    rafz_b3_store32(out + 4u, s[1] ^ s[9]);
    rafz_b3_store32(out + 8u, s[2] ^ s[10]);
    rafz_b3_store32(out + 12u, s[3] ^ s[11]);
    rafz_b3_store32(out + 16u, s[4] ^ s[12]);
    rafz_b3_store32(out + 20u, s[5] ^ s[13]);
    rafz_b3_store32(out + 24u, s[6] ^ s[14]);
    rafz_b3_store32(out + 28u, s[7] ^ s[15]);
}

#endif /* RAFZ_PURE_BLAKE3_SINGLE_H */
