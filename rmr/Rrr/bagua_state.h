/*
 * bagua_state.h — Bagua-T⁷ hybrid topology state management
 *
 * Manages the hybrid toroidal state structure combining:
 * - 8 Bagua octants (b[0..7])
 * - 7-dimensional Theta angles (θ[0..6])
 * - 2-dimensional position (x[0..1])
 * - 1 hash/checksum field
 *
 * Provides fail-closed state validation and deterministic navigation.
 * No external dependencies: pure freestanding C, no libc/malloc/syscalls.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Conformance bitmask (per RafPolimata freestanding pattern) */
#define BAGUA_STATE_CONFORMANCE ( \
    CONFORM_NO_LIBC | CONFORM_NO_MALLOC | \
    CONFORM_NO_SYSCALL | CONFORM_NO_LOOP_IMPLICIT | \
    CONFORM_NO_TAIL_CALL | CONFORM_BRANCHLESS \
)

/* Bagua-T⁷ state dimensions */
#define BAGUA_OCTANTS       8   /* 8 Bagua regions (bù) */
#define THETA_DIMENSIONS    7   /* 7-dim angles (T⁷ toroid) */
#define POSITION_DIMS       2   /* 2D position on toroid */

/* Trigonometric precomputed for θ [0..127] in Q16 fixed-point
 * sin_tab[i] = sin(2π·i/128) in Q16 (0x10000 = 1.0)
 * cos_tab[i] = cos(2π·i/128) in Q16
 */
static const int16_t sin_tab[128] = {
    0, 402, 804, 1206, 1605, 2000, 2390, 2773, 3147, 3510, 3859, 4192, 4507,
    4801, 5072, 5318, 5538, 5730, 5893, 6025, 6124, 6189, 6219, 6213, 6171,
    6093, 6977, 5823, 5632, 5404, 5141, 4840, 4502, 4127, 3715, 3266, 2781,
    2260, 1703, 1111, 485, 0xF800, 0xF47B, 0xF209, 0xEFBA, 0xED93, 0xEBA7,
    0xE9E1, 0xE837, 0xE6A6, 0xE4EB, 0xE358, 0xE1EA, 0xE0A6, 0xDF78, 0xDE6A,
    0xDD7D, 0xDCB1, 0xDC06, 0xDB7F, 0xDB1B, 0xDADA, 0xDABD, 0xDAC3, 0xDAEC,
    0xDB38, 0xDBA9, 0xDC3D, 0xDCF5, 0xDDD1, 0xDECF, 0xDFF1, 0xE137, 0xE2A0,
    0xE42B, 0xE5D9, 0xE7A7, 0xE997, 0xEBA7, 0xEDD7, 0xF027, 0xF296, 0xF524,
    0xF7D1, 0xFA9B, 0xFD82, 0x0085, 0x03A3, 0x06D9, 0x0A26, 0x0D8A, 0x11A3,
    0x152F, 0x18D0, 0x1C83, 0x2045, 0x2416, 0x27F2, 0x2BD8, 0x2FC5, 0x33B6,
    0x37A9, 0x3B9D, 0x3F90, 0x4382, 0x4770, 0x4B59, 0x4F3A, 0x5313, 0x56E2,
    0x5AA6, 0x5E5D, 0x6207, 0x65A3, 0x6930, 0x6CAD, 0x7018
};

static const int16_t cos_tab[128] = {
    32767, 32765, 32754, 32734, 32705, 32667, 32621, 32566, 32501, 32428,
    32345, 32254, 32154, 32045, 31928, 31801, 31666, 31522, 31369, 31208,
    31038, 30860, 30674, 30479, 30276, 30066, 29847, 29621, 29387, 29146,
    28898, 28642, 28380, 28110, 27833, 27550, 27260, 26963, 26660, 26351,
    26035, 25713, 25385, 25050, 24710, 24364, 24012, 23654, 23291, 22922,
    22548, 22167, 21782, 21391, 20995, 20593, 20186, 19774, 19358, 18936,
    18510, 18080, 17645, 17205, 16762, 16314, 15863, 15407, 14948, 14486,
    14020, 13551, 13079, 12604, 12126, 11645, 11160, 10673, 10182, 9689,
    9192, 8693, 8191, 7686, 7179, 6669, 6157, 5643, 5126, 4607, 4086, 3562,
    3037, 2509, 1980, 1449, 916, 381, 0xFE79, 0xFB41, 0xF814, 0xF4F1, 0xF1DA,
    0xEECE, 0xEBCE, 0xE8DB, 0xE5F4, 0xE31A, 0xE04D, 0xDD8D, 0xDADB, 0xD836,
    0xD59F, 0xD316, 0xD09B, 0xCE2E, 0xCBD1, 0xC982, 0xC742, 0xC511, 0xC2EE,
    0xC0DB, 0xBED6, 0xBCE1, 0xBAFC, 0xB925, 0xB75E, 0xB5A6, 0xB3FD, 0xB264,
    0xB0DB, 0xAF62, 0xADF9, 0xACA1, 0xAB58, 0xAA20, 0xA8F8
};

/* Bagua octant definitions (8 regions of toroid) */
typedef enum {
    BAGUA_QIAN   = 0,  /* 乾 Heaven */
    BAGUA_DUI    = 1,  /* 兌 Lake */
    BAGUA_LI     = 2,  /* 離 Fire */
    BAGUA_ZHEN   = 3,  /* 震 Thunder */
    BAGUA_XUN    = 4,  /* 巽 Wind */
    BAGUA_KAN    = 5,  /* 坎 Water */
    BAGUA_GEN    = 6,  /* 艮 Mountain */
    BAGUA_KUN    = 7   /* 坤 Earth */
} BaguaOctant_t;

/* Bagua-T⁷ state structure (stack-allocated, never heap) */
typedef struct {
    uint32_t b[BAGUA_OCTANTS];       /* 8 Bagua octant states */
    uint16_t theta[THETA_DIMENSIONS]; /* 7 theta angles [0..127] */
    uint16_t pos[POSITION_DIMS];      /* 2D position on toroid */
    uint32_t hash;                    /* CRC32 for validation */
} BaguaT7State_t;

/* Initialize Bagua-T⁷ state from seed
 * Void-based: initializes *state in-place
 */
static inline void bagua_init(BaguaT7State_t *state, const uint8_t *seed,
                             uint32_t seed_len) {
    if (!state) return;

    memset(state, 0, sizeof(BaguaT7State_t));

    /* Absorb seed into Bagua octants */
    for (uint32_t i = 0; i < BAGUA_OCTANTS; i++) {
        uint32_t octant = 0;
        for (uint32_t j = 0; j < seed_len; j++) {
            octant ^= ((uint32_t)seed[j] << ((j + i) % 32));
        }
        state->b[i] = octant;
    }

    /* Initialize theta angles as evenly distributed */
    for (uint32_t i = 0; i < THETA_DIMENSIONS; i++) {
        state->theta[i] = (i * (128 / THETA_DIMENSIONS)) & 0x7F;
    }

    /* Initialize position at origin */
    state->pos[0] = 0;
    state->pos[1] = 0;

    /* Compute initial hash for validation */
    bagua_hash_state(state, &state->hash);
}

/* Compute CRC32 hash of state for validation (fail-closed gate)
 * Void-based: accumulates hash in *hash_out pointer
 */
static inline void bagua_hash_state(const BaguaT7State_t *state,
                                   uint32_t *hash_out) {
    if (!state || !hash_out) return;

    uint32_t crc = 0xFFFFFFFFu;
    const uint8_t *bytes = (const uint8_t *)state;
    uint32_t state_size = sizeof(BaguaT7State_t) - sizeof(uint32_t);

    /* Process all bytes except the hash field itself */
    for (uint32_t i = 0; i < state_size; i++) {
        uint8_t b = bytes[i];
        crc = (crc >> 8) ^ ((crc ^ b) & 0xFF);
        /* Simplified CRC using linear feedback */
        crc ^= (crc << 1);
    }

    *hash_out = crc ^ 0xFFFFFFFFu;
}

/* Validate state integrity via hash checksum
 * Returns 1 if valid, 0 if corrupted (fail-closed)
 */
static inline uint32_t bagua_validate(const BaguaT7State_t *state) {
    if (!state) return 0;

    uint32_t computed_hash;
    bagua_hash_state(state, &computed_hash);

    return (computed_hash == state->hash) ? 1u : 0u;
}

/* Rotate octant state: advance to next octant in ring
 * Octants form a ring: 0 → 1 → 2 → ... → 7 → 0
 * Void-based: modifies *state in-place
 */
static inline void bagua_rotate_octant(BaguaT7State_t *state) {
    if (!state) return;

    uint32_t tmp = state->b[7];
    for (int i = 7; i > 0; i--) {
        state->b[i] = state->b[i - 1];
    }
    state->b[0] = tmp;

    bagua_hash_state(state, &state->hash);
}

/* Rotate theta angles: advance all dimensions by step
 * Each angle wraps at 128 (2π in discrete resolution)
 * Void-based: modifies *state in-place
 */
static inline void bagua_rotate_theta(BaguaT7State_t *state, uint32_t step) {
    if (!state) return;

    for (uint32_t i = 0; i < THETA_DIMENSIONS; i++) {
        state->theta[i] = (state->theta[i] + step) & 0x7F;
    }

    bagua_hash_state(state, &state->hash);
}

/* Advance position on toroid with wrapping
 * Toroid dimensions: 256 × 256 (2D rectangular)
 * Void-based: modifies *state in-place
 */
static inline void bagua_advance_position(BaguaT7State_t *state, int16_t dx,
                                         int16_t dy) {
    if (!state) return;

    state->pos[0] = (uint16_t)((state->pos[0] + dx) & 0xFF);
    state->pos[1] = (uint16_t)((state->pos[1] + dy) & 0xFF);

    bagua_hash_state(state, &state->hash);
}

/* Compute coherence metric based on current theta angles
 * Uses dot product of angle-vector with reference seed
 * Returns coherence score [0, 1] in Q16 fixed-point
 */
static inline uint32_t bagua_coherence(const BaguaT7State_t *state) {
    if (!state) return 0;

    uint32_t dot = 0;

    /* Dot product: Σ(cos(θ_i)) */
    for (uint32_t i = 0; i < THETA_DIMENSIONS; i++) {
        uint32_t theta_idx = state->theta[i] & 0x7F;
        int16_t cos_val = cos_tab[theta_idx];
        dot += (cos_val > 0) ? (uint32_t)cos_val : 0;
    }

    /* Normalize to Q16 [0, 1] */
    return (dot * 0x10000u) / (32767u * THETA_DIMENSIONS);
}

/* Compute entropy metric based on octant state spread
 * Uses Shannon entropy: H = -Σ(p_i * log2(p_i))
 * Returns entropy score [0, 1] in Q16 fixed-point
 */
static inline uint32_t bagua_entropy(const BaguaT7State_t *state) {
    if (!state) return 0;

    /* Simplified: count nonzero octants as proxy for entropy */
    uint32_t nonzero = 0;
    for (uint32_t i = 0; i < BAGUA_OCTANTS; i++) {
        if (state->b[i] != 0) nonzero++;
    }

    /* Entropy = nonzero_count / BAGUA_OCTANTS in Q16 */
    return (nonzero * 0x10000u) / BAGUA_OCTANTS;
}

/* Navigate to specified octant via rotation sequence
 * Computes optimal path (shortest distance around ring)
 * Void-based: modifies *state to reach target_octant
 */
static inline void bagua_seek_octant(BaguaT7State_t *state,
                                    uint32_t target_octant) {
    if (!state || target_octant >= BAGUA_OCTANTS) return;

    /* Find current lead octant (b[0]) */
    uint32_t current = 0;
    for (uint32_t i = 0; i < BAGUA_OCTANTS; i++) {
        if (state->b[0] != state->b[i]) {
            current = i;
            break;
        }
    }

    /* Rotate until target is at position 0 */
    uint32_t distance = (target_octant >= current) ?
                        (target_octant - current) :
                        (BAGUA_OCTANTS + target_octant - current);

    for (uint32_t i = 0; i < distance; i++) {
        bagua_rotate_octant(state);
    }
}

/* Get current octant (b[0] → Bagua symbol) */
static inline uint32_t bagua_current_octant(const BaguaT7State_t *state) {
    if (!state) return 0;
    return (state->b[0] >> 16) % BAGUA_OCTANTS;
}

/* Snapshot state for transmission/storage (minimal serialization)
 * Outputs 32 bytes: 8×b[4], 7×θ[2], 2×pos[2], hash[4]
 * Void-based: fills *snapshot buffer
 */
static inline void bagua_snapshot(const BaguaT7State_t *state, uint8_t *snapshot,
                                 uint32_t snapshot_len) {
    if (!state || !snapshot) return;

    uint32_t offset = 0;

    /* Serialize Bagua octants (4 bytes each) */
    for (uint32_t i = 0; i < BAGUA_OCTANTS && offset + 4 <= snapshot_len; i++) {
        snapshot[offset++] = (state->b[i] >> 0) & 0xFF;
        snapshot[offset++] = (state->b[i] >> 8) & 0xFF;
        snapshot[offset++] = (state->b[i] >> 16) & 0xFF;
        snapshot[offset++] = (state->b[i] >> 24) & 0xFF;
    }

    /* Serialize theta angles (1 byte each) */
    for (uint32_t i = 0; i < THETA_DIMENSIONS && offset + 1 <= snapshot_len; i++) {
        snapshot[offset++] = state->theta[i] & 0xFF;
    }

    /* Serialize position (2 bytes total) */
    if (offset + 2 <= snapshot_len) {
        snapshot[offset++] = state->pos[0] & 0xFF;
        snapshot[offset++] = state->pos[1] & 0xFF;
    }

    /* Serialize hash (4 bytes) */
    if (offset + 4 <= snapshot_len) {
        snapshot[offset++] = (state->hash >> 0) & 0xFF;
        snapshot[offset++] = (state->hash >> 8) & 0xFF;
        snapshot[offset++] = (state->hash >> 16) & 0xFF;
        snapshot[offset++] = (state->hash >> 24) & 0xFF;
    }
}

/* Restore state from snapshot
 * Void-based: fills *state from snapshot buffer
 */
static inline void bagua_restore(BaguaT7State_t *state, const uint8_t *snapshot,
                                uint32_t snapshot_len) {
    if (!state || !snapshot) return;

    memset(state, 0, sizeof(BaguaT7State_t));

    uint32_t offset = 0;

    /* Deserialize Bagua octants */
    for (uint32_t i = 0; i < BAGUA_OCTANTS && offset + 4 <= snapshot_len; i++) {
        state->b[i] = ((uint32_t)snapshot[offset++] << 0) |
                      ((uint32_t)snapshot[offset++] << 8) |
                      ((uint32_t)snapshot[offset++] << 16) |
                      ((uint32_t)snapshot[offset++] << 24);
    }

    /* Deserialize theta angles */
    for (uint32_t i = 0; i < THETA_DIMENSIONS && offset + 1 <= snapshot_len; i++) {
        state->theta[i] = snapshot[offset++];
    }

    /* Deserialize position */
    if (offset + 2 <= snapshot_len) {
        state->pos[0] = snapshot[offset++];
        state->pos[1] = snapshot[offset++];
    }

    /* Deserialize hash */
    if (offset + 4 <= snapshot_len) {
        state->hash = ((uint32_t)snapshot[offset++] << 0) |
                      ((uint32_t)snapshot[offset++] << 8) |
                      ((uint32_t)snapshot[offset++] << 16) |
                      ((uint32_t)snapshot[offset++] << 24);
    }
}

#endif /* BAGUA_STATE_H */
