/**
 * vectra_pulse.h — AArch64 toroidal phase space orchestration (header)
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#ifndef VECTRA_PULSE_H
#define VECTRA_PULSE_H

#include <stdint.h>

/**
 * AttractorState structure layout (aligned to 20 bytes)
 * Used by vectra_pulse_step for phase space dynamics
 */
typedef struct {
    uint32_t reserved;      /* Offset +0: Reserved for alignment */
    uint32_t coherence;     /* Offset +4: C (Q16.16) */
    uint32_t entropy;       /* Offset +8: H (Q16.16) */
    uint32_t lyapunov;      /* Offset +12: φ (Q16.16) */
    uint32_t phase;         /* Offset +16: phase [0..40] */
} __attribute__((packed, aligned(4))) AttractorState;

/**
 * vectra_pulse_step: Single cycle of toroidal orchestration
 *
 * Entry registers (AArch64 calling convention):
 *   x0 = state_ptr (pointer to AttractorState)
 *   x1 = C (coherence, Q16.16)
 *   x2 = H (entropy, Q16.16)
 *   x3 = phase [0..40]
 *   x4 = attractor_idx [0..40]
 *
 * Exit registers:
 *   x5 = φ (lyapunov, Q16.16) → φ = (1-H)·C
 *   x3 = phase_new (wrapped to [0..40])
 *   x0 = updated state (written atomically with dmb ish)
 *
 * Guarantees:
 *   - All 4 BUG-03 issues fixed
 *   - Phase wrapping via gcd(Δr, 41)=1 (single subs/csel, no udiv)
 *   - Load-use hazard eliminated via independent instruction insertion
 *   - Bounds checking on attractor_idx (fallback to state[0])
 *   - Multi-core cache coherency via dmb ish
 *   - Cycle count ≤ 30 on Cortex-A (optimized)
 */
extern void vectra_pulse_step(void);

/**
 * vectra_pulse_bulk: Process array of states
 *
 * Entry:
 *   x0 = array of state_ptr (8-byte pointers)
 *   x1 = count (number of states to process)
 *
 * Calls vectra_pulse_step in loop for each state
 */
extern void vectra_pulse_bulk(void);

#endif /* VECTRA_PULSE_H */
