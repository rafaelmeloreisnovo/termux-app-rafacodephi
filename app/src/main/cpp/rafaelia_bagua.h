/*
 * RAFAELIA Bagua bare-metal core
 * Author: Rafael Melo Reis — RAFCODE-Φ / ∆RafaelVerboΩ
 * Date: 2026-07-10
 *
 * Freestanding-friendly C99/C11 header:
 * - no heap
 * - no malloc
 * - deterministic state transition
 * - 3-bit trigram rotation
 * - two Yin/Yang bit layers
 * - Q15 rotation by 30 degrees
 * - seven modular phases for a discrete T^7 representation
 *
 * Epistemic boundary:
 * This file implements a machine model. It does not claim that 42 stable
 * orbits or Omega=23.158 have been proved. Those claims require a defined
 * return map, orbit enumeration, Jacobian/spectral tests and reproducible
 * evidence.
 */

#ifndef RAFAELIA_BAGUA_H
#define RAFAELIA_BAGUA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RAF_BAGUA_MASK       UINT8_C(0x07)
#define RAF_YINYANG_MASK     UINT8_C(0x01)
#define RAF_T7_DIMENSIONS    7u
#define RAF_Q15_ONE          INT32_C(32768)
#define RAF_Q15_COS_30       INT32_C(28378) /* round(sqrt(3)/2 * 32768) */
#define RAF_Q15_SIN_30       INT32_C(16384) /* 1/2 * 32768 */

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert(RAF_T7_DIMENSIONS == 7u, "T7 must contain seven phases");
_Static_assert((RAF_BAGUA_MASK + 1u) == 8u, "Bagua requires eight states");
#endif

typedef struct RafVec2Q15 {
    int32_t x;
    int32_t y;
} RafVec2Q15;

typedef struct RafBaguaState {
    uint8_t trigram;              /* 0..7 */
    uint8_t yin_layer;            /* 0 or 1 */
    uint8_t yang_layer;           /* 0 or 1 */
    uint8_t direction;            /* 0 = ROR, 1 = ROL */
    uint16_t phase[RAF_T7_DIMENSIONS];
    RafVec2Q15 vector;
    uint64_t audit;
} RafBaguaState;

/* Rotate a three-bit trigram left by one position. */
static inline uint8_t raf_rol3(uint8_t value)
{
    value &= RAF_BAGUA_MASK;
    return (uint8_t)(((value << 1u) | (value >> 2u)) & RAF_BAGUA_MASK);
}

/* Rotate a three-bit trigram right by one position. */
static inline uint8_t raf_ror3(uint8_t value)
{
    value &= RAF_BAGUA_MASK;
    return (uint8_t)(((value >> 1u) | (value << 2u)) & RAF_BAGUA_MASK);
}

static inline uint8_t raf_yin_yang_not(uint8_t bit)
{
    return (uint8_t)((bit ^ UINT8_C(1)) & RAF_YINYANG_MASK);
}

static inline uint8_t raf_yin_yang_xor(uint8_t a, uint8_t b)
{
    return (uint8_t)((a ^ b) & RAF_YINYANG_MASK);
}

/*
 * Q15 multiplication with a 64-bit intermediate.
 * The result is deterministic and avoids signed 32-bit multiplication
 * overflow for ordinary Q15 state ranges.
 */
static inline int32_t raf_q15_mul(int32_t a, int32_t b)
{
    int64_t product = (int64_t)a * (int64_t)b;
    return (int32_t)(product / (int64_t)RAF_Q15_ONE);
}

/*
 * Orthogonal 30-degree rotation approximation in Q15:
 * [ cos -sin ] [x]
 * [ sin  cos ] [y]
 *
 * Rotation alone is norm-preserving up to quantization error. It is not a
 * contraction. A contraction must be introduced explicitly with kappa.
 */
static inline RafVec2Q15 raf_rotate30_q15(RafVec2Q15 input)
{
    RafVec2Q15 output;
    output.x = raf_q15_mul(RAF_Q15_COS_30, input.x)
             - raf_q15_mul(RAF_Q15_SIN_30, input.y);
    output.y = raf_q15_mul(RAF_Q15_SIN_30, input.x)
             + raf_q15_mul(RAF_Q15_COS_30, input.y);
    return output;
}

/* Apply an explicit Q15 contraction/expansion factor kappa. */
static inline RafVec2Q15 raf_scale_q15(RafVec2Q15 input, int32_t kappa_q15)
{
    RafVec2Q15 output;
    output.x = raf_q15_mul(input.x, kappa_q15);
    output.y = raf_q15_mul(input.y, kappa_q15);
    return output;
}

/*
 * Deterministic non-cryptographic audit mixer.
 * It is an integrity/state-mixing primitive, not compression and not an
 * entropy-cancellation mechanism.
 */
static inline uint64_t raf_audit_mix64(uint64_t state, uint64_t word)
{
    state ^= word + UINT64_C(0x9E3779B97F4A7C15)
                  + (state << 6u)
                  + (state >> 2u);
    state ^= state >> 30u;
    state *= UINT64_C(0xBF58476D1CE4E5B9);
    state ^= state >> 27u;
    state *= UINT64_C(0x94D049BB133111EB);
    state ^= state >> 31u;
    return state;
}

/* Seven unsigned phases; uint16_t overflow implements modulo 2^16. */
static inline void raf_t7_advance(uint16_t phase[RAF_T7_DIMENSIONS],
                                  const uint16_t omega[RAF_T7_DIMENSIONS])
{
    uint8_t i;
    for (i = 0u; i < RAF_T7_DIMENSIONS; ++i) {
        phase[i] = (uint16_t)(phase[i] + omega[i]);
    }
}

/*
 * One auditable hybrid transition.
 *
 * feedback bit 0 controls the Yin layer.
 * feedback bit 1 couples Yin into Yang.
 * feedback bit 2 selects Bagua rotation direction.
 * feedback bits 3..7 perturb the trigram before circular rotation.
 */
static inline void raf_bagua_step(RafBaguaState *state,
                                  uint8_t feedback,
                                  const uint16_t omega[RAF_T7_DIMENSIONS],
                                  int32_t kappa_q15)
{
    uint8_t previous_yin;
    uint8_t injected;
    uint64_t packed;

    if (state == (RafBaguaState *)0) {
        return;
    }

    previous_yin = (uint8_t)(state->yin_layer & RAF_YINYANG_MASK);
    state->yin_layer = raf_yin_yang_xor(previous_yin, feedback);
    state->yang_layer = raf_yin_yang_xor(
        state->yang_layer,
        (uint8_t)(previous_yin ^ ((feedback >> 1u) & RAF_YINYANG_MASK))
    );

    state->direction = (uint8_t)((feedback >> 2u) & RAF_YINYANG_MASK);
    injected = (uint8_t)((state->trigram ^ (feedback >> 3u))
                         & RAF_BAGUA_MASK);
    state->trigram = state->direction ? raf_rol3(injected)
                                      : raf_ror3(injected);

    raf_t7_advance(state->phase, omega);
    state->vector = raf_rotate30_q15(state->vector);
    state->vector = raf_scale_q15(state->vector, kappa_q15);

    packed = (uint64_t)state->trigram
           | ((uint64_t)state->yin_layer << 8u)
           | ((uint64_t)state->yang_layer << 9u)
           | ((uint64_t)state->direction << 10u)
           | ((uint64_t)feedback << 16u);
    state->audit = raf_audit_mix64(state->audit, packed);
}

/* Exact inverse checks for the discrete trigram rotations. */
static inline uint8_t raf_bagua_rotation_selftest(void)
{
    uint8_t value;
    for (value = 0u; value < 8u; ++value) {
        if (raf_ror3(raf_rol3(value)) != value) {
            return UINT8_C(0);
        }
        if (raf_rol3(raf_ror3(value)) != value) {
            return UINT8_C(0);
        }
    }
    return UINT8_C(1);
}

#ifdef __cplusplus
}
#endif

#endif /* RAFAELIA_BAGUA_H */
