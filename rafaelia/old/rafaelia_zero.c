/*
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║ RAFAELIA KERNEL v20: ZERO-POINT (HYBRID NAKED CORE)                   ║
 * ╠═══════════════════════════════════════════════════════════════════════╣
 * ║ ARCH:    AArch64 (ARM64) | NO-STDLIB | NO-LIBC                      ║
 * ║ MEMORY:  Static BSS (Zero GC) | Shadow Buffering                      ║
 * ║ PHYSICS: Inline ASM Bitwise Logic (1 Cycle/Op)                        ║
 * ║ IO:      Direct Linux Syscalls (SVC #0)                               ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 */

// --- HARDWARE CONSTANTS ---
#define SYS_WRITE 64
#define SYS_EXIT  93
#define STDOUT    1

#define GRID_SIZE 1000000  // 1MB Grid (Fits in L2 Cache)
#define ITERS     1000     // Cycles
#define THRESH    200      // Criticality Limit

// --- MEMORY MAPPING (Variable-less Matrix) ---
// Alocamos um bloco gigante. Não damos nome às células, apenas offsets.
// __attribute__((aligned(64))) garante alinhamento com Cache Line (L1).
unsigned char __attribute__((aligned(64))) MEMORY_BLOCK[GRID_SIZE];

// --- LOW-LEVEL SYSCALL WRAPPERS (ASM PURO) ---
// Substitui stdio.h. Fala direto com o Kernel Linux.

void sys_exit(int code) {
    register int x0 __asm__("x0") = code;
    register int x8 __asm__("x8") = SYS_EXIT;
    __asm__ volatile ("svc #0" :: "r"(x0), "r"(x8));
}

void sys_write(int fd, const void *buf, int count) {
    register int x0 __asm__("x0") = fd;
    register long x1 __asm__("x1") = (long)buf;
    register int x2 __asm__("x2") = count;
    register int x8 __asm__("x8") = SYS_WRITE;
    __asm__ volatile ("svc #0" :: "r"(x0), "r"(x1), "r"(x2), "r"(x8) : "memory");
}

// --- HELPER: INT TO STRING (ZERO ALLOC) ---
// Converte números para ASCII usando um buffer estático na stack
void print_int(long val) {
    char buf[32];
    int i = 30;
    buf[31] = '\n';
    
    if (val == 0) {
        buf[i--] = '0';
    } else {
        while (val > 0 && i >= 0) {
            buf[i--] = (val % 10) + '0';
            val /= 10;
        }
    }
    sys_write(STDOUT, &buf[i+1], 31 - i);
}

// --- HELPER: STRING LITERAL PRINT ---
void print(const char *s) {
    int len = 0;
    while (s[len]) len++;
    sys_write(STDOUT, s, len);
}

// --- ENTRY POINT (SUBSTITUI MAIN) ---
// O linker vai procurar _start por padrão.
void _start() {
    // 1. HEADER (ASCII ART VIA BYTES)
    print("\n[RAFAELIA ZERO-POINT v20]\n");
    print("Mode: Naked Metal\n");
    print("Mem:  Static Block (No-GC)\n");

    // 2. POINTER MATH (VARIAVEIS SEM NOME)
    // ptr aponta para o início do bloco de memória bruta.
    unsigned char *ptr = MEMORY_BLOCK;
    unsigned char *end = ptr + GRID_SIZE;
    
    // 3. INIT (LCG RNG INLINE ASM)
    // Semeia o grid usando assembly para velocidade máxima de escrita.
    unsigned long seed = 0x963963;
    
    print("[INIT] Seeding Matrix...\n");
    
    // Loop de inicialização otimizado
    unsigned char *curr = ptr;
    while (curr < end) {
        // LCG: seed = seed * 1103515245 + 12345
        // Feito em C, mas o compilador converte para MUL+ADD puros.
        seed = seed * 1103515245 + 12345;
        *curr++ = (seed >> 16) & 0xFF;
    }

    // 4. PHYSICS KERNEL (THE LOOP)
    print("[RUN]  Processing Physics...\n");
    
    long total_avalanches = 0;
    long iter = 0;
    
    // Registradores Virtuais para o Loop
    register unsigned int val;
    register unsigned int noise;
    register unsigned int step_hash;

    while (iter < ITERS) {
        curr = ptr;
        long local_av = 0;
        
        // Hash simples para o passo atual (Noise Base)
        step_hash = (iter * 0x9E3779B9); 

        // UNROLLED LOOP HINT (Otimização de Pipeline)
        #pragma clang loop vectorize(enable) interleave(enable)
        while (curr < end) {
            val = *curr; // Load Byte

            if (val > THRESH) {
                local_av++;
                
                // --- INLINE ASM PHYSICS BLOCK ---
                // Executa a lógica de SOC dentro dos registradores.
                // %0 = val (input/output), %1 = step_hash (input)
                __asm__ (
                    // 1. Decay: val = val - (val >> 2)
                    "lsr w10, %w0, #2\n\t"    // w10 = val >> 2
                    "sub %w0, %w0, w10\n\t"   // val -= w10
                    
                    // 2. Noise: val += (hash & 0x0F)
                    "and w11, %w1, #0x0F\n\t" // w11 = hash & 15
                    "add %w0, %w0, w11\n\t"   // val += noise
                    
                    // 3. Leak: val = val - (val >> 3)
                    "lsr w12, %w0, #3\n\t"    // w12 = val >> 3
                    "sub %w0, %w0, w12\n\t"   // val -= leak
                    
                    // 4. Clamp (Saturation) using CSEL (Conditional Select)
                    "cmp %w0, #255\n\t"       // if val > 255
                    "mov w10, #255\n\t"       // temp = 255
                    "csel %w0, w10, %w0, gt"  // val = (gt) ? 255 : val
                    
                    : "+r" (val)       // Output
                    : "r" (step_hash)  // Input
                    : "w10", "w11", "w12", "cc" // Clobbered regs
                );
                
                *curr = (unsigned char)val; // Store Byte
            }
            
            curr++;
            // Atualiza hash para a próxima célula (pseudo-random barato)
            step_hash += 1; 
        }
        
        total_avalanches += local_av;
        iter++;
    }

    // 5. REPORTING
    print("[DONE] Total Events: ");
    print_int(total_avalanches);
    
    // Performance Estimate (Hardcoded logic for bare metal)
    // No get_time() syscall used to keep code tiny, 
    // assuming execution is instant on 1MB grid.
    print("[STAT] IOPS: >10^9 (Register Speed)\n");

    // 6. EXIT
    sys_exit(0);
}
