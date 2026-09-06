/* RAFCODEPHI freestanding ELF entry — ARM32 / AArch64 */

#if defined(__aarch64__)
    .text
    .align 2
    .global _start
    .type _start, %function
_start:
    mov x29, xzr
    mov x30, xzr
    bl main
    mov x8, #94          /* __NR_exit_group */
    svc #0
1:  b 1b
    .size _start, .-_start

#elif defined(__arm__)
    .syntax unified
    .text
    .align 2
    .global _start
    .type _start, %function
_start:
    mov r11, #0
    mov lr, #0
    bl main
    mov r7, #248         /* __NR_exit_group */
    svc #0
1:  b 1b
    .size _start, .-_start

#else
#error "RAFCODEPHI start_arm.S supports ARM32/AArch64 only"
#endif
