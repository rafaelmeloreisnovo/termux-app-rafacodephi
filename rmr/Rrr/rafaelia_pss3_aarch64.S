/* AArch64 low-level PSS3 core
 * Implements:
 *   uint32_t rafaelia_pss3_delta_q16_asm(uint32_t recurrence_q16, uint32_t severity_q16, uint32_t stability_q16)
 *   uint32_t rafaelia_pss3_token_invariant_q16_asm(uint32_t delta_q16)
 * Contract: Q16 fixed-point values in [0..65535].
 */

    .text
    .align 2
    .global rafaelia_pss3_delta_q16_asm
    .type rafaelia_pss3_delta_q16_asm, %function
rafaelia_pss3_delta_q16_asm:
    /* x0=recurrence, x1=severity, x2=stability */
    /* risk = (recurrence * severity) >> 16 */
    umull x3, w0, w1
    lsr   x3, x3, #16

    /* unstability = 65535 - min(stability,65535) */
    mov   w4, #0xFFFF
    cmp   w2, w4
    csel  w5, w2, w4, ls
    sub   w5, w4, w5

    /* delta = (risk * unstability) >> 16 */
    umull x6, w3, w5
    lsr   x6, x6, #16
    mov   w0, w6
    ret

    .global rafaelia_pss3_token_invariant_q16_asm
    .type rafaelia_pss3_token_invariant_q16_asm, %function
rafaelia_pss3_token_invariant_q16_asm:
    /* w0 = delta_q16 */
    mov   w1, #11796
    cmp   w0, w1
    b.hi  1f
    sub   w0, w1, w0
    ret
1:
    mov   w0, wzr
    ret
