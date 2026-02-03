/*
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║ RAFAELIA KERNEL v21b: ALCHEMIST (TIME-AWARE NEON)                     ║
 * ╠═══════════════════════════════════════════════════════════════════════╣
 * ║ ARCH:    ARM64 (AArch64) | NEON SIMD (128-bit)                        ║
 * ║ IO:      Manual Syscalls (Time/File) -> Rich JSON Generation          ║
 * ║ MEM:     Static BSS | Zero-GC                                         ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 */

// --- SYSCALL CONSTANTS ---
#define SYS_OPENAT 56
#define SYS_CLOSE  57
#define SYS_WRITE  64
#define SYS_EXIT   93
#define SYS_CLOCK_GETTIME 113
#define STDOUT     1
#define AT_FDCWD   -100
#define O_WRONLY   1
#define O_CREAT    64
#define O_TRUNC    512
#define MODE_RW    0644
#define CLOCK_MONOTONIC 1

// --- CONFIG ---
#define GRID_SIZE 1000000
#define ITERS     1000
#define THRESH    200

// --- TYPES ---
typedef long time_t;
struct timespec {
    time_t tv_sec;
    long   tv_nsec;
};
typedef unsigned char uint8x16_t __attribute__((vector_size(16)));

// --- MEMORY ---
unsigned char __attribute__((aligned(128))) MEMORY_BLOCK[GRID_SIZE];
char IO_BUFFER[4096]; 

// --- NAKED SYSCALLS ---
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

int sys_clock_gettime(int clk_id, struct timespec *tp) {
    register long x0 __asm__("x0") = clk_id;
    register long x1 __asm__("x1") = (long)tp;
    register long x8 __asm__("x8") = SYS_CLOCK_GETTIME;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x8) : "memory");
    return (int)x0;
}

void sys_exit(int code) {
    register long x0 __asm__("x0") = code;
    register long x8 __asm__("x8") = SYS_EXIT;
    __asm__ volatile ("svc #0" :: "r"(x0), "r"(x8));
}

// --- ALCHEMIST HELPERS ---
int append_str(char *buf, int offset, const char *s) {
    while(*s) buf[offset++] = *s++;
    return offset;
}

int append_long(char *buf, int offset, long val) {
    if (val == 0) { buf[offset++] = '0'; return offset; }
    char temp[32]; int i=0;
    while(val > 0) { temp[i++] = (val % 10) + '0'; val /= 10; }
    while(i > 0) buf[offset++] = temp[--i];
    return offset;
}

// Fixed point print (2 decimal places)
int append_double_approx(char *buf, int offset, long num, long den) {
    long val = num / den;
    long rem = (num % den) * 100 / den; // 2 decimal places
    offset = append_long(buf, offset, val);
    buf[offset++] = '.';
    if(rem < 10) buf[offset++] = '0';
    offset = append_long(buf, offset, rem);
    return offset;
}

// --- ENTRY POINT ---
void _start() {
    sys_write(STDOUT, "\n[RAFAELIA ALCHEMIST v21b]\nArch: NEON (Time-Aware)\n", 49);

    // 1. INIT
    unsigned long seed = 0x963963;
    unsigned char *ptr = MEMORY_BLOCK;
    for(int i=0; i<GRID_SIZE; i++) {
        seed = seed * 1103515245 + 12345;
        *ptr++ = (seed >> 16) & 0xFF;
    }

    // 2. TIMING START
    struct timespec t0, t1;
    sys_clock_gettime(CLOCK_MONOTONIC, &t0);

    // 3. PHYSICS (NEON VECTORIZED)
    uint8x16_t v_thresh = {THRESH,THRESH,THRESH,THRESH,THRESH,THRESH,THRESH,THRESH,
                           THRESH,THRESH,THRESH,THRESH,THRESH,THRESH,THRESH,THRESH};
    uint8x16_t v_mask_f = {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};

    for(int step=0; step<ITERS; step++) {
        uint8x16_t *v_ptr = (uint8x16_t*)MEMORY_BLOCK;
        int chunks = GRID_SIZE / 16;
        unsigned char s = (unsigned char)step;
        uint8x16_t v_noise = {s, s+1, s+2, s+3, s+4, s+5, s+6, s+7, s+8, s+9, s+10, s+11, s+12, s+13, s+14, s+15};

        for(int i=0; i<chunks; i++) {
            uint8x16_t data = v_ptr[i];
            uint8x16_t mask = data > v_thresh;
            uint8x16_t decay = data >> 2;
            uint8x16_t res = data - decay;
            uint8x16_t noise = v_noise & v_mask_f;
            res = res + noise;
            uint8x16_t leak = data >> 3;
            res = res - leak;
            data = (mask & res) | (~mask & data);
            v_ptr[i] = data;
        }
    }

    // 4. TIMING END
    sys_clock_gettime(CLOCK_MONOTONIC, &t1);
    long time_ns = (t1.tv_sec - t0.tv_sec) * 1000000000 + (t1.tv_nsec - t0.tv_nsec);
    long total_ops = (long)GRID_SIZE * ITERS;
    
    // Avoid div by zero
    if(time_ns == 0) time_ns = 1;

    // 5. JSON GENERATION
    int off = 0;
    off = append_str(IO_BUFFER, off, "{\n  \"ver\": \"21.0-ALCHEMIST-B\",\n");
    off = append_str(IO_BUFFER, off, "  \"mode\": \"NEON_SIMD_NAKED\",\n");
    off = append_str(IO_BUFFER, off, "  \"grid\": ");
    off = append_long(IO_BUFFER, off, GRID_SIZE);
    off = append_str(IO_BUFFER, off, ",\n  \"cycles\": ");
    off = append_long(IO_BUFFER, off, ITERS);
    off = append_str(IO_BUFFER, off, ",\n  \"time_ns\": ");
    off = append_long(IO_BUFFER, off, time_ns);
    off = append_str(IO_BUFFER, off, ",\n  \"iops\": ");
    // Calculate IOPS approx: (ops * 1e9) / ns
    long iops = (total_ops / (time_ns/1000)) * 1000000; // Crude integer math to avoid overflow
    off = append_long(IO_BUFFER, off, iops);
    off = append_str(IO_BUFFER, off, "\n}\n");

    // 6. FILE IO
    int fd = sys_open("rafaelia_alchemy.json", O_WRONLY | O_CREAT | O_TRUNC, MODE_RW);
    if(fd > 0) {
        sys_write(fd, IO_BUFFER, off);
        sys_close(fd);
        sys_write(STDOUT, "[IO]   JSON Metrics Exported.\n", 30);
    }

    sys_write(STDOUT, "[DONE] Benchmark Complete.\n", 27);
    sys_exit(0);
}
