/**
 * lyapunov_convergence.c — Lyapunov convergence validation
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Implements φ = (1-H)·C convergence metric with bounds validation
 */

#include "lyapunov_convergence.h"
#include <string.h>

#define Q16_ONE     0x00010000u  /* 1.0 in Q16.16 */
#define Q16_MAX     Q16_ONE      /* Maximum valid φ value */

/* Global receipt buffer (stateless, overwritten on each record) */
static LyapunovReceipt _receipt_buffer = {0};

/**
 * Compute Lyapunov convergence: φ = (1-H)·C
 *
 * Both H and C are Q16.16 fixed-point values normalized to [0, 1].
 * Result φ is also Q16.16, guaranteed to stay in [0, 1].
 *
 * Q16.16 multiplication:
 *   (1 - H) is computed as: Q16_ONE - H
 *   (1-H) × C is computed as full 64-bit multiply, then shift >> 16
 *   Result is truncated to 32-bit, staying within [0, Q16_ONE]
 *
 * Mathematical proof of bounds:
 *   H ∈ [0, 1] ⟹ (1-H) ∈ [0, 1]
 *   C ∈ [0, 1]
 *   φ = (1-H) × C ≤ 1 × 1 = 1 ✓
 *   φ ≥ 0 × 0 = 0 ✓
 */
uint32_t lyapunov_compute(uint32_t entropy, uint32_t coherence) {
    /* Clamp to valid range if needed */
    if (entropy > Q16_ONE) entropy = Q16_ONE;
    if (coherence > Q16_ONE) coherence = Q16_ONE;

    /* Compute (1 - H) */
    uint32_t one_minus_h = Q16_ONE - entropy;

    /* Multiply (1-H) × C using 64-bit accumulation to prevent overflow */
    uint64_t product = (uint64_t)one_minus_h * (uint64_t)coherence;

    /* Shift right by 16 to convert Q32.32 → Q16.16 */
    uint32_t phi = (uint32_t)(product >> 16);

    /* Clamp to Q16_ONE as final safety check */
    if (phi > Q16_ONE) phi = Q16_ONE;

    return phi;
}

/**
 * Validate that φ is within convergence bounds [0, 1]
 */
bool lyapunov_validate_bound(uint32_t phi) {
    return phi <= Q16_ONE;
}

/**
 * Record convergence receipt with state context
 */
LyapunovReceipt* lyapunov_record_receipt(
    uint32_t entropy,
    uint32_t coherence,
    uint32_t phase,
    uint32_t attractor_idx) {

    _receipt_buffer.entropy = entropy;
    _receipt_buffer.coherence = coherence;
    _receipt_buffer.phi = lyapunov_compute(entropy, coherence);
    _receipt_buffer.phase = phase;
    _receipt_buffer.attractor_idx = attractor_idx;

    return &_receipt_buffer;
}
