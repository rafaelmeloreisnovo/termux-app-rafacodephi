#ifndef RAFCODEPHI_LEGACY_BLAKE3_PLACEHOLDER_H
#define RAFCODEPHI_LEGACY_BLAKE3_PLACEHOLDER_H

/*
 * HISTORICAL NON-CONFORMANT PLACEHOLDER — NOT BLAKE3.
 *
 * This header previously claimed freestanding/branchless BLAKE3 conformance,
 * while its source explicitly used a simplified XOR placeholder, hosted
 * headers, runtime branches and loops. It also had an unmatched final #endif.
 *
 * It is intentionally quarantined. Do not enable it for cryptographic,
 * integrity, signing, custody or production decisions.
 *
 * Conformant bounded leaf:
 *   rafaelia/src/main/cpp/zero/include/rafz_pure_blake3_single.h
 * Full streaming authority:
 *   rafaelmeloreisnovo/BLAKE3@29e4525e5b1e6dfd79167586da8d19722c3cc65b
 */

#define RAFCODEPHI_BLAKE3_PLACEHOLDER 1u
#define RAFCODEPHI_BLAKE3_CONFORMANT 0u
#define RAFCODEPHI_BLAKE3_CRYPTOGRAPHIC_USE_ALLOWED 0u
#define BLAKE3_WRAPPER_CONFORMANCE 0u

#if !defined(RAFCODEPHI_ENABLE_NONCRYPTO_BLAKE3_PLACEHOLDER)
#error "Legacy blake3_wrapper.h is non-conformant and quarantined; use the RAFAELIA ZERO bounded leaf or canonical RMR BLAKE3."
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define BLAKE3_BLOCK_SIZE 64
#define BLAKE3_OUT_LEN 32

typedef struct {
    uint32_t h[8];
    uint32_t t_low;
    uint32_t t_high;
    uint8_t buf[BLAKE3_BLOCK_SIZE];
    uint32_t buf_len;
} Blake3State_t;

/*
 * Kept only so old source snapshots can be inspected/compiled deliberately
 * with RAFCODEPHI_ENABLE_NONCRYPTO_BLAKE3_PLACEHOLDER. This is not a hash
 * implementation and must never be compared with BLAKE3 digests.
 */
static inline void blake3_placeholder_xor(
    Blake3State_t *st,
    const uint8_t block[BLAKE3_BLOCK_SIZE]) {
    uint32_t i;
    for (i = 0u; i < 8u; ++i) {
        const uint32_t word =
            (uint32_t)block[i * 4u] |
            ((uint32_t)block[i * 4u + 1u] << 8u) |
            ((uint32_t)block[i * 4u + 2u] << 16u) |
            ((uint32_t)block[i * 4u + 3u] << 24u);
        st->h[i] ^= word;
    }
}

#endif /* RAFCODEPHI_LEGACY_BLAKE3_PLACEHOLDER_H */
