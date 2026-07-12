/* api_ll_asm.S — ARM64/ARM32 hot paths for api_lowlevel
 * freestanding · no-libc · NEON SIMD · CRC32C HW · cycle counter
 * Register contract: x0=state x1=C x2=H x3=phase x4=att x6=fnv x7=flags
 */

/* ════════════════════════════════════════════════════════════════════════════
 *  ARM64 (AArch64)
 * ════════════════════════════════════════════════════════════════════════════ */
#ifdef __aarch64__

.text
.align 6   /* 64-byte instruction cache alignment */

/* ── api_ll_cycle_read: read ARM virtual counter (no fence hazard) ──────── */
/* uint64_t api_ll_cycle_read(void) */
.global api_ll_cycle_read
.type   api_ll_cycle_read, %function
api_ll_cycle_read:
    isb
    mrs     x0, cntvct_el0
    ret
.size api_ll_cycle_read, . - api_ll_cycle_read

/* ── api_ll_crc32c_hw: CRC32C using ARM hardware instructions ───────────── */
/* uint32_t api_ll_crc32c_hw(uint32_t crc, const void *buf, uint32_t len)   */
/* x0=crc  x1=buf  w2=len  → returns updated crc in w0                      */
.global api_ll_crc32c_hw
.type   api_ll_crc32c_hw, %function
api_ll_crc32c_hw:
    /* 8-byte bulk: processes 8 bytes/iteration — max throughput path */
.L_crc8:
    cmp     w2, #8
    b.lt    .L_crc4
    ldr     x3, [x1], #8
    crc32cx w0, w0, x3
    sub     w2, w2, #8
    b       .L_crc8
.L_crc4:
    cmp     w2, #4
    b.lt    .L_crc1
    ldr     w3, [x1], #4
    crc32cw w0, w0, w3
    sub     w2, w2, #4
.L_crc1:
    cbz     w2, .L_crc_done
.L_crc_byte:
    ldrb    w3, [x1], #1
    crc32cb w0, w0, w3
    subs    w2, w2, #1
    b.ne    .L_crc_byte
.L_crc_done:
    ret
.size api_ll_crc32c_hw, . - api_ll_crc32c_hw

/* ── api_ll_neon_xor_fold: 64-byte NEON XOR fold → 32-bit scalar ───────── */
/* uint32_t api_ll_neon_xor_fold(const void *buf, uint32_t len)              */
/* x0=buf  w1=len  → w0=xor_fold_result                                      */
/* Processes 64 bytes/iteration (4× vld1q, 4× veor) — max NEON bandwidth    */
.global api_ll_neon_xor_fold
.type   api_ll_neon_xor_fold, %function
api_ll_neon_xor_fold:
    movi    v0.16b, #0             /* acc = 0 */
.L_xf64:
    cmp     w1, #64
    b.lt    .L_xf16
    ld1     {v1.16b, v2.16b, v3.16b, v4.16b}, [x0], #64
    eor     v0.16b, v0.16b, v1.16b
    eor     v0.16b, v0.16b, v2.16b
    eor     v0.16b, v0.16b, v3.16b
    eor     v0.16b, v0.16b, v4.16b
    sub     w1, w1, #64
    b       .L_xf64
.L_xf16:
    cmp     w1, #16
    b.lt    .L_xf_tail
    ld1     {v1.16b}, [x0], #16
    eor     v0.16b, v0.16b, v1.16b
    sub     w1, w1, #16
    b       .L_xf16
.L_xf_tail:
    /* fold 128-bit → 64-bit → 32-bit via XOR */
    mov     x2, v0.d[0]
    mov     x3, v0.d[1]
    eor     x2, x2, x3
    lsr     x3, x2, #32
    eor     w0, w2, w3
    ret
.size api_ll_neon_xor_fold, . - api_ll_neon_xor_fold

/* ── api_ll_state_update: NEON PHI32 coherence/entropy step ────────────── */
/* void api_ll_state_update(ApiLLState *st)                                   */
/* ApiLLState layout (packed):                                                */
/*   +0  fnv(8)  +8  crc32c(4)  +12 event_cnt(4)  +16 phase(4)              */
/*   +20 flags(4)  +24 coherence(4)  +28 entropy(4)  +32 attractor(4)        */
.global api_ll_state_update
.type   api_ll_state_update, %function
api_ll_state_update:
    /* x0 = ApiLLState ptr */
    /* Load coherence and entropy */
    ldr     w1, [x0, #24]          /* w1 = coherence (C, Q16.16) */
    ldr     w2, [x0, #28]          /* w2 = entropy (H, Q16.16) */
    ldr     x6, [x0, #0]           /* x6 = FNV accumulator */

    /* PHI32 step: C = C * PHI32 (32-bit wrap — toroidal diffusion) */
    movz    w3, #0x79B9
    movk    w3, #0x9E37, lsl #16   /* w3 = PHI32 = 0x9E3779B9 */
    mul     w1, w1, w3             /* C *= PHI32 (mod 2^32) */

    /* Entropy XOR: H ^= fnv[31:0] */
    eor     w2, w2, w6             /* H ^= fnv_lo32 */

    /* Lyapunov contraction: φ = (1-H)·C — limit to Q16 range via AND */
    movz    w4, #0xFFFF            /* Q16 mask */
    and     w1, w1, w4             /* keep Q16.16 fractional part */
    and     w2, w2, w4

    /* FNV step: fnv ^= (C << 16 | H) */
    orr     w5, w2, w1, lsl #16   /* w5 = C:H packed */
    eor     x6, x6, x5            /* fnv ^= C:H */
    movz    x5, #0x01B3
    movk    x5, #0x0001, lsl #16  /* FNV prime low 32 */
    mul     x6, x6, x5            /* fnv *= FNV_PRIME (low 64-bit) */

    /* Store back */
    str     w1, [x0, #24]
    str     w2, [x0, #28]
    str     x6, [x0, #0]
    ret
.size api_ll_state_update, . - api_ll_state_update

#endif /* __aarch64__ */

/* ════════════════════════════════════════════════════════════════════════════
 *  ARM32 (Thumb-2 / ARMv7-A fallback)
 * ════════════════════════════════════════════════════════════════════════════ */
#ifdef __arm__

.syntax unified
.arch armv7-a
.fpu neon
.text
.align 5

/* ── api_ll_neon_xor_fold: 32-byte NEON XOR fold → 32-bit scalar ───────── */
/* uint32_t api_ll_neon_xor_fold(const void *buf, uint32_t len)              */
/* r0=buf  r1=len  → r0=result                                               */
.global api_ll_neon_xor_fold
.type   api_ll_neon_xor_fold, %function
api_ll_neon_xor_fold:
    push    {r4, lr}
    vmov.i8 q0, #0              @ acc = 0
.L_arm_xf32:
    cmp     r1, #32
    blt     .L_arm_xf_tail
    vld1.8  {d2, d3, d4, d5}, [r0]!  @ load 32 bytes
    veor    q0, q0, q1
    veor    q0, q0, q2
    sub     r1, r1, #32
    b       .L_arm_xf32
.L_arm_xf_tail:
    @ fold q0 (128-bit) → r0 (32-bit)
    vdup.32 d2, d0[0]
    veor    d0, d0, d1          @ d0 ^= d1
    vmov    r4, s0
    vmov    r0, s1
    eor     r0, r0, r4
    pop     {r4, pc}
.size api_ll_neon_xor_fold, . - api_ll_neon_xor_fold

/* ── api_ll_cycle_read: ARM generic timer (if available) ────────────────── */
/* uint64_t api_ll_cycle_read(void) → r0=lo, r1=hi */
.global api_ll_cycle_read
.type   api_ll_cycle_read, %function
api_ll_cycle_read:
    /* Use CNTVCT (virtual count) if CNTVCT_EL0 accessible via MRC */
    /* On most ARMv7 Android devices this is available in userspace */
    mrrc    p15, 1, r0, r1, c14  @ CNTVCT read → r0=lo r1=hi
    bx      lr
.size api_ll_cycle_read, . - api_ll_cycle_read

#endif /* __arm__ */
