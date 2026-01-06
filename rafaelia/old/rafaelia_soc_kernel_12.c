/*
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║ RAFAELIA KERNEL 12.0: METAL GEAR (PURE C LOW-LEVEL)                   ║
 * ╠═══════════════════════════════════════════════════════════════════════╣
 * ║ TYPE:    Bare-Metal SOC Simulator (Fixed-Point 8.8)                   ║
 * ║ TARGET:  Android 15 (Termux/ARM64) | -O3 Optimization                 ║
 * ║ NORM:    ISO/IEC 9899:2011 (C11), IEEE 754, NIST FIPS (Entropy)       ║
 * ║ FEATURE: SIMD-Ready, Zero-GC, Residual Feedback Loop                  ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

// --- HARDWARE TUNING (ARM64) ---
#define FP_SHIFT 8
#define FP_ONE   (1 << FP_SHIFT)
#define FP_MAX   (255 * FP_ONE)
#define UINT16_MAX_VAL 65535

// ANSI COLORS
#define C_CYAN   "\033[36m"
#define C_GREEN  "\033[32m"
#define C_YELLOW "\033[33m"
#define C_RED    "\033[31m"
#define C_RESET  "\033[0m"

// --- DATA STRUCTURES ---

typedef struct {
    uint16_t *T;          // Main Grid (Fixed Point 8.8)
    uint16_t *Flow;       // Buffer de Fluxo (Zero-Alloc reuse)
    uint8_t  *Residual;   // Buffer de Resíduos (2 bits precision recovery)
    int N;
    int size;             // N*N*N
} SystemState;

typedef struct {
    double time_taken;
    double entropy;
    double iops;
    double omega;
} Metrics;

// --- UTILS ---

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Fast Pseudo-RNG (Xorshift) for Init
uint32_t rng_state = 963;
uint32_t fast_rand() {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rng_state = x;
}

// --- PHYSICS KERNEL (LOW LEVEL) ---

// Shannon Entropy (Histogram Based)
double calc_entropy(const uint16_t *data, int size) {
    uint64_t counts[256] = {0};
    
    // Histogram Loop (Vectorizable)
    for (int i = 0; i < size; i++) {
        counts[data[i] >> FP_SHIFT]++; // Analyze Integer Part
    }

    double entropy = 0.0;
    double inv_size = 1.0 / size;

    for (int i = 0; i < 256; i++) {
        if (counts[i] > 0) {
            double p = counts[i] * inv_size;
            entropy -= p * log2(p);
        }
    }
    return entropy;
}

// THE SINGULARITY KERNEL
// Uses 'restrict' to tell compiler pointers do not overlap (enables SIMD)
int step_bitwise_optimized(SystemState *sys, int threshold_fp, int step_idx) {
    uint16_t *restrict T = sys->T;
    uint16_t *restrict Flow = sys->Flow;
    uint8_t  *restrict Res = sys->Residual;
    int N = sys->N;
    int stride_z = N * N;
    int stride_y = N;
    
    int avalanches = 0;
    
    // 1. UPDATE PHASE (Local Physics + Residuals)
    // Flattened loop 1D for max cache locality
    // Avoid boundaries to remove IF checks inside loop (1 to N-1)
    
    // We iterate strictly inside the boundary box to avoid OOB checks
    // This is mathematically cleaner for speed.
    for (int z = 1; z < N-1; z++) {
        for (int y = 1; y < N-1; y++) {
            // Inner loop X is where SIMD happens
            int row_offset = z * stride_z + y * stride_y;
            for (int x = 1; x < N-1; x++) {
                int i = row_offset + x;
                uint16_t val = T[i];
                
                // Active Check
                if ((val >> FP_SHIFT) > (threshold_fp >> FP_SHIFT)) {
                    avalanches++;
                    
                    // --- RESIDUAL PRECISION FEEDBACK ---
                    // Recover previous lost bits
                    uint16_t val_full = val + Res[i]; 
                    
                    // Calc Decay (val >> 2)
                    uint16_t decay = val_full >> 2;
                    
                    // Store NEW residuals (bits 0 and 1 lost in >>2)
                    Res[i] = val_full & 0x03; 
                    
                    // --- PHYSICS ---
                    // Noise: XOR Deterministic
                    uint16_t noise = (val ^ (val >> 4) ^ (step_idx & 0xFF)) & 0x1F;
                    uint16_t leak = val >> 3; // Flow preparation
                    
                    // Update
                    int32_t res = (int32_t)val_full - decay + noise - (leak * 6);
                    
                    // Clamp
                    if (res < 0) res = 0;
                    if (res > FP_MAX) res = FP_MAX;
                    
                    T[i] = (uint16_t)res;
                    Flow[i] = val >> 7; // Store flow for Phase 2
                } else {
                    Flow[i] = 0; // Inactive cells flow nothing
                }
            }
        }
    }

    if (avalanches == 0) return 0; // Early exit

    // 2. DIFFUSION PHASE (Global Coupling)
    // Neighbors: +X, -X, +Y, -Y, +Z, -Z
    // Optimized: Read from Flow, Write to T
    
    for (int z = 1; z < N-1; z++) {
        for (int y = 1; y < N-1; y++) {
            int row_offset = z * stride_z + y * stride_y;
            for (int x = 1; x < N-1; x++) {
                int i = row_offset + x;
                
                // Sum neighbors flow
                uint16_t inflow = 
                    Flow[i + 1] + Flow[i - 1] +           // X neighbors
                    Flow[i + stride_y] + Flow[i - stride_y] + // Y neighbors
                    Flow[i + stride_z] + Flow[i - stride_z];  // Z neighbors
                
                if (inflow > 0) {
                    int32_t res = (int32_t)T[i] + inflow;
                    if (res > FP_MAX) res = FP_MAX;
                    T[i] = (uint16_t)res;
                }
            }
        }
    }
    
    return avalanches;
}

// --- MAIN CONTROLLER ---

int main(int argc, char *argv[]) {
    // Config Defaults
    int N = 64; 
    int ITERS = 100;
    
    // Simple Args Parsing
    if (argc > 1) N = atoi(argv[1]);
    if (argc > 2) ITERS = atoi(argv[2]);

    long long total_cells = (long long)N * N * N;

    // BANNER
    printf(C_CYAN);
    printf("   ╔════ RAFAELIA METAL GEAR (C11) ════╗\n");
    printf("   ║ Arch: ARM64 | Zero-GC | Residual  ║\n");
    printf("   ╚═══════════════════════════════════╝\n");
    printf(C_RESET);
    printf("   [CONF] N=%d (%lld cells) | Iters=%d\n\n", N, total_cells, ITERS);

    // ALLOCATION (Heap - Only once)
    SystemState sys;
    sys.N = N;
    sys.size = N*N*N;
    sys.T = (uint16_t*)calloc(sys.size, sizeof(uint16_t));
    sys.Flow = (uint16_t*)calloc(sys.size, sizeof(uint16_t));
    sys.Residual = (uint8_t*)calloc(sys.size, sizeof(uint8_t));

    if (!sys.T || !sys.Flow || !sys.Residual) {
        printf(C_RED "[ERR] Memory Allocation Failed\n" C_RESET);
        return 1;
    }

    // INIT GRID
    printf(C_YELLOW "[INIT]" C_RESET " Generating Entropy Field...\n");
    for (int i = 0; i < sys.size; i++) {
        sys.T[i] = fast_rand() % FP_MAX;
    }

    // RUN BITWISE KERNEL
    printf(C_GREEN "[RUN]" C_RESET " Executing Singularity Kernel...\n");
    
    int threshold = 200 * FP_ONE;
    double t0 = get_time();
    
    long long total_avalanches = 0;

    for (int i = 0; i < ITERS; i++) {
        int av = step_bitwise_optimized(&sys, threshold, i);
        total_avalanches += av;

        // Adaptive Threshold (Mean) - Simplified sampling for speed
        // Or strict mean (O(N^3) cost). Let's do a fast approximate update
        // to keep speed high, or skip update every few frames.
        if (i % 5 == 0) {
            // Simple mean estimation not to kill perf
            long long sum = 0;
            int step = 10; // Sample 10%
            for(int k=0; k<sys.size; k+=step) sum += sys.T[k];
            threshold = (sum / (sys.size/step));
            if (threshold < 20*FP_ONE) threshold = 20*FP_ONE;
            if (threshold > 240*FP_ONE) threshold = 240*FP_ONE;
        }
        
        // Progress Bar
        if (i % 10 == 0 || i == ITERS-1) {
            double dt = get_time() - t0;
            double iops = (total_cells * (i+1)) / (dt + 1e-9);
            printf("\r   Step %d/%d | IOPS: %.2e | Av: %d   ", i+1, ITERS, iops, av);
            fflush(stdout);
        }
    }
    
    double tf = get_time() - t0;
    double final_entropy = calc_entropy(sys.T, sys.size);
    double total_iops = (total_cells * ITERS) / tf;

    printf("\n\n" C_CYAN "╔════ RESULTS ═══════════════════════╗" C_RESET "\n");
    printf("║ Time:    %.4fs                     ║\n", tf);
    printf("║ IOPS:    " C_GREEN "%.2e" C_RESET " (Raw Throughput) ║\n", total_iops);
    printf("║ Entropy: %.4f bits                ║\n", final_entropy);
    printf("║ Events:  %lld                   ║\n", total_avalanches);
    printf(C_CYAN "╚════════════════════════════════════╝" C_RESET "\n");

    // JSONL Export for Interoperability
    FILE *log = fopen("rafaelia_metrics.log.jsonl", "a");
    if (log) {
        fprintf(log, "{\"ver\":\"12.0-C\",\"ts\":%.0f,\"n\":%d,\"iters\":%d,\"time\":%.4f,\"entropy\":%.4f,\"iops\":%.2e}\n", 
                get_time(), N, ITERS, tf, final_entropy, total_iops);
        fclose(log);
        printf(C_YELLOW "\n[LOG] Saved to rafaelia_metrics.log.jsonl" C_RESET "\n");
    }

    free(sys.T);
    free(sys.Flow);
    free(sys.Residual);
    
    return 0;
}

