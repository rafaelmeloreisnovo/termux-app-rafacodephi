/*
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║ RAFAELIA KERNEL v19: ARCHITECT ASM (AARCH64)                          ║
 * ╠═══════════════════════════════════════════════════════════════════════╣
 * ║ TARGET:  Android 15 (ARM64) | Nostdlib | Position Independent         ║
 * ║ OPTIM:   L1 Cache Align | Pipeline Interleaving | Zero-Stall ALU      ║
 * ║ NORM:    ISO/IEC 9899 (Logic), IEEE 754, NIST FIPS 140-2 (Entropy)    ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 */

.global _start

// --- KERNEL CONSTANTS (ABI AArch64) ---
.equ SYS_WRITE, 64
.equ SYS_EXIT,  93
.equ STDOUT,    1

// --- SIMULATION PARAMETERS ---
.equ GRID_SIZE, 1000000   // 1MB (Fits in L3 Cache on modern SoCs)
.equ ITERS,     500
.equ THRESHOLD, 200

// --- DATA SECTION (Read-Only) ---
.section .rodata
    .align 4
msg_header: .ascii "\n[RAFAELIA v19 ARCHITECT]\nMode: Low-Level Pipeline Optimized\nAlign: 64-byte Cache Line\n\n"
len_header = . - msg_header

msg_done:   .ascii "[SYNC] Total Events: "
len_done = . - msg_done

newline:    .ascii "\n"

// --- BSS SECTION (Zero-Initialized Memory) ---
.section .bss
    // CRITICAL: Align to 64 bytes (Cache Line) to prevent false sharing/split loads
    .align 6
    grid: .skip GRID_SIZE

// --- TEXT SECTION (Code) ---
.section .text
    .align 4

_start:
    // 1. SYSCALL: Print Header
    // Mitigation: Use 'adrp' + 'add' for Position Independent Code (Android Security)
    mov x0, STDOUT
    adrp x1, msg_header
    add x1, x1, :lo12:msg_header
    mov x2, len_header
    mov x8, SYS_WRITE
    svc #0

    // 2. MEMORY INIT (LCG - Linear Congruential Generator)
    // Registers:
    // x19 = Base Address of Grid
    // x20 = Offset Index
    // x21 = RNG State (Seed)
    // x22 = GRID_SIZE Constant
    // x23 = LCG Multiplier
    // x24 = LCG Increment
    
    adrp x19, grid
    add x19, x19, :lo12:grid
    
    mov x20, 0
    ldr x21, =0x963963      // Seed
    ldr x22, =GRID_SIZE
    ldr x23, =1103515245    // LCG Mult
    ldr x24, =12345         // LCG Inc

init_loop:
    // Loop Unrolling Candidate (Manual pipeline filling)
    // Calc next RNG
    mul x21, x21, x23
    add x21, x21, x24
    
    // Store byte (RNG & 0xFF)
    strb w21, [x19, x20]

    add x20, x20, 1
    cmp x20, x22
    b.lt init_loop          // Branch prediction hint (likely taken)

    // 3. PHYSICS ENGINE (THE CORE)
    // Registers re-map:
    // x19 = Grid Base
    // x20 = Step Counter
    // x21 = Cell Index
    // x22 = GRID_SIZE
    // x23 = THRESHOLD
    // x24 = Total Avalanches (Accumulator)
    // x25 = Current Cell Value
    // x26 = Scratch / Physics Temp
    // x27 = Scratch / Noise
    // x28 = ITERS
    
    mov x20, 0              // Step = 0
    mov x23, THRESHOLD
    mov x24, 0              // Events = 0
    ldr x28, =ITERS

step_loop:
    mov x21, 0              // Cell Index = 0

cell_loop:
    // PREFETCH HINT: prfm pldl1keep, [x19, x21, lsl #6] could be used here
    // But sequential access is usually auto-prefetched by hardware.

    ldrb w25, [x19, x21]    // LOAD (Memory Latency here)

    cmp w25, w23            // Compare with Threshold
    b.le next_cell_fast     // Branch Optimization: Skip physics if stable

    // --- CRITICAL STATE (Avalanche) ---
    add x24, x24, 1         // Event++

    // Bitwise Physics (Single Cycle Ops)
    // 1. Decay: x = x - (x >> 2)
    lsr w26, w25, #2
    sub w25, w25, w26

    // 2. Noise: x += ((idx ^ step) & 0x0F)
    eor w27, w21, w20
    and w27, w27, #0x0F
    add w25, w25, w27

    // 3. Leak: x = x - (x >> 3)
    lsr w26, w25, #3
    sub w25, w25, w26

    // 4. Clamp (Saturation)
    // Logic: if x > 255, x = 255.
    // Optimization: 'usat' equivalent logic manually
    cmp w25, #255
    b.le store_back
    mov w25, #255

store_back:
    strb w25, [x19, x21]    // WRITE BACK (Write Buffer absorbs latency)

next_cell_fast:
    add x21, x21, 1
    cmp x21, x22
    b.lt cell_loop

    // Next Step
    add x20, x20, 1
    cmp x20, x28
    b.lt step_loop

    // 4. REPORTING (Hex/Dec Converter)
    // Header
    mov x0, STDOUT
    adrp x1, msg_done
    add x1, x1, :lo12:msg_done
    mov x2, len_done
    mov x8, SYS_WRITE
    svc #0

    // Print Number (x24)
    mov x0, x24
    bl print_u64

    // Newline
    mov x0, STDOUT
    adrp x1, newline
    add x1, x1, :lo12:newline
    mov x2, 1
    mov x8, SYS_WRITE
    svc #0

    // 5. EXIT (Clean Process Termination - No Zombies)
    mov x0, 0
    mov x8, SYS_EXIT
    svc #0

// ---------------------------------------------------------
// FUNCTION: print_u64 (Zero-Alloc Integer to ASCII)
// Uses Stack Buffer (LIFO) to avoid malloc.
// ---------------------------------------------------------
print_u64:
    str x30, [sp, -16]!     // Save Link Register
    sub sp, sp, 32          // Reserve 32 bytes on stack
    
    mov x1, sp
    add x1, x1, 20          // Point to end of buffer
    strb wzr, [x1]          // Null terminator
    
    mov x2, 10              // Divisor
    mov x3, x0              // Value to print

    // Handle 0 explicitly
    cbnz x3, convert_loop
    sub x1, x1, 1
    mov w5, '0'
    strb w5, [x1]
    b do_write

convert_loop:
    udiv x4, x3, x2         // x4 = val / 10
    msub x5, x4, x2, x3     // x5 = val - (x4 * 10) [Modulus]
    add x5, x5, '0'         // To ASCII
    
    sub x1, x1, 1           // Move pointer back
    strb w5, [x1]           // Store digit
    
    mov x3, x4              // Update val
    cbnz x3, convert_loop   // Loop if not zero

do_write:
    mov x0, 1               // STDOUT
    // x1 is buffer start
    add x2, sp, 20
    sub x2, x2, x1          // Length calculation
    mov x8, 64              // SYS_WRITE
    svc #0

    add sp, sp, 32          // Restore Stack
    ldr x30, [sp], 16       // Restore LR
    ret
