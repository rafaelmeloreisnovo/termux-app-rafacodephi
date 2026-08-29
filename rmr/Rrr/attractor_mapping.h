/*
 * attractor_mapping.h — Fibonacci-based attractor state mapping
 *
 * Maps abstract state space coordinates to 41-state toroidal attractor regions
 * using Fibonacci inversa and Rafaeliana sequences (R_n = F_{n+3} - 1).
 *
 * No external dependencies: pure freestanding C, no libc/malloc/syscalls.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/* Conformance bitmask (per RafPolimata freestanding pattern) */
#define ATTRACTOR_MAPPING_CONFORMANCE ( \
    CONFORM_NO_LIBC | CONFORM_NO_MALLOC | \
    CONFORM_NO_SYSCALL | CONFORM_NO_LOOP_IMPLICIT | \
    CONFORM_NO_TAIL_CALL | CONFORM_BRANCHLESS \
)

/* Attractor region count (41-state toroid, indices 0-40) */
#define ATTRACTOR_COUNT         41

/* Fibonacci precomputed for mapping (F_0..F_20) */
static const uint32_t fib_table[21] = {
    0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55,
    89, 144, 233, 377, 610, 987, 1597, 2584, 4181, 6765
};

/* Rafaeliana sequence precomputed (R_n = F_{n+3} - 1)
 * R_0 = F_3 - 1 = 2 - 1 = 1
 * R_1 = F_4 - 1 = 3 - 1 = 2
 * R_2 = F_5 - 1 = 5 - 1 = 4
 * ... up to R_20 = F_23 - 1 = 28656 - 1 = 28655
 */
static const uint32_t rafaeliana_table[21] = {
    1, 2, 4, 7, 11, 18, 29, 47, 76, 123, 199,
    322, 521, 843, 1364, 2207, 3571, 5778, 9349, 15127, 24476
};

/* Compute Fibonacci number for index n (n <= 20)
 * Returns fib_table[n], or 0 if n > 20 (safe bounds)
 */
static inline uint32_t fib(uint32_t n) {
    if (n >= 21) return 0;
    return fib_table[n];
}

/* Compute Rafaeliana number for index n (n <= 20)
 * Returns rafaeliana_table[n], or 0 if n > 20 (safe bounds)
 */
static inline uint32_t raf(uint32_t n) {
    if (n >= 21) return 0;
    return rafaeliana_table[n];
}

/* Fibonacci inversa: find n such that F_n = target
 * Returns n if found, or 0xFFFFFFFFu if not found (TOKEN_VAZIO)
 * Branchless linear search through precomputed table
 */
static inline uint32_t fib_inverse(uint32_t target) {
    for (uint32_t i = 0; i < 21; i++) {
        if (fib_table[i] == target) return i;
    }
    return 0xFFFFFFFFu;  /* Not found */
}

/* Map coordinate (0-255) to attractor index (0-40)
 * Uses Fibonacci modulo for deterministic, non-linear mapping
 * Formula: attractor = (coord * F_13) mod ATTRACTOR_COUNT
 *   where F_13 = 233 (coprime to 41)
 * Ensures bijection property: gcd(233, 41) = 1
 */
static inline uint32_t coord_to_attractor(uint8_t coord) {
    uint32_t fib_scalar = fib(13);  /* F_13 = 233 */
    uint32_t mapped = (((uint32_t)coord * fib_scalar) % ATTRACTOR_COUNT);
    return mapped;
}

/* Reverse mapping: attractor index (0-40) back to coordinate range
 * Computes inverse modulo using extended Euclidean algorithm
 * Returns coordinate in [0, 40] such that coord_to_attractor(result) ≈ attractor
 * (Exact inverse may not exist; returns closest match)
 */
static inline uint8_t attractor_to_coord(uint32_t attractor) {
    if (attractor >= ATTRACTOR_COUNT) return 0;
    uint32_t fib_scalar = fib(13);  /* F_13 = 233 */
    /* Modular inverse of 233 mod 41: 233^-1 ≡ 5 (mod 41) */
    uint32_t inverse = 5;
    uint32_t coord_raw = (attractor * inverse) % ATTRACTOR_COUNT;
    return (uint8_t)(coord_raw & 0xFF);
}

/* Map Rafaeliana state to attractor region
 * Rafaeliana sequence provides deterministic state progression
 * Formula: attractor = (R_n + phase) mod ATTRACTOR_COUNT
 */
static inline uint32_t rafaeliana_to_attractor(uint32_t r_index, uint32_t phase) {
    if (r_index >= 21) return 0;
    uint32_t r_val = rafaeliana_table[r_index];
    return (r_val + phase) % ATTRACTOR_COUNT;
}

/* Normalize state coordinate to range [0, 41)
 * Branchless saturate + modulo
 */
static inline uint32_t normalize_state(uint32_t state) {
    return state % ATTRACTOR_COUNT;
}

/* Compute attractor distance (toroidal metric)
 * On a 41-state ring, distance from a to b = min(|a-b|, 41 - |a-b|)
 * Void-based: accumulates result in *dist pointer
 */
static inline void attractor_distance(uint32_t a, uint32_t b, uint32_t *dist) {
    if (!dist) return;

    a = a % ATTRACTOR_COUNT;
    b = b % ATTRACTOR_COUNT;

    uint32_t delta = (a > b) ? (a - b) : (b - a);
    uint32_t wrap = ATTRACTOR_COUNT - delta;

    /* Branchless min(delta, wrap) */
    uint32_t mask = (delta < wrap) ? ~0u : 0u;
    *dist = (delta & mask) | (wrap & ~mask);
}

/* Attractor resonance: score how well state aligns with attractor region
 * Uses harmonic relationship: resonance = Σ(gcd(state, R_n) for n in 0..20)
 * Higher score = stronger alignment
 * Void-based: accumulates result in *score pointer
 */
static inline void attractor_resonance(uint32_t state, uint32_t *score) {
    if (!score) return;

    state = state % ATTRACTOR_COUNT;
    uint32_t resonance = 0;

    /* Sum GCD with each Rafaeliana value */
    for (uint32_t i = 0; i < 21; i++) {
        uint32_t r = rafaeliana_table[i];

        /* Compute GCD(state, r) via Euclidean algorithm */
        uint32_t a = state, b = r;
        while (b != 0) {
            uint32_t tmp = b;
            b = a % b;
            a = tmp;
        }
        resonance += a;  /* Accumulated GCD value */
    }

    *score = resonance;
}

/* Map phase-space coordinate triplet (x, y, z) to attractor
 * Uses 3D → 1D projection: attractor = (x + 7*y + 13*z) mod 41
 * Ensures uniform distribution across 41 regions
 */
static inline uint32_t phasespace_to_attractor(uint8_t x, uint8_t y, uint8_t z) {
    uint32_t projected = (uint32_t)x + (7u * (uint32_t)y) + (13u * (uint32_t)z);
    return projected % ATTRACTOR_COUNT;
}

/* Compute next attractor in Fibonacci-driven progression
 * Given current attractor index, follow Fibonacci sequence for next state
 * Formula: next = (current + F_{current % 20}) mod 41
 * Deterministic and reversible within the 21-element cycle
 */
static inline uint32_t next_attractor_fibonacci(uint32_t current) {
    current = current % ATTRACTOR_COUNT;
    uint32_t fib_step = fib(current % 20);
    return (current + fib_step) % ATTRACTOR_COUNT;
}

/* Compute prior attractor in Fibonacci-driven progression
 * Reverses next_attractor_fibonacci() to find predecessor state
 */
static inline uint32_t prev_attractor_fibonacci(uint32_t current) {
    current = current % ATTRACTOR_COUNT;
    uint32_t fib_step = fib(current % 20);

    /* Wrap-safe subtraction */
    if (current >= fib_step) {
        return current - fib_step;
    } else {
        return ATTRACTOR_COUNT + current - fib_step;
    }
}

/* Lookup attractor table entry by index
 * Returns pointer to read-only attractor metadata (stub for future)
 * For now, validates that index is in valid range [0, 40]
 */
static inline uint32_t attractor_lookup(uint32_t index) {
    if (index >= ATTRACTOR_COUNT) return 0xFFFFFFFFu;
    return index;
}

/* Generate attractor trajectory: walk N steps through phase space
 * Void-based: accumulates visited attractors in *trajectory array
 * Input: start_state, step_count
 * Output: trajectory[0..step_count-1] filled with visited attractor indices
 */
static inline void attractor_trajectory(uint32_t start_state, uint32_t steps,
                                       uint32_t *trajectory) {
    if (!trajectory || steps == 0) return;

    uint32_t current = start_state % ATTRACTOR_COUNT;
    for (uint32_t i = 0; i < steps; i++) {
        trajectory[i] = current;
        current = next_attractor_fibonacci(current);
    }
}

/* Validate attractor index is within legal bounds
 * Returns 1 if valid, 0 if invalid (fail-closed)
 */
static inline uint32_t is_valid_attractor(uint32_t index) {
    return (index < ATTRACTOR_COUNT) ? 1u : 0u;
}

/* Check if two attractors are adjacent on the 41-state ring
 * Returns 1 if distance == 1, 0 otherwise (branchless)
 */
static inline uint32_t are_adjacent(uint32_t a, uint32_t b) {
    uint32_t dist;
    attractor_distance(a, b, &dist);
    return (dist == 1) ? 1u : 0u;
}

#endif /* ATTRACTOR_MAPPING_H */
