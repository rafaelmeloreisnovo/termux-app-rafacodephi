#include "../rmr/Rrr/zipraf_bit_layer_phase_a_v1.h"

static ztr_i32 ztr_check_sample(
    ztr_u8 value, ztr_u8 e1, ztr_u8 e2, ztr_u8 e4, ztr_u8 e8)
{
    ztr_u8 p[8];
    ztr_u8 out = 0u;
    if (ztr_phase_a_decompose(value, p) != ZTR_OK) return 1;
    if (ztr_phase_a_reconstruct(p, &out) != ZTR_OK || out != value) return 2;
    if (ztr_phase_a_reconstruct_q(p, 1u, &out) != ZTR_OK || out != e1) return 3;
    if (ztr_phase_a_reconstruct_q(p, 2u, &out) != ZTR_OK || out != e2) return 4;
    if (ztr_phase_a_reconstruct_q(p, 4u, &out) != ZTR_OK || out != e4) return 5;
    if (ztr_phase_a_reconstruct_q(p, 8u, &out) != ZTR_OK || out != e8) return 6;
    return 0;
}

static ztr_i32 ztr_check_order(const ztr_u8 order[8])
{
    static const ztr_u8 p[8] = {1u,0u,1u,0u,0u,1u,0u,1u};
    ZtrPhaseAAccumulatorV1 acc;
    ztr_u8 out = 0u;
    ztr_u32 i;
    ztr_phase_a_acc_init(&acc);
    for (i = 0u; i < 8u; ++i) {
        ztr_u8 k = order[i];
        if (ztr_phase_a_acc_put(&acc, k, p[k]) != ZTR_OK) return 1;
    }
    if (ztr_phase_a_acc_finish(&acc, 0xffu, &out) != ZTR_OK) return 2;
    return out == 165u ? 0 : 3;
}

ztr_i32 ztr_phase_a_selftest(void)
{
    static const ztr_u8 order_a[8] = {7u,0u,5u,2u,6u,1u,4u,3u};
    static const ztr_u8 order_b[8] = {0u,1u,2u,3u,4u,5u,6u,7u};
    ZtrPhaseAAccumulatorV1 acc;
    ztr_u8 out = 0u;
    ztr_u32 v;
    ztr_u32 q;

    if (ztr_phase_a_header_validate(1u, 30u, 8u) != ZTR_OK) return 10;
    if (ztr_phase_a_header_validate(1u, 60u, 4u) != ZTR_OK) return 11;
    if (ztr_phase_a_header_validate(1u, 120u, 1u) != ZTR_OK) return 12;
    if (ztr_phase_a_header_validate(2u, 30u, 8u) != ZTR_E_VERSION) return 13;
    if (ztr_phase_a_header_validate(1u, 31u, 8u) != ZTR_E_WIDTH) return 14;
    if (ztr_phase_a_header_validate(1u, 30u, 0u) != ZTR_E_Q) return 15;

    /* Producer witness IDs: BLV-00, 01, 55, 80, A5, FF. */
    if (ztr_check_sample(0u,   0u,   0u,   0u,   0u) != 0) return 20;
    if (ztr_check_sample(1u,   0u,   0u,   0u,   1u) != 0) return 21;
    if (ztr_check_sample(85u,  0u,  64u,  80u,  85u) != 0) return 22;
    if (ztr_check_sample(128u, 128u,128u,128u,128u) != 0) return 23;
    if (ztr_check_sample(165u, 128u,128u,160u,165u) != 0) return 24;
    if (ztr_check_sample(255u, 128u,192u,240u,255u) != 0) return 25;

    for (v = 0u; v <= 255u; ++v) {
        ztr_u8 p[8];
        ztr_u8 full = 0u;
        if (ztr_phase_a_decompose((ztr_u8)v, p) != ZTR_OK) return 26;
        if (ztr_phase_a_reconstruct(p, &full) != ZTR_OK) return 27;
        if ((ztr_u32)full != v) return 28;
        for (q = 1u; q <= 8u; ++q) {
            ztr_u8 partial = 0u;
            ztr_u8 mask = ztr_phase_a_qmask((ztr_u8)q);
            if (ztr_phase_a_reconstruct_q(p, (ztr_u8)q, &partial) != ZTR_OK) return 29;
            if (partial != (ztr_u8)(((ztr_u8)v) & mask)) return 30;
        }
    }

    if (ztr_check_order(order_a) != 0) return 31;
    if (ztr_check_order(order_b) != 0) return 32;

    ztr_phase_a_acc_init(&acc);
    if (ztr_phase_a_acc_put(&acc, 7u, 1u) != ZTR_OK) return 40;
    if (ztr_phase_a_acc_put(&acc, 7u, 1u) != ZTR_OK) return 41;
    if (ztr_phase_a_acc_put(&acc, 7u, 0u) != ZTR_E_CONFLICT) return 42;
    if (ztr_phase_a_acc_finish(&acc, 0x80u, &out) != ZTR_E_CONFLICT) return 43;

    ztr_phase_a_acc_init(&acc);
    if (ztr_phase_a_acc_put(&acc, 7u, 1u) != ZTR_OK) return 44;
    if (ztr_phase_a_acc_finish(&acc, 0xc0u, &out) != ZTR_E_INCOMPLETE) return 45;

    return 0;
}

#ifdef ZIPRAF_PHASE_A_HOSTED_MAIN
int main(void)
{
    return (int)ztr_phase_a_selftest();
}
#endif
