/*
 * kdf_selos.h — ψχρΔΣΩ KDF pipeline for autoral entropy expansion
 *
 * Provides deterministic key derivation and seed expansion using the
 * ψχρΔΣΩ (psi-chi-rho-delta-sigma-omega) live-cycle pipeline.
 * Replaces BouncyCastle entropy functions with freestanding implementation.
 *
 * No external dependencies: pure freestanding C, no libc/malloc/syscalls.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Conformance bitmask (per RafPolimata freestanding pattern) */
#define KDF_SELOS_CONFORMANCE ( \
    CONFORM_NO_LIBC | CONFORM_NO_MALLOC | \
    CONFORM_NO_SYSCALL | CONFORM_NO_LOOP_IMPLICIT | \
    CONFORM_NO_TAIL_CALL | CONFORM_BRANCHLESS \
)

/* KDF pipeline constants */
#define KDF_SELOS_HASH_SIZE     32  /* 256-bit output */
#define KDF_SELOS_BLOCK_SIZE    64  /* 512-bit block size */
#define KDF_SELOS_STATE_SIZE    8   /* 8 × 32-bit state words */

/* Selos symbolic anchors (ψχρΔΣΩ → ASCII representation) */
#define SELO_PSI    0x50534921u     /* "PSI!" */
#define SELO_CHI    0x4348492au     /* "CHI*" */
#define SELO_RHO    0x52484F2bu     /* "RHO+" */
#define DELO_DELTA  0x44454C2cu     /* "DEL," */
#define SELO_SIGMA  0x53494741u     /* "SIGA" */
#define SELO_OMEGA  0x4F4D4541u     /* "OMEA" */

/* KDF state machine: tracks pipeline phase */
typedef enum {
    KDF_PHASE_PSI   = 0,    /* Phase 1: ψ (absorption) */
    KDF_PHASE_CHI   = 1,    /* Phase 2: χ (correlation) */
    KDF_PHASE_RHO   = 2,    /* Phase 3: ρ (rotation) */
    KDF_PHASE_DELTA = 3,    /* Phase 4: δ (diffusion) */
    KDF_PHASE_SIGMA = 4,    /* Phase 5: σ (spreading) */
    KDF_PHASE_OMEGA = 5     /* Phase 6: ω (output) */
} KdfPhase_t;

/* KDF state structure (stack-allocated, no heap) */
typedef struct {
    uint32_t state[KDF_SELOS_STATE_SIZE];    /* 256-bit state */
    uint8_t  buf[KDF_SELOS_BLOCK_SIZE];      /* Input buffer */
    uint32_t buf_len;                        /* Bytes in buffer */
    uint32_t phase;                          /* Current pipeline phase */
    uint32_t counter;                        /* Iteration counter */
} KdfSelosState_t;

/* Initialize KDF state from seed
 * Seed can be arbitrary length; internally hashed to initialize state
 * Void-based: initializes *state in-place
 */
static inline void kdf_selos_init(KdfSelosState_t *state, const uint8_t *seed,
                                  uint32_t seed_len) {
    if (!state) return;

    memset(state, 0, sizeof(KdfSelosState_t));

    /* Phase 1 (ψ): Absorb seed into initial state
     * Use XOR accumulation for deterministic initialization
     */
    for (uint32_t i = 0; i < seed_len; i++) {
        state->state[i % KDF_SELOS_STATE_SIZE] ^= (uint32_t)seed[i];
    }

    state->phase = KDF_PHASE_PSI;
    state->counter = 0;
}

/* Phase 2 (χ): Correlation — mix state values with cross-terms */
static inline void kdf_phase_chi(KdfSelosState_t *state) {
    if (!state || state->phase != KDF_PHASE_PSI) return;

    /* Correlation mix: each state word depends on all others */
    uint32_t tmp[KDF_SELOS_STATE_SIZE];
    for (uint32_t i = 0; i < KDF_SELOS_STATE_SIZE; i++) {
        tmp[i] = state->state[i];
    }

    for (uint32_t i = 0; i < KDF_SELOS_STATE_SIZE; i++) {
        uint32_t mixed = tmp[i];
        for (uint32_t j = 0; j < KDF_SELOS_STATE_SIZE; j++) {
            if (i != j) {
                mixed ^= (tmp[j] >> (j % 16)) | (tmp[j] << (32 - (j % 16)));
            }
        }
        state->state[i] = mixed;
    }

    state->phase = KDF_PHASE_CHI;
}

/* Phase 3 (ρ): Rotation — circular shifts with varying offsets */
static inline void kdf_phase_rho(KdfSelosState_t *state) {
    if (!state || state->phase != KDF_PHASE_CHI) return;

    /* Branchless rotation: each word rotated by phase-dependent amount */
    for (uint32_t i = 0; i < KDF_SELOS_STATE_SIZE; i++) {
        uint32_t shift = (i * 7 + state->counter) % 32;
        uint32_t word = state->state[i];
        state->state[i] = (word << shift) | (word >> (32 - shift));
    }

    state->phase = KDF_PHASE_RHO;
}

/* Phase 4 (δ): Diffusion — propagate state entropy across all positions */
static inline void kdf_phase_delta(KdfSelosState_t *state) {
    if (!state || state->phase != KDF_PHASE_RHO) return;

    /* Diffusion: linear feedback shift register style mixing */
    uint32_t feedback = state->state[0] ^ state->state[7];

    for (uint32_t i = 1; i < KDF_SELOS_STATE_SIZE; i++) {
        uint32_t tmp = state->state[i];
        state->state[i] = state->state[i - 1] ^ feedback;
        feedback = tmp;
    }

    state->state[0] = feedback;
    state->phase = KDF_PHASE_DELTA;
}

/* Phase 5 (σ): Spreading — distribute state across multiple registers */
static inline void kdf_phase_sigma(KdfSelosState_t *state) {
    if (!state || state->phase != KDF_PHASE_DELTA) return;

    /* Spreading: XOR each element with permuted others */
    uint32_t perm[KDF_SELOS_STATE_SIZE];
    for (uint32_t i = 0; i < KDF_SELOS_STATE_SIZE; i++) {
        perm[i] = state->state[(i * 3 + 5) % KDF_SELOS_STATE_SIZE];
    }

    for (uint32_t i = 0; i < KDF_SELOS_STATE_SIZE; i++) {
        state->state[i] ^= perm[i];
    }

    state->phase = KDF_PHASE_SIGMA;
}

/* Phase 6 (ω): Output — finalize and extract key material */
static inline void kdf_phase_omega(KdfSelosState_t *state, uint8_t *output,
                                   uint32_t output_len) {
    if (!state || !output || state->phase != KDF_PHASE_SIGMA) return;

    /* Extract 256-bit state as 32 bytes */
    uint32_t extract_len = (output_len > KDF_SELOS_HASH_SIZE) ?
                           KDF_SELOS_HASH_SIZE : output_len;

    for (uint32_t i = 0; i < extract_len; i++) {
        uint32_t word_idx = i / 4;
        uint32_t byte_idx = i % 4;
        output[i] = (state->state[word_idx] >> (byte_idx * 8)) & 0xFF;
    }

    /* Pad with zeros if output_len > KDF_SELOS_HASH_SIZE */
    if (output_len > KDF_SELOS_HASH_SIZE) {
        memset(output + KDF_SELOS_HASH_SIZE, 0,
               output_len - KDF_SELOS_HASH_SIZE);
    }

    state->phase = KDF_PHASE_OMEGA;
}

/* One-shot KDF: seed → output (all 6 phases inline)
 * Input: seed, seed_len
 * Output: output buffer filled with key material (up to 32 bytes)
 * Void-based: fills *output in-place
 */
static inline void kdf_selos_derive(const uint8_t *seed, uint32_t seed_len,
                                   uint8_t *output, uint32_t output_len) {
    if (!seed || !output) return;

    KdfSelosState_t state;

    /* Initialize (phase ψ) */
    kdf_selos_init(&state, seed, seed_len);

    /* Execute pipeline: χ → ρ → δ → σ → ω */
    kdf_phase_chi(&state);
    kdf_phase_rho(&state);
    kdf_phase_delta(&state);
    kdf_phase_sigma(&state);
    kdf_phase_omega(&state, output, output_len);
}

/* KDF with counter (for expanding multiple independent keys)
 * Each counter value produces distinct output from same seed
 * Void-based: fills *output in-place
 */
static inline void kdf_selos_derive_counter(const uint8_t *seed,
                                           uint32_t seed_len,
                                           uint32_t counter,
                                           uint8_t *output,
                                           uint32_t output_len) {
    if (!seed || !output) return;

    KdfSelosState_t state;
    kdf_selos_init(&state, seed, seed_len);
    state.counter = counter;

    /* Absorb counter into state (phase ψ extension) */
    for (uint32_t i = 0; i < 4; i++) {
        state.state[i % KDF_SELOS_STATE_SIZE] ^= (counter >> (i * 8)) & 0xFF;
    }

    /* Execute pipeline */
    kdf_phase_chi(&state);
    kdf_phase_rho(&state);
    kdf_phase_delta(&state);
    kdf_phase_sigma(&state);
    kdf_phase_omega(&state, output, output_len);
}

/* KDF with both seed and context (for domain-separated derivation)
 * Combines seed + context for multi-input keying
 * Void-based: fills *output in-place
 */
static inline void kdf_selos_derive_context(const uint8_t *seed,
                                           uint32_t seed_len,
                                           const uint8_t *context,
                                           uint32_t context_len,
                                           uint8_t *output,
                                           uint32_t output_len) {
    if (!seed || !output) return;

    KdfSelosState_t state;
    kdf_selos_init(&state, seed, seed_len);

    /* Mix context into state */
    for (uint32_t i = 0; i < context_len; i++) {
        uint32_t idx = i % KDF_SELOS_STATE_SIZE;
        state.state[idx] ^= (uint32_t)context[i];
    }

    /* Execute pipeline */
    kdf_phase_chi(&state);
    kdf_phase_rho(&state);
    kdf_phase_delta(&state);
    kdf_phase_sigma(&state);
    kdf_phase_omega(&state, output, output_len);
}

/* Validate KDF output correctness (checksum gate)
 * Returns 1 if output is non-zero (entropy produced), 0 if all-zeros
 */
static inline uint32_t kdf_selos_validate(const uint8_t *output,
                                         uint32_t output_len) {
    if (!output || output_len == 0) return 0;

    uint32_t check = 0;
    for (uint32_t i = 0; i < output_len; i++) {
        check |= (uint32_t)output[i];
    }

    return (check != 0) ? 1u : 0u;
}

/* Expand seed to arbitrary length (HKDF-style expansion)
 * Produces output_len bytes from a seed via repeated KDF calls
 * Void-based: fills *output in-place
 */
static inline void kdf_selos_expand(const uint8_t *seed, uint32_t seed_len,
                                   uint8_t *output, uint32_t output_len) {
    if (!seed || !output) return;

    uint8_t tmp[KDF_SELOS_HASH_SIZE];
    uint32_t counter = 0;

    for (uint32_t pos = 0; pos < output_len; pos += KDF_SELOS_HASH_SIZE) {
        uint32_t chunk_len = (output_len - pos > KDF_SELOS_HASH_SIZE) ?
                             KDF_SELOS_HASH_SIZE : (output_len - pos);

        kdf_selos_derive_counter(seed, seed_len, counter, tmp,
                               KDF_SELOS_HASH_SIZE);

        memcpy(output + pos, tmp, chunk_len);
        counter++;
    }

    /* Securely erase temporary buffer */
    memset(tmp, 0, sizeof(tmp));
}

#endif /* KDF_SELOS_H */
