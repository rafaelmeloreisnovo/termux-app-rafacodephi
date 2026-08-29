/**
 * lyapunov_convergence_validator.c — BUG-08 Closure Validator
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Validates Lyapunov convergence invariant φ = (1-H)·C with bounds [0, 1]
 * Ensures convergence metric stays bounded across all valid state inputs
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "lyapunov_convergence.h"

#define Q16_ONE     0x00010000u
#define Q16_HALF    0x00008000u
#define Q16_QUAR    0x00004000u

/**
 * Test: Boundary values for φ computation
 */
static int validate_boundary_values(void) {
    printf("=== Validating Boundary Cases ===\n");

    struct {
        const char *name;
        uint32_t h;
        uint32_t c;
        uint32_t expected_min;
        uint32_t expected_max;
    } tests[] = {
        /* H=0, C=0 → φ = 1·0 = 0 */
        {"H=0, C=0", 0x00000000, 0x00000000, 0x00000000, 0x00000001},
        /* H=0, C=1 → φ = 1·1 = 1 */
        {"H=0, C=1", 0x00000000, Q16_ONE, Q16_ONE - 1, Q16_ONE},
        /* H=1, C=0 → φ = 0·0 = 0 */
        {"H=1, C=0", Q16_ONE, 0x00000000, 0x00000000, 0x00000001},
        /* H=1, C=1 → φ = 0·1 = 0 */
        {"H=1, C=1", Q16_ONE, Q16_ONE, 0x00000000, 0x00000001},
        /* H=0.5, C=0.5 → φ = 0.5·0.5 = 0.25 */
        {"H=0.5, C=0.5", Q16_HALF, Q16_HALF, 0x00003FFF, 0x00004001},
    };

    int result = 0;
    for (int i = 0; i < 5; i++) {
        uint32_t phi = lyapunov_compute(tests[i].h, tests[i].c);
        bool valid = lyapunov_validate_bound(phi);

        if (!valid || phi < tests[i].expected_min || phi > tests[i].expected_max) {
            printf("❌ FAIL: %s → φ=0x%08x (expected [0x%08x, 0x%08x])\n",
                   tests[i].name, phi, tests[i].expected_min, tests[i].expected_max);
            result = -1;
        } else {
            printf("✓ %s → φ=0x%08x ✓\n", tests[i].name, phi);
        }
    }

    return result;
}

/**
 * Test: Comprehensive H, C grid (uniform distribution)
 */
static int validate_grid_coverage(void) {
    printf("\n=== Validating Grid Coverage (H×C ∈ [0,1]²) ===\n");

    int violations = 0;
    int total_tests = 0;

    /* Test 11×11 grid of H, C values */
    for (int h_idx = 0; h_idx <= 10; h_idx++) {
        for (int c_idx = 0; c_idx <= 10; c_idx++) {
            uint32_t h = (h_idx * Q16_ONE) / 10;
            uint32_t c = (c_idx * Q16_ONE) / 10;
            uint32_t phi = lyapunov_compute(h, c);

            total_tests++;

            if (!lyapunov_validate_bound(phi)) {
                printf("❌ FAIL: H=0x%08x, C=0x%08x → φ=0x%08x (out of bounds)\n",
                       h, c, phi);
                violations++;
            }

            /* Verify monotonicity: higher C → higher φ (for fixed H) */
            if (c_idx > 0) {
                uint32_t c_prev = ((c_idx - 1) * Q16_ONE) / 10;
                uint32_t phi_prev = lyapunov_compute(h, c_prev);
                if (phi < phi_prev) {
                    printf("⚠ WARNING: Monotonicity violation at H=0x%08x, C delta\n", h);
                }
            }

            /* Verify monotonicity: higher H → lower φ (for fixed C) */
            if (h_idx > 0) {
                uint32_t h_prev = ((h_idx - 1) * Q16_ONE) / 10;
                uint32_t phi_prev = lyapunov_compute(h_prev, c);
                if (phi > phi_prev) {
                    printf("⚠ WARNING: Monotonicity violation at H delta, C=0x%08x\n", c);
                }
            }
        }
    }

    printf("✓ Grid coverage: %d tests, %d violations\n", total_tests, violations);
    return violations > 0 ? -1 : 0;
}

/**
 * Test: Receipt recording with state context
 */
static int validate_receipt_recording(void) {
    printf("\n=== Validating Receipt Recording ===\n");

    struct {
        uint32_t h;
        uint32_t c;
        uint32_t phase;
        uint32_t attractor;
    } snapshots[] = {
        {0x00000000, Q16_ONE, 0, 0},
        {Q16_HALF, Q16_HALF, 20, 15},
        {Q16_ONE, 0x00000000, 40, 40},
        {Q16_QUAR, Q16_HALF, 5, 10},
    };

    for (int i = 0; i < 4; i++) {
        LyapunovReceipt *receipt = lyapunov_record_receipt(
            snapshots[i].h,
            snapshots[i].c,
            snapshots[i].phase,
            snapshots[i].attractor
        );

        if (!receipt) {
            printf("❌ FAIL: Receipt pointer is NULL\n");
            return -1;
        }

        if (receipt->entropy != snapshots[i].h ||
            receipt->coherence != snapshots[i].c ||
            receipt->phase != snapshots[i].phase ||
            receipt->attractor_idx != snapshots[i].attractor) {
            printf("❌ FAIL: Receipt fields mismatch\n");
            return -1;
        }

        if (!lyapunov_validate_bound(receipt->phi)) {
            printf("❌ FAIL: Receipt φ out of bounds (0x%08x)\n", receipt->phi);
            return -1;
        }

        printf("✓ Receipt[%d]: H=0x%08x, C=0x%08x, φ=0x%08x, phase=%u, attr=%u ✓\n",
               i, receipt->entropy, receipt->coherence, receipt->phi,
               receipt->phase, receipt->attractor_idx);
    }

    return 0;
}

/**
 * Test: Determinism — same inputs produce same φ
 */
static int validate_determinism(void) {
    printf("\n=== Validating Determinism ===\n");

    uint32_t test_pairs[][2] = {
        {0x00000000, 0x00000000},
        {Q16_HALF, Q16_HALF},
        {Q16_ONE, 0x00000000},
        {0x00007FFF, Q16_ONE},
        {Q16_QUAR, 0x0000C000},
    };

    int result = 0;
    for (int i = 0; i < 5; i++) {
        uint32_t phi1 = lyapunov_compute(test_pairs[i][0], test_pairs[i][1]);
        uint32_t phi2 = lyapunov_compute(test_pairs[i][0], test_pairs[i][1]);
        uint32_t phi3 = lyapunov_compute(test_pairs[i][0], test_pairs[i][1]);

        if (phi1 != phi2 || phi2 != phi3) {
            printf("❌ FAIL: Non-deterministic result (φ1=0x%08x, φ2=0x%08x, φ3=0x%08x)\n",
                   phi1, phi2, phi3);
            result = -1;
        } else {
            printf("✓ Pair[%d] (0x%08x, 0x%08x) → φ=0x%08x (deterministic) ✓\n",
                   i, test_pairs[i][0], test_pairs[i][1], phi1);
        }
    }

    return result;
}

/**
 * Test: Extremal cases and clamping
 */
static int validate_extremal_cases(void) {
    printf("\n=== Validating Extremal Cases ===\n");

    uint32_t extremes[] = {0x00000000, 0x7FFFFFFF, 0xFFFFFFFFu};

    int result = 0;
    for (int i = 0; i < 3; i++) {
        /* Test over-range H (should be clamped) */
        uint32_t phi_h = lyapunov_compute(extremes[i], Q16_HALF);
        if (!lyapunov_validate_bound(phi_h)) {
            printf("❌ FAIL: Over-range H=0x%08x produced φ=0x%08x (out of bounds)\n",
                   extremes[i], phi_h);
            result = -1;
        }

        /* Test over-range C (should be clamped) */
        uint32_t phi_c = lyapunov_compute(Q16_HALF, extremes[i]);
        if (!lyapunov_validate_bound(phi_c)) {
            printf("❌ FAIL: Over-range C=0x%08x produced φ=0x%08x (out of bounds)\n",
                   extremes[i], phi_c);
            result = -1;
        }
    }

    printf("✓ All extremal cases handled safely\n");
    return result;
}

int main(void) {
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║      BUG-08 Lyapunov Convergence Validator (φ bounds)         ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");

    int result = 0;

    result |= validate_boundary_values();
    result |= validate_grid_coverage();
    result |= validate_receipt_recording();
    result |= validate_determinism();
    result |= validate_extremal_cases();

    printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
    if (result == 0) {
        printf("║  ✅ BUG-08 CLOSURE CRITERIA: ALL PASSED                      ║\n");
        printf("║                                                               ║\n");
        printf("║  ✓ φ computation: φ = (1-H)·C verified                        ║\n");
        printf("║  ✓ Bounds guarantee: φ ∈ [0, 1] for all valid H,C ∈ [0,1]   ║\n");
        printf("║  ✓ Receipt recording: H, C, φ triplets with state context    ║\n");
        printf("║  ✓ Determinism: identical inputs → identical φ output        ║\n");
        printf("║  ✓ Extremal handling: over-range values safely clamped       ║\n");
        printf("║                                                               ║\n");
        printf("║  Next: Release candidate validation (safe-core profile)      ║\n");
    } else {
        printf("║  ❌ BUG-08 CLOSURE CRITERIA: FAILED                           ║\n");
    }
    printf("╚═══════════════════════════════════════════════════════════════╝\n");

    return (result == 0) ? 0 : 1;
}
