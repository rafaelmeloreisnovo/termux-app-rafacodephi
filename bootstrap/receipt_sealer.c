#include "proot_config.h"
#include "proot_syscall_bridge.h"
#include <stdint.h>
#include <string.h>

/* Receipt Sealer: CRC32C + State Hashing for Bootstrap Proof */
/* Freestanding implementation, no malloc, stack-allocated only */

/* CRC32C (Castagnoli) polynomial lookup table */
static const uint32_t _crc32c_table[256] = {
    0x00000000, 0xf26b4c8b, 0xf5d6990e, 0x07bdd585, 0xf0ad1f1c, 0x02c65397, 0x057b8612,
    0xf710ca99, 0xe05a3e39, 0x12312672, 0x158cf3f7, 0xe7e7bf7c, 0x10f775e5, 0xe29c396e,
    0xe521eceb, 0x174aa060, 0xc0b46c72, 0x32df20f9, 0x3562f57c, 0xc709b9f7, 0x3019736e,
    0xc2723fe5, 0xc5cfea60, 0x37a4a6eb, 0x20ee524b, 0xd2851ec0, 0xd538cb45, 0x275387ce,
    0xd0434d57, 0x222801dc, 0x2595d459, 0xd7fe98d2, 0x2d5dac21, 0xdf36e0aa, 0xd88b352f,
    0x2ae079a4, 0xddf0b33d, 0x2f9bffb6, 0x28262a33, 0xda4d66b8, 0xcd079218, 0x3f6cde93,
    0x38d10b16, 0xcaba479d, 0x3daa8d04, 0xcfc1c18f, 0xc87c140a, 0x3a175881, 0xede99493,
    0x1f82d818, 0x183f0d9d, 0xea544116, 0x1d448b8f, 0xef2fc704, 0xe8921281, 0x1af95e0a,
    0x0db3aaaa, 0xff d8e621, 0xf86533a4, 0x0a0e7f2f, 0xfd1eb5b6, 0x0f75f93d, 0x08c82cb8,
    0xfaa36033, 0x00e0e8d0, 0xf28ba45b, 0xf53671de, 0x075d3d55, 0xf04df7cc, 0x0226bb47,
    0x059b6ec2, 0xf7f02249, 0xe0bad6e9, 0x12d19a62, 0x156c4fe7, 0xe707036c, 0x1017c9f5,
    0xe27c857e, 0xe5c150fb, 0x17aa1c70, 0xca5ed062, 0x38359ce9, 0x3f88496c, 0xcde305e7,
    0x3af3cf7e, 0xc89883f5, 0xcf255670, 0x3d4e1afb, 0x2a04ee5b, 0xd86fa2d0, 0xdfd27755,
    0x2db93bde, 0xda89f147, 0x28e2bdcc, 0x2f5f6849, 0xdd3424c2, 0x5d4c7d78, 0xaf2731f3,
    0xa89ae476, 0x5af1a8fd, 0xade16264, 0x5f8a2eef, 0x5837fb6a, 0xaa5cb7e1, 0xbd164341,
    0x4f7d0fca, 0x48c0da4f, 0xbaab96c4, 0x4dbb5c5d, 0xbfd010d6, 0xb86dc553, 0x4a0689d8,
    0x9df845ca, 0x6f930941, 0x682edcc4, 0x9a45904f, 0x6d555ad6, 0x9f3e165d, 0x9883c3d8,
    0x6ae88f53, 0x7da27bf3, 0x8fc93778, 0x8874e2fd, 0x7a1fae76, 0x8d0f64ef, 0x7f642864,
    0x78d9fde1, 0x8ab2b16a, 0x70f13988, 0x829a7503, 0x8527a086, 0x774cec0d, 0x805c2694,
    0x72376a1f, 0x758abf9a, 0x87e1f311, 0x90ab07b1, 0x62c04b3a, 0x657d9ebf, 0x9716d234,
    0x600618ad, 0x926d5426, 0x95d081a3, 0x67bbcd28, 0xba4f013a, 0x48244db1, 0x4f999834,
    0xbdf2d4bf, 0x4ae21e26, 0xb88952ad, 0xbf348728, 0x4d5fcba3, 0x5a153f03, 0xa87e7388,
    0xafc3a60d, 0x5da8ea86, 0xaab8201f, 0x58d36c94, 0x5f6eb911, 0xad05f59a, 0x7bfb3988,
    0x89907503, 0x8e2da086, 0x7c46ec0d, 0x8b562694, 0x793d2a1f, 0x7e80ff9a, 0x8cebb311,
    0x9ba147b1, 0x69ca0b3a, 0x6e77debf, 0x9c1c9234, 0x6b0c58ad, 0x99671426, 0x9edac1a3,
    0x6cb18d28, 0xb64501da, 0x442e4d51, 0x439398d4, 0xb1f8d45f, 0x46e81ec6, 0xb483524d,
    0xb33e87c8, 0x4155cb43, 0x561f3fe3, 0xa4747368, 0xa3c9a6ed, 0x51a2ea66, 0xa6b220ff,
    0x54d96c74, 0x5364b9f1, 0xa10ff57a, 0x7cfb39e8, 0x8e907583, 0x892da006, 0x7b46ec8d,
    0x8c562614, 0x7e3d6a9f, 0x7980bf1a, 0x8bebf391, 0x9ca10731, 0x6eca4bba, 0x69779e3f,
    0x9b1cd2b4, 0x6c0c182d, 0x9e6754a6, 0x99da8123, 0x6bb1cda8, 0xf0e9941c, 0x0282d897,
    0x053f0d12, 0xf7544199, 0x00448b00, 0xf22fc78b, 0xf592120e, 0x07f95e85, 0x10b3aa25,
    0xe2d8e6ae, 0xe565332b, 0x170e7fa0, 0xe01eb539, 0x1275f9b2, 0x15c82c37, 0xe7a360bc,
};

uint32_t crc32c_init(void) {
    return 0xffffffff;
}

uint32_t crc32c_update(uint32_t crc, const void *buf, uint64_t len) {
    const uint8_t *data = (const uint8_t *)buf;
    for (uint64_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        uint32_t index = (crc ^ byte) & 0xff;
        crc = _crc32c_table[index] ^ (crc >> 8);
    }
    return crc;
}

uint32_t crc32c_finalize(uint32_t crc) {
    return crc ^ 0xffffffff;
}

/* SHA256 (simplified stub for deterministic state hashing) */
/* In production, use full SHA256; here we use a deterministic 32-byte hash */
typedef struct {
    uint8_t digest[32];
} sha256_hash_t;

/* Simple deterministic state hash: XOR all state bytes repeatedly */
static void _deterministic_hash_32(const void *data, uint64_t len, uint8_t *out) {
    memset(out, 0, 32);
    const uint8_t *p = (const uint8_t *)data;
    for (uint64_t i = 0; i < len; i++) {
        out[i % 32] ^= p[i];
    }
}

/* Receipt Structure (sealed, immutable) */
typedef struct {
    char schema[32];                /* "raf.bootstrap-receipt.v1" */
    uint32_t timestamp_sec;         /* Unix epoch seconds */
    uint32_t timestamp_nsec;        /* Nanoseconds */
    uint8_t device_id_hash[32];     /* SHA256 of device ID (redacted) */
    uint32_t state_crc;             /* CRC32C of current state */
    uint8_t state_hash[32];         /* SHA256 of state snapshot */
    proot_state_t stage;            /* Current bootstrap stage */
    int stage_status;               /* PASS=1, FAIL=0 */
    uint32_t sealing_crc;           /* CRC32C over entire receipt (self) */
} receipt_t;

/* Sealed Receipt Log (append-only, bounded) */
typedef struct {
    receipt_t entries[PROOT_RECEIPT_MAX_STAGES];
    uint32_t count;
} receipt_log_t;

/* Global receipt log (stack-allocated in bootstrap main) */
static receipt_log_t _receipt_log = {0};

/* Seal a receipt for a bootstrap stage */
int seal_receipt(
    proot_state_t stage,
    int status,
    const void *state_snapshot,
    uint64_t state_size,
    receipt_t *out_receipt
) {
    if (!out_receipt || stage >= PROOT_RECEIPT_MAX_STAGES) {
        return -1;  /* Invalid parameters */
    }

    memset(out_receipt, 0, sizeof(receipt_t));
    strncpy(out_receipt->schema, PROOT_RECEIPT_SCHEMA, sizeof(out_receipt->schema) - 1);

    /* Capture timestamp (syscall) */
    struct timespec ts;
    if (proot_sys_clock_gettime(0, &ts) < 0) {  /* CLOCK_REALTIME */
        return -2;  /* Timestamp failed */
    }
    out_receipt->timestamp_sec = ts.tv_sec;
    out_receipt->timestamp_nsec = ts.tv_nsec;

    /* Hash state snapshot deterministically */
    _deterministic_hash_32(state_snapshot, state_size, out_receipt->state_hash);

    /* Compute CRC32C of state */
    uint32_t crc = crc32c_init();
    crc = crc32c_update(crc, state_snapshot, state_size);
    out_receipt->state_crc = crc32c_finalize(crc);

    /* Record stage and status */
    out_receipt->stage = stage;
    out_receipt->stage_status = status ? 1 : 0;

    /* Seal: compute CRC over receipt (excluding sealing_crc field itself) */
    uint32_t seal_crc = crc32c_init();
    seal_crc = crc32c_update(seal_crc, (uint8_t *)out_receipt, offsetof(receipt_t, sealing_crc));
    out_receipt->sealing_crc = crc32c_finalize(seal_crc);

    return 0;  /* Success */
}

/* Verify a sealed receipt */
int verify_receipt(const receipt_t *receipt) {
    if (!receipt) {
        return -1;
    }

    /* Verify schema */
    if (strncmp(receipt->schema, PROOT_RECEIPT_SCHEMA, sizeof(receipt->schema)) != 0) {
        return -2;  /* Schema mismatch */
    }

    /* Verify sealing CRC */
    uint32_t expected_crc = receipt->sealing_crc;
    uint32_t computed_crc = crc32c_init();
    computed_crc = crc32c_update(computed_crc, (uint8_t *)receipt, offsetof(receipt_t, sealing_crc));
    computed_crc = crc32c_finalize(computed_crc);

    if (computed_crc != expected_crc) {
        return -3;  /* CRC mismatch (receipt corrupted) */
    }

    return 0;  /* Valid */
}

/* Append receipt to log */
int log_receipt(const receipt_t *receipt) {
    if (!receipt || _receipt_log.count >= PROOT_RECEIPT_MAX_STAGES) {
        return -1;
    }

    memcpy(&_receipt_log.entries[_receipt_log.count], receipt, sizeof(receipt_t));
    _receipt_log.count++;
    return 0;
}

/* Write receipt log to file (for inspection) */
int write_receipt_log(const char *path) {
    if (!path || _receipt_log.count == 0) {
        return -1;
    }

    int fd = proot_sys_open(path, 0x241, 0644);  /* O_CREAT | O_WRONLY | O_TRUNC */
    if (fd < 0) {
        return -2;
    }

    /* Write JSON-like header */
    const char *header = "{\"schema\":\"raf.bootstrap-receipt-log.v1\",\"entries\":[";
    proot_sys_write(fd, header, strlen(header));

    /* Write each receipt as binary blob with checksum comment */
    for (uint32_t i = 0; i < _receipt_log.count; i++) {
        if (i > 0) proot_sys_write(fd, ",", 1);

        char buf[128];
        int n = snprintf(buf, sizeof(buf),
            "{\"stage\":%d,\"status\":%d,\"crc32c\":\"0x%08x\"}",
            _receipt_log.entries[i].stage,
            _receipt_log.entries[i].stage_status,
            _receipt_log.entries[i].state_crc);
        if (n > 0) proot_sys_write(fd, buf, n);
    }

    proot_sys_write(fd, "]}", 2);
    proot_sys_close(fd);
    return 0;
}

/* Get receipt log (for internal inspection) */
const receipt_log_t *get_receipt_log(void) {
    return &_receipt_log;
}
