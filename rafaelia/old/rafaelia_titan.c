/*
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║ RAFAELIA KERNEL 13.0: TITAN (MULTI-CORE OPENMP)                       ║
 * ╠═══════════════════════════════════════════════════════════════════════╣
 * ║ TYPE:    Parallel Bare-Metal SOC Simulator                            ║
 * ║ TARGET:  Android 15 (Termux/ARM64) | 8-Core Optimization              ║
 * ║ NORM:    ISO/IEC 9899:2011, OpenMP 4.5 API                            ║
 * ║ FEATURE: Multi-Threading, Cache-Line Alignment, Billion-Scale IOPS    ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <omp.h>  // HEADER DO OPENMP

// --- HARDWARE TUNING ---
#define FP_SHIFT 8
#define FP_ONE   (1 << FP_SHIFT)
#define FP_MAX   (255 * FP_ONE)

// ANSI COLORS
#define C_CYAN   "\033[36m"
#define C_GREEN  "\033[32m"
#define C_YELLOW "\033[33m"
#define C_RED    "\033[31m"
#define C_RESET  "\033[0m"

typedef struct {
    uint16_t *T;
    uint16_t *Flow;
    uint8_t  *Residual;
    int N;
    int size;
} SystemState;

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Thread-Safe Random via Hashing (Avoids lock contention)
static inline uint16_t fast_hash(int i, int step) {
    uint32_t x = i ^ (step * 0x5DEECE66D);
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x & 0x1F; // 5 bits noise
}

// Shannon Entropy (Parallel Reduction)
double calc_entropy(const uint16_t *data, int size) {
    uint64_t counts[256] = {0};
    
    #pragma omp parallel
    {
        uint64_t local_counts[256] = {0};
        #pragma omp for nowait
        for (int i = 0; i < size; i++) {
            local_counts[data[i] >> FP_SHIFT]++;
        }
        #pragma omp critical
        {
            for(int j=0; j<256; j++) counts[j] += local_counts[j];
        }
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

// THE PARALLEL KERNEL
int step_titan_parallel(SystemState *sys, int threshold_fp, int step_idx) {
    uint16_t *restrict T = sys->T;
    uint16_t *restrict Flow = sys->Flow;
    uint8_t  *restrict Res = sys->Residual;
    int N = sys->N;
    int stride_z = N * N;
    int stride_y = N;
    
    int total_avalanches = 0;

    // PHASE 1: UPDATE (Parallel Collapse)
    // Distribui blocos Z/Y entre todos os núcleos disponiveis
    #pragma omp parallel for collapse(2) reduction(+:total_avalanches)
    for (int z = 1; z < N-1; z++) {
        for (int y = 1; y < N-1; y++) {
            int row_offset = z * stride_z + y * stride_y;
            // Vectorization (SIMD) happens here
            for (int x = 1; x < N-1; x++) {
                int i = row_offset + x;
                uint16_t val = T[i];
                
                if ((val >> FP_SHIFT) > (threshold_fp >> FP_SHIFT)) {
                    total_avalanches++;
                    
                    uint16_t val_full = val + Res[i]; 
                    uint16_t decay = val_full >> 2;
                    Res[i] = val_full & 0x03; 
                    
                    uint16_t noise = fast_hash(i, step_idx);
                    uint16_t leak = val >> 3; 
                    
                    int32_t res = (int32_t)val_full - decay + noise - (leak * 6);
                    if (res < 0) res = 0;
                    if (res > FP_MAX) res = FP_MAX;
                    
                    T[i] = (uint16_t)res;
                    Flow[i] = val >> 7; 
                } else {
                    Flow[i] = 0;
                }
            }
        }
    }

    if (total_avalanches == 0) return 0;

    // PHASE 2: DIFFUSION (Parallel Collapse)
    #pragma omp parallel for collapse(2)
    for (int z = 1; z < N-1; z++) {
        for (int y = 1; y < N-1; y++) {
            int row_offset = z * stride_z + y * stride_y;
            for (int x = 1; x < N-1; x++) {
                int i = row_offset + x;
                uint16_t inflow = 
                    Flow[i+1] + Flow[i-1] +
                    Flow[i+stride_y] + Flow[i-stride_y] +
                    Flow[i+stride_z] + Flow[i-stride_z];
                
                if (inflow > 0) {
                    int32_t res = (int32_t)T[i] + inflow;
                    if (res > FP_MAX) res = FP_MAX;
                    T[i] = (uint16_t)res;
                }
            }
        }
    }
    
    return total_avalanches;
}

int main(int argc, char *argv[]) {
    int N = 100; 
    int ITERS = 500;
    if (argc > 1) N = atoi(argv[1]);
    if (argc > 2) ITERS = atoi(argv[2]);

    long long total_cells = (long long)N * N * N;
    int procs = omp_get_num_procs();

    printf(C_CYAN);
    printf("   ╔════ RAFAELIA TITAN (v13.0) ═══════╗\n");
    printf("   ║ Arch: Multi-Core OpenMP (%%d Cores) ║\n", procs);
    printf("   ╚═══════════════════════════════════╝\n");
    printf(C_RESET);
    printf("   [CONF] N=%%d | Iters=%%d | Mem: %%.2f MB\n", 
           N, ITERS, (double)(total_cells * sizeof(uint16_t) * 2) / (1024*1024));

    SystemState sys;
    sys.N = N;
    sys.size = N*N*N;
    sys.T = calloc(sys.size, sizeof(uint16_t));
    sys.Flow = calloc(sys.size, sizeof(uint16_t));
    sys.Residual = calloc(sys.size, sizeof(uint8_t));
    
    if (!sys.T || !sys.Flow || !sys.Residual) {
        printf(C_RED "[ERR] Memory Fail\n" C_RESET); return 1;
    }

    // Parallel Init
    #pragma omp parallel for
    for (int i = 0; i < sys.size; i++) sys.T[i] = fast_hash(i, 0) * 100;

    printf(C_GREEN "[RUN]" C_RESET " Igniting Parallel Kernel...\n");
    
    int threshold = 200 * FP_ONE;
    double t0 = get_time();
    long long total_ev = 0;

    for (int i = 0; i < ITERS; i++) {
        total_ev += step_titan_parallel(&sys, threshold, i);
        
        if (i %% 20 == 0 || i == ITERS-1) {
            double dt = get_time() - t0;
            double iops = (total_cells * (i+1)) / (dt + 1e-9);
            printf("\r   Step %%d/%%d | IOPS: " C_YELLOW "%%.2e" C_RESET, i+1, ITERS, iops);
            fflush(stdout);
        }
    }
    
    double tf = get_time() - t0;
    double iops = (total_cells * ITERS) / tf;
    double ent = calc_entropy(sys.T, sys.size);

    printf("\n\n" C_CYAN "╔════ TITAN RESULTS ═════════════════╗" C_RESET "\n");
    printf("║ Time:    %%.4fs                     ║\n", tf);
    printf("║ IOPS:    " C_GREEN "%%.2e" C_RESET "                  ║\n", iops);
    printf("║ Cores:   %%d Active                  ║\n", procs);
    printf("║ Entropy: %%.4f bits                ║\n", ent);
    printf(C_CYAN "╚════════════════════════════════════╝" C_RESET "\n");
    
    // Log simples para integracao
    FILE *log = fopen("rafaelia_metrics.log.jsonl", "a");
    if (log) {
        fprintf(log, "{\"ver\":\"13.0-Titan\",\"ts\":%%.0f,\"n\":%%d,\"iops\":%%.2e,\"cores\":%%d}\n", 
                get_time(), N, iops, procs);
        fclose(log);
    }

    free(sys.T); free(sys.Flow); free(sys.Residual);
    return 0;
}
