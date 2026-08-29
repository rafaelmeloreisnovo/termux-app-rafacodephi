/*
 * q16_fixed.h — Q16 fixed-point arithmetic for freestanding φ calculations
 *
 * Replaces floating-point math for Lyapunov convergence φ = (1-H)·C.
 * Q16 format: 0x10000 = 1.0, enabling integer-only calculations with no FPU.
 *
 * No external dependencies: pure freestanding C, no libc/malloc/FPU.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/* Conformance bitmask (per RafPolimata freestanding pattern) */
#define Q16_FIXED_CONFORMANCE ( \
    CONFORM_NO_LIBC | CONFORM_NO_MALLOC | \
    CONFORM_NO_SYSCALL | CONFORM_NO_LOOP_IMPLICIT | \
    CONFORM_NO_TAIL_CALL | CONFORM_BRANCHLESS \
)

/* Q16 constants */
#define Q16_ONE         0x00010000u      /* 1.0 in Q16 */
#define Q16_HALF        0x00008000u      /* 0.5 in Q16 */
#define Q16_QUARTER     0x00004000u      /* 0.25 in Q16 */
#define Q16_MIN         0x00000000u      /* 0.0 (minimum) */
#define Q16_MAX         0xFFFFFFFFu      /* ~65535.0 (maximum) */

/* Q16 scale factor (fractional bits) */
#define Q16_SHIFT       16
#define Q16_MASK        0xFFFFu          /* Fractional part mask */

/* Convert integer to Q16 */
static inline uint32_t q16_from_int(uint32_t x) {
    return x << Q16_SHIFT;
}

/* Convert Q16 to integer (truncate fractional part) */
static inline uint32_t q16_to_int(uint32_t q) {
    return q >> Q16_SHIFT;
}

/* Convert from byte (0-255) to Q16 normalized (0-1) */
static inline uint32_t q16_from_byte(uint8_t b) {
    /* (b / 256) = (b * 256) / 65536 */
    return ((uint32_t)b << 8);
}

/* Convert Q16 normalized (0-1) to byte (0-255) */
static inline uint8_t q16_to_byte(uint32_t q) {
    return (uint8_t)((q + Q16_HALF) >> Q16_SHIFT);
}

/* Q16 addition (a + b) */
static inline uint32_t q16_add(uint32_t a, uint32_t b) {
    return a + b;
}

/* Q16 subtraction (a - b) */
static inline uint32_t q16_sub(uint32_t a, uint32_t b) {
    /* Branchless saturate at zero */
    uint32_t result = a - b;
    uint32_t underflow = (a < b) ? 1u : 0u;
    return result & ~(underflow << 31);
}

/* Q16 multiplication (a * b) / 2^16
 * Uses 64-bit intermediate to avoid overflow
 */
static inline uint32_t q16_mul(uint32_t a, uint32_t b) {
    uint64_t prod = ((uint64_t)a * (uint64_t)b);
    return (uint32_t)(prod >> Q16_SHIFT);
}

/* Q16 division (a / b) * 2^16
 * Returns 0 if b == 0 (safe fail-closed)
 */
static inline uint32_t q16_div(uint32_t a, uint32_t b) {
    if (b == 0) return 0u;
    uint64_t scaled = ((uint64_t)a << Q16_SHIFT);
    return (uint32_t)(scaled / (uint64_t)b);
}

/* Q16 inverse (1 / x) * 2^16
 * Returns Q16_MAX if x == 0 (safe fail-closed with max value)
 */
static inline uint32_t q16_inv(uint32_t x) {
    if (x == 0) return Q16_MAX;
    return q16_div(Q16_ONE, x);
}

/* Q16 one minus (1 - x), clamped to [0, 1] */
static inline uint32_t q16_one_minus(uint32_t x) {
    if (x >= Q16_ONE) return 0u;
    return Q16_ONE - x;
}

/* Q16 min(a, b) — branchless */
static inline uint32_t q16_min(uint32_t a, uint32_t b) {
    uint32_t mask = ((a < b) ? ~0u : 0u);
    return (a & mask) | (b & ~mask);
}

/* Q16 max(a, b) — branchless */
static inline uint32_t q16_max(uint32_t a, uint32_t b) {
    uint32_t mask = ((a > b) ? ~0u : 0u);
    return (a & mask) | (b & ~mask);
}

/* Q16 clamp(x, min, max) — branchless saturation */
static inline uint32_t q16_clamp(uint32_t x, uint32_t min_val, uint32_t max_val) {
    x = q16_max(x, min_val);
    x = q16_min(x, max_val);
    return x;
}

/* Q16 average (a + b) / 2 */
static inline uint32_t q16_avg(uint32_t a, uint32_t b) {
    return (a >> 1) + (b >> 1) + ((a & b & 1) << Q16_SHIFT);
}

/* Lyapunov φ = (1 - H) · C
 * Input: H = entropy (Q16, 0-1), C = coherence (Q16, 0-1)
 * Output: φ (Q16, guaranteed ∈ [0, 1] via clamping)
 */
static inline uint32_t q16_lyapunov_phi(uint32_t H, uint32_t C) {
    /* Clamp inputs to [0, 1] for safety */
    H = q16_clamp(H, 0, Q16_ONE);
    C = q16_clamp(C, 0, Q16_ONE);

    /* φ = (1 - H) · C */
    uint32_t one_minus_H = q16_one_minus(H);
    uint32_t phi = q16_mul(one_minus_H, C);

    /* Final clamp to enforce φ ∈ [0, 1] */
    return q16_clamp(phi, 0, Q16_ONE);
}

/* Square root via Newton-Raphson (Q16)
 * Converges in ~5-6 iterations for typical values
 * Void-based: accumulates result in *out pointer
 */
static inline void q16_sqrt(uint32_t x, uint32_t *out) {
    if (!out) return;

    if (x == 0) {
        *out = 0;
        return;
    }

    /* Initial guess: x / 2 */
    uint32_t guess = x >> 1;

    /* Newton-Raphson: guess = (guess + x/guess) / 2 */
    for (int i = 0; i < 5; i++) {
        uint32_t next = q16_avg(guess, q16_div(x, guess));
        if (next == guess) break;  /* Converged */
        guess = next;
    }

    *out = guess;
}

/* Dot product (coherence proxy via frequency vector)
 * Computes: dot = Σ(freq[i] * seed[i]) in Q16
 * Input: 8 frequency values (Q16), 8 seed values (fixed)
 * Void-based: accumulates result in *out pointer
 */
static inline void q16_dot_product(const uint32_t *freq, const uint32_t *seed,
                                   uint32_t *out) {
    if (!freq || !seed || !out) return;

    uint32_t dot = 0;
    for (int i = 0; i < 8; i++) {
        dot += q16_mul(freq[i], seed[i]);
    }
    *out = dot;
}

/* Normalize vector to unit length (L2 norm)
 * Input: values (Q16 fixed-point)
 * Output: normalized values (Q16)
 * Void-based: modifies *vec in-place
 */
static inline void q16_normalize(uint32_t *vec, uint32_t len_count) {
    if (!vec || len_count == 0) return;

    /* Compute L2 norm: ||v|| = sqrt(Σ(v[i]^2)) */
    uint32_t sum_sq = 0;
    for (uint32_t i = 0; i < len_count; i++) {
        uint32_t v_sq = q16_mul(vec[i], vec[i]);
        sum_sq += v_sq;
    }

    if (sum_sq == 0) return;  /* Already zero vector */

    uint32_t norm;
    q16_sqrt(sum_sq, &norm);

    if (norm == 0) return;  /* Avoid division by zero */

    /* Normalize each component: v[i] / ||v|| */
    for (uint32_t i = 0; i < len_count; i++) {
        vec[i] = q16_div(vec[i], norm);
    }
}

#endif /* Q16_FIXED_H */
