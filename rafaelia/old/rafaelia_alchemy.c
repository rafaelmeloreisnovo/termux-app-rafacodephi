/*
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║ RAFAELIA KERNEL v21: THE ALCHEMIST (NEON + NAKED IO)                  ║
 * ╠═══════════════════════════════════════════════════════════════════════╣
 * ║ ARCH:    ARM64 (AArch64) | NEON SIMD (128-bit)                        ║
 * ║ IO:      Manual Syscalls (Open/Write/Close) -> JSON Generation        ║
 * ║ MEM:     Static BSS | Zero-GC                                         ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 */

// --- SYSCALL CONSTANTS (Android/Linux ARM64) ---
#define SYS_OPENAT 56
#define SYS_CLOSE  57
#define SYS_WRITE  64
#define SYS_EXIT   93
#define STDOUT     1
#define AT_FDCWD   -100
#define O_WRONLY   1
#define O_CREAT    64
#define O_TRUNC    512
#define MODE_RW    0644

// --- CONFIG ---
#define GRID_SIZE 1000000 // 1MB
#define ITERS     1000
#define THRESH    200

// --- NEON INTRINSICS (GCC/Clang Native Vector Types) ---
// Define um vetor de 16 bytes (128 bits) que cabe num registrador 'v' do ARM
typedef unsigned char uint8x16_t __attribute__((vector_size(16)));

// --- MEMORY (Static & Aligned to 128-bit) ---
unsigned char __attribute__((aligned(128))) MEMORY_BLOCK[GRID_SIZE];
char IO_BUFFER[4096]; 

// --- NAKED SYSCALL WRAPPERS (No Libc) ---
long sys_write(int fd, const void *buf, long count) {
    register long x0 __asm__("x0") = fd;
    register long x1 __asm__("x1") = (long)buf;
    register long x2 __asm__("x2") = count;
    register long x8 __asm__("x8") = SYS_WRITE;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory");
    return x0;
}

int sys_open(const char *path, int flags, int mode) {
    register long x0 __asm__("x0") = AT_FDCWD;
    register long x1 __asm__("x1") = (long)path;
    register long x2 __asm__("x2") = flags;
    register long x3 __asm__("x3") = mode;
    register long x8 __asm__("x8") = SYS_OPENAT;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x8) : "memory");
    return (int)x0;
}

void sys_close(int fd) {
    register long x0 __asm__("x0") = fd;
    register long x8 __asm__("x8") = SYS_CLOSE;
    __asm__ volatile ("svc #0" :: "r"(x0), "r"(x8) : "memory");
}

void sys_exit(int code) {
    register long x0 __asm__("x0") = code;
    register long x8 __asm__("x8") = SYS_EXIT;
    __asm__ volatile ("svc #0" :: "r"(x0), "r"(x8));
}

// --- ALCHEMIST HELPERS (String/Int Logic) ---
int append_str(char *buf, int offset, const char *s) {
    while(*s) buf[offset++] = *s++;
    return offset;
}

int append_int(char *buf, int offset, long val) {
    if (val == 0) { buf[offset++] = '0'; return offset; }
    char temp[32]; int i=0;
    while(val > 0) { temp[i++] = (val % 10) + '0'; val /= 10; }
    while(i > 0) buf[offset++] = temp[--i];
    return offset;
}

// --- ENTRY POINT ---
void _start() {
    // 1. Header
    sys_write(STDOUT, "\n[RAFAELIA ALCHEMIST v21]\n", 26);
    sys_write(STDOUT, "Arch:  NEON SIMD (128-bit)\n", 27);
    sys_write(STDOUT, "Logic: Transmuted Physics\n", 26);

    // 2. INIT (Vectorized-friendly Init)
    unsigned long seed = 0x963963;
    unsigned char *ptr = MEMORY_BLOCK;
    // Sequential init is fast enough for 1MB
    for(int i=0; i<GRID_SIZE; i++) {
        seed = seed * 1103515245 + 12345;
        *ptr++ = (seed >> 16) & 0xFF;
    }

    sys_write(STDOUT, "[RUN]  Vectorizing Reality...\n", 30);

    // 3. PHYSICS KERNEL (MANUAL NEON VECTORIZATION)
    // Processa 16 células SIMULTANEAMENTE por ciclo
    
    // Constantes Vetoriais
    uint8x16_t v_thresh = {THRESH,THRESH,THRESH,THRESH,THRESH,THRESH,THRESH,THRESH,
                           THRESH,THRESH,THRESH,THRESH,THRESH,THRESH,THRESH,THRESH};
    uint8x16_t v_mask_f = {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};

    for(int step=0; step<ITERS; step++) {
        uint8x16_t *v_ptr = (uint8x16_t*)MEMORY_BLOCK;
        int chunks = GRID_SIZE / 16;
        
        // Vetor de Ruído Rotativo (Pseudo-RNG barato para SIMD)
        unsigned char s = (unsigned char)step;
        uint8x16_t v_noise_base = {s, s+1, s+2, s+3, s+4, s+5, s+6, s+7, 
                                   s+8, s+9, s+10, s+11, s+12, s+13, s+14, s+15};

        // LOOP PRINCIPAL (HOT PATH)
        for(int i=0; i<chunks; i++) {
            uint8x16_t data = v_ptr[i];
            
            // Compare: mask = (data > thresh) ? 0xFF : 0x00
            // Instrução Nativa: vcgt.u8 (Vector Compare Greater Than)
            uint8x16_t mask = data > v_thresh;
            
            // Física: Decay (x -= x >> 2)
            // Instrução Nativa: ushra.u8 (Unsigned Shift Right & Accumulate - logicamente)
            uint8x16_t decay = data >> 2;
            uint8x16_t res = data - decay;
            
            // Física: Noise
            // Instrução Nativa: vand, vadd
            uint8x16_t noise = v_noise_base & v_mask_f;
            res = res + noise;
            
            // Física: Leak (x -= x >> 3)
            uint8x16_t leak = data >> 3;
            res = res - leak;
            
            // Branchless Selection (Bitwise Multiplexer)
            // Se mask é FF, pega res. Se mask é 00, pega data (original).
            // Instrução Nativa: bsl (Bitwise Select)
            data = (mask & res) | (~mask & data);
            
            // Store Back
            v_ptr[i] = data;
        }
    }

    // 4. ALCHEMY REPORT (Manual JSON Construction)
    int off = 0;
    off = append_str(IO_BUFFER, off, "{\n  \"ver\": \"21.0-ALCHEMIST\",\n");
    off = append_str(IO_BUFFER, off, "  \"status\": \"TRANSMUTED\",\n");
    off = append_str(IO_BUFFER, off, "  \"mode\": \"NEON_SIMD_NAKED\",\n");
    off = append_str(IO_BUFFER, off, "  \"grid_size\": ");
    off = append_int(IO_BUFFER, off, GRID_SIZE);
    off = append_str(IO_BUFFER, off, ",\n  \"cycles\": ");
    off = append_int(IO_BUFFER, off, ITERS);
    off = append_str(IO_BUFFER, off, "\n}\n");

    // 5. FILE IO (Direct Syscall)
    int fd = sys_open("rafaelia_alchemy.json", O_WRONLY | O_CREAT | O_TRUNC, MODE_RW);
    if(fd > 0) {
        sys_write(fd, IO_BUFFER, off);
        sys_close(fd);
        sys_write(STDOUT, "[IO]   JSON Artifact Created.\n", 30);
    }

    sys_write(STDOUT, "[DONE] 100% Vectorized Execution.\n", 32);
    sys_exit(0);
}
