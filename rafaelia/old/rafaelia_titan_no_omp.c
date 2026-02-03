/*
 * RAFAELIA KERNEL 13.1: TITAN NO-OMP (PURE C, SINGLE-CORE, HIGH-IOPS)
 * TYPE:   Bare-Metal SOC Simulator (Fixed-Point 8.8)
 * TARGET: Android 15 (Termux/ARM64) | -O3 Optimization
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#define FP_SHIFT 8
#define FP_ONE   (1 << FP_SHIFT)
#define FP_MAX   (255 * FP_ONE)

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

// Hash simples, determinístico (5 bits de ruído)
static inline uint16_t fast_hash(int i, int step) {
    uint32_t x = (uint32_t)i ^ (uint32_t)(step * 0x5DEECE66D);
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (uint16_t)(x & 0x1F);
}

// Entropia de Shannon 0..255 (parte inteira do fixed-point)
double calc_entropy(const uint16_t *data, int size) {
    uint64_t counts[256] = {0};
    for (int i = 0; i < size; i++) {
        counts[data[i] >> FP_SHIFT]++;
    }
    double entropy = 0.0;
    double inv_size = 1.0 / (double)size;
    for (int i = 0; i < 256; i++) {
        if (counts[i] > 0) {
            double p = counts[i] * inv_size;
            entropy -= p * log2(p);
        }
    }
    return entropy;
}

// Kernel TITAN em C puro (sem OpenMP)
int step_titan_single(SystemState *sys, int threshold_fp, int step_idx) {
    uint16_t *T   = sys->T;
    uint16_t *Flow= sys->Flow;
    uint8_t  *Res = sys->Residual;
    int N = sys->N;
    int stride_z = N * N;
    int stride_y = N;

    int total_avalanches = 0;

    // Fase 1: atualização local + resíduos
    for (int z = 1; z < N-1; z++) {
        for (int y = 1; y < N-1; y++) {
            int row_offset = z * stride_z + y * stride_y;
            for (int x = 1; x < N-1; x++) {
                int i = row_offset + x;
                uint16_t val = T[i];

                if ((val >> FP_SHIFT) > (threshold_fp >> FP_SHIFT)) {
                    total_avalanches++;

                    uint16_t val_full = val + Res[i];
                    uint16_t decay = val_full >> 2;
                    Res[i] = (uint8_t)(val_full & 0x03);

                    uint16_t noise = fast_hash(i, step_idx);
                    uint16_t leak  = val >> 3;

                    int32_t res = (int32_t)val_full - (int32_t)decay
                                  + (int32_t)noise - (int32_t)(leak * 6);

                    if (res < 0)     res = 0;
                    if (res > FP_MAX) res = FP_MAX;

                    T[i]    = (uint16_t)res;
                    Flow[i] = (val >> 7);
                } else {
                    Flow[i] = 0;
                }
            }
        }
    }

    if (total_avalanches == 0) return 0;

    // Fase 2: difusão (usa Flow)
    for (int z = 1; z < N-1; z++) {
        for (int y = 1; y < N-1; y++) {
            int row_offset = z * stride_z + y * stride_y;
            for (int x = 1; x < N-1; x++) {
                int i = row_offset + x;

                uint16_t inflow =
                    Flow[i + 1] + Flow[i - 1] +
                    Flow[i + stride_y] + Flow[i - stride_y] +
                    Flow[i + stride_z] + Flow[i - stride_z];

                if (inflow > 0) {
                    int32_t res = (int32_t)T[i] + (int32_t)inflow;
                    if (res > FP_MAX) res = FP_MAX;
                    T[i] = (uint16_t)res;
                }
            }
        }
    }

    return total_avalanches;
}

int main(int argc, char *argv[]) {
    int N     = 100;
    int ITERS = 500;

    if (argc > 1) N     = atoi(argv[1]);
    if (argc > 2) ITERS = atoi(argv[2]);

    long long total_cells = (long long)N * N * N;

    printf(C_CYAN);
    printf("   ╔════ RAFAELIA TITAN (v13.1 NO-OMP) ════╗\n");
    printf("   ║ Arch: ARM64 | -O3 | Single-Core SOC   ║\n");
    printf("   ╚═══════════════════════════════════════╝\n");
    printf(C_RESET);
    printf("   [CONF] N=%d (%lld cells) | Iters=%d\n\n",
           N, total_cells, ITERS);

    SystemState sys;
    sys.N    = N;
    sys.size = N * N * N;

    sys.T        = (uint16_t*)calloc(sys.size, sizeof(uint16_t));
    sys.Flow     = (uint16_t*)calloc(sys.size, sizeof(uint16_t));
    sys.Residual = (uint8_t*) calloc(sys.size, sizeof(uint8_t));

    if (!sys.T || !sys.Flow || !sys.Residual) {
        printf(C_RED "[ERR] Memory Allocation Failed\n" C_RESET);
        return 1;
    }

    // Init simples (sem OpenMP, mas rápido o bastante)
    for (int i = 0; i < sys.size; i++) {
        sys.T[i] = (fast_hash(i, 0) * 100) % FP_MAX;
    }

    printf(C_GREEN "[RUN]" C_RESET " Executing TITAN Single-Core...\n");

    int threshold = 200 * FP_ONE;
    double t0 = get_time();
    long long total_ev = 0;

    for (int i = 0; i < ITERS; i++) {
        total_ev += step_titan_single(&sys, threshold, i);

        if (i % 5 == 0) {
            long long sample_sum = 0;
            int step = 10;
            for (int k = 0; k < sys.size; k += step)
                sample_sum += sys.T[k];

            int approx_mean = (int)(sample_sum / (sys.size / step));
            if (approx_mean < 20 * FP_ONE)  approx_mean = 20 * FP_ONE;
            if (approx_mean > 240 * FP_ONE) approx_mean = 240 * FP_ONE;
            threshold = approx_mean;
        }

        if (i % 20 == 0 || i == ITERS - 1) {
            double dt   = get_time() - t0;
            double iops = (total_cells * (double)(i + 1)) / (dt + 1e-9);
            printf("\r   Step %d/%d | IOPS: " C_YELLOW "%.2e" C_RESET " | Ev: %d",
                   i + 1, ITERS, iops, (int)total_ev);
            fflush(stdout);
        }
    }

    double tf   = get_time() - t0;
    double iops = (total_cells * (double)ITERS) / (tf + 1e-9);
    double ent  = calc_entropy(sys.T, sys.size);

    printf("\n\n" C_CYAN "╔════ TITAN NO-OMP RESULTS ═══════════╗" C_RESET "\n");
    printf("║ Time:    %.4fs                     ║\n", tf);
    printf("║ IOPS:    " C_GREEN "%.2e" C_RESET "                  ║\n", iops);
    printf("║ Entropy: %.4f bits                ║\n", ent);
    printf("║ Events:  %lld                   ║\n", total_ev);
    printf(C_CYAN "╚════════════════════════════════════╝" C_RESET "\n");

    FILE *log = fopen("rafaelia_metrics.log.jsonl", "a");
    if (log) {
        fprintf(log,
                "{\"ver\":\"13.1-Titan-NoOMP\",\"ts\":%.0f,"
                "\"n\":%d,\"iters\":%d,\"time\":%.4f,"
                "\"entropy\":%.4f,\"iops\":%.2e}\n",
                get_time(), N, ITERS, tf, ent, iops);
        fclose(log);
        printf(C_YELLOW "\n[LOG] Saved to rafaelia_metrics.log.jsonl" C_RESET "\n");
    }

    free(sys.T);
    free(sys.Flow);
    free(sys.Residual);
    return 0;
}
