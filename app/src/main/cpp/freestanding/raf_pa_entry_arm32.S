.syntax unified
.arm
.section .text._start,"ax",%progbits
.align 2
.global _start
.type _start,%function
_start:
    mov r0, sp
    bl v
    mov r0, #0
    mov r7, #248
    svc #0
1:  b 1b
.size _start, .-_start
.section .note.GNU-stack,"",%progbits
