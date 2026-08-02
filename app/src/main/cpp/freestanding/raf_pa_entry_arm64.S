.section .text._start,"ax",%progbits
.align 4
.global _start
.type _start,%function
_start:
    mov x0, sp
    bl v
    mov x0, #0
    mov x8, #94
    svc #0
1:  b 1b
.size _start, .-_start
.section .note.GNU-stack,"",%progbits
