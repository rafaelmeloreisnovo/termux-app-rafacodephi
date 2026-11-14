#ifndef RAF_INTERNETMOUNT_ECC_V1_H
#define RAF_INTERNETMOUNT_ECC_V1_H
/*
 * RAFCODEPHI INTERNETMOUNT: bounded two-data/one-XOR-parity stripe.
 * PURE_CORE: freestanding C99, no heap, libc, syscalls, JNI, or networking.
 * CRC32 detects accidental corruption; NOT cryptographic authenticity.
 * External trusted manifest must bind expected_crc[] to exact object version.
 * SOURCE != ARTIFACT != EXECUTION != EVIDENCE != CLAIM.
 */
#include <stdint.h>
#define RAF_IM_MAX_CHUNK 4096u

enum raf_im_ecc_result {
    RAF_IM_INTACT = 0,
    RAF_IM_RECOVER_DATA0 = 1,
    RAF_IM_RECOVER_DATA1 = 2,
    RAF_IM_RECOVER_PARITY = 3,
    RAF_IM_INVALID = -1,
    RAF_IM_UNCORRECTABLE = -2,
    RAF_IM_CHECKSUM_MISMATCH = -3
};

static inline uint32_t raf_im_crc32(const uint8_t *data, uint32_t n) {
    uint32_t c = 0xffffffffu;
    uint32_t i, j;
    for (i = 0; i < n; ++i) {
        c ^= (uint32_t)data[i];
        for (j = 0; j < 8u; ++j) {
            uint32_t mask = 0u - (c & 1u);
            c = (c >> 1u) ^ (0xedb88320u & mask);
        }
    }
    return ~c;
}

/* Inputs and output must NOT overlap. n denotes bytes, not capacity. */
static inline int raf_im_xor_parity(const uint8_t *a, const uint8_t *b,
                                    uint8_t *parity, uint32_t n) {
    uint32_t i;
    if (!a || !b || !parity || !n || n > RAF_IM_MAX_CHUNK ||
        a == b || a == parity || b == parity) return RAF_IM_INVALID;
    for (i = 0; i < n; ++i) parity[i] = (uint8_t)(a[i] ^ b[i]);
    return RAF_IM_INTACT;
}

/*
 * Recover exactly ONE corrupted stripe member into caller-owned scratch.
 * a,b,parity and recovered must have >=n accessible bytes; no overlap.
 * expected_crc[0..2] must come from independently authenticated metadata.
 * The function NEVER mutates input blocks. Only on positive return may
 * recovered[] replace the indicated corrupt member after durable journal.
 */
static inline int raf_im_recover_one(const uint8_t *a, const uint8_t *b,
                                     const uint8_t *parity,
                                     const uint32_t *expected_crc,
                                     uint8_t *recovered, uint32_t n) {
    uint32_t i;
    unsigned bad0, bad1, badp, count;
    int result;
    if (!a || !b || !parity || !expected_crc || !recovered ||
        !n || n > RAF_IM_MAX_CHUNK ||
        a == b || a == parity || b == parity ||
        recovered == a || recovered == b || recovered == parity)
        return RAF_IM_INVALID;
    bad0 = (unsigned)(raf_im_crc32(a, n) != expected_crc[0]);
    bad1 = (unsigned)(raf_im_crc32(b, n) != expected_crc[1]);
    badp = (unsigned)(raf_im_crc32(parity, n) != expected_crc[2]);
    count = bad0 + bad1 + badp;
    if (count == 0u) {
        /* Detect an inconsistent yet individually CRC-matching stripe. */
        for (i = 0; i < n; ++i)
            if ((uint8_t)(a[i] ^ b[i]) != parity[i])
                return RAF_IM_CHECKSUM_MISMATCH;
        return RAF_IM_INTACT;
    }
    if (count != 1u) return RAF_IM_UNCORRECTABLE;
    result = bad0 ? RAF_IM_RECOVER_DATA0 :
             (bad1 ? RAF_IM_RECOVER_DATA1 : RAF_IM_RECOVER_PARITY);
    for (i = 0; i < n; ++i)
        recovered[i] = bad0 ? (uint8_t)(b[i] ^ parity[i]) :
                       (bad1 ? (uint8_t)(a[i] ^ parity[i]) :
                               (uint8_t)(a[i] ^ b[i]));
    if (raf_im_crc32(recovered, n) != expected_crc[
        bad0 ? 0u : (bad1 ? 1u : 2u)])
        return RAF_IM_CHECKSUM_MISMATCH;
    return result;
}
#endif
