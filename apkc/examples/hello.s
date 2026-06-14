; hello.s — minimal NativeActivity ARM assembly
; Symbols:
;   ANativeActivity_onCreate  — Android calls this on launch
;   android_main              — optional secondary entry
;
; Build with:
;   ./apkc64 hello.s -o hello.apk -p com.example.hello -l Hello -n hello -both
;
; Architecture: arm64 (assembled with -64 or -both)
; The compiler auto-selects the arch pass; write portable mnemonics.

.sym1
ANativeActivity_onCreate:
    ; x0 = ANativeActivity*, x1 = savedState, x2 = savedStateSize
    ; Just return immediately — freestanding stub
    ret

.sym2
android_main:
    ; x0 = android_app*
    ret

; — You can add more code below. Labels become branch targets.
; Example: branchless min(x0, x1) -> x0
branchless_min:
    stp x29, x30, [sp, #-16]!
    cmp x0, x1
    csel x0, x0, x1, lt
    ldp x29, x30, [sp], #16
    ret

; Example: syscall write(1, msg, 5)
; msg: .word 0x6c6c6548   ; 'Hell'
;      .word 0x0000000a   ; 'o\n'
; Use .sym1/.sym2 markers above the symbols Android needs.
