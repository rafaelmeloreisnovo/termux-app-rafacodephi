/**
 * ARM/ARM64 Assembly optimizations for low-level operations
 * Bare-metal optimized for Android devices
 */

#ifdef __arm__
.syntax unified
.arch armv7-a
.fpu neon
#elif defined(__aarch64__)
.arch armv8-a
#endif

.text
.align 4

/**
 * Optimized vector dot product for ARM NEON
 * float va_dot_product_asm(const float* v1, const float* v2, size_t n)
 * Parameters:
 *   r0/x0: pointer to v1
 *   r1/x1: pointer to v2
 *   r2/x2: length n
 * Returns: float (in s0/d0)
 */

#ifdef __aarch64__
// ARM64 implementation
.global va_dot_product_asm
.type va_dot_product_asm, %function
va_dot_product_asm:
    // x0 = v1, x1 = v2, x2 = n
    movi v0.4s, #0              // Initialize accumulator to 0
    cbz x2, .Ldot_done          // If n == 0, return
    
    // Process 4 floats at a time
    lsr x3, x2, #2              // x3 = n / 4
    cbz x3, .Ldot_remainder     // If less than 4 elements, go to remainder
    
.Ldot_loop:
    ld1 {v1.4s}, [x0], #16      // Load 4 floats from v1
    ld1 {v2.4s}, [x1], #16      // Load 4 floats from v2
    fmla v0.4s, v1.4s, v2.4s    // Multiply-accumulate
    subs x3, x3, #1
    b.ne .Ldot_loop
    
    // Horizontal add of accumulator
    faddp v0.4s, v0.4s, v0.4s   // Pairwise add
    faddp v0.4s, v0.4s, v0.4s   // Final add
    
.Ldot_remainder:
    // Handle remaining elements (0-3)
    and x3, x2, #3
    cbz x3, .Ldot_done
    
.Ldot_rem_loop:
    ldr s1, [x0], #4
    ldr s2, [x1], #4
    fmadd s0, s1, s2, s0
    subs x3, x3, #1
    b.ne .Ldot_rem_loop
    
.Ldot_done:
    // Result is in s0/v0
    ret

#elif defined(__arm__)
// ARM32 NEON implementation
.global va_dot_product_asm
.type va_dot_product_asm, %function
va_dot_product_asm:
    // r0 = v1, r1 = v2, r2 = n
    vmov.f32 q0, #0.0           // Initialize accumulator
    cmp r2, #0
    bxeq lr                     // Return if n == 0
    
    // Process 4 floats at a time
    lsr r3, r2, #2              // r3 = n / 4
    cmp r3, #0
    beq .Ldot_remainder_arm32
    
.Ldot_loop_arm32:
    vld1.32 {q1}, [r0]!         // Load 4 floats from v1
    vld1.32 {q2}, [r1]!         // Load 4 floats from v2
    vmla.f32 q0, q1, q2         // Multiply-accumulate
    subs r3, r3, #1
    bne .Ldot_loop_arm32
    
    // Horizontal add
    vadd.f32 d0, d0, d1
    vpadd.f32 d0, d0, d0
    
.Ldot_remainder_arm32:
    and r3, r2, #3
    cmp r3, #0
    bxeq lr
    
.Ldot_rem_loop_arm32:
    vldr s2, [r0], #4
    vldr s4, [r1], #4
    vmla.f32 s0, s2, s4
    subs r3, r3, #1
    bne .Ldot_rem_loop_arm32
    
    bx lr

#else
// Generic C fallback (will be linked)
.global va_dot_product_asm
.type va_dot_product_asm, %function
va_dot_product_asm:
    // For non-ARM architectures, this will link to C implementation
    b va_dot_product_fallback
#endif

/**
 * Optimized memory copy for ARM
 * void* va_memcpy_asm(void* dest, const void* src, size_t n)
 */

#ifdef __aarch64__
.global va_memcpy_asm
.type va_memcpy_asm, %function
va_memcpy_asm:
    // x0 = dest, x1 = src, x2 = n
    mov x3, x0                  // Save dest for return
    cbz x2, .Lmemcpy_done
    
    // Copy 16 bytes at a time for large blocks
    cmp x2, #64
    b.lt .Lmemcpy_small
    
.Lmemcpy_large:
    ldp q0, q1, [x1], #32       // Load 32 bytes
    ldp q2, q3, [x1], #32       // Load 32 more bytes
    stp q0, q1, [x0], #32       // Store 32 bytes
    stp q2, q3, [x0], #32       // Store 32 more bytes
    sub x2, x2, #64
    cmp x2, #64
    b.ge .Lmemcpy_large
    
.Lmemcpy_small:
    // Copy remaining bytes
    cbz x2, .Lmemcpy_done
    ldrb w4, [x1], #1
    strb w4, [x0], #1
    sub x2, x2, #1
    b .Lmemcpy_small
    
.Lmemcpy_done:
    mov x0, x3                  // Return dest
    ret

#elif defined(__arm__)
.global va_memcpy_asm
.type va_memcpy_asm, %function
va_memcpy_asm:
    // r0 = dest, r1 = src, r2 = n
    push {r0, r4-r7}
    cmp r2, #0
    beq .Lmemcpy_done_arm32
    
    cmp r2, #32
    blt .Lmemcpy_small_arm32
    
.Lmemcpy_large_arm32:
    vld1.64 {q0, q1}, [r1]!
    vst1.64 {q0, q1}, [r0]!
    sub r2, r2, #32
    cmp r2, #32
    bge .Lmemcpy_large_arm32
    
.Lmemcpy_small_arm32:
    cmp r2, #0
    beq .Lmemcpy_done_arm32
    ldrb r3, [r1], #1
    strb r3, [r0], #1
    sub r2, r2, #1
    b .Lmemcpy_small_arm32
    
.Lmemcpy_done_arm32:
    pop {r0, r4-r7}
    bx lr
#endif

.section .note.GNU-stack,"",%progbits
