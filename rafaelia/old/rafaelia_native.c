/* RAFAELIA TITAN v14 - NATIVE PTHREAD EDITION */
/* Target: Android Bare-Metal (No external libs) */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define FP_SHIFT 8
#define FP_ONE   (1 << FP_SHIFT)
#define FP_MAX   (255 * FP_ONE)

// --- ESTRUTURAS ---
typedef struct {
    uint16_t *T;
    uint16_t *Flow;
    uint8_t  *Residual;
    int N;
    long long total_cells;
} SystemState;

typedef struct {
    int thread_id;
    int num_threads;
    SystemState *sys;
    int threshold;
    int start_z;
    int end_z;
    long long local_avalanches;
    int iters;
    pthread_barrier_t *barrier; // Sincronização nativa
} ThreadArg;

// --- UTILS ---
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static inline uint16_t fast_hash(int i, int step) {
    uint32_t x = i ^ (step * 0x5DEECE66D);
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return x & 0x1F;
}

// --- WORKER THREAD (O CORAÇÃO DO PROCESSAMENTO) ---
void *worker_func(void *arg) {
    ThreadArg *data = (ThreadArg *)arg;
    SystemState *sys = data->sys;
    int N = sys->N;
    int stride_z = N * N;
    int stride_y = N;
    
    uint16_t *restrict T = sys->T;
    uint16_t *restrict Flow = sys->Flow;
    uint8_t  *restrict Res = sys->Residual;

    // Loop principal de iterações
    for (int step = 0; step < data->iters; step++) {
        
        // --- FASE 1: UPDATE (Physics) ---
        // Cada thread processa apenas sua fatia de Z (start_z até end_z)
        for (int z = data->start_z; z < data->end_z; z++) {
            // Evita bordas globais (z=0 e z=N-1)
            if (z == 0 || z == N-1) continue;

            for (int y = 1; y < N-1; y++) {
                int row_offset = z * stride_z + y * stride_y;
                for (int x = 1; x < N-1; x++) {
                    int i = row_offset + x;
                    uint16_t val = T[i];
                    
                    if ((val >> FP_SHIFT) > (data->threshold >> FP_SHIFT)) {
                        data->local_avalanches++;
                        
                        uint16_t val_full = val + Res[i];
                        uint16_t decay = val_full >> 2;
                        Res[i] = val_full & 0x03;
                        
                        uint16_t noise = fast_hash(i, step);
                        uint16_t leak = val >> 3;
                        
                        int32_t res = (int32_t)val_full - decay + noise - (leak * 6);
                        if (res < 0) res = 0; else if (res > FP_MAX) res = FP_MAX;
                        
                        T[i] = (uint16_t)res;
                        Flow[i] = val >> 7;
                    } else {
                        Flow[i] = 0;
                    }
                }
            }
        }

        // BARREIRA 1: Espera todos calcularem o fluxo antes de difundir
        pthread_barrier_wait(data->barrier);

        // --- FASE 2: DIFFUSION (Coupling) ---
        for (int z = data->start_z; z < data->end_z; z++) {
            if (z == 0 || z == N-1) continue;

            for (int y = 1; y < N-1; y++) {
                int row_offset = z * stride_z + y * stride_y;
                for (int x = 1; x < N-1; x++) {
                    int i = row_offset + x;
                    
                    // Lê o fluxo vizinho (que pode ter sido escrito por outra thread)
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

        // BARREIRA 2: Espera todos terminarem a difusão antes do próximo passo
        pthread_barrier_wait(data->barrier);
    }
    
    return NULL;
}

// --- ENTROPIA (SINGLE THREAD PARA SIMPLIFICAR OUTPUT FINAL) ---
double calc_entropy(const uint16_t *data, int size) {
    uint64_t counts[256] = {0};
    for (int i = 0; i < size; i++) counts[data[i] >> FP_SHIFT]++;
    
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

int main(int argc, char *argv[]) {
    int N = 100;
    int ITERS = 500;
    
    if (argc > 1) N = atoi(argv[1]);
    if (argc > 2) ITERS = atoi(argv[2]);

    // Detecta núcleos (fallback seguro para sysconf)
    int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads < 1) num_threads = 1;

    printf("RAFAELIA NATIVE PTHREAD (v14)\n");
    printf("Hardware: ARM64 | Cores: %d\n", num_threads);
    printf("Grid: %dx%dx%d | Iters: %d\n", N, N, N, ITERS);

    // Alocação
    SystemState sys;
    sys.N = N;
    long long sz = (long long)N * N * N;
    sys.total_cells = sz;
    
    sys.T = calloc(sz, sizeof(uint16_t));
    sys.Flow = calloc(sz, sizeof(uint16_t));
    sys.Residual = calloc(sz, sizeof(uint8_t));

    if (!sys.T || !sys.Flow || !sys.Residual) {
        printf("MEM ERROR\n"); return 1;
    }

    // Init (Sequencial é rápido o suficiente para init)
    for (long long i = 0; i < sz; i++) sys.T[i] = fast_hash(i, 0) * 100;

    // Configuração Threads
    pthread_t threads[num_threads];
    ThreadArg t_args[num_threads];
    pthread_barrier_t barrier;
    
    // Inicializa barreira para N threads + thread principal (monitoramento)? 
    // Não, apenas worker threads. O main thread vai esperar o join.
    pthread_barrier_init(&barrier, NULL, num_threads);

    int slice = N / num_threads;
    int threshold = 200 * FP_ONE;
    
    double t0 = get_time();

    // Spawn Threads
    for (int i = 0; i < num_threads; i++) {
        t_args[i].thread_id = i;
        t_args[i].num_threads = num_threads;
        t_args[i].sys = &sys;
        t_args[i].threshold = threshold;
        t_args[i].iters = ITERS;
        t_args[i].barrier = &barrier;
        t_args[i].local_avalanches = 0;
        
        // Divisão de fatias Z
        t_args[i].start_z = i * slice;
        t_args[i].end_z = (i == num_threads - 1) ? N : (i + 1) * slice;

        pthread_create(&threads[i], NULL, worker_func, &t_args[i]);
    }

    // Wait for completion
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    double tf = get_time() - t0;
    
    // Aggregate stats
    long long total_ev = 0;
    for (int i = 0; i < num_threads; i++) total_ev += t_args[i].local_avalanches;

    double iops = (sys.total_cells * ITERS) / tf;
    double ent = calc_entropy(sys.T, sz);

    printf("--- DONE ---\n");
    printf("Time: %.4fs\n", tf);
    printf("IOPS: %.2e\n", iops);
    printf("Entropy: %.4f\n", ent);
    printf("Events: %lld\n", total_ev);

    // Clean Export
    FILE *log = fopen("rafaelia_native.jsonl", "a");
    if (log) {
        fprintf(log, "{\"ts\":%.0f,\"n\":%d,\"iops\":%.2e,\"cores\":%d}\n", 
                get_time(), N, iops, num_threads);
        fclose(log);
    }

    pthread_barrier_destroy(&barrier);
    free(sys.T); free(sys.Flow); free(sys.Residual);
    return 0;
}

