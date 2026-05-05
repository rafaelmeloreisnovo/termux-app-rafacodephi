/* AArch64 low-level PSS3 core
 * Hex-oriented and flag-driven implementation.
 * Offsets in rafaelia_pss3_state:
 *  0 recurrence_q16
 *  4 severity_q16
 *  8 stability_q16
 * 12 delta_q16
 * 16 gate
 * 20 token_invariant_q16
 * 24 crc32c
 */

    .text
    .align 2

/* uint32_t rafaelia_pss3_delta_q16_asm(uint32_t recurrence_q16, uint32_t severity_q16, uint32_t stability_q16) */
    .global rafaelia_pss3_delta_q16_asm
    .type rafaelia_pss3_delta_q16_asm, %function
rafaelia_pss3_delta_q16_asm:
    umull x3, w0, w1              // risk 64
    lsr   x3, x3, #16
    mov   w4, #0xFFFF
    cmp   w2, w4                  // NZCV flags
    csel  w5, w2, w4, ls
    sub   w5, w4, w5
    umull x6, w3, w5
    lsr   x6, x6, #16
    mov   w0, w6
    ret

/* uint32_t rafaelia_pss3_token_invariant_q16_asm(uint32_t delta_q16) */
    .global rafaelia_pss3_token_invariant_q16_asm
    .type rafaelia_pss3_token_invariant_q16_asm, %function
rafaelia_pss3_token_invariant_q16_asm:
    mov   w1, #11796
    cmp   w0, w1                  // flags for branch
    b.hi  1f
    sub   w0, w1, w0
    ret
1:
    mov   w0, wzr
    ret

/* void rafaelia_pss3_step_asm(rafaelia_pss3_state *s, uint32_t recurrence_q16, uint32_t severity_q16, uint32_t stability_q16, uint32_t gate)
 * x0=s, w1=recurrence, w2=severity, w3=stability, w4=gate
 */
    .global rafaelia_pss3_step_asm
    .type rafaelia_pss3_step_asm, %function
rafaelia_pss3_step_asm:
    str   w1, [x0, #0]
    str   w2, [x0, #4]
    str   w3, [x0, #8]
    str   w4, [x0, #16]

    /* delta */
    umull x9, w1, w2
    lsr   x9, x9, #16
    mov   w10, #0xFFFF
    cmp   w3, w10
    csel  w11, w3, w10, ls
    sub   w11, w10, w11
    umull x12, w9, w11
    lsr   x12, x12, #16
    mov   w13, w12
    str   w13, [x0, #12]

    /* token invariant */
    mov   w14, #11796
    cmp   w13, w14
    b.hi  2f
    sub   w15, w14, w13
    b     3f
2:
    mov   w15, wzr
3:
    str   w15, [x0, #20]

    /* lightweight deterministic checksum (xor-mix), hex-immediate path */
    ldr   w16, [x0, #0]
    ldr   w17, [x0, #4]
    eor   w16, w16, w17
    ldr   w17, [x0, #8]
    eor   w16, w16, w17
    ldr   w17, [x0, #12]
    eor   w16, w16, w17
    ldr   w17, [x0, #16]
    eor   w16, w16, w17
    ldr   w17, [x0, #20]
    eor   w16, w16, w17
    movz  w17, #0x6F41
    movk  w17, #0x1EDC, lsl #16
    eor   w16, w16, w17
    str   w16, [x0, #24]

    /* raw hexadecimal instruction words for auditability */
    .inst 0xD503201F   // nop
    ret
