/*
 * ZIPRAF Bit Layer Phase-A Termux runtime adapter V1.
 *
 * Independent consumer implementation of the geometry-independent subset of:
 * rafaelmeloreisnovo/RafPolimata
 * canonical/zipraf-hw-v1/vectors/bit_layer_reference_vectors_phase_a_v1.json
 * Producer merge: d52afbc38acf6d9580b32cbf9f7f259fa4afdf4b
 * Producer Git blob: 4c3ba2202afb6f62bb465d435273031ebb16c3b3
 *
 * This file does not define canonical block mask M or geometry G(M).
 * No includes. No libc. No libm. No heap. No syscall.
 */
#ifndef RAFCODEPHI_ZIPRAF_BIT_LAYER_PHASE_A_V1_H
#define RAFCODEPHI_ZIPRAF_BIT_LAYER_PHASE_A_V1_H 1

typedef unsigned char ztr_u8;
typedef unsigned int  ztr_u32;
typedef signed int    ztr_i32;

typedef char ztr_assert_u8[(sizeof(ztr_u8) == 1u) ? 1 : -1];
typedef char ztr_assert_u32[(sizeof(ztr_u32) == 4u) ? 1 : -1];

#define ZTR_OK 0
#define ZTR_E_ARGUMENT -1
#define ZTR_E_VERSION -2
#define ZTR_E_WIDTH -3
#define ZTR_E_Q -4
#define ZTR_E_LAYER -5
#define ZTR_E_CONFLICT -6
#define ZTR_E_INCOMPLETE -7

typedef struct {
    ztr_u8 seen;
    ztr_u8 value;
    ztr_u8 conflict;
    ztr_u8 reserved;
} ZtrPhaseAAccumulatorV1;

static inline ztr_i32 ztr_phase_a_width_valid(ztr_u32 width)
{
    return (width == 30u || width == 60u || width == 120u) ? 1 : 0;
}

static inline ztr_i32 ztr_phase_a_q_valid(ztr_u8 q)
{
    return (q >= 1u && q <= 8u) ? 1 : 0;
}

static inline ztr_i32 ztr_phase_a_header_validate(
    ztr_u32 version, ztr_u32 width, ztr_u8 q)
{
    if (version != 1u) return ZTR_E_VERSION;
    if (!ztr_phase_a_width_valid(width)) return ZTR_E_WIDTH;
    if (!ztr_phase_a_q_valid(q)) return ZTR_E_Q;
    return ZTR_OK;
}

static inline ztr_u8 ztr_phase_a_qmask(ztr_u8 q)
{
    if (!ztr_phase_a_q_valid(q)) return 0u;
    return (ztr_u8)(0xffu << (8u - (ztr_u32)q));
}

static inline ztr_i32 ztr_phase_a_decompose(ztr_u8 value, ztr_u8 out[8])
{
    if (!out) return ZTR_E_ARGUMENT;
    out[0] = (ztr_u8)((value >> 0u) & 1u);
    out[1] = (ztr_u8)((value >> 1u) & 1u);
    out[2] = (ztr_u8)((value >> 2u) & 1u);
    out[3] = (ztr_u8)((value >> 3u) & 1u);
    out[4] = (ztr_u8)((value >> 4u) & 1u);
    out[5] = (ztr_u8)((value >> 5u) & 1u);
    out[6] = (ztr_u8)((value >> 6u) & 1u);
    out[7] = (ztr_u8)((value >> 7u) & 1u);
    return ZTR_OK;
}

static inline ztr_i32 ztr_phase_a_reconstruct(
    const ztr_u8 planes[8], ztr_u8 *out)
{
    ztr_u32 k;
    ztr_u32 value = 0u;
    if (!planes || !out) return ZTR_E_ARGUMENT;
    for (k = 0u; k < 8u; ++k) {
        if (planes[k] > 1u) return ZTR_E_LAYER;
        value |= ((ztr_u32)planes[k]) << k;
    }
    *out = (ztr_u8)value;
    return ZTR_OK;
}

static inline ztr_i32 ztr_phase_a_reconstruct_q(
    const ztr_u8 planes[8], ztr_u8 q, ztr_u8 *out)
{
    ztr_u8 value;
    ztr_i32 rc;
    if (!out) return ZTR_E_ARGUMENT;
    if (!ztr_phase_a_q_valid(q)) return ZTR_E_Q;
    rc = ztr_phase_a_reconstruct(planes, &value);
    if (rc != ZTR_OK) return rc;
    *out = (ztr_u8)(value & ztr_phase_a_qmask(q));
    return ZTR_OK;
}

static inline void ztr_phase_a_acc_init(ZtrPhaseAAccumulatorV1 *acc)
{
    if (!acc) return;
    acc->seen = 0u;
    acc->value = 0u;
    acc->conflict = 0u;
    acc->reserved = 0u;
}

static inline ztr_i32 ztr_phase_a_acc_put(
    ZtrPhaseAAccumulatorV1 *acc, ztr_u8 layer, ztr_u8 bit)
{
    ztr_u8 mask;
    ztr_u8 prior;
    if (!acc) return ZTR_E_ARGUMENT;
    if (layer > 7u || bit > 1u) return ZTR_E_LAYER;
    mask = (ztr_u8)(1u << layer);

    if ((acc->seen & mask) != 0u) {
        prior = (ztr_u8)((acc->value >> layer) & 1u);
        if (prior != bit) {
            acc->conflict = (ztr_u8)(acc->conflict | mask);
            return ZTR_E_CONFLICT;
        }
        return ZTR_OK;
    }

    acc->seen = (ztr_u8)(acc->seen | mask);
    if (bit != 0u) acc->value = (ztr_u8)(acc->value | mask);
    return ZTR_OK;
}

static inline ztr_i32 ztr_phase_a_acc_finish(
    const ZtrPhaseAAccumulatorV1 *acc, ztr_u8 required, ztr_u8 *out)
{
    if (!acc || !out) return ZTR_E_ARGUMENT;
    if (acc->conflict != 0u) return ZTR_E_CONFLICT;
    if ((acc->seen & required) != required) return ZTR_E_INCOMPLETE;
    *out = (ztr_u8)(acc->value & required);
    return ZTR_OK;
}

#endif
