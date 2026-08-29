/**
 * lyapunov_convergence.h — Lyapunov convergence validation (header)
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Implements the Lyapunov convergence invariant: φ = (1-H)·C
 * where H (entropy) and C (coherence) are normalized Q16.16 fixed-point values.
 */

#pragma once
#ifndef LYAPUNOV_CONVERGENCE_H
#define LYAPUNOV_CONVERGENCE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * LyapunovReceipt: Convergence metric snapshot
 * Used to record H, C, φ triplets during execution
 */
typedef struct {
    uint32_t entropy;       /* H (entropy), Q16.16 ∈ [0, 1] */
    uint32_t coherence;     /* C (coherence), Q16.16 ∈ [0, 1] */
    uint32_t phi;           /* φ (convergence), Q16.16 ∈ [0, 1] */
    uint32_t phase;         /* Phase index [0..40] (diagnostic) */
    uint32_t attractor_idx; /* Attractor index [0..40] (diagnostic) */
} __attribute__((packed, aligned(4))) LyapunovReceipt;

/**
 * Lyapunov convergence metric: φ = (1 - H) · C (Q16.16)
 *
 * Entry:
 *   entropy: H in Q16.16 (normalized to [0, 1], where 0x10000 = 1.0)
 *   coherence: C in Q16.16 (normalized to [0, 1])
 *
 * Exit:
 *   Returns φ in Q16.16, guaranteed bounded to [0, 1]
 *
 * Guarantees:
 *   - No overflow: (1-H) ∈ [0, 1] and C ∈ [0, 1] → φ ∈ [0, 1]
 *   - Associativity with fixed-point: verified by gate
 *   - Deterministic for same H,C pair (bit-exact reproducibility)
 */
extern uint32_t lyapunov_compute(uint32_t entropy, uint32_t coherence);

/**
 * Validate convergence bound: φ ∈ [0, 1]
 *
 * Returns:
 *   true if φ is in valid range [0, 0x10000]
 *   false if φ exceeds bounds
 */
extern bool lyapunov_validate_bound(uint32_t phi);

/**
 * Record convergence receipt with state context
 */
extern LyapunovReceipt* lyapunov_record_receipt(
    uint32_t entropy,
    uint32_t coherence,
    uint32_t phase,
    uint32_t attractor_idx
);

#endif /* LYAPUNOV_CONVERGENCE_H */
