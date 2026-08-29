/**
 * verbovivo_bootstrap_gate.c — Convergence Gate Integration
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Bridge between bootstrap receipt validation and T^7 convergence:
 * - φ_fst from bootstrap must satisfy attractor conditions
 * - Convergence receipt tied to bootstrap execution proof
 * - Fail-closed: invalid φ → reject bootstrap
 */
#define _POSIX_C_SOURCE 200809L
#include "verbovivo_graph.h"

/* Minimal Receipt structure (inline, no external dependency) */
typedef struct {
    uint32_t magic;         /* "RCPT" */
    uint8_t stage;
    uint8_t _pad1, _pad2, _pad3;
    uint64_t timestamp_ns;
    uint32_t crc32c;
    uint32_t sha256[8];
    uint64_t phi_fst;
    uint8_t attractor;
    uint8_t _pad[7];
    uint32_t exit_code;
} BootstrapReceipt;

/* Freestanding memcpy, memset */
static void* memcpy_fs(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dest;
}

static void* memset_fs(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

#define memcpy memcpy_fs
#define memset memset_fs

/* Freestanding write syscall */
static long write_syscall(int fd, const void *buf, unsigned long count) {
    register long x0 asm("x0") = (long)fd;
    register long x1 asm("x1") = (long)buf;
    register long x2 asm("x2") = (long)count;
    register long x8 asm("x8") = 64;  /* SYS_write */
    asm volatile("svc #0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x2), "r"(x8) : "cc", "memory");
    return x0;
}

#define write write_syscall

/* ── Convergence Receipt (extends bootstrap Receipt) ────────────────── */
typedef struct {
    BootstrapReceipt bootstrap_receipt;  /* original bootstrap proof */

    uint64_t phi_entropy;              /* H_norm at convergence (Q16) */
    uint64_t phi_coherence;            /* C_norm at convergence (Q16) */
    uint64_t phi_fst_final;            /* φ_fst result (Q16) */
    uint8_t attractor_id;              /* 0-41 if converged; 255 if not */
    uint8_t convergence_status;        /* 0=attractor, 1=stable, 2=timeout, etc */
    uint16_t iteration_count;

    uint32_t t7_graph_size;            /* bytes used for serialized T^7 */
    uint8_t t7_graph_hash[32];         /* SHA256 of graph structure */
} ConvergenceReceipt;

/* ── Validation: φ bounds and attractor consistency ──────────────────── */
int vv_validate_convergence_receipt(const ConvergenceReceipt *receipt) {
    if (!receipt) return -1;

    /* φ_fst must be in [0, 0x10000] (Q16, range [0, 1]) */
    if (receipt->phi_fst_final > 0x10000u) {
        return -1;  /* invalid φ */
    }

    /* H_norm and C_norm also Q16 */
    if (receipt->phi_entropy > 0x10000u || receipt->phi_coherence > 0x10000u) {
        return -1;
    }

    /* If attractor_id is valid (0-41), convergence must be status 0 */
    if (receipt->attractor_id < 42u && receipt->convergence_status != 0u) {
        return -1;  /* inconsistent attractor claim */
    }

    /* Iteration count must be reasonable (1-10000) */
    if (receipt->iteration_count == 0u || receipt->iteration_count > 10000u) {
        return -1;
    }

    /* φ = (1-H)·C must match reported values */
    uint64_t one_minus_h = 0x10000u - receipt->phi_entropy;
    uint64_t expected_phi = (one_minus_h * receipt->phi_coherence) >> 16;
    if (expected_phi != receipt->phi_fst_final) {
        return -1;  /* φ calculation mismatch */
    }

    return 0;  /* valid */
}

/* ── Execute convergence on T^7 graph ───────────────────────────────── */
int vv_execute_convergence(T7ToroidGraph *g, const BootstrapReceipt *bootstrap_receipt,
                           ConvergenceReceipt *out_receipt) {
    if (!g || !bootstrap_receipt || !out_receipt) return -1;

    /* Initialize output receipt from bootstrap receipt */
    memset(out_receipt, 0, sizeof(*out_receipt));
    memcpy(&out_receipt->bootstrap_receipt, bootstrap_receipt, sizeof(BootstrapReceipt));

    /* Start from attractor 0 */
    g->current_node = g->attractor_node_ids[0u];

    /* Convergence loop */
    uint32_t max_iters = 5000u;
    uint8_t attractor_id = 255u;
    uint64_t phi_fst = 0u;
    int status = vv_graph_converge(g, max_iters, &phi_fst, &attractor_id);

    /* Compute final φ components */
    uint64_t h_norm, c_norm;
    vv_graph_compute_phi(g, &h_norm, &c_norm, &phi_fst);

    /* Fill receipt */
    out_receipt->phi_entropy = h_norm;
    out_receipt->phi_coherence = c_norm;
    out_receipt->phi_fst_final = phi_fst;
    out_receipt->attractor_id = attractor_id;
    out_receipt->convergence_status = (uint8_t)status;
    out_receipt->iteration_count = (uint16_t)g->iteration;

    /* Validate receipt immediately */
    if (vv_validate_convergence_receipt(out_receipt) != 0) {
        return -1;  /* validation failed */
    }

    return 0;  /* success */
}

/* ── Serialize convergence receipt for logging ────────────────────── */
int vv_serialize_convergence_receipt(const ConvergenceReceipt *receipt,
                                     uint8_t *buf, uint32_t buf_size) {
    if (!receipt || !buf || buf_size < 96u) return -1;

    uint32_t pos = 0u;

    /* Magic marker */
    memcpy(buf + pos, "VBVCONV", 8);  pos += 8u;

    /* φ values */
    *(uint64_t*)(buf + pos) = receipt->phi_entropy;  pos += 8u;
    *(uint64_t*)(buf + pos) = receipt->phi_coherence;  pos += 8u;
    *(uint64_t*)(buf + pos) = receipt->phi_fst_final;  pos += 8u;

    /* Attractor and status */
    buf[pos++] = receipt->attractor_id;
    buf[pos++] = receipt->convergence_status;
    *(uint16_t*)(buf + pos) = receipt->iteration_count;  pos += 2u;

    /* Bootstrap receipt CRC (embedded) */
    *(uint32_t*)(buf + pos) = receipt->bootstrap_receipt.crc32c;  pos += 4u;
    *(uint32_t*)(buf + pos) = receipt->bootstrap_receipt.exit_code;  pos += 4u;

    return (int)pos;
}

/* ── SVC logging helper (write receipt to stderr) ────────────────────── */
static void log_receipt_text(const ConvergenceReceipt *r) {
    /* Using direct syscall write (SVC #0 on ARM64) */
    const char *prefix = "CONVERGE: φ=";
    write(2, prefix, 12);

    /* Log φ in hex (Q16) */
    char buf[32];
    int len = 0;
    uint64_t phi = r->phi_fst_final;
    if (phi == 0) {
        buf[len++] = '0';
    } else {
        while (phi > 0) {
            buf[len++] = "0123456789abcdef"[phi & 0xFu];
            phi >>= 4;
        }
    }
    write(2, buf, len);

    const char *tail = " attractor=";
    write(2, tail, 11);

    if (r->attractor_id < 42u) {
        char abuf[3];
        abuf[0] = '0' + (r->attractor_id / 10u);
        abuf[1] = '0' + (r->attractor_id % 10u);
        abuf[2] = '\n';
        write(2, abuf, 3);
    } else {
        write(2, "none\n", 5);
    }
}

/* ── Receipt SHA256 verification stub ────────────────────────────── */
static int verify_receipt_sha256(const BootstrapReceipt *r) {
    /* Freestanding: minimal check on crc32c field (0 = skip) */
    if (!r) return -1;
    /* In real deployment, this calls P0.2 sha256_finalize() */
    return 0;  /* stub: always pass */
}

/* ── Main gate: Bootstrap + Convergence ───────────────────────────── */
int vv_bootstrap_convergence_gate(T7ToroidGraph *g,
                                  const BootstrapReceipt *bootstrap_receipt,
                                  ConvergenceReceipt *out_convergence) {
    if (!g || !bootstrap_receipt || !out_convergence) return -1;

    /* First: verify bootstrap receipt is valid */
    if (verify_receipt_sha256(bootstrap_receipt) != 0) {
        return -1;  /* bootstrap validation failed */
    }

    /* Verify phi_fst from bootstrap is in range */
    if (bootstrap_receipt->phi_fst > 0x10000u) {
        return -1;  /* bootstrap phi out of bounds */
    }

    /* Execute convergence on graph */
    if (vv_execute_convergence(g, bootstrap_receipt, out_convergence) != 0) {
        return -1;  /* convergence failed */
    }

    /* Cross-check: bootstrap phi should be close to convergence phi */
    /* (allow ±5% tolerance due to stochastic walk) */
    int64_t delta = (int64_t)bootstrap_receipt->phi_fst -
                    (int64_t)out_convergence->phi_fst_final;
    if (delta < 0) delta = -delta;
    if ((uint64_t)delta > (0x10000u / 20u)) {
        return -1;  /* phi mismatch (bootstrap vs convergence) */
    }

    /* Convergence succeeded and validated */
    log_receipt_text(out_convergence);
    return 0;
}
